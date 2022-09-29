#pragma once

/// \file eio.hpp
/// Simple embededded C++ io library that is compact and does not use dynamic allocation or exceptions

#include <cstdint>

#include "estring.hpp"

namespace eio {
  
  typedef estd::string_view::size_type size_type;
  using estd::char_type;
  using estd::string_view;

  struct Driver;
  
  static bool isendline(char C) NOEXCEPT { return C == '\n' || C == '\r'; }

  /// Class similar to a stream buffer which supports asynchronously buffering data to write
  struct buffer
  {
    /// \brief Construct a buffer base class with pointers to internal buffers
    constexpr buffer(char_type* pbase, char_type* pend, char_type* gbase) NOEXCEPT
      : pbase_(pbase), pptr_(pbase), epptr_(pend), gbase_(gbase), gptr_(gbase), egptr_(gbase) { }
    
    /// \defgroup PutFunctions Put area functions
    /// @{

    /// \brief Put a character to put area
    int sputc(char_type c) NOEXCEPT
    {
      if(pptr_ != epptr_) return *pptr_++ = c;
      else return overflow(c);
    }
    
    /// \brief Put N characters to put area
    int sputn(const char* begin, size_type n) NOEXCEPT
    {
      do {
        int incr = epptr_ - pptr_;
        if(n < incr) incr = n;
        memcpy(pptr_, begin, incr);
        pptr_ += incr;
        begin += incr;
        
        n -= incr;
        
      } while(n != 0 && overflow(*begin++) >= 0 && --n > 0);
      
      return n > 0 ? EOF : 0;
    }
    
    /// \brief Flush buffer, explicity writing contents to device
    /// \param timeout Number to times to retry flush before returning
    /// \returns Timeout remaining on success (>=0), negative on error
    virtual int flush(int timeout=5) NOEXCEPT = 0;

    /// \brief Flush buffer, and wait for UART to finish transmitting
    /// \param timeout Number to times to retry before retuning
    /// \returns Timeout remaining on success (>=0), negative on error
    virtual int sync(int timeout=10000) NOEXCEPT = 0;

    /// @}

    /// \defgroup GetFunctions Get area functions
    /// @{
   
    /// \brief Get next token from input buffer, using predicate to find delimeters
    /// \returns View of next token in input buffer
    template<class Predicate=decltype(estd::isblank)>
    string_view get_next_token(Predicate isdelim=estd::isspace) NOEXCEPT
    {
      auto stop = egptr_;
      
      // Trim delimiters from start
      gptr_ = estd::find_if_not(gptr_, stop, isdelim);
      
      auto start = gptr_;
      
      // Search for end delimeter
      auto loc = estd::find_if(start, stop, isdelim);
          
      // If we didn't find a delimeter at the end, return empty
      if(loc < stop) gptr_ = loc;
      else loc = start;

      // Set current token view
      return string_view{start, loc};
    }
    
    
    string_view getline() NOEXCEPT
    {
      poll();
      return get_next_token(isendline);
    }
    
    bool is_endline() NOEXCEPT
    {
      auto loc = estd::find_if_not(gptr_, egptr_, estd::isblank);
      return loc < egptr_ && isendline(*loc);
    }
    
    /// \brief Get view of input area
    estd::string_view get() NOEXCEPT
    {
      return string_view(gptr_, egptr_);
    }
    
    void gflush() NOEXCEPT { egptr_ = gptr_; }
    
    /// \brief Poll device for more data
    /// \returns Number of bytes read, 0 if no data, EOF if buffer is full
    virtual int poll(int timeout=0) NOEXCEPT = 0;
    
    /// \brief Advance input area to specified location
    /// \param to Pointer to input area position to advance to
    /// \returns Characters remaining in input area, or EOF on error
    int gadvance(const char_type* to) NOEXCEPT
    {
      if(to >= gptr_ && to <= egptr_) 
      {
        gptr_ = const_cast<char_type*>(to); return in_avail();
      }
      else
      {
        return EOF;
      }
    }
    
    /// \brief Get the first character in the get area, if any
    int sgetc() const NOEXCEPT { return gbase_ < gptr_ ? *gbase_ : EOF; }
    
    /// \brief Return how many characters are available in the get area
    int in_avail() const NOEXCEPT { return gptr_ - gbase_; }
    
    /// @}

  protected:
      
    ///  \defgroup PutArea Pointers to put area
    /// @{
    char_type* pbase_; ///< Beginning of current put area
    char_type* pptr_;  ///< Current position in current put area
    char_type* epptr_; ///< End of current put area
    /// @}

    /// \defgroup GetArea Pointers to get area
    /// @{
    char_type* gbase_; ///< Beginning of current get area
    char_type* gptr_;  ///< Current position in current get area
    char_type* egptr_; ///< End of current get aread
    /// @}

    /// \brief Write buffer overflow -- request more write buffer space, without writing if possible
    /// \param c Character to write to put buffer
    /// \returns Value of c if successfully written, EOF if buffer is full
    virtual int overflow(char_type c) NOEXCEPT = 0;
  };

/// \brief IO Device class, which wraps driver and common functions
class IODevice
{
public:

   /// \brief base driver class, which can be subclassed to implement specific drivers
   struct Driver 
   {
      virtual int write(const void* buffer, uint16_t count) NOEXCEPT = 0;
      virtual int read(void* buffer, uint16_t count) NOEXCEPT = 0;
      /// \brief Synchronize with underlying device
      /// \param timeout Number of times to attempt to synchronize
      /// \returns time remaining if synchronization succeeded (>=0), negative on timeout
      virtual int sync(int timeout) NOEXCEPT = 0;
      virtual buffer& getbuf() NOEXCEPT = 0;
   };
 
  /// \brief Write buffer to device. 
  /// \param buffer Pointer to memory to read into
  /// \param count Number of bytes to write
  /// \returns Number of bytes written, or Status code
  int write(const void* buffer, uint16_t count) NOEXCEPT { return driver_->write(buffer, count); }
  
  /// \brief Wait on pending operations
  int sync(uint16_t timeout) NOEXCEPT { return driver_->sync(timeout); }
  
  /// \brief Get read buffered contents from device
  /// \param buffer  Pointer to memory to read into
  /// \param count   Maximum number of bytes to read.
  /// \param Number of bytes read, or Status code
  int read(void* buffer, uint16_t count) NOEXCEPT { return driver_->read(buffer, count); }
  
  /// \brief Get IO buffer to enable buffered IO
  /// \returns Pointer to IO buffer
  buffer& getbuf() NOEXCEPT { return driver_->getbuf(); }
  
  constexpr IODevice(Driver* driver) NOEXCEPT : driver_(driver) {}
  IODevice(IODevice&& other) NOEXCEPT = default;
  IODevice(const IODevice& other) NOEXCEPT = default;
private:
  Driver* driver_;
};

extern IODevice console;
}
