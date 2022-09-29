#include "console.hpp"

#include "eobject.hpp"

using eobject::Error;
using eobject::DataType;
using eobject::Object;
using eobject::Record;
using eobject::Array;
using eobject::Variable;

namespace {

eformat::stream& print_object_not_found(eformat::stream& so, const estd::string_view& object_name)
{
  return so << "Object " << object_name << " not found";
}

eformat::stream& operator<<(eformat::stream so, std::nullptr_t)
{
  return so << "null";
}
  
eformat::stream& operator<<(eformat::stream& so, Error err)
{
  return so << to_string(err);
}

eformat::stream& operator<<(eformat::stream& so, DataType type)
{
  return so << to_string(type);
}

eformat::stream& operator<<(eformat::stream& so, Object::ClassId type)
{
  return so << to_string(type);
}

eformat::stream& operator<<(eformat::stream& so, const Object::Info& info)
{
  so << info.otype << ':';
  switch(info.otype)
  {
    case Object::ClassId::Array: return so << info.type << '(' << info.nelem << ')';
    case Object::ClassId::Variable: return so << info.type;
    case Object::ClassId::Record: return so << static_cast<const Record::Info&>(info);
    default: return so;
  }
}

template<class T>
eformat::stream& print(eformat::stream& so, const void* data, size_t size)
{
  const T* temp = static_cast<const T*>(data);
  if(temp == nullptr) return so << nullptr;
  else return so << *temp;
}

eformat::stream& print_string(eformat::stream& so, const void* data, size_t size)
{
  return so << '\"' << estd::string_view{static_cast<const char*>(data), size} << '\"';
}

eformat::stream& print_value(eformat::stream& so, const void* data, size_t size, DataType type)
{
  switch(type)
  {
  case DataType::U8:     return print<uint8_t> (so, data, size);
  case DataType::U16:    return print<uint16_t>(so, data, size);
  case DataType::U32:    return print<uint32_t>(so, data, size);
  case DataType::I8:     return print<int8_t>  (so, data, size);
  case DataType::I16:    return print<int16_t> (so, data, size);
  case DataType::I32:    return print<int32_t> (so, data, size);
  case DataType::String: return print_string   (so, data, size);
  case DataType::Record: return so << "{...}";
  default: return so << "Type Invalid";
  }
}

eformat::stream& print_field(eformat::stream& so, Object::const_iterator field)
{
  auto info = field.info();
  so << "\n\t" << *info.name << ": ";
  uint8_t buffer[64];
  int e = field.get_to(buffer, sizeof(buffer));
  if(e > 0){
    return print_value(so, buffer, e, info.info->type);
  }
  else
  {
    return so << static_cast<eobject::Error>(e);
  }
}

eformat::stream& operator<<(eformat::stream& so, const Object& object)
{
  if(object.otype() == Object::ClassId::Variable)
  {
    uint8_t buffer[64];
    int e =  object.get(buffer, sizeof(buffer));
    so << ' ';
    if(e > 0){
       return print_value(so, buffer, e, object.type());
    }
    else
    {
      return so << static_cast<eobject::Error>(e);
    }
  }
  else
  {
    for(const auto& value : object)
    {
      print_field(so, value);
    }
  }
  return so;
}


template<class T>
Error parse_value(estd::string_view& str, void* data, size_t& size)
{
  if(sizeof(T) > size) return Error::ParamTooLong;
  size = sizeof(T);
  T* ptr = static_cast<T*>(data);
  eformat::ParseStatus status = eformat::parse(str, *ptr);
  if(eformat::ParseStatus::OK == status)
  {
    return Error::OK;
  }
  return Error::DataTypeError;
}

Error parse_string(estd::string_view& str,  void* data, size_t& size)
{
  if(str.size()-2 > size) return Error::ParamTooLong;
  size = str.size() - 2;

  if(str.front() == '\"' && str.back() == '\"') 
  {
    memcpy(data, str.data()+1, size);
    return Error::OK;
  }
  return Error::DataTypeError;
}

Error parse(void* buffer, size_t& size, estd::string_view& vstring, DataType type)
{
  switch(type)
  {
  case DataType::U8:  return parse_value<uint8_t>(vstring, buffer, size);
  case DataType::U16: return parse_value<uint16_t>(vstring, buffer, size);
  case DataType::U32: return parse_value<uint32_t>(vstring, buffer, size);
  case DataType::I8:  return parse_value<int8_t>(vstring, buffer, size);
  case DataType::I16: return parse_value<int16_t>(vstring, buffer, size);
  case DataType::I32: return parse_value<int32_t>(vstring, buffer, size);
  case DataType::String: return parse_string(vstring, buffer, size);
  default: return eobject::Error::DataTypeError;
  }
}
}

namespace console
{
 
  Console::Console(eformat::stream s, const eobject::Dictionary& dictionary_in, estd::string_view prompt)
    : so(s), dictionary(dictionary_in),  prompt_(prompt)
    {
      so.width(0);
      
      pprompt();
    }
    
  void Console::pprompt() 
  {
    so << prompt_; so.sync();
  }
  
    void Console::command_list(estd::string_view& line)
    {
      if(line.empty())
      {
        so << "\nObjects:\n";
        for(auto& item : dictionary)
          so << "  " << item.object.name() << '\n';
      }
      else
      {
        auto item = dictionary.find(line);
        if(item != nullptr) so << item->object.info();
        else print_object_not_found(so, line);
      }
    }
    
    void Console::command_get(estd::string_view& line)
    {
      if(line.empty()) { so << "Usage: get <object>(.<item>)"; return; }
      eobject::Dictionary::Query query{line};
      
      auto e = dictionary.query(query);
      if(e == eobject::Error::OK)
      {
        so << query.item->object.name();
        if(false == query.subobject_name.empty()) so << '.' << query.subobject_name;
        so << ':';
        
        if(query.subIdx < 0)
        {
          so << query.item->object;
        }
        else
        {
          uint8_t buffer[64];
          int s = query.item->object.get(query.subIdx, buffer, sizeof(buffer));
          if(s > 0)
          {
            print_value(so, buffer, s, query.info->type);
            return;
          }
          else
          {
            so << static_cast<eobject::Error>(s);
          }
        }
        return;
      }
      so << static_cast<eobject::Error>(e);
    }
    
    static bool isequal(char C) { return C == '='; }

    void Console::command_set(estd::string_view& line)
    {
      auto name = estd::next_token(line, isequal);
      trim_suffix(name, estd::isspace);
      trim_prefix(line, estd::isspace);
      
      // Got an endline before second token
      if(line.empty()) { so << "Usage: set <object>(.<item>) <value>"; return; }
      
      eobject::Dictionary::Query query{name};
      auto e = dictionary.query(query);
      if(e == Error::OK)
      {
        uint8_t buffer[64];
        size_t size = sizeof(buffer);
        e = parse(buffer, size, line, query.info->type);
        if(Error::OK == e)
        {
          if(query.subIdx < 0)
          {
            if(query.item->object.otype() == Object::ClassId::Variable)
            {
              query.subIdx = 0;
            }
            else
            {
              so << "Must select subobject to set";
              return;
            }
          }
        }
        e = query.item->object.set(query.subIdx, buffer, size);
      }
      so << e;
    }
  
  int Console::poll() NOEXCEPT
  {
    auto status = so.buf.poll();
      
    if(status < 0)
    {
      so.buf.gflush();
      so << "Input buffer overflow!";
      pprompt();
    }
    else if(status > 0)
    { 
      estd::string_view line = so.buf.getline();
      if(false == line.empty()) 
      {
        so.write(line) << eformat::endl;

        auto commandstr = estd::next_token(line, estd::isspace);
        estd::trim_prefix(line, estd::isspace);
        
        if(commandstr == "ls") command_list(line);
        else if(commandstr == "get") command_get(line);
        else if(commandstr == "set") command_set(line);
        else if(commandstr == "status") { so << "Status not implemented\n"; }
        else {  so << "Unknown command: " << commandstr; }
        
        pprompt();
      }
      else if(eio::isendline(so.buf.sgetc()))
      {
        pprompt();
      }
    }
    return status;
  }
  
}