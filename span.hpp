#pragma once

/// \file span.hpp Implements span class similar to std::span

#include <iterator>

namespace estd {

/// \brief similar to std::span, a type that represents a view of a contiguous data structure (e.g. an array)
template<class T>
struct span
{
    typedef T value_type;

    typedef value_type       * pointer;
    typedef value_type const * const_pointer;
    typedef value_type       & reference;
    typedef value_type const & const_reference;

    typedef pointer       iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator< const_iterator > reverse_iterator;
    typedef std::reverse_iterator< const_iterator > const_reverse_iterator;

    typedef std::uint32_t   size_type;
    typedef std::ptrdiff_t  difference_type;
    
    /// \brief Get a subspan from this span, in range (offset, offset+count)
    constexpr span subspan(size_type offset, size_type count) const NOEXCEPT
    { 
      return offset + count < size_ ? span{data_ + offset, count} : span{nullptr, 0};
    }
    
    /// \brief Get a span with the first count items from this span
    constexpr span first(size_type count) const NOEXCEPT
    {
      return span{data_, min(count, size_)};
    }
    
    /// \brief Get a span with the last count items from this span
    constexpr span last(size_type count) const NOEXCEPT
    {
      auto newsize = min(count, size_);
      return span{data_ + (size_-newsize), newsize};
    }
    
    /// \brief Get a item from this span by index
    constexpr reference operator[](size_type idx) const NOEXCEPT
    {
      CHECK(idx < size_);
      return data_[idx];
    }
    
    /// \defgroup IteratorAccess Functions that return iterators
    /// @{
    constexpr iterator begin()  NOEXCEPT { return data_; }
    constexpr iterator end()    NOEXCEPT { return data_+size_; }
   
    constexpr const_iterator begin() const NOEXCEPT { return data_; }
    constexpr const_iterator end()   const NOEXCEPT { return data_+size_; }
    
    constexpr const_iterator cbegin() const NOEXCEPT { return data_; }
    constexpr const_iterator cend()   const NOEXCEPT { return data_+size_; }
    /// @}

    /// \brief Get actual size of string contained in this buffer
    constexpr size_type size()   const NOEXCEPT { return size_; }
    /// \brief Return true if range is empty
    constexpr bool empty() const NOEXCEPT { return size_ == 0; }
    /// \brief Return a reference to the first character in this range
    constexpr const_reference front() const NOEXCEPT { return data_[0]; }
    /// \brief Return a const pointer to the data referred to by this range
    constexpr const_pointer   data()  const NOEXCEPT { return data_; }
   
    /// \brief Set string contents from an iterator and a count
    constexpr bool set(const_pointer begin, size_type count) NOEXCEPT
    {
      if(count > size_) return false;
      memcpy(data_, begin, count);
      /*
      size_ = count;
      pointer data = data_;
      pointer newend = data + count;
      while(data < newend) *data++ = *begin++;
      */
      return true;
    }
   
    /// \brief Set string contents using pair of iterators
    template <class Iterator> constexpr bool set(Iterator begin, Iterator end) NOEXCEPT
    {
      return set(begin, end-begin);
    }
    
    /// \brief set string contents from source container
    template<class Container, estd::is_range_t<Container>* = nullptr > 
    constexpr bool set(const Container& container) NOEXCEPT
    {
      return set(container.begin(), container.end());
    }
    
    /// \brief Assign string contents from other container
    template<class Container, estd::is_range_t<Container>* = nullptr > 
    constexpr span& operator=(const Container& other) NOEXCEPT
    {
      set(other);
      return *this;
    }
    
    template< class It >
    constexpr span( It first, It last, std::random_access_iterator_tag)
      : data_(first), size_(last - first)
      {}
      
    template< class It >
    constexpr span( It first, size_type size, std::random_access_iterator_tag)
      : data_(first), size_(size)
      {}
      
    /// \brief Create a span from a pair of random access iterators
    template< class It, class category = typename std::iterator_traits<It>::iterator_category>
    constexpr span( It first, It last )
      : span(first, last, category{})
      {}
      
    /// \brief create a span from a random access iterator + size
    template< class It, class category = typename std::iterator_traits<It>::iterator_category >
    constexpr span( It first, size_type size)
      : span(first, size, category{})
      {}
      
private:
  pointer   data_;
  size_type size_;
};

}