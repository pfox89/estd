#pragma once

/// \file eio_buffer
/// Implementation of a simple circular buffer for input and outbuf
/// A simplified version of file_buffer (from C++ IO library) suitable for terminal/logging input and output

#include "eio.hpp"
#include "span.hpp"

namespace eio {
  
  /// Contains writing range [tx_begin, tx_pend) which is data currently being written (e.g. by DMA)
  /// Contains ready range [tx_pend, tx_ready) which is data which is ready to be written. 
  /// This range may wrap around, in which case tx_ready < tx_pend
  /// If the data ready to write needs to wrap around [tx_pend < tx_ready), 
  /// it will be split into two ranges, [tx_pend, pend_last) and [buffer_start(), tx_ready)
  template<class DriverType, size_type OutbufSize, size_type InbufSize>
  class iobuffer final : public buffer
  {
  public:
    
    static const size_type outbuf_size = OutbufSize;
    static const size_type inbuf_size = InbufSize;
    
    constexpr char_type* outbuf_start() NOEXCEPT { return &outbuf[0]; }
    constexpr char_type* outbuf_end()   NOEXCEPT { return &outbuf[0] + sizeof(outbuf); }
    
    constexpr char_type* inbuf_start() NOEXCEPT { return &inbuf[0]; }
    constexpr char_type* inbuf_end()  NOEXCEPT { return &inbuf[0] + sizeof(inbuf); }
    
     constexpr iobuffer(DriverType& d) NOEXCEPT
       : buffer(outbuf_start(), outbuf_end(), inbuf_start()), 
        driver_(d), 
        outbuf(), inbuf()
        {}

    int flush(int timeout) NOEXCEPT
    {
      if(pptr_ == pbase_) return 0;
      
      int count = 0;
      do {
        // Poll input buffer while we flush
        try_get();

        count = driver_.write(pbase_, pptr_ - pbase_);
        if(count > 0) 
        {
          pptr_ = pbase_;
          return timeout;
        }
      } while(timeout-- > 0);

      return timeout;
    }

    int sync(int timeout) NOEXCEPT
    {
      // Flush buffers
      timeout = flush(timeout);

      // Synchronize UART
      do
      {
        // Poll input while we sync
        try_get();
      } while(timeout-- > 0 && driver_.sync(0) < 0);

      return timeout;
    }

  protected:

    int try_get() NOEXCEPT
    {
      int read = 0;
      // Adjust egptr, if necessary to make room
      if(gptr_ == egptr_)
      {
        // If the get pointer has reached the end of the buffer, reset all the pointers
        egptr_ = gbase_ = gptr_ = inbuf_start();
      }
        
      read = inbuf_end() - egptr_;
        
      // If buffer is full, return EOF
      if(read == 0) return EOF;
        
      // Read as much as we can
      read = driver_.read(egptr_, read);
      if(read > 0)
      {
        egptr_ += read;
      }
      return read;
    }

    int overflow(char_type c) NOEXCEPT
    {
      // Try to flush
      auto count = flush(20);
      if(count <= 0) 
      {
        return EOF;
      }
      return *pptr_++ = c;
    }
    
    int poll(int timeout=0) NOEXCEPT
    {
      int read = 0;
      do {
        // Flush write buffers
        flush(0);
        read = try_get();
      } while(timeout-- > 0 && read <= 0);

      return egptr_ - gptr_;
    }

  private:
    
    DriverType& driver_; ///< Reference to drive to use to flush buffers
    
    char_type outbuf[outbuf_size]; ///< Output buffer
    char_type inbuf[inbuf_size];   ///< Input buffer
  };
  
}