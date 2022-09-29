#pragma once

/// \file eformant.hpp
/// Simple formatting functions with high-performance, compact code for embedded systems that does not require
/// dynamic allocation or exceptions

#include "eio.hpp"
#include "estd.hpp"
#include "span.hpp"

namespace eformat {
  typedef eio::buffer buffer;
  typedef estd::string_view string_view;
  using estd::char_type;
  typedef uint32_t size_type;
  
  /// Format option to specify field alignment
  enum class Align { Left = 0, Right=1, Center=2  };
  /// Format option to specify numeric base
  enum class Base  { Decimal = 0, Hex=1, Binary=2 };
  
  /// \brief Field formatting options
  struct Options 
  {
    Align   align  : 2;
    Base    base   : 2;
    uint32_t width : 7;
  };
  
  enum ParseStatus
  {
    OK,
    Incomplete,
    NotMatched,
    Overflow,
  };
  
  /// \defgroup BasicFormat Basic formatting functions that are the foundation of this formatting code
  /// @{
  int format_decimal(char* out, uint16_t size, uint32_t value, Options fmt=Options{}) NOEXCEPT;
  int format_decimal(char* out, uint16_t size, int32_t value, Options fmt=Options{})  NOEXCEPT;
  int format_hex(char* out, uint16_t size, uint32_t value, Options fmt=Options{}) NOEXCEPT;
  int format_binary(char* out, uint16_t size, uint32_t value, Options fmt=Options{}) NOEXCEPT;

  int format(buffer& out, uint8_t value, Options options) NOEXCEPT;
  int format(buffer& out, uint16_t value, Options options) NOEXCEPT;
  int format(buffer& out, uint32_t value, Options options) NOEXCEPT;
  int format(buffer& out, int8_t value, Options options) NOEXCEPT;
  int format(buffer& out, int16_t value, Options options) NOEXCEPT;
  int format(buffer& out, int32_t value, Options options) NOEXCEPT;
  int format(buffer& out, string_view value, Options options) NOEXCEPT;
  int format(buffer& out, const void* const value, Options options) NOEXCEPT;
  int format(buffer& out, bool value, Options options) NOEXCEPT;
  /// @}
            
  template<class T> struct false_type { static constexpr bool value = false; };
  template<class T> struct true_type { static constexpr bool value = true; typedef T type; };
    
  /// brief Template to determine if the specified type is a custom type (requiring a custom formatter)
  template<class T> struct is_custom_type       : true_type<T> {};
  template<> struct is_custom_type<uint8_t>     : false_type<uint8_t>  {};
  template<> struct is_custom_type<uint16_t>    : false_type<uint16_t> {};
  template<> struct is_custom_type<uint32_t>    : false_type<uint32_t> {};
  template<> struct is_custom_type<int8_t>      : false_type<int8_t>   {};
  template<> struct is_custom_type<int16_t>     : false_type<int16_t>  {};
  template<> struct is_custom_type<int32_t>     : false_type<int32_t>  {};
  template<> struct is_custom_type<bool>        : false_type<bool>     {};
  template<> struct is_custom_type<char>        : false_type<char>     {};
  template<> struct is_custom_type<string_view> : false_type<string_view> {};
  template<> struct is_custom_type<void*>       : false_type<void*> {};
  template<> struct is_custom_type<const void*> : false_type<void*> {};
  
  template<class T> using custom_type_t = typename is_custom_type<T>::type;
  
  /// \brief Context for parsing format string
  struct parse_context
  {
    const char* fmt_pos;
    const char* fmt_end;
  };
  
  /// \brief Default formatter implementation
  struct base_formatter
  {
    Options          options;
    
    constexpr base_formatter()  NOEXCEPT : options{} {}
    
    /// \brief Parse format string and setup this formatter
    constexpr bool parse_options(parse_context& c)  NOEXCEPT
    {
      options.width = 0;
      const char* pos = c.fmt_pos;
      if(*pos != '{') return false;
      ++pos;
      for(; pos < c.fmt_end; ++pos)
      {
        switch(*pos)
        {
        case '<': options.align = Align::Left; break;
        case '^': options.align = Align::Center; break;
        case '>': options.align = Align::Right; break;
        case 'd': options.base = Base::Decimal; break;
        case 'x': options.base = Base::Hex; break;
        case 'b': options.base = Base::Binary; break;
        case '}': c.fmt_pos = ++pos; return true;
        default:
          if(std::isdigit(*pos)) options.width = options.width * 10 + *pos - '0';
          else return false;
          break;
        }
      }
      return false;
    }
  };
  
  /// \brief Default formatter definitions for builtin types
  template<class T> struct formatter : base_formatter
  {
    int format(buffer& ctx, const T& value) const  NOEXCEPT
    {
      static_assert(!is_custom_type<T>::value, "A custom formatter must be provided for this type");
      return ::eformat::format(ctx, value, options);
    }
  };
  
  /// \brief Pointer specialization for void* pointer
  template<> 
  struct formatter<void*> : base_formatter
  {
    int format(buffer& ctx, const void* value) const  NOEXCEPT
    {
      return ::eformat::format(ctx, value, options);
    }
  };
  
  /// \brief Boxed argument value with format function
  struct arg_value 
  {
  public:
      /// \brief Pointer to argument
      const void* ptr;
      /// \brief Formatting function
      bool (*fformat)(parse_context& parse_ctx, const void* arg, buffer& ctx);
      
      __FORCEINLINE constexpr arg_value(const int8_t& i)  NOEXCEPT   : ptr(&i), fformat(format_arg<int8_t>){ }
      __FORCEINLINE constexpr arg_value(const int16_t& i)  NOEXCEPT  : ptr(&i), fformat(format_arg<int16_t>) { }
      __FORCEINLINE constexpr arg_value(const int32_t& i)  NOEXCEPT : ptr(&i), fformat(format_arg<int32_t>) { }
      __FORCEINLINE constexpr arg_value(const uint8_t& i)  NOEXCEPT : ptr(&i), fformat(format_arg<uint8_t>) { }
      __FORCEINLINE constexpr arg_value(const uint16_t& i) NOEXCEPT : ptr(&i), fformat(format_arg<uint16_t>) {}
      __FORCEINLINE constexpr arg_value(const uint32_t& i) NOEXCEPT : ptr(&i), fformat(format_arg<uint32_t>) {}
      __FORCEINLINE constexpr arg_value(const bool& i)  NOEXCEPT    : ptr(&i), fformat(format_arg<bool>) {}
      __FORCEINLINE constexpr arg_value(const char& i)  NOEXCEPT    : ptr(&i), fformat(format_arg<char>) {}
      __FORCEINLINE constexpr arg_value(string_view& i)  NOEXCEPT   : ptr(&i), fformat(format_arg<string_view>) {}
      __FORCEINLINE constexpr arg_value(const void* i)  NOEXCEPT    : ptr(i), fformat(format_arg<const void*>){}
      #ifdef __ICCARM__
      __FORCEINLINE 
      template<class T, typename = custom_type_t<T>>
      constexpr arg_value(const T& arg) NOEXCEPT : ptr(&arg), fformat(format_arg<T>) {}
      #else  
      template<class T, typename = custom_type_t<T>>
      __FORCEINLINE constexpr arg_value(const T& arg) NOEXCEPT : ptr(&arg), fformat(format_arg<T>) {}
      #endif
        
      /// \brief Format this argument
      bool format(parse_context& parse, buffer& fmt) const  NOEXCEPT;
      
    private:

    /// Wraper for formatter class to freestanding function
    template <typename T>
    static bool format_arg(parse_context& parse_ctx, const void* arg, buffer& ctx) 
    {
      formatter<T> f{};
      if(f.parse_options(parse_ctx) &&
         f.format(ctx, *reinterpret_cast<const T*>(arg)) >= 0)
        return true;
      else
        return false;
     }
    };
  
  /// \brief Base class representing an argument pack to be used by the formatter
  struct basic_format_args
  {   
    const arg_value* start_;
    const arg_value* end_;
    
    __FORCEINLINE const arg_value* begin() const  NOEXCEPT { return start_; }
    __FORCEINLINE const arg_value* end() const  NOEXCEPT { return end_; }
  };
  
  /// \brief Structure to package arguments for use by the formatter
  template<uint16_t N>
  struct arg_store : basic_format_args
  {
  public:
    
     __FORCEINLINE
    template<class... T>
    constexpr arg_store(const T&... args)  NOEXCEPT
      :  basic_format_args{ &values[0], &values[0] + N}, values{args...}
    {
      static_assert(sizeof...(T) <= N, "Too many args passed to format");
    }
      
  private:
    arg_value values[N];
  };
  
  /// \brief Takes packaged arguments and performs formatting
  bool vformat_to(buffer& buffer, string_view fmt, const basic_format_args& args)  NOEXCEPT;
  
  /// \brief Format to specified IO buffer
  /// \returns True if successful, false on error
  template<class... Ts>
  inline bool format_to(buffer& buffer, string_view fmt, Ts... ts)  NOEXCEPT
  { 
    arg_store<sizeof...(Ts)> fmt_args{ts...};
    return vformat_to(buffer, fmt, fmt_args);
  }
  
  /// \brief Print formatted string to specified IO device
  /// \returns True if successful, false on error
  template<class... Ts>
  inline bool print(eio::IODevice& device, string_view fmt, Ts... ts)  NOEXCEPT
  {
    auto& buffer = device.getbuf();

      bool status = format_to(buffer, fmt, ts...);
      buffer.flush();
      return status;
  }
  
  /// \brief Simplified stream object similar to std::ostream
  struct stream
  {
    stream(eio::IODevice& device) NOEXCEPT
      : o(), buf(device.getbuf())
      {}
      
    Options o;
    buffer& buf;
    
    void set(Base base)  NOEXCEPT
    {
      o.base = base;
    }
    void set(Align align) NOEXCEPT
    {
      o.align = align;
    }
    
    uint8_t width(uint8_t width) NOEXCEPT
    {
      uint8_t w = o.width;
      o.width = width;
      return w;
    }
    
    stream& flush() NOEXCEPT
    {
      buf.flush();
      return *this;
    }

    stream& sync(int timeout = 100000) NOEXCEPT
    {
      buf.sync(timeout);
      return *this;
    }

    stream& write(string_view view) NOEXCEPT
    {
      buf.sputn(view.data(), view.size());
      return *this;
    }

    stream& put(char c) NOEXCEPT
    {
      buf.sputc(c); return *this;
    }
  };

  /// \brief dummy tag type to pass to indicate a buffer flush
  static inline stream& flush(stream& s) NOEXCEPT { return s.flush(); }
  /// \brief dummy tag type to pass to indicate a newline flush
  static inline stream& endl(stream& s) NOEXCEPT { s.buf.sputc('\n'); return s.flush(); }

   /// \brief dummy tag type to pass to indicate a newline flush
  static inline stream& sync(stream& s) NOEXCEPT { return s.sync(); }

  /// \brief dummy tag type to pass to indicate a newline flush
  static inline stream& syncl(stream& s) NOEXCEPT { s.buf.sputc('\n'); return s.sync(); }

  inline stream& operator<<(stream& s, stream& (*fptr)(stream&)) NOEXCEPT { return fptr(s); }
  
  /// \brief Wrapper for values that should not be formatted
  struct unformatted_string_view { const string_view& value; };
  
  /// \brief Function to indicate value should not be formatted
  inline constexpr unformatted_string_view unformatted(const string_view& value) NOEXCEPT 
  { return unformatted_string_view{value}; }

  struct padded_string { const string_view& value; uint8_t width; };

  inline constexpr padded_string padded(const string_view& value, uint8_t field_width) NOEXCEPT 
  { return padded_string{value, field_width}; }

    /// \brief Format string 
    stream& operator<<(stream& s, const string_view value) NOEXCEPT;
    stream& operator<<(stream& s, const int32_t value) NOEXCEPT;
    stream& operator<<(stream& s, const uint32_t value) NOEXCEPT;
    stream& operator<<(stream& s, const int16_t value) NOEXCEPT;
    stream& operator<<(stream& s, const uint16_t value) NOEXCEPT;
    stream& operator<<(stream& s, const int8_t value) NOEXCEPT;
    stream& operator<<(stream& s, const uint8_t value) NOEXCEPT;
    
    /// \brief Write character to buffer (unformatted)
    stream& operator<<(stream& s, const char value) NOEXCEPT;


    /// \brief Unformatted string writes directly to buffer
    inline stream& operator<<(stream& s, unformatted_string_view value) NOEXCEPT
    {
      s.buf.sputn(value.value.data(), value.value.size());
      return s;
    }

    /// \brief Unformatted string writes directly to buffer
    inline stream& operator<<(stream& s, padded_string value) NOEXCEPT
    {
      uint8_t oldwidth = s.width(value.width);
      s << value.value;
      s.width(oldwidth);
      return s;
    }
  
    /// \brief Wrapper for range to be formatted
    template<class Iterator>
    struct fmt_range
    {
      Iterator begin;
      Iterator end;
      char delim;
    };

    /// \brief Function to create range formatter for insertion into stream
    /// \param begin Iterator to beginning of range
    /// \param end Iterator to end of range
    /// \param delim Optional delimiting character
    template<class Iterator>
    inline fmt_range<Iterator> range(Iterator begin, Iterator end, char delim=' ')
    {
      return fmt_range<Iterator>{begin, end, delim};
    }
    
    /// \brief Function to create range formatter from container for insertion into stream
    /// \param c Container to print, must have begin() and end() functions returning input interators
    template<class Container, typename Iterator= typename Container::const_iterator>
    inline fmt_range<Iterator> range(const Container& c, char delim=' ')
    {
      return fmt_range<Iterator> {c.begin(), c.end(), delim};
    }
    
    /// \brief Operator to format range of values, with delimiters between
    template<class Iterator>
    inline stream& operator<<(stream& s, const fmt_range<Iterator>& value) NOEXCEPT {
      Iterator it = value.begin;
      while(it != value.end)
      {
        s << *it++;
        if(it != value.end) s << value.delim;
      }
      return s;
    }
        
    namespace color {

      struct Code
      {
        const char code[2];
        constexpr Code(char const str[3])
          : code{str[0], str[1]}
          {}
      };

      namespace foreground {
        static constexpr Code Red = "31";
        static constexpr Code Green = "32";
        static constexpr Code Blue = "34";
        static constexpr Code Default = "39";
      };

      namespace background {
        static constexpr Code Red = "31";
        static constexpr Code Green = "32";
        static constexpr Code Blue = "34";
        static constexpr Code Default = "39";
      };
      
      stream& operator<<(stream& os, Code mod) NOEXCEPT;
    }

    /// \brief Match input buffer contents with a given string
    ParseStatus match(const string_view& buf, const string_view& str);
      
    ParseStatus parse(string_view& in, uint8_t& value)  NOEXCEPT;
    ParseStatus parse(string_view& in, uint16_t& value) NOEXCEPT;
    ParseStatus parse(string_view& in, uint32_t& value) NOEXCEPT;
    ParseStatus parse(string_view& in, int8_t& value)   NOEXCEPT;
    ParseStatus parse(string_view& in, int16_t& value)  NOEXCEPT;
    ParseStatus parse(string_view& in, int32_t& value)  NOEXCEPT;
    ParseStatus parse(string_view& in, bool& value)     NOEXCEPT;
    ParseStatus parse(string_view& in, estd::span<char_type>& value, char_type delimeter) NOEXCEPT;
  
    template<class Predicate=bool (*)(char ch)>
    inline constexpr ParseStatus parse(string_view& in, estd::span<char_type>& value, Predicate pred=estd::isspace) NOEXCEPT
    {
      auto begin = value.begin();
      auto pos = begin;
      auto end = value.end();
      
      auto c = in.cbegin();
      
      while(false == pred(*c)) {
        if(pos == end) return ParseStatus::Overflow;
        if(c == in.cend()) return ParseStatus::Incomplete;
        *pos++ = *c++;  
      }
      
      // Consume characters from string_view and resize span to represent actual size
      auto count = pos - begin;
      value = value.first(count);
      in.remove_prefix(count);
      
      return ParseStatus::OK;
    }
}
