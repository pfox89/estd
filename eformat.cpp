#include "eformat.hpp"

#include <arm_acle.h> // __clz intrinsic

namespace {
  using namespace eformat;
  
  using estd::char_type;
  
  
    static constexpr string_view true_string = "true";
    static constexpr string_view false_string = "false";
    
    char* fill(char* out, size_type count, char_type fillchar)
    {
      while(count-- > 0) {
        *out++ = fillchar;
      }
      return out;
    }
    
    
  __FORCEINLINE
  template<class T>
  T max(const T l, const T r)  NOEXCEPT
  {
    return l > r ? l : r;
  }
  
  template<class T>
    T abs(T value)  NOEXCEPT { return value > 0 ? value : -value; }
  
  inline constexpr uint32_t pow10(uint32_t value)  NOEXCEPT
  {
    if(value == 0) return 1;
    else return 10 * pow10(value-1);
  }
  
  struct digits
  {
    bool    negative;
    uint8_t number;
  };
  
  uint32_t constexpr digits(uint32_t v)  NOEXCEPT
  {
     return 1 + ((v >= 1000000000u) ? 9 : (v >= 100000000u) ? 8 : 
                (v >= 10000000u) ? 7 : (v >= 1000000u) ? 6 : 
                (v >= 100000u) ? 5 : (v >= 10000u) ? 4 :
                (v >= 1000u) ? 3 : (v >= 100u) ? 2 : (v >= 10u) ? 1u : 0u); 
  }
  
   static constexpr uint32_t digit_values[] = {
       0,
       1,
       10,
       100,
       1000,
       10000,
       100000,
       1000000,
       10000000,
       100000000,
       1000000000  
   };
   
   static constexpr uint8_t digits_by_leading_zeroes[] = {
       digits(UINT32_MAX),
       digits(UINT32_MAX >> 1),
       digits(UINT32_MAX >> 2),
       digits(UINT32_MAX >> 3),
       digits(UINT32_MAX >> 4),
       digits(UINT32_MAX >> 5),
       digits(UINT32_MAX >> 6),
       digits(UINT32_MAX >> 7),
       digits(UINT32_MAX >> 8),
       digits(UINT32_MAX >> 9),
       digits(UINT32_MAX >> 10),
       digits(UINT32_MAX >> 11),
       digits(UINT32_MAX >> 12),
       digits(UINT32_MAX >> 13),
       digits(UINT32_MAX >> 14),
       digits(UINT32_MAX >> 15),
       digits(UINT32_MAX >> 16),
       digits(UINT32_MAX >> 17),
       digits(UINT32_MAX >> 18),
       digits(UINT32_MAX >> 19),
       digits(UINT32_MAX >> 20),
       digits(UINT32_MAX >> 21),
       digits(UINT32_MAX >> 22),
       digits(UINT32_MAX >> 23),
       digits(UINT32_MAX >> 24),
       digits(UINT32_MAX >> 25),
       digits(UINT32_MAX >> 26),
       digits(UINT32_MAX >> 27),
       digits(UINT32_MAX >> 28),
       digits(UINT32_MAX >> 29),
       digits(UINT32_MAX >> 30),
       digits(UINT32_MAX >> 31),
       1
     };
   constexpr int16_t get_digits(uint32_t value)  NOEXCEPT
   { 
     // Estimate number of digits by counting leading zeroes
     uint8_t digits = digits_by_leading_zeroes[__clz(value)];
     
     if(digits > 1 && value < digit_values[digits]) --digits;
     return digits;
   }
   
    constexpr int16_t get_digits(int32_t value)  NOEXCEPT
    {
      // Add sign digit
      return get_digits(static_cast<uint32_t>(abs(value))) + 1;
    }
    
     
   struct field_formatter 
   {
     char* out;
     uint16_t cwidth;
     uint16_t rpad_width; 
   };
   
  field_formatter format_field(char* out, Options fmt, uint32_t actual_width)  NOEXCEPT
  {   
    uint32_t field_width = fmt.width > 0 ? fmt.width : actual_width;
    
    field_formatter f;
      
    f.cwidth = actual_width;
    f.rpad_width = 0;
    f.out = out;

    if(field_width > actual_width)
    {
      uint32_t total_pad = field_width - actual_width;
      uint32_t pad_left=0;

      
      if(fmt.align == eformat::Align::Right) 
      {
        pad_left = total_pad;
      }
      else if(fmt.align == eformat::Align::Center)
      {
        pad_left = total_pad/2U;
        f.rpad_width = total_pad/2U+1U;
      }
      else
      {
        f.rpad_width = total_pad;
      }
      f.out = fill(f.out, pad_left, ' ');
    }
    return f;
  }
  
  int do_format_decimal(char* out, uint32_t value, Options fmt, int16_t digits, char sign)  NOEXCEPT
  {
    auto field = format_field(out, fmt, digits);
    digits = field.cwidth;
    
    if(sign != '\0')
    { 
      *out++ = sign; --digits;
    }
    
    // print out digits in descenting order until we hit 0
    while(digits > 0)
    {
      uint32_t dvalue = digit_values[digits--];
      *out++ = (value / dvalue) + '0';
      value = value % dvalue;
    }
    
    fill(out, field.rpad_width, ' ');
    
    return digits;
  }
  uint8_t get_hex_digits(uint32_t value)  NOEXCEPT {
    uint8_t d = (32U-__clz(value) + 3U) / 4U + 2U; 
    if(d < 3) d = 3;
    return d;
  }
  
  char* do_format_hex(char* b, uint32_t value, Options fmt, uint8_t digits)  NOEXCEPT
  { 
    *b++ = '0';
    *b++ = 'x';
    
    // 4 bits per hex digit
    uint32_t bit = (digits-2) * 4U;
    
    while(bit > 0)
    {
      bit -= 4U;
      uint32_t digit = (value >> bit) & 0xF;
      char dchar = digit < 10 ? '0' + digit : 'A' + (digit - 10);
      *b++ = dchar;
    }
    return b;
  }
  
}
        
namespace eformat 
{
  int format_decimal(char* out, uint16_t size, uint32_t value, Options fmt) NOEXCEPT
  {
    auto digits = get_digits(value);
    if(digits > size) digits = size;
    do_format_decimal(&out[0], value, fmt, digits, '\0');
    return digits;
  }
  
  int format_decimal(char* out, uint16_t size, int32_t value, Options fmt)  NOEXCEPT
  {
    auto digits = get_digits(value);
    if(digits > size) digits = size;

    uint32_t absval = value;
    char sign = ' ';
    if(value < 0)
    {
      absval = -value;
      sign = '-';
    }
    do_format_decimal(&out[0], absval, fmt, digits, sign);
    return digits;
  }
  
  int format_hex(char* out, uint16_t size, uint32_t value, Options fmt) NOEXCEPT
  {
    uint8_t digits = get_hex_digits(value);
    if(digits > size) digits = size;

    auto field = format_field(&out[0], fmt, digits);
    field.out = do_format_hex(field.out, value, fmt, digits);
    field.out = fill(field.out, field.rpad_width, ' ');
    return field.out - out;
  }
  
  int format_binary(char* out, uint16_t size, uint32_t value, Options fmt) NOEXCEPT
  {
    int32_t digits = 32 - __clz(value) + 2;
    if(digits > size) digits = size;

    auto field = format_field(&out[0], fmt, digits);
    
    if(digits-- > 0) *field.out++ = '0';
    if(digits-- > 0) *field.out++ = 'b';

    uint8_t bit = 1 << digits;
    
    while(bit > 0)
    {  
      *field.out++ = (bit & value) != 0U ? '1' : '0';
      bit >>= 1U;
    }

    return digits + 2;
  }
  
  int format_int(buffer& out, int32_t value, const Options fmt)  NOEXCEPT
  {
    char temp[32];
    int count = 0;
    if(fmt.base == Base::Decimal)  { count = format_decimal(temp, sizeof(temp), value, fmt);  }
    else if(fmt.base == Base::Hex) { count = format_hex(temp, sizeof(temp), value, fmt); }
    else                           { count = format_binary(temp, sizeof(temp), value, fmt); }
    return out.sputn(temp, count);
  }
  
  int format_int(buffer& out, uint32_t value, const Options fmt)  NOEXCEPT
  {
    char temp[32];
    int count = 0;
    if(fmt.base == Base::Decimal)  { count = format_decimal(temp, sizeof(temp), value, fmt); }
    else if(fmt.base == Base::Hex) { count = format_hex(temp, sizeof(temp),  value, fmt); }
    else                           { count = format_binary(temp, sizeof(temp), value, fmt); }
    return out.sputn(temp, count);
  }

  
    ParseStatus match(string_view& buf, const string_view& str)
    {
      auto c = buf.begin();
      
      auto it = str.begin();
      
      while(c != buf.end() && *c++ == *it++)
      {
        if(it == str.end()) 
        {
          buf.remove_prefix(c - buf.begin());
          return ParseStatus::OK;
        }
      }
      
      return ParseStatus::NotMatched;
    }
    
 
#define FORMAT_INT_TYPE(TYPE) \
  int format(buffer& out, const TYPE& value, Options options) NOEXCEPT { return format_int(out, value, options ); }
  FORMAT_INT_TYPE(uint8_t)
  FORMAT_INT_TYPE(uint16_t)
  FORMAT_INT_TYPE(uint32_t)
  FORMAT_INT_TYPE(int8_t)
  FORMAT_INT_TYPE(int16_t)
  FORMAT_INT_TYPE(int32_t)
    
    
  ParseStatus parse(string_view& in, uint8_t& value)  NOEXCEPT
  {
    uint32_t temp = 0;
    auto ret = parse(in, temp);
    if(ret != ParseStatus::OK) return ret;
    
    if(temp > UINT8_MAX) return ParseStatus::Overflow;
    value = temp;
    return ParseStatus::OK;
  }
  
  ParseStatus parse(string_view& in, uint16_t& value) NOEXCEPT
  {
    uint32_t temp = 0;
    auto ret = parse(in, temp);
    if(ret != ParseStatus::OK) return ret;
    
    if(temp > UINT16_MAX) return ParseStatus::Overflow;
    value = temp;
    return ParseStatus::OK;
  }
  
  ParseStatus parse(string_view& in, uint32_t& value) NOEXCEPT
  {
    auto c = in.begin();
    if(c != in.end() && std::isdigit(*c))
    {
      value = (*c - '0');
      while(c != in.end() && std::isdigit(*(++c)))
      {
        uint32_t v = (*c - '0');
        if(value > UINT32_MAX / 10U) return ParseStatus::Overflow;
        value = value * 10U;
        if(value > UINT32_MAX - v)  return ParseStatus::Overflow;
        value += v;
      }
      in.remove_prefix(c - in.data());
      return ParseStatus::OK;
    }
    return ParseStatus::NotMatched;
  }
  
  ParseStatus parse(string_view& in, int32_t& value)  NOEXCEPT
  {
    auto c = in.begin();
    int sign = 0;
    if(*c == '-') 
    {
      sign = -1;
      ++c;
    }
    else if(*c == '+')
    {
      sign = 1;
      ++c;
    }
    
    if(std::isdigit(*c))
    {
      if(sign == 0) sign = 1;
      value = sign * (*c - '0');
      while(c < in.end() && std::isdigit(*(++c)))
      {
        int32_t v = *c - '0';
        if(value > INT32_MAX / 10) return ParseStatus::Overflow;
        value = value * 10;
        if(value > INT32_MAX - v)  return ParseStatus::Overflow;
        value += v;
      }
      in.remove_prefix(c - in.data());
      
      return ParseStatus::OK;
    }
    
    return ParseStatus::NotMatched;
  }
  
  
  ParseStatus parse(string_view& in, int8_t& value)   NOEXCEPT
  {
    int32_t temp;
    auto ret = parse(in, temp);
    if(ret != ParseStatus::OK) return ret;
    
    if(temp > INT8_MAX) return ParseStatus::Overflow;
    value = temp;
    return ParseStatus::OK;
  }
  
  ParseStatus parse(string_view& in, int16_t& value)  NOEXCEPT
  {
    int32_t temp;
    auto ret = parse(in, temp);
    if(ret != ParseStatus::OK) return ret;
    
    if(temp > INT16_MAX) return ParseStatus::Overflow;
    value = temp;
    return ParseStatus::OK;
  }
  
  ParseStatus parse(string_view& in, bool& value) NOEXCEPT
  {
    ParseStatus status;
    if((status = match(in, true_string)) == ParseStatus::OK) value = true;
    else if(status == ParseStatus::NotMatched && 
            (status = match(in, false_string)) == ParseStatus::OK) value = false;
    return status;
  }
  
  ParseStatus parse(string_view& in, estd::span<char_type>& value, char_type delimeter) NOEXCEPT
  {
    auto begin = value.begin();
    auto pos = begin;
    auto end = value.end();
    
    auto c = in.cbegin();
    
    while(*c != delimeter) {
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
  
  int format(buffer& buf, estd::string_view value, Options fmt) NOEXCEPT
  {
    if(fmt.width < value.size()) return buf.sputn(&value[0], value.size());

    char temp[32];
    uint32_t size = value.size() < sizeof(temp) ? value.size() : sizeof(temp);

    auto field = format_field(temp, fmt, size);
    memcpy(field.out, value.data(), size);
    field.out += size;
    field.out = fill(field.out, field.rpad_width, ' ');
    
    return buf.sputn(temp, field.out - &temp[0]);
  }
  
  int format(buffer& out, bool value, Options fmt) NOEXCEPT
  {
    return format(out, value ? true_string : false_string, fmt);
  }
  
  int format(buffer& buf, const void* value, Options fmt) NOEXCEPT
  {
    const intptr_t ptrval = reinterpret_cast<intptr_t>(value);
    uint32_t digits = get_hex_digits(ptrval);
    uint32_t size = digits+2;
    char temp[32];
    auto field = format_field(temp, fmt, size);
    *field.out++ = '<';

    field.out = do_format_hex(field.out, ptrval, fmt, digits);
    *field.out++ = '>';
    return buf.sputn(temp, field.out - &temp[0]);
  }
  
  bool arg_value::format(parse_context& parse, buffer&  fmt) const  NOEXCEPT
  {
    if(fformat(parse, ptr, fmt) == true) 
    {
       return true;
    }
    else return false;
  }
  
  bool vformat_to(buffer& buffer, string_view fmt, const basic_format_args& args)  NOEXCEPT
  {
    parse_context parse = {fmt.begin(), fmt.end()};
    auto argiter = args.begin();
    
    while(parse.fmt_pos < parse.fmt_end)
    {
      while(parse.fmt_pos < parse.fmt_end 
            && *parse.fmt_pos != '{') buffer.sputc(*parse.fmt_pos++);
      
      if(parse.fmt_pos == parse.fmt_end) return true;
      
      // Write arg
      if(argiter != args.end())
      {
        if(argiter->format(parse, buffer) == false) return false;
        ++argiter;
      }
      else
      {
        // Format string error, ran out of arguments
        return false;
      }
    }
    return true;
  }
  

  
    /// \brief Format string 
    stream& operator<<(stream& s, const string_view value) NOEXCEPT 
    {
      format(s.buf, value, s.o);
      return s;
    }
  
      /// \brief Write character to buffer (unformatted)
     stream& operator<<(stream& s, const char value) NOEXCEPT 
     {
       s.buf.sputc(value);
       return s;
     }
    
    /// \brief Write signed integer to buffer
    stream& operator<<(stream& s, const int32_t value) NOEXCEPT 
    {
      format_int(s.buf, value, s.o);
      return s;
    }
  
    /// \brief Write unsigned integer to buffer
    stream& operator<<(stream& s, const uint32_t value) NOEXCEPT 
    {
      format_int(s.buf, value, s.o);
      return s;
    }
  
    /// \brief Write signed integer to buffer
    stream& operator<<(stream& s, const int16_t value) NOEXCEPT 
    {
      format_int(s.buf, static_cast<int32_t>(value), s.o);
      return s;
    }
  
    /// \brief Write unsigned integer to buffer
    stream& operator<<(stream& s, const uint16_t value) NOEXCEPT 
    {
      format_int(s.buf, static_cast<uint32_t>(value), s.o);
      return s;
    }
  
    /// \brief Write signed integer to buffer
    stream& operator<<(stream& s, const int8_t value) NOEXCEPT 
    {
      format_int(s.buf, static_cast<int32_t>(value), s.o);
      return s;
    }
  
    /// \brief Write unsigned integer to buffer
    stream& operator<<(stream& s, const uint8_t value) NOEXCEPT 
    {
      format_int(s.buf, static_cast<uint32_t>(value), s.o);
      return s;
    }

    namespace color
    {
      stream& operator<<(stream& os, Code mod) NOEXCEPT
      {
        char temp[5] = { '\033','[', mod.code[0], mod.code[1], 'm'};
        os.buf.sputn(temp, sizeof(temp));
        return os;
      }
    }
}