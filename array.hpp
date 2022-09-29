#pragma once

/// \file array.hpp std::array-compatible implementation with C++17 features, including constexpr modification support

#include <iterator>
#include <type_traits>
#include <utility>

#include <cassert>
#include "estd.hpp"

#ifdef __cpp_exceptions
#include <stdexcept>
#define RANGE_CHECK(CHECK, MESSAGE) if(CHECK) throw std::out_of_range(MESSAGE)
#define NOEXCEPT_(...) noexcept(__VA_ARGS__)
#else
#define RANGE_CHECK(CHECK, MESSAGE) ASSERT_M(!(CHECK), MESSAGE)
#define NOEXCEPT_(...)
#endif

#if (__cplusplus >= 201103L && defined(__cpp_exceptions))
/// \brief Defined noexcept specifier only if C++11 and exceptions are enabled
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

#define ASSERT_M(CONDITION, MESSAGE) assert(CONDITION && MESSAGE)

namespace estd {

  template <class _ForwardIterator1, class _ForwardIterator2>
  static inline constexpr _ForwardIterator2 swap_ranges(_ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2)
  {
      for(; __first1 != __last1; ++__first1, (void) ++__first2)
          swap(*__first1, *__first2);
      return __first2;
  }

  
  #ifdef __cpp_exceptions
  template<class T>
   inline constexpr bool is_nothrow_move_constructable_v = std::is_nothrow_move_constructable<T>::value;
  
   template<class T>
   inline constexpr bool is_nothrow_constructable_v = std::is_nothrow_constructable<T>::value;
#endif
   
   template< class T, class... Args >
   inline constexpr bool is_constructible_v = std::is_constructible<T, Args...>::value;
   
   template< class T >
   inline constexpr bool is_move_constructible_v = std::is_move_constructible<T>::value;
 
   template< class T >
   inline constexpr bool is_array_v = std::is_array<T>::value;

template <class _Tp, size_t _Size>
struct array
{
    // types:
    typedef array __self;
    typedef _Tp                                   value_type;
    typedef value_type&                           reference;
    typedef const value_type&                     const_reference;
    typedef value_type*                           iterator;
    typedef const value_type*                     const_iterator;
    typedef value_type*                           pointer;
    typedef const value_type*                     const_pointer;
    typedef size_t                                size_type;
    typedef ptrdiff_t                             difference_type;
    typedef std::reverse_iterator<iterator>       reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    _Tp __elems_[_Size];

    // No explicit construct/copy/destroy for aggregate type
    inline constexpr void fill(const value_type& __u) NOEXCEPT {
      std::fill_n(data(), _Size, __u);
    }

    inline constexpr void swap(array& __a) NOEXCEPT {
        swap_ranges(data(), data() + _Size, __a.data());
    }

    // iterators:
    inline constexpr iterator begin() NOEXCEPT {return iterator(data());}
    inline constexpr const_iterator begin() const NOEXCEPT {return const_iterator(data());}
    inline constexpr iterator end() NOEXCEPT {return iterator(data() + _Size);}
    inline constexpr  const_iterator end() const NOEXCEPT {return const_iterator(data() + _Size);}

    inline constexpr reverse_iterator rbegin() NOEXCEPT {return reverse_iterator(end());}
    inline constexpr const_reverse_iterator rbegin() const NOEXCEPT {return const_reverse_iterator(end());}
    inline constexpr reverse_iterator rend() NOEXCEPT {return reverse_iterator(begin());}
    inline constexpr const_reverse_iterator rend() const NOEXCEPT {return const_reverse_iterator(begin());}

    inline constexpr const_iterator cbegin() const NOEXCEPT {return begin();}
    inline constexpr const_iterator cend() const NOEXCEPT {return end();}
    inline constexpr const_reverse_iterator crbegin() const NOEXCEPT {return rbegin();}
    inline constexpr const_reverse_iterator crend() const NOEXCEPT {return rend();}

    // capacity:
    inline constexpr size_type size() const NOEXCEPT {return _Size;}
    inline constexpr size_type max_size() const NOEXCEPT {return _Size;}
    inline constexpr bool empty() const NOEXCEPT {return _Size == 0;}

    // element access:
    inline constexpr reference operator[](size_type __n) NOEXCEPT {
      
        ASSERT_M(__n < _Size, "out-of-bounds access in std::array<T, N>");
        return __elems_[__n];
    }
    inline constexpr const_reference operator[](size_type __n) const NOEXCEPT {
        ASSERT_M(__n < _Size, "out-of-bounds access in std::array<T, N>");
        return __elems_[__n];
    }

    inline constexpr reference at(size_type __n)
    {
        RANGE_CHECK(__n >= _Size, "array::at");
        return __elems_[__n];
    }

    inline constexpr const_reference at(size_type __n) const
    {
        RANGE_CHECK(__n >= _Size, "array::at");
        return __elems_[__n];
    }

    inline constexpr reference front()             NOEXCEPT {return (*this)[0];}
    inline constexpr const_reference front() const NOEXCEPT {return (*this)[0];}
    inline constexpr reference back()              NOEXCEPT {return (*this)[_Size - 1];}
    inline constexpr const_reference back() const  NOEXCEPT {return (*this)[_Size - 1];}

    inline constexpr value_type* data() NOEXCEPT {return __elems_;}
    inline constexpr const value_type* data() const NOEXCEPT {return __elems_;}
};

template <class _Tp>
struct array<_Tp, 0>
{
    // types:
    typedef array __self;
    typedef _Tp                                   value_type;
    typedef value_type&                           reference;
    typedef const value_type&                     const_reference;
    typedef value_type*                           iterator;
    typedef const value_type*                     const_iterator;
    typedef value_type*                           pointer;
    typedef const value_type*                     const_pointer;
    typedef size_t                                size_type;
    typedef ptrdiff_t                             difference_type;
    typedef std::reverse_iterator<iterator>       reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef typename std::conditional<std::is_const<_Tp>::value, const char,
                                char>::type _CharType;

    struct  _ArrayInStructT { _Tp __data_[1]; };
    alignas(_ArrayInStructT) _CharType __elems_[sizeof(_ArrayInStructT)];

    inline constexpr value_type* data() NOEXCEPT {return nullptr;}
    inline constexpr const value_type* data() const NOEXCEPT {return nullptr;}

    // No explicit construct/copy/destroy for aggregate type
    inline constexpr void fill(const value_type&) {
      static_assert(!std::is_const<_Tp>::value,
                    "cannot fill zero-sized array of type 'const T'");
    }

    inline constexpr void swap(array&) NOEXCEPT {
      static_assert(!std::is_const<_Tp>::value,
                    "cannot swap zero-sized array of type 'const T'");
    }

    // iterators:
    inline constexpr iterator begin() NOEXCEPT {return iterator(data());}
    inline constexpr const_iterator begin() const NOEXCEPT {return const_iterator(data());}
    inline constexpr iterator end() NOEXCEPT {return iterator(data());}
    inline constexpr const_iterator end() const NOEXCEPT {return const_iterator(data());}

    inline constexpr reverse_iterator rbegin() NOEXCEPT {return reverse_iterator(end());}
    inline constexpr const_reverse_iterator rbegin() const NOEXCEPT {return const_reverse_iterator(end());}
    inline constexpr reverse_iterator rend() NOEXCEPT {return reverse_iterator(begin());}
    inline constexpr const_reverse_iterator rend() const NOEXCEPT {return const_reverse_iterator(begin());}

    inline constexpr const_iterator cbegin() const NOEXCEPT {return begin();}
    inline constexpr const_iterator cend() const NOEXCEPT {return end();}
    inline constexpr const_reverse_iterator crbegin() const NOEXCEPT {return rbegin();}
    inline constexpr const_reverse_iterator crend() const NOEXCEPT {return rend();}

    // capacity:
    inline constexpr size_type size() const NOEXCEPT {return 0; }
    inline constexpr size_type max_size() const NOEXCEPT {return 0;}
    inline constexpr bool empty() const NOEXCEPT {return true;}

    // element access:
    inline constexpr reference operator[](size_type) NOEXCEPT {
      ASSERT_M(false, "cannot call array<T, 0>::operator[] on a zero-sized array");
      return __elems_[0];
    }

    inline constexpr const_reference operator[](size_type) const NOEXCEPT {
      ASSERT_M(false, "cannot call array<T, 0>::operator[] on a zero-sized array");
      return __elems_[0];
    }

    inline constexpr reference at(size_type) {
      RANGE_CHECK(false, "array<T, 0>::at");
      return __elems_[0];
    }

    inline constexpr const_reference at(size_type) const {
      RANGE_CHECK(false, "array<T, 0>::at");
      return __elems_[0];
    }

    inline constexpr reference front() NOEXCEPT {
      ASSERT_M(false, "cannot call array<T, 0>::front() on a zero-sized array");
      return __elems_[0];
    }

    inline constexpr const_reference front() const NOEXCEPT {
      ASSERT_M(false, "cannot call array<T, 0>::front() on a zero-sized array");
      return __elems_[0];
    }

    inline constexpr reference back() NOEXCEPT {
      ASSERT_M(false, "cannot call array<T, 0>::back() on a zero-sized array");
      return __elems_[0];
    }

    inline constexpr const_reference back() const NOEXCEPT {
      ASSERT_M(false, "cannot call array<T, 0>::back() on a zero-sized array");
      return __elems_[0];
    }
};


template<class _Tp, class... _Args, std::enable_if_t<conjunction<std::is_same<_Tp, _Args>...>::value, bool> = true>
array(_Tp, _Args...)  -> array<_Tp, 1 + sizeof...(_Args)>;

template <class _Tp, size_t _Size>
inline constexpr bool operator==(const array<_Tp, _Size>& __x, const array<_Tp, _Size>& __y)
{
    return std::equal(__x.begin(), __x.end(), __y.begin());
}

template <class _Tp, size_t _Size>
inline constexpr bool operator!=(const array<_Tp, _Size>& __x, const array<_Tp, _Size>& __y)
{
    return !(__x == __y);
}

template <class _Tp, size_t _Size>
inline constexpr bool operator<(const array<_Tp, _Size>& __x, const array<_Tp, _Size>& __y)
{
    return std::lexicographical_compare(__x.begin(), __x.end(),
                                        __y.begin(), __y.end());
}

template <class _Tp, size_t _Size>
static inline constexpr bool operator>(const array<_Tp, _Size>& __x, const array<_Tp, _Size>& __y)
{
    return __y < __x;
}

template <class _Tp, size_t _Size>
static inline constexpr bool operator<=(const array<_Tp, _Size>& __x, const array<_Tp, _Size>& __y)
{
    return !(__y < __x);
}

template <class _Tp, size_t _Size>
static inline constexpr bool operator>=(const array<_Tp, _Size>& __x, const array<_Tp, _Size>& __y)
{
    return !(__x < __y);
}

namespace detail {
    struct tag {};

    template<class T>
    tag swap(T&, T&);

      using std::swap;

    template<typename T>
    struct can_call_swap_impl {

        template<typename U>
        static auto check(int)
        -> decltype( swap(std::declval<T&>(), std::declval<T&>()),
                std::true_type());

        template<typename>
        static std::false_type check(...);

        using type = decltype(check<T>(0));
    };

    template<typename T>
    struct can_call_swap : can_call_swap_impl<T>::type { };
    
    template<typename T>
    struct would_call_std_swap_impl {

        template<typename U>
        static auto check(int)
        -> std::integral_constant<bool, std::is_same<decltype( swap(std::declval<U&>(), std::declval<U&>())), tag>::value>;

        template<typename>
        static std::false_type check(...);

        using type = decltype(check<T>(0));
    };

    template<typename T>
    struct would_call_std_swap : would_call_std_swap_impl<T>::type { };
}

template<typename T>
struct is_swappable :
    std::integral_constant<bool,
        detail::can_call_swap<T>::value &&
        (!detail::would_call_std_swap<T>::value ||
        (std::is_move_assignable<T>::value &&
        std::is_move_constructible<T>::value))
    > { };
    
    
template<typename T, std::size_t N>
struct is_swappable<T[N]> : is_swappable<T> {};

template <class _Tp, size_t _Size>
static inline constexpr typename std::enable_if
<
    _Size == 0 ||
    is_swappable<_Tp>::value,
    void
>::type
swap(array<_Tp, _Size>& __x, array<_Tp, _Size>& __y) NOEXCEPT_(__x.swap(__y))
{
    __x.swap(__y);
}

template <size_t _Ip, class _Tp, size_t _Size>
static inline constexpr _Tp& get(array<_Tp, _Size>& __a) NOEXCEPT
{
    static_assert(_Ip < _Size, "Index out of bounds in std::get<> (std::array)");
    return __a.__elems_[_Ip];
}

template <size_t _Ip, class _Tp, size_t _Size>
static inline constexpr const _Tp& get(const array<_Tp, _Size>& __a) NOEXCEPT
{
    static_assert(_Ip < _Size, "Index out of bounds in std::get<> (const std::array)");
    return __a.__elems_[_Ip];
}

template <size_t _Ip, class _Tp, size_t _Size>
static inline constexpr _Tp&& get(array<_Tp, _Size>&& __a) NOEXCEPT
{
    static_assert(_Ip < _Size, "Index out of bounds in std::get<> (std::array &&)");
    return std::move(__a.__elems_[_Ip]);
}

template <size_t _Ip, class _Tp, size_t _Size>
static inline constexpr const _Tp&& get(const array<_Tp, _Size>&& __a) NOEXCEPT
{
    static_assert(_Ip < _Size, "Index out of bounds in std::get<> (const std::array &&)");
    return std::move(__a.__elems_[_Ip]);
}


template <typename _Tp, size_t _Size, size_t... _Index>
static inline constexpr array<std::remove_cv_t<_Tp>, _Size> 
__to_array_lvalue_impl(_Tp (&__arr)[_Size], std::index_sequence<_Index...>) NOEXCEPT_(is_nothrow_constructible_v<_Tp, _Tp&>) {
  return {{__arr[_Index]...}};
}

template <typename _Tp, size_t _Size, size_t... _Index>
static inline constexpr array<std::remove_cv_t<_Tp>, _Size>
__to_array_rvalue_impl(_Tp(&&__arr)[_Size], std::index_sequence<_Index...>) NOEXCEPT_(is_nothrow_move_constructible_v<_Tp, _Tp&>) {
  return {{std::move(__arr[_Index])...}};
}

template <typename _Tp, size_t _Size>
static inline constexpr array<std::remove_cv_t<_Tp>, _Size>
to_array(_Tp (&__arr)[_Size]) NOEXCEPT_(is_nothrow_constructible_v<_Tp, _Tp&>) {
  static_assert(
                !std::is_array<_Tp>::value,
      "to_array does not accept multidimensional arrays.");
  static_assert(
      is_constructible_v<_Tp, _Tp&>,
      "to_array requires copy constructible elements.");
  return __to_array_lvalue_impl(__arr, std::make_index_sequence<_Size>());
}

template <typename _Tp, size_t _Size>
static inline constexpr array<std::remove_cv_t<_Tp>, _Size>
to_array(_Tp(&&__arr)[_Size]) NOEXCEPT_(is_nothrow_move_constructible_v<_Tp>) {
  static_assert(
                !is_array_v<_Tp>,
      "to_array does not accept multidimensional arrays.");
  static_assert(
      is_move_constructible_v<_Tp>,
      "to_array requires move constructible elements.");
  return __to_array_rvalue_impl(std::move(__arr),
                                std::make_index_sequence<_Size>());
}



template<class T, size_t N>
static inline constexpr typename array<T, N>::iterator begin(array<T, N>& arr) NOEXCEPT { return arr.begin(); }
template<class T, size_t N>
static inline constexpr typename array<T, N>::iterator end(array<T, N>& arr) NOEXCEPT { return arr.end(); }
template<class T, size_t N>
static inline constexpr typename array<T, N>::const_iterator begin(const array<T, N>& arr) NOEXCEPT { return arr.begin(); }
template<class T, size_t N>
static inline constexpr typename array<T, N>::const_iterator end(const array<T, N>& arr) NOEXCEPT { return arr.end(); }

}


