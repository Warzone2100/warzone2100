///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __INCLUDED_GSL_FINALLY_H__
#define __INCLUDED_GSL_FINALLY_H__

#include <type_traits>

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
#endif // _MSC_VER

#if defined(__cplusplus) && (__cplusplus >= 201703L)
#define GSL_NODISCARD [[nodiscard]]
#else
#define GSL_NODISCARD
#endif // defined(__cplusplus) && (__cplusplus >= 201703L)

namespace gsl {

	template<class T, class U = T>
	T exchange(T& obj, U&& new_val)
	{
		T original_val = std::move(obj);
		obj = std::forward<U>(new_val);
		return original_val;
	}

	// final_action allows you to ensure something gets run at the end of a scope
	template <class F>
	class final_action
	{
	public:
		static_assert(!std::is_reference<F>::value && !std::is_const<F>::value &&
						  !std::is_volatile<F>::value,
					  "Final_action should store its callable by value");

		explicit final_action(F f) noexcept : f_(std::move(f)) {}

		final_action(final_action&& other) noexcept
			: f_(std::move(other.f_)), invoke_(exchange(other.invoke_, false))
		{}

		final_action(const final_action&) = delete;
		final_action& operator=(const final_action&) = delete;
		final_action& operator=(final_action&&) = delete;

		~final_action() noexcept
		{
			if (invoke_) f_();
		}

	private:
		F f_;
		bool invoke_{true};
	};

	// finally() - convenience function to generate a final_action
	template <class F>
	GSL_NODISCARD final_action<typename std::remove_cv<typename std::remove_reference<F>::type>::type>
	finally(F&& f) noexcept
	{
		return final_action<typename std::remove_cv<typename std::remove_reference<F>::type>::type>(
			std::forward<F>(f));
	}
}

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif // _MSC_VER

#endif // __INCLUDED_GSL_FINALLY_H__
