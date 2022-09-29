#pragma once
#include <iterator>

#if (__cplusplus >= 201103L && defined(__cpp_exceptions))
/// \brief Defined noexcept specifier only if C++11 and exceptions are enabled
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

namespace estd {
  
  struct not_this_one {}; // Tag type for detecting which begin/ end are being selected

  // Import begin/ end from std here so they are considered alongside the fallback (...) overloads in this namespace
  namespace detail {
    using std::begin;
    using std::end;
    
    not_this_one begin( ... );
    not_this_one end( ... );

    /// \brief Check if a type meets the conditions of a range
   template<typename T> 
    struct is_range 
      : std::integral_constant<bool, 
                              !std::is_same<decltype(begin(std::declval<T&>())), not_this_one>::value &&
                              !std::is_same<decltype(end(std::declval<T&>())), not_this_one>::value> 
                                
    { };
  }
  
  template<class T> using is_range_t = typename std::enable_if<detail::is_range<T>::value, T>::type;
  
  /// \brief Implement std::conjunction for pre-C++17 libraries
  template<class...> struct conjunction : std::true_type { };
  template<class B1> struct conjunction<B1> : B1 { };
  template<class B1, class... Bn>
  struct conjunction<B1, Bn...> 
      : std::conditional_t<bool(B1::value), conjunction<Bn...>, B1> {};
    
      
   /// \brief Forward declaration of array and helper functions
   template <class _Tp, size_t _Size> struct array;
          
    template<class T, size_t N>
    static inline constexpr typename array<T, N>::iterator begin(array<T, N>& arr) NOEXCEPT;
    template<class T, size_t N>
    static inline constexpr typename array<T, N>::iterator end(array<T, N>& arr) NOEXCEPT;
    template<class T, size_t N>
    static inline constexpr typename array<T, N>::const_iterator begin(const array<T, N>& arr) NOEXCEPT;
    template<class T, size_t N>
    static inline constexpr typename array<T, N>::const_iterator end(const array<T, N>& arr) NOEXCEPT;

  /// \brief Helper template to confirm that type is a random access iterator
template<class Iterator>
inline constexpr bool is_random_access_iterator_v 
  = std::is_same<typename std::iterator_traits<Iterator>::iterator_category, std::random_access_iterator_tag>::value;

  template<class Iterator>
    static inline constexpr typename std::iterator_traits<Iterator>::difference_type distance(Iterator a, Iterator b)
  {
    return b - a;
  }
  
  template<class Iterator, class Distance>
  static inline constexpr void advance(Iterator a, Distance n)
  {
    return a + n;
  }
  
namespace detail {
  
  /// \brief Constexpr version of std::iter_swap, filling in for C++20 function
  template<class Iterator>
  static inline constexpr void iter_swap(Iterator a, Iterator b)
  {
      auto temp = std::move(*a);
      *a = std::move(*b);
      *b = std::move(temp);
  }

  
  /// \brief Helper function for heapifying a max heap
template <class RandomAccessIterator, class Compare>
static inline constexpr void
__sift_down(RandomAccessIterator first, typename std::iterator_traits<RandomAccessIterator>::difference_type len,
            RandomAccessIterator start, Compare comp)
{
  typedef typename std::iterator_traits<RandomAccessIterator>::difference_type difference_type;
  typedef typename std::iterator_traits<RandomAccessIterator>::value_type value_type;
    // left-child of __start is at 2 * __start + 1
    // right-child of __start is at 2 * __start + 2
    difference_type child = start - first;

    if (len < 2 || (len - 2) / 2 < child)
        return;

    child = 2 * child + 1;
    RandomAccessIterator cit = first + child;

    if ((child + 1) < len && comp(*cit, *(cit + 1)) ) {
        // right-child exists and is greater than left-child
        ++cit;
        ++child;
    }

    // check if we are in heap-order
    if (comp(*cit, *start))
        // we are, start is larger than it's largest child
        return;

    value_type top(std::move(*start));
    do
    {
      // we are not in heap-order, swap the parent with it's largest child
      *start = std::move(*cit);
      start = cit;

      if ((len - 2) / 2 < child)
        break;

      // Get next children
      child = 2 * child + 1;
      cit = first + child;

      if ((child + 1) < len && comp(*cit, *(cit + 1)) ) {
        // Right child is greatest, select it
        ++cit;
        ++child;
      }

        // check if we are in heap-order
    } while (!comp(*cit, top));
    // Move top value to current place in stack
    *start = std::move(top);
}

/// \brief Helper function for popping from a max heap
template <class RandomAccessIterator, class Compare>
static inline constexpr void
__pop_heap(RandomAccessIterator first, RandomAccessIterator last,
           typename std::iterator_traits<RandomAccessIterator>::difference_type len,
           Compare comp)
{
  if (len > 1)
  {
    iter_swap(first, last - 1);
    __sift_down(first, len - 1, first, comp);
  }
}

}

/// \brief constexpr version of std::make_heap, filling in for C++20 make_heap
template<class Iterator, class Compare>
static inline constexpr void make_heap(Iterator begin, Iterator end, Compare comp)
{
  static_assert(is_random_access_iterator_v<Iterator>, "Requires random access iterators");
  typedef typename std::iterator_traits<Iterator>::difference_type difference_type;
  auto const N = end - begin;
  if(N < 1) return;
   
  for(auto start = (N -2)/2; start >= 0; --start)
  {
    detail::__sift_down(begin, N, begin + start, comp);
  }
}

/// \brief constexpr version of std::sort_heap, filling in for C++20 sort_heap
template <class RandomAccessIterator, class Compare>
static inline constexpr void sort_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
  static_assert(is_random_access_iterator_v<RandomAccessIterator>, "Requires random access iterators");
  typedef typename std::iterator_traits<RandomAccessIterator>::difference_type difference_type;
    
  // Remove elements from heap one by one and place them at end
  for(difference_type n = last - first; n > 1; --last, (void) --n)
  {
    detail::__pop_heap(first, last, n, comp);
  }
}


/// \brief Constexpr version of std::sort, similar to C++20 std::sort. uses heapsort for reasonable worst-case performance
template<class Iterator, class Compare=std::less<typename std::iterator_traits<Iterator>::value_type> >
static inline constexpr void sort(Iterator begin, Iterator end, Compare comp=Compare())
{
  static_assert(is_random_access_iterator_v<Iterator>, "Requires random access iterators");
  make_heap(begin, end, comp);
  sort_heap(begin, end, comp);
}


/// \brief Constexpr version of std::sort, similar to C++20 std::sort
template <typename Range, class Compare=std::less<typename Range::value_type>, estd::is_range_t<Range>* = nullptr  >
static inline constexpr auto sort(Range&& range, Compare comp=Compare())
{
  sort(estd::begin(range), estd::end(range), comp);
  return std::move(range);
}

template<class ForwardIt, class T, class Compare>
constexpr ForwardIt lower_bound(ForwardIt first, ForwardIt last, const T& value, Compare comp)
{
    ForwardIt it;
    typename std::iterator_traits<ForwardIt>::difference_type count, step;
    count = distance(first, last);
 
    while (count > 0) {
        it = first;
        step = count / 2;
        std::advance(it, step);
        if (comp(*it, value)) {
            first = ++it;
            count -= step + 1;
        }
        else
            count = step;
    }
    return first;
}

template< class Iterator, class T, class Compare >
inline constexpr Iterator find_sorted( Iterator first, Iterator last, const T& value, Compare comp )
{
  first = lower_bound(first, last, value, comp);
  return comp(value, *first) ? last : first;
}


template< class Iterator, class UnaryPredicate >
inline constexpr Iterator find_if( Iterator first, Iterator last, UnaryPredicate pred)
{
  while(first < last && false == pred(*first)) ++first;
  return first;
}


template< class Iterator, class UnaryPredicate >
inline constexpr Iterator find_if_not( Iterator first, Iterator last, UnaryPredicate pred)
{
  while(first < last && true == pred(*first)) ++first;
  return first;
}


}