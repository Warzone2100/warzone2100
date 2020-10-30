///
// TL library - A collection of small C++ utilities
// Written in 2017 by Simon Brand (@TartanLlama)
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
///
// An implementation of std::index_sequence in C++11
// Also includes tl::make_index_range
///

#ifndef TL_INTEGER_SEQUENCE_HPP
#define TL_INTEGER_SEQUENCE_HPP

#include <cstddef>

namespace tl {
    template <class T, T... Ns>
    struct integer_sequence {
        using type = integer_sequence;
        using value_type = T;
        static constexpr std::size_t size() { return sizeof...(Ns); }
    };

    namespace detail {
        template <class T, class Seq1, class Seq2>
        struct merge;

        template <class T, std::size_t... Idx1, std::size_t... Idx2>
        struct merge <T, integer_sequence<T,Idx1...>, integer_sequence<T,Idx2...>>
            : integer_sequence<T, Idx1..., (sizeof...(Idx1)+Idx2)...>
        { };

        // Adds Offset to every index in an integer sequence
        template <class T, std::size_t Offset, std::size_t... Idx>
        integer_sequence<T, (Idx + Offset)...> offset_integer_sequence(
            integer_sequence<T, Idx...>);

        template <class T, std::size_t N>
        struct make_integer_sequence
            : detail::merge<T, typename make_integer_sequence<T,N/2>::type,
                            typename make_integer_sequence<T,N - N/2>::type>
        { };

        template<class T> struct make_integer_sequence<T, 0> : integer_sequence<T> {};
        template<class T> struct make_integer_sequence<T, 1> : integer_sequence<T,0> {};
    }

    template <class T, std::size_t N>
    using make_integer_sequence = typename detail::make_integer_sequence<T, N>::type;

    template <std::size_t... Idx>
    using index_sequence = integer_sequence<std::size_t, Idx...>;

    template <std::size_t N>
    using make_index_sequence = make_integer_sequence<std::size_t, N>;

    template <class... Ts>
    using index_sequence_for = make_index_sequence<sizeof...(Ts)>;

    template <std::size_t From, std::size_t N>
    using make_index_range = decltype(
        detail::offset_integer_sequence<std::size_t, From>(make_index_sequence<N>{}));
}

#endif
