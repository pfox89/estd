/// \file eio.cpp
/// \brief Implementation of simple stream type

#include "eio.hpp"
#include "eio_buffer.hpp"

#include <cstdio>

#include <LowLevelIOInterface.h>

namespace eio
{
  struct iar_llio_driver final : public IODevice::Driver
  {
    typedef iobuffer<iar_llio_driver, 1024, 128> BufferType;
    
    BufferType buffer_;
     
    constexpr iar_llio_driver()
      : buffer_(*this)
      {}
      
    int write(const void* data, uint16_t count) NOEXCEPT
    {
      const uint8_t* d = static_cast<const uint8_t*>(data);
      
      return __write(_LLIO_STDOUT, d, count);
    }
    int sync(uint16_t timeout) NOEXCEPT
    {
      return 0;
    }
    int read(void* buffer, uint16_t count) NOEXCEPT
    {
      uint8_t* buf = reinterpret_cast<uint8_t*>(buffer);
        
      return __read(_LLIO_STDIN, buf, count);
    }
    
    int flush(BufferType& buf) NOEXCEPT
    {
      auto next = buf.start_next_transaction();
      if(next.empty()) return 0;
       int status;
      do {
        status = write(next.data(), next.size());
        if(status == next.size())  next = buf.start_next_write();
        else next = estd::span<char_type>{next.begin() + status, next.size()};
        
      } while(status > 0 && !next.empty());
      
      return status;
    }
    
    buffer& getbuf() NOEXCEPT
    {
      return buffer_;
    }
  };
  
  
  static iar_llio_driver iar_llio_driver_;
  
  IODevice console(&iar_llio_driver_);
}