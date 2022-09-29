#pragma once

/// \file estring.hpp
/// Implements string_view, span, and fixed string_buffer objects for embedded systems
/// string_view is a simplified version of C++17 string_view class

// For standard integer types
#include <cstdint>
// For character manipulation
//#include <cctype>
// For integer_sequence
#include <utility>
// For iterator traits
#include <iterator>
// For memcpy
#include <cstring>

#include "estd.hpp"

#if defined(__cpp_exceptions)
#include <stdexcept>
/// \brief Define check with out_of_range exception, if exceptions are enabled
#define CHECK(...) if(false == (__VA_ARGS__)) throw std::out_of_range(__func__);
/// \brief Define pointer check with invalid_argument exception, if exceptions are enabled
#define CHECKPTR(PTR) if(nullptr == PTR) throw std::invalid_argument(__func__);
#else
#include <cassert>
#define CHECK(...) assert(__VA_ARGS__)
#define CHECKPTR(PTR) assert(nullptr != PTR)
#endif

#if defined(__ICCARM__)
  #define __FORCEINLINE _Pragma("inline=forced")
#else
  /// \brief Define directive to force-inline small functions
  #define __FORCEINLINE [[gnu::always_inline]]
#endif


namespace estd
{
  /// \brief Character type to be used for this application. Usually char on embedded systems
 typedef char char_type;
 
 /// \brief Test if character is whitespace
 __FORCEINLINE static inline constexpr bool isspace(char _C) NOEXCEPT
 {
    return (_C >= '\x09' && _C <= '\x0d') || _C == ' ';
 }
 
 /// \brief Test if character is a space other than newline
 __FORCEINLINE static inline constexpr bool isblank(char _C) NOEXCEPT
 {
    return _C == '\x09' || _C == ' ';
 }
 
 /// \brief Test if character is a separator that can be used to 
 __FORCEINLINE static inline constexpr bool issep(char _C) NOEXCEPT
 {
   return _C == '.' || _C == ':' || _C == '/' || _C == '\\';
 }

 /// \brief Return the smaller of two values
 __FORCEINLINE static inline constexpr uint16_t min(uint16_t l, uint16_t r) NOEXCEPT
 {
   return l < r ? l : r;
 }
  /// \brief Return the smaller of two values
  __FORCEINLINE static inline constexpr uint32_t min(uint32_t l, uint32_t r) NOEXCEPT
 {
   return l < r ? l : r;
 }
  /// \brief Return the smaller of two values
 __FORCEINLINE static inline constexpr char_type min(char_type l, char_type r) NOEXCEPT
 {
   return l < r ? l : r;
 }

 /// \brief Convert a character to lower-case
 static constexpr char_type tolower(char_type c) NOEXCEPT
 {
     return c >= 'A' && c <= 'Z' ? 
       c + ('a' - 'A')
       : c;
 }
 
/// \brief Non-owning string view, simplified implementation of std::string_view
struct string_view
{
    typedef char_type  value_type;

    typedef value_type       * pointer;
    typedef value_type const * const_pointer;
    typedef value_type       & reference;
    typedef value_type const & const_reference;

    typedef const_pointer iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator< const_iterator > reverse_iterator;
    typedef std::reverse_iterator< const_iterator > const_reverse_iterator;

    typedef std::uint32_t   size_type;
    typedef std::ptrdiff_t  difference_type;
    
    /// \brief Create string view with no contents
    constexpr string_view() NOEXCEPT = default;

    /// \brief Create view from string literal
    template <size_type N> constexpr string_view(const value_type (&lit)[N]) NOEXCEPT 
      : data_(&lit[0]), size_( lit[N - 1] == '\0' ? N - 1 : N) {}
    
    /// \brief Create view from data & size
    constexpr string_view(const value_type* data, size_type count)  NOEXCEPT : data_(data), size_(count) {}
  
    /// \brief construct string_view from a pair of iterators
    template<class It>
    constexpr string_view(It fiter, It liter) NOEXCEPT : data_(fiter), size_(liter-fiter) {}

    /// Copy view (does not copy underlying data)
    constexpr string_view(string_view const & other ) NOEXCEPT = default;

    constexpr string_view(string_view&& other ) NOEXCEPT = default;

    /// Assign view (does not copy underlying data)
    constexpr string_view & operator=( string_view const & other ) NOEXCEPT = default;
    
    /// \defgroup IteratorAccess Functions that return iterators
    /// @{
    __FORCEINLINE constexpr const_iterator begin()  const NOEXCEPT { return data_;         }
    __FORCEINLINE constexpr const_iterator end()    const NOEXCEPT { return data_ + size_; }
    __FORCEINLINE constexpr const_iterator cbegin() const NOEXCEPT { return begin(); }
    __FORCEINLINE constexpr const_iterator cend()   const NOEXCEPT { return end();   }
    /// @}

    __FORCEINLINE constexpr size_type size()     const NOEXCEPT { return size_; }
    __FORCEINLINE constexpr size_type length()   const NOEXCEPT { return size_; }

    constexpr const_reference front() const  NOEXCEPT { return at( 0 );          }
    constexpr const_reference back()  const  NOEXCEPT { return at( size() - 1 ); }
    __FORCEINLINE constexpr const_pointer data() const  NOEXCEPT { return data_; }
    __FORCEINLINE constexpr bool empty() const NOEXCEPT { return size_ == 0; }
    
    constexpr void remove_prefix(size_type n) NOEXCEPT
    {
      if(n <= size_)
      {
        data_ += n;
        size_ -= n;
      }
    }
    
    constexpr void remove_suffix(size_type n) NOEXCEPT
    {
      if(n <= size_)
      {
        size_ -= n;
      }
    }
    
    /// Checked access to elements
    constexpr const_reference at( size_type pos ) const NOEXCEPT
    {
      CHECKPTR(data_);
      CHECK(pos < size_);
      return data_[pos];
    }
    
    /// Access to elements
    constexpr const_reference operator[]( size_type pos ) const NOEXCEPT
    {
      return data_[pos];
    }
    
    /// \brief Compare strings stored in string_views
    constexpr int compare( const string_view& other ) const NOEXCEPT
    {
      auto n = size() < other.size() ? size() : other.size();
      auto s1 = cbegin();
      auto s2 = other.cbegin();
      while ( n-- != 0 )
      {
        if ( tolower(*s1) < tolower(*s2) ) return -1;
        if ( tolower(*s1++) > tolower(*s2++) ) return 1;
      }
      return size() == other.size() ? 0 : size() < other.size() ? -1 : 1;
    }
    
    /// \brief Determines if this string_view starts with a given string
    constexpr bool starts_with(const string_view& other) const NOEXCEPT
    {
      if(other.size() > size_) return false;
      auto n = other.size();
      auto s1 = cbegin();
      auto s2 = other.cbegin();
      while(n-- != 0)
      {
        if(tolower(*s1++) != tolower(*s2++)) return false;
      }
      return true;
    }
private:
    const_pointer data_ = nullptr;
    size_type     size_ = 0;
};

constexpr bool operator== (const string_view& lhs, const string_view& rhs) NOEXCEPT
{ return lhs.compare( rhs ) == 0 ; }

constexpr bool operator!= (const string_view& lhs, const string_view& rhs) NOEXCEPT
{ return lhs.compare( rhs ) != 0 ; }

constexpr bool operator<  (const string_view& lhs, const string_view& rhs) NOEXCEPT
{ return lhs.compare( rhs ) < 0 ; }

constexpr bool operator<= (const string_view& lhs, const string_view& rhs) NOEXCEPT
{ return lhs.compare( rhs ) <= 0 ; }

constexpr bool operator>  (const string_view& lhs, const string_view& rhs) NOEXCEPT
{ return lhs.compare( rhs ) > 0 ; }

constexpr bool operator>= (const string_view& lhs, const string_view& rhs) NOEXCEPT
{ return lhs.compare( rhs ) >= 0 ; }

/// \brief Helper function to create string view from literal
template<size_t NPlusOne>
static inline constexpr auto make_string_view(const char_type (&lit)[NPlusOne]) NOEXCEPT -> string_view
{
  return string_view(lit);
}

static inline constexpr auto view_from_cstring(const char_type *str, size_t length) NOEXCEPT -> string_view
{
  size_t i=0;
  while(i < length && str[i] != '\0') ++i;
  return string_view(&str[0], i);
}


/// \brief Base class for string buffers. 
/// \remarks Uses a size-1 array hack since C++ does not yet support flexible array members as standard.
struct string_buffer
{
  typedef char_type  value_type;

  typedef value_type       * pointer;
  typedef value_type const * const_pointer;
  typedef value_type       & reference;
  typedef value_type const & const_reference;

  typedef pointer       iterator;
  typedef const_pointer const_iterator;
  typedef std::reverse_iterator< const_iterator > reverse_iterator;
  typedef std::reverse_iterator< const_iterator > const_reverse_iterator;

  typedef std::uint16_t  size_type;
  typedef std::ptrdiff_t difference_type;

  size_type  capacity_;
  size_type  size_;
  value_type buffer_[1];

  /// \defgroup IteratorAccess Functions that return iterators
  /// @{
  constexpr iterator begin()  NOEXCEPT { return buffer_; }
  constexpr iterator end()    NOEXCEPT { return buffer_+size_; }
   
  constexpr const_iterator begin() const NOEXCEPT { return buffer_; }
  constexpr const_iterator end()   const NOEXCEPT { return buffer_+size_; }
  
  constexpr const_iterator cbegin() const NOEXCEPT { return buffer_; }
  constexpr const_iterator cend()   const NOEXCEPT { return buffer_+size_; }
  /// @}

  constexpr size_type size() const NOEXCEPT { return size_; }
  constexpr size_type capacity() const NOEXCEPT { return capacity_; }

  constexpr pointer data() NOEXCEPT { return buffer_; }
  constexpr const_pointer data() const NOEXCEPT { return buffer_; }

   /// \brief Set contents of buffer from iterator and count
  bool set(const_pointer begin, size_type count) NOEXCEPT
  {
    if(count > capacity_) return false;
    size_ = count;

    auto ptr = &buffer_[0];
    memcpy(ptr, begin, count);
    return true;
  }

  
  template <class RandomAccessIterator> 
  bool set(RandomAccessIterator begin, RandomAccessIterator end, std::random_access_iterator_tag) NOEXCEPT
  {
    return set(&(*begin), size_type(estd::distance(begin, end)));
  }

  template <class InputIterator>
  bool set(InputIterator begin, InputIterator end, std::input_iterator_tag) NOEXCEPT
  {
    auto count = estd::distance(begin, end);
    if(count > capacity_) return false;
    size_ = count;
    auto ptr = &buffer_[0];
    while(count-- > 0) *ptr++ = *begin++;
    return true;
  }

    /// \brief Set string contents using pair of iterators
    template <class Iterator> 
    bool set(Iterator begin, Iterator end) NOEXCEPT
    {
      return set(begin, end, std::iterator_traits<Iterator>::iterator_category);
    }

    /// \brief Assign string contents from other container
    template<class Container, estd::is_range_t<Container>* = nullptr > 
    string_buffer& operator=(const Container& other) NOEXCEPT
    {
      set(other);
      return *this;
    }

  /// \brief set string contents from source container
  template<class Container> 
  bool set(const Container& container) NOEXCEPT
  {
    return set(container.data(), container.size());
  }
};

/// \brief A fixed size string buffer that can store variable-sized strings up to its maximum size
template<uint8_t N>
struct static_string_buffer : string_buffer
{
  value_type buffer_n_[N-1];
  static_assert(offsetof(static_string_buffer, buffer_n_) == offsetof(string_buffer, buffer_) + 1, "Array hack storage not contiguous!");

    /// \brief Construct empty string buffer
    constexpr static_string_buffer() NOEXCEPT : string_buffer{N, 0, 0} {}
    
    
    /// \brief Construct buffer using contents of other container
    template<class Container, estd::is_range_t<Container>* = nullptr > 
    constexpr static_string_buffer( Container const & other ) NOEXCEPT 
      : string_buffer{N, other.size(), other.data()[0]}
    {
      CHECK_BOUNDS("static_string_buffer", other.size() < N);
      auto ptr = ++other.data();
      for(size_type i=1; i < other.size(); ++i) buffer_n_[i-1] = *ptr++;
    }
      
    /// \brief Construct buffer using contents of string literal
    template <size_type N1> 
    constexpr static_string_buffer(const char (&lit)[N1]) NOEXCEPT
      : static_string_buffer(lit, std::make_integer_sequence<size_type, N1-1>{})
    {
      static_assert(N1-1 < N, "String literal must be smaller than buffer");
    }
    
    /// \brief Constexpr assignment operator is possible with subclass
    template<class Container, estd::is_range_t<Container>* = nullptr > 
    constexpr static_string_buffer& operator=(const Container& other) NOEXCEPT
    {
      auto ptr = other.data();
      buffer_[0] = *ptr++;
      for(size_type i=1; i < other.size(); ++i) buffer_n_[i-1] = *ptr++;
      return *this;
    }
  
  private:
    /// \brief Helper to enable constexpr construction from string literal
    template<size_type N1, size_type... Ns>
    constexpr static_string_buffer(const char (&lit)[N1], std::integer_sequence<size_type, 0, Ns...>) NOEXCEPT
      : string_buffer{N, N1-1, lit[0]}, buffer_n_{lit[Ns]...}
    {}
    
};


  /// \brief Remove characters at beginning of string matching condition
  /// \tparam Predicate type of predicate callable
  /// \param str View to modify
  /// \param pred Predicate callable, taking a character and returning a boolean
  template<class Predicate>
  constexpr inline string_view& trim_prefix(string_view& str, Predicate pred=isspace) NOEXCEPT
  {
    auto trim_to = estd::find_if_not(str.begin(), str.end(), pred);
    str.remove_prefix(trim_to - str.begin());
    return str;  
  }
  
  /// \brief Remove characters at end of string matching condition
  /// \tparam Predicate type of predicate callable
  /// \param str View to modify
  /// \param pred Predicate callable, taking a character and returning a boolean
  template<class Predicate>
  constexpr inline string_view& trim_suffix(string_view& str, Predicate pred=isspace) NOEXCEPT
  {
    auto it= str.end();
    for(;it > str.begin() && pred(*(it-1)); --it)
    { }
   
    str.remove_suffix(str.end() - it);
    return str;
  }
  
  /// \brief Return view to next token and advance input view
  /// \tparam Predicate type of predicate callable
  /// \param str View of string to extract token from, modified to refer to remaining string after token is extracted
  /// \param pred Predicate callable, taking a character and returning a boolean
  /// \returns string_view with token extracted from input string
  template<class Predicate>
  constexpr inline string_view next_token(string_view& str, Predicate pred) NOEXCEPT
  {
    // Trim delimiters from start
    trim_prefix(str, pred);
        
    auto start = str.begin();
    auto stop = str.end();
    
    // Search for end delimeter
    auto loc = estd::find_if(start, stop, pred);
    
    // Consume token from input view
    if(loc < stop) str.remove_prefix(loc - start + 1);
    else str.remove_prefix(loc - start);
                    
    // Set current token view
    return string_view{start, loc};
  }
}