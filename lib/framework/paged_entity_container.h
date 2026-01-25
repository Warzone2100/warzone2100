/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** @file paged_entity_container.h
 * Optimized paged container for various in-game entities,
 * capable of recycling erased elements, which greatly reduces
 * memory fragmentation for rapid allocation/deallocation patterns.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <queue>
#include <vector>
#include <utility>

/// <summary>
/// Optimized paged container for various in-game entities,
/// capable of recycling erased elements, which greatly reduces
/// memory fragmentation for rapid allocation/deallocation patterns.
///
/// As noted above, the container allocates memory in fixed-size
/// continuous chunks, or pages, hence the name.
///
/// Each page is set to hold exactly `MaxElementsPerPage` elements,
/// which is 1024 by default.
///
/// Also, each element is equipped with some additional metadata,
/// which allows the container to reuse the same memory (also called "slots")
/// for consequent allocations, after the current element was freed.
///
/// Such a structure is often called the "slot map".
/// More info can be found by following these links:
/// * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0661r0.pdf
/// * https://www.youtube.com/watch?v=SHaAR7XPtNU
///
/// Each slot can currently survive up to `std::numeric_limits<uint32_t>::max() - 1`
/// incarnations (also called "generations"). When the generation counter overflows,
/// the slot will become "expired", meaning that it won't ever return back
/// to the freelist queue.
/// The slot expiration mechanism can help prevent various memory-related
/// errors and reduce the risks of accessing bad/stale pointers.
///
/// There isn't currently an API to explicitly check which "generation"
/// an element belongs to, but it is planned for the future.
///
/// `PagedEntityContainer` further tries to optimize rapid
/// allocation/deallocation patterns by calling destructors only
/// when it's needed (e.g. it won't call any destructors for
/// instances of `T`, which are trivially destructible, and will just
/// re-write one object on top of the other while reallocating
/// an existing slot).
///
/// The container provides `begin()`/`end()` methods which return an instance
/// of forward iterator, which traverses only slots, which are currently
/// alive (meaning: there is always a valid allocated item at the current
/// position pointed-to by iterator, except for `end()` iterator).
///
/// There are `emplace()` and `erase()` methods, to insert or remove a single
/// element from the container. `emplace()` does not ever invalidate any
/// iterators and references. `erase()` invalidates only the reference/iterator,
/// which is being erased from the container.
///
/// Also, `clear()` method invalidates all existing references and iterators.
/// This method erases all elements (calling their destructors, if needed),
/// frees all pages except the first one and resets its metadata
/// completely, so that the container can be used as if it was
/// constructed anew.
///
/// Algorithmic complexities of common operations are as follows:
/// * `emplace()` is `O(1)` + complexity of `T(Args&&...)` constructor.
/// * `erase()` is `O(1)` + complexity of `~T()` destructor.
/// * `clear()` can be up to `O(N)` if destructors need to be called.
///
/// This implementation is loosely inspired by the following implementations,
/// which can be found on the GitHub:
/// * https://github.com/SergeyMakeev/slot_map/blob/main/slot_map/slot_map.h
/// * https://github.com/Masstronaut/slot_array/blob/master/slot_map.hpp
/// </summary>
/// <typeparam name="T">Entity type. Should be a complete type.</typeparam>
/// <typeparam name="MaxElementsPerPage">The fixed number of elements each page may hold.</typeparam>
/// <typeparam name="ReuseSlots">If `false`, slots are one-shot and set to expire after single use.</typeparam>
template <typename T, size_t MaxElementsPerPage = 1024, bool ReuseSlots = true>
class PagedEntityContainer
{
	using SlotIndexType = size_t;

	// Default initial capacity is exactly 1 page.
	static constexpr size_t DEFAULT_INITIAL_CAPACITY = 1 * MaxElementsPerPage;

	static constexpr SlotIndexType INVALID_SLOT_IDX = std::numeric_limits<SlotIndexType>::max();
	static constexpr size_t INVALID_PAGE_IDX = std::numeric_limits<size_t>::max();

	/// <summary>
	/// Guaranteed to be standard-layout type.
	///
	/// This means the address of `AlignedElementStorage` instance
	/// should always be the same as the address of its first data member.
	/// </summary>
	struct AlignedElementStorage
	{
		// Align raw storage to the requirements of `T` type, which allows
		// to use placement-new to construct new instances of `T`
		// within the contiguous storage of `AlignmentElementStorage:s`.
		alignas(T) char rawStorage[sizeof(T)];
	};

	// Aggregate struct which contains an page-wise element index, i.e. <page index, index within that page>.
	using PageIndex = std::pair<size_t, size_t>;

	/// <summary>
	/// Structure to describe a single element slot.
	/// Provides information whether the current element is alive or not,
	/// as well as the current incarnation ordinal number.
	/// </summary>
	class SlotMetadata
	{
		static constexpr uint32_t INVALID_GENERATION = 0;
		uint32_t _generation = INVALID_GENERATION;
		bool _isAlive = false;

	public:

		bool is_valid() const
		{
			return _generation != INVALID_GENERATION;
		}

		bool is_alive() const
		{
			return _isAlive;
		}

		void invalidate()
		{
			_generation = INVALID_GENERATION;
		}

		void set_dead()
		{
			_isAlive = false;
		}

		void set_alive()
		{
			_isAlive = true;
		}

		void reset_generation()
		{
			_generation = 1;
		}

		void advance_generation()
		{
			++_generation;
		}
	};

	/// <summary>
	/// This class holds the actual contiguous storage and metadata for the elements,
	/// as well as the queue for recycled indices.
	///
	/// Always allocates storage for exactly `MaxElementsPerPage` elements.
	///
	/// If the parent container is instantiated with the `ReuseSlots=false` mode,
	/// expired pages will deallocate their storage to keep memory consumption
	/// under control.
	/// </summary>
	class Page
	{
		// Represents the free list of recycled IDs (i.e. IDs of elements,
		// which were erased earlier and are eligible to be recycled and used again).
		std::queue<SlotIndexType> _recycledFreeIndices;
		std::unique_ptr<AlignedElementStorage[]> _storage = nullptr;
		std::unique_ptr<SlotMetadata[]> _slotMetadata = nullptr;
		// The number of allocated (i.e., alive) elements in the page.
		size_t _currentSize = 0;
		// Max valid index within a single page. Monotonically increasing value as
		// the page gets filled up.
		SlotIndexType _maxValidIndex = INVALID_SLOT_IDX;
		// The number of expired slots, i.e. which have overflowed generation number.
		size_t _expiredSlotsCount = 0;

	public:

		bool is_full() const
		{
			return _currentSize + _expiredSlotsCount == MaxElementsPerPage;
		}

		void allocate_storage()
		{
			assert(_storage == nullptr);
			assert(_slotMetadata == nullptr);

			// Allocate storage for `MaxElementsPerPage` elements.
			_storage = std::make_unique<AlignedElementStorage[]>(MaxElementsPerPage);
			_slotMetadata = std::make_unique<SlotMetadata[]>(MaxElementsPerPage);
		}

		bool has_recycled_indices() const
		{
			return !_recycledFreeIndices.empty();
		}

		void recycle_index(const SlotIndexType& idx)
		{
			_recycledFreeIndices.emplace(idx);
		}

		SlotIndexType pop_free_index()
		{
			assert(!_recycledFreeIndices.empty());

			auto res = _recycledFreeIndices.front();
			_recycledFreeIndices.pop();
			return res;
		}

		SlotIndexType max_valid_index() const
		{
			return _maxValidIndex;
		}

		void set_max_valid_index(SlotIndexType idx)
		{
			_maxValidIndex = idx;
		}

		SlotMetadata* slotMetadata()
		{
			return _slotMetadata.get();
		}

		const SlotMetadata* slotMetadata() const
		{
			return _slotMetadata.get();
		}

		AlignedElementStorage* storage()
		{
			return _storage.get();
		}

		const AlignedElementStorage* storage() const
		{
			return _storage.get();
		}

		void decrease_current_size()
		{
			--_currentSize;
		}

		void increase_current_size()
		{
			++_currentSize;
		}

		void increase_expired_slots_count()
		{
			++_expiredSlotsCount;
		}

		// When `ReuseSlots=false`, expired pages will also deallocate
		// their storage upon reaching "expired" state.
		bool is_expired() const
		{
			return _expiredSlotsCount == MaxElementsPerPage;
		}

		// This method renders the page unusable!
		//
		// The page will then need to call `allocate_storage()` + `reset_metadata()`
		// to be usable once again.
		void deallocate_storage()
		{
			_storage.reset();
			_slotMetadata.reset();
		}

		// Reset generations to least possible valid value for all slots,
		// plus mark all slots as dead, so that the page appears clean and empty.
		void reset_metadata()
		{
			auto* meta = slotMetadata();
			assert(meta != nullptr);
			for (size_t i = 0; i < MaxElementsPerPage; ++i)
			{
				auto& slot = meta[i];
				slot.reset_generation();
				slot.set_dead();
			}
			_currentSize = 0;
			_maxValidIndex = INVALID_SLOT_IDX;
			_expiredSlotsCount = 0;
			// There's no `.clear()` for `std::queue`, unfortunately.
			_recycledFreeIndices = {};
		}
	};

public:

	explicit PagedEntityContainer()
		: PagedEntityContainer(DEFAULT_INITIAL_CAPACITY)
	{}

	explicit PagedEntityContainer(size_t initialCapacity)
	{
		reserve(initialCapacity);
	}

	~PagedEntityContainer()
	{
		// No need to perform any additional cleanup steps
		// aside from calling destructors, if needed.
		destroy_live_elements();
	}

	bool empty() const
	{
		return _size == 0;
	}

	size_t size() const
	{
		return _size;
	}

	// Raw capacity minus part which represents storage,
	// not accessible anymore (i.e., expired slots).
	size_t usable_capacity() const
	{
		return _capacity - _expiredSlotsCount;
	}

	// Reserve the storage based on the raw capacity of the container, not `usable_capacity()`.
	void reserve(size_t capacity)
	{
		// Check `capacity` against current `_capacity`. If <= do nothing,
		// else calculate the necessary amount of pages to be allocated
		// and extend the storage.
		if (_capacity >= capacity)
		{
			return;
		}
		size_t needed_nr_of_pages = (capacity / MaxElementsPerPage) - _pages.size();
		while (needed_nr_of_pages-- != 0)
		{
			allocate_new_page();
		}
	}

	void allocate_new_page()
	{
		Page newPage;
		newPage.allocate_storage();
		_pages.emplace_back(std::move(newPage));
		_capacity += MaxElementsPerPage;
	}

	template <typename... Args>
	T& emplace(Args&&... args)
	{
		// Find first page with free slots available,
		// record found page index,
		// then pop free index from the relevant page.
		auto pageId = find_first_page_with_recycled_ids();
		if (pageId != INVALID_PAGE_IDX)
		{
			Page& p = _pages[pageId];
			// Retrieve a spare ID from the freelist.
			PageIndex pageIdx = {pageId, p.pop_free_index()};
			// Mark the current slot as alive.
			get_slot_metadata(pageIdx).set_alive();
			_pages[pageIdx.first].increase_current_size();
			// Construct the element.
			T* res = allocate_element_impl(page_index_to_storage_addr(pageIdx), std::forward<Args>(args)...);
			++_size;
			return *res;
		}
		if (_size == usable_capacity())
		{
			allocate_new_page();
		}
		// No IDs available for recycling, allocate a new ID.
		auto newIdx = allocate_new_idx();
		_maxIndex = page_index_to_global(newIdx);
		_pages[newIdx.first].increase_current_size();
		// Construct the element.
		T* res = allocate_element_impl(page_index_to_storage_addr(newIdx), std::forward<Args>(args)...);
		++_size;
		return *res;
	}

	void erase(const PageIndex& pageIdx)
	{
		auto& slotMetadata = get_slot_metadata(pageIdx);
		if (!slotMetadata.is_alive())
		{
			return;
		}

		// Either advance slot generation number or invalidate the slot right away,
		// the behavior depends on whether we reuse the slots or not.
		advance_slot_generation<ReuseSlots>(slotMetadata);

		// Ensure that the element pointed-to by this slot is dead.
		slotMetadata.set_dead();

		const bool isSlotExpired = !slotMetadata.is_valid();

		// Deallocate the object (i.e. call the destructor, if needed).
		T* valuePtr = reinterpret_cast<T*>(page_index_to_storage_addr(pageIdx));
		deallocate_element_impl(valuePtr);

		// Decrease both page and container sizes.
		_pages[pageIdx.first].decrease_current_size();
		--_size;

		if (isSlotExpired)
		{
			// Increase counters for expired slots tracking.
			_pages[pageIdx.first].increase_expired_slots_count();
			++_expiredSlotsCount;
			// Looks like at least GCC is smart enough to optimize the following
			// instructions away, even when this is not a `constexpr if`, but a regular `if`,
			// See https://godbolt.org/z/bY8oEYsdz.
			if (!ReuseSlots)
			{
				if (_pages[pageIdx.first].is_expired())
				{
					// Free storage pointers for expired pages when not reusing slots,
					// this should alleviate the problem of excessive memory consumption
					// in such cases.
					_pages[pageIdx.first].deallocate_storage();
				}
			}
		}
		else
		{
			// Put the erased index into freelist for recycled IDs.
			_pages[pageIdx.first].recycle_index(pageIdx.second);
		}
	}

	void erase(SlotIndexType idx)
	{
		erase(global_to_page_index(idx));
	}

	/// <summary>
	/// Forward iterator, which traverses live elements in the parent container.
	/// </summary>
	/// <typeparam name="IsConst">Marks whether iterator is const or not.</typeparam>
	template <bool IsConst>
	class IteratorImpl
	{
	public:

		using iterator_category = std::forward_iterator_tag;
		using value_type = std::conditional_t<IsConst, std::add_const_t<T>, T>;
		using difference_type = std::ptrdiff_t;
		using pointer = std::add_pointer_t<value_type>;
		using reference = std::add_lvalue_reference_t<value_type>;

		using ParentContainerType = std::conditional_t<
			IsConst,
			std::add_const_t<PagedEntityContainer>,
			PagedEntityContainer
		>;

		IteratorImpl(ParentContainerType& c, PageIndex idx)
			: _pageIdx(idx), _c(c)
		{}

		IteratorImpl(const IteratorImpl& other)
			: _pageIdx(other._pageIdx), _c(other._c)
		{}

		// Allow promotion of non-const iterator to const iterator.
		template <bool DummyConst = IsConst, std::enable_if_t<DummyConst, bool> = true>
		IteratorImpl(const IteratorImpl<false>& other)
			: _pageIdx(other._pageIdx), _c(other._c)
		{}

		const PageIndex& index() const
		{
			return _pageIdx;
		}

		bool operator==(const IteratorImpl& other) const
		{
			return _pageIdx == other._pageIdx && &_c == &other._c;
		}

		bool operator!=(const IteratorImpl& other) const
		{
			return !(*this == other);
		}

		reference operator*() const
		{
			return *get_value_impl();
		}

		pointer operator->() const
		{
			return get_value_impl();
		}

		// Prefix increment
		IteratorImpl& operator++()
		{
			do
			{
				advance_page_index();
			} while (page_index_to_global(_pageIdx) <= _c._maxIndex && !get_metadata_impl().is_alive());

			if (page_index_to_global(_pageIdx) > _c._maxIndex)
			{
				_pageIdx = invalid_page_index();
			}

			return *this;
		}

		// Postfix increment
		IteratorImpl operator++(int)
		{
			auto& self = *this;
			IteratorImpl copy(self);
			++self;
			return copy;
		}

	private:

		pointer get_value_impl() const
		{
			return reinterpret_cast<pointer>(_c.page_index_to_storage_addr(_pageIdx));
		}

		const SlotMetadata& get_metadata_impl() const
		{
			return _c.get_slot_metadata(_pageIdx);
		}

		SlotMetadata& get_metadata_impl()
		{
			return _c.get_slot_metadata(_pageIdx);
		}

		void advance_page_index()
		{
			const auto pagesCount = _c._pages.size();
			assert(_pageIdx.first < pagesCount);
			assert(_pageIdx.second <= current_page().max_valid_index());
			// Move to the next page if already pointing to the last valid index or if the current page is expired.
			if (_pageIdx.second == current_page().max_valid_index() || current_page().is_expired())
			{
				// Skip expired pages when transitioning to the next page.
				do
				{
					++_pageIdx.first;
				} while (_pageIdx.first < pagesCount && current_page().is_expired());
				_pageIdx.second = 0;
			}
			else
			{
				// Move along within the current page.
				++_pageIdx.second;
			}
		}

		Page& current_page()
		{
			return _c._pages[_pageIdx.first];
		}

		// Make both possible instantiations of iterator friends
		// to allow promotion to from non-const to const iterator.
		template<bool IsConst2>
		friend class IteratorImpl;

		friend class PagedEntityContainer;

		PageIndex _pageIdx;
		ParentContainerType& _c;
	};

	template <bool IsConst>
	friend class IteratorImpl;

	using iterator = IteratorImpl<false>;
	using const_iterator = IteratorImpl<true>;

	const_iterator begin() const
	{
		return const_iterator(const_cast<PagedEntityContainer*>(this)->begin());
	}

	iterator begin()
	{
		if (empty())
		{
			return end();
		}
		for (size_t pageIdx = 0, pagesEnd = _pages.size(); pageIdx != pagesEnd; ++pageIdx)
		{
			const Page& p = _pages[pageIdx];
			if (p.is_expired())
			{
				continue;
			}
			const auto* slotMeta = p.slotMetadata();
			for (size_t elemIdx = 0, elemEnd = p.max_valid_index(); elemIdx <= elemEnd; ++elemIdx)
			{
				if (slotMeta[elemIdx].is_alive())
				{
					return iterator(*this, {pageIdx, elemIdx});
				}
			}
		}
		// No live elements, return end iterator.
		return end();
	}

	const_iterator end() const
	{
		return const_iterator(*this, invalid_page_index());
	}

	iterator end()
	{
		return iterator(*this, invalid_page_index());
	}

	const_iterator cbegin() const
	{
		return begin();
	}

	const_iterator cend() const
	{
		return end();
	}

	// Tries to look up the relevant page index for `x` and return an iterator to it.
	// Returns `end()` if not found.
	iterator find(T& x)
	{
		auto* addr = reinterpret_cast<AlignedElementStorage*>(&x);
		assert(is_properly_aligned_ptr(addr));
		if (empty())
		{
			return end();
		}
		for (size_t i = 0, end = _pages.size(); i != end; ++i)
		{
			Page& p = _pages[i];
			if (p.is_expired())
			{
				continue;
			}
			auto* storage = p.storage();
			assert(storage != nullptr);
			// If the address is inside the current page, calculate the index from raw offset.
			if (addr >= storage && addr <= &storage[p.max_valid_index()])
			{
				// Pointer subtraction of yields the difference between
				// array subscripts in the current case.
				const ptrdiff_t idx = addr - storage;
				return iterator(*this, {i, static_cast<size_t>(idx)});
			}
		}
		return end();
	}

	const_iterator find(const T& x) const
	{
		return const_iterator(const_cast<PagedEntityContainer*>(this)->find(const_cast<T&>(x)));
	}

	void erase(const_iterator it)
	{
		assert(&it._c == this);
		erase(it.index());
	}

	void erase(iterator it)
	{
		assert(&it._c == this);
		erase(it.index());
	}

	// Destroys all elements and frees all data pages except first to save up on memory.
	// Reset metadata of the only remaining page to make container usable once again.
	void clear()
	{
		// Deallocate all live elements, call destructors, if needed.
		for (auto it = begin(), endIt = end(); it != endIt; ++it)
		{
			erase(it);
		}
		// Shrink the storage to just a single page.
		_pages.resize(1);
		_capacity = MaxElementsPerPage;
		// No valid items in the container now.
		_maxIndex = INVALID_SLOT_IDX;
		if (!ReuseSlots)
		{
			// If the first page is in the expired state, then
			// its storage is deallocated, too. So, reallocate it.
			//
			// NOTE: expired pages free their storage only when
			// the slot recycling mechanism is disabled!
			if (_pages.front().is_expired())
			{
				_pages.front().allocate_storage();
			}
		}
		_pages.front().reset_metadata();
		_expiredSlotsCount = 0;
	}

private:

	PageIndex allocate_new_idx()
	{
		// Assume there are no "holes" in the existing data and the storage isn't full.
		// Look at the last page and allocate a new idx from it.
		Page& lastPage = _pages.back();
		assert(!lastPage.is_full());
		SlotIndexType newElementIdx = lastPage.max_valid_index() != INVALID_SLOT_IDX ? lastPage.max_valid_index()  + 1 : 0;

		PageIndex pageIdx = {_pages.size() - 1, newElementIdx};

		auto& slot = get_slot_metadata(pageIdx);
		slot.reset_generation();
		slot.set_alive();

		lastPage.set_max_valid_index(newElementIdx);

		return pageIdx;
	}

	template <typename... Args>
	T* allocate_element_impl(void* address, Args&&... args)
	{
		return new (address) T(std::forward<Args>(args)...);
	}

	// Use some hacks for SFINAE to make it compliant to C++ standard,
	// see https://stackoverflow.com/a/11056319 for details.
	template <typename U = T, std::enable_if_t<std::is_trivially_destructible<U>::value, bool> = true>
	void deallocate_element_impl(T* /*element*/)
	{}

	template <typename U = T, std::enable_if_t<!std::is_trivially_destructible<U>::value, bool> = true>
	void deallocate_element_impl(T* element)
	{
		element->~T();
	}

	SlotMetadata& get_slot_metadata(const PageIndex& idx)
	{
		auto* metadata = _pages[idx.first].slotMetadata();
		return metadata[idx.second];
	}

	void* page_index_to_storage_addr(const PageIndex& idx)
	{
		auto* storage = _pages[idx.first].storage();
		return &storage[idx.second].rawStorage;
	}

	static PageIndex global_to_page_index(SlotIndexType idx)
	{
		return { idx / MaxElementsPerPage, idx % MaxElementsPerPage };
	}

	static SlotIndexType page_index_to_global(const PageIndex& idx)
	{
		return idx.first * MaxElementsPerPage + idx.second;
	}

	// No need to call destructors manually for trivially destructible types.
	template <typename U = T, std::enable_if_t<std::is_trivially_destructible<U>::value, bool> = true>
	void destroy_live_elements()
	{}

	template <typename U = T, std::enable_if_t<!std::is_trivially_destructible<U>::value, bool> = true>
	void destroy_live_elements()
	{
		for (auto it = begin(), endIt = end(); it != endIt; ++it)
		{
			deallocate_element_impl(&*it);
		}
	}

	size_t find_first_page_with_recycled_ids() const
	{
		for (size_t i = 0, end = _pages.size(); i != end; ++i)
		{
			if (_pages[i].has_recycled_indices())
			{
				return i;
			}
		}
		return INVALID_PAGE_IDX;
	}

	static PageIndex invalid_page_index()
	{
		return { INVALID_PAGE_IDX, INVALID_SLOT_IDX };
	}

	static bool is_properly_aligned_ptr(void* addr)
	{
		return (reinterpret_cast<uintptr_t>(addr) % alignof(T)) == 0;
	}

	template <bool ReuseSlotsAux = ReuseSlots, std::enable_if_t<ReuseSlotsAux, bool> = true>
	void advance_slot_generation(SlotMetadata& meta)
	{
		// Advance slot generation number, when `ReuseSlots=true`.
		meta.advance_generation();
	}

	// Specialization for the case when `ReuseSlots=false`.
	template <bool ReuseSlotsAux = ReuseSlots, std::enable_if_t<!ReuseSlotsAux, bool> = true>
	void advance_slot_generation(SlotMetadata& meta)
	{
		// Invalidate slot right away so that it cannot be reused anymore.
		meta.invalidate();
	}

	std::vector<Page> _pages;
	SlotIndexType _maxIndex = INVALID_SLOT_IDX;
	size_t _size = 0;
	size_t _capacity = 0;
	size_t _expiredSlotsCount = 0;
};

template <typename T, size_t MaxElementsPerPage, bool ReuseSlots>
constexpr typename PagedEntityContainer<T, MaxElementsPerPage, ReuseSlots>::SlotIndexType
	PagedEntityContainer<T, MaxElementsPerPage, ReuseSlots>::INVALID_SLOT_IDX;

template <typename T, size_t MaxElementsPerPage, bool ReuseSlots>
constexpr size_t PagedEntityContainer<T, MaxElementsPerPage, ReuseSlots>::INVALID_PAGE_IDX;

