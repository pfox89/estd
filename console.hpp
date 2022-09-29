#pragma once

/// \file console.hpp
/// Defines console class for embedded systems

#include "estring.hpp"
#include "eformat.hpp"

#include "eobject.hpp"

namespace console
{

struct Console
{
 
public:
  /// \brief Create console
  /// \param stream     Stream to use as console I/O
  /// \param dictionary Dictionary to access for parameters
  /// \param prompt     Prompt to display for console
  Console(eformat::stream stream, const eobject::Dictionary& dictionary, estd::string_view prompt="\n>>");
  
  /// \brief Poll the console to check for commands
  int poll() NOEXCEPT;
  
private:
  const eobject::Dictionary& dictionary;

  eformat::stream so;
  estd::string_view prompt_;
  
  void pprompt() NOEXCEPT;
  
  void command_list(estd::string_view& line) NOEXCEPT;
    
  void command_get(estd::string_view& line) NOEXCEPT;
    
  void command_set(estd::string_view& line) NOEXCEPT;
  
};

}