#include "eobject.hpp"

namespace eobject
{
string_view to_string(Error error) NOEXCEPT
{
  switch (error)
  {
    case Error::OK: return "OK";
    case Error::DataTypeError: return "Data type mismatch";
    case Error::ParamTooLong: return "Parameter too large";
    case Error::ParamTooShort: return "Parameter too short";
    case Error::ValueTooHigh: return "Value too high";
    case Error::ValueTooLow: return "Value too low";
    case Error::ObjectNotFound: return "Object not found";
    case Error::FieldNotFound: return "Field not found in object";
    case Error::ReadOnly: return "Object is read only";
    case Error::UnableToSet: return "Unable to set value";
    case Error::WriteOnly: return "Object is write only";
  }
  return "Unknown";
}

string_view to_string(DataType type) NOEXCEPT
{
  switch (type)
  {
    case DataType::Invalid: return "invalid";
    case DataType::U8: return "u8";
    case DataType::U16: return "u16";
    case DataType::U32: return "u32";
    case DataType::I8: return "i8";
    case DataType::I16: return "i16";
    case DataType::I32: return "i32";
    case DataType::String: return "string";
    case DataType::BinString: return "bstring";
    case DataType::Record: return "record";
  }
  return "ERR";
}

string_view to_string(Object::ClassId type) NOEXCEPT
{
  switch (type)
  {
    case Object::ClassId::Variable: return "Variable";
    case Object::ClassId::Record: return "Record";
    case Object::ClassId::Array: return "Array";
    default: return "Object";
  }
}

size_t type_size(DataType type) NOEXCEPT
{
  switch (type)
  {
    case DataType::Invalid: return 0;
    case DataType::U8: return 1;
    case DataType::U16: return 2;
    case DataType::U32: return 4;
    case DataType::I8: return 1;
    case DataType::I16: return 2;
    case DataType::I32: return 4;
    case DataType::String: return 1;
    case DataType::BinString: return 1;
    case DataType::Record: return 0;
  }
  return 0;
}

int32_t Object::get(uint8_t subIdx, void* buffer, size_t size) const NOEXCEPT
{
  if(data_ == nullptr) return  static_cast<int>(Error::WriteOnly);

  switch (info_->otype)
  {
    case Object::ClassId::Variable:
      if (subIdx == 0) return get(buffer, size);
      else return static_cast<int>(Error::FieldNotFound);
    case Object::ClassId::Array:
      if (subIdx <= info_->nelem)
      {
        if(subIdx == 0) 
        {
          *reinterpret_cast<uint8_t*>(buffer) = info_->nelem;
          return sizeof(uint8_t);
        }
        else
        {
          size_t      elem_size = type_size(info_->type);
          const void* dataptr   = data(info_->data_offset + elem_size * (subIdx - 1u));
          memcpy(buffer, dataptr, elem_size);
          return elem_size;
        }
      }
      else
      {
        return static_cast<int>(Error::FieldNotFound);
      }
    case Object::ClassId::Record:
      if (subIdx <= info_->nelem)
      {
        if(subIdx == 0) 
        {
          *reinterpret_cast<uint8_t*>(buffer) = info_->nelem;
          return sizeof(uint8_t);
        }
        else
        {
          const Record::FieldInfo& info      = static_cast<const Record::Info&>(this->info()).fields[subIdx-1];
          const void*              dataptr   = data(info.data_offset);
          size_t                   data_size = info.data_size;
          if (data_size > size) return static_cast<int>(Error::ParamTooShort);
          memcpy(buffer, dataptr, data_size);
          return data_size;
        }
      }
      else
      {
        return static_cast<int>(Error::FieldNotFound);
      }
    default: return static_cast<int>(Error::ObjectNotFound);
  }
}

Object::FieldInfo Object::info(uint8_t subIdx) const NOEXCEPT
{
  FieldInfo finfo = { &info(), nullptr };

  if (subIdx > 0 && subIdx <= finfo.info->nelem)
  {
    finfo.offset = info_->data_offset;
    finfo.size = info_->data_size;

    switch (info_->otype)
    {
      case ClassId::Record: {
        auto temp  = static_cast<const Record::Info*>(info_)->fields + subIdx - 1;
        finfo.info = temp;
        finfo.name = &temp->name;
        finfo.size = temp->data_size;
        break;
      }
      case ClassId::Array: {
        auto temp  = static_cast<const Array::Info*>(info_)->names + subIdx - 1;
        finfo.name = temp;
        finfo.size = type_size(info_->type);
        finfo.offset = info_->data_offset + finfo.size * (subIdx - 1u);
        break;
      }
      default: break;
    }
  }
  return finfo;
}

Record::Info::iterator Record::Info::find(string_view name) const NOEXCEPT
{
  auto it = fields;
  for (; it != fields + nelem; ++it)
  {
    if (it->name == name) return it;
  }
  return it;
}

const Dictionary::Item* Dictionary::find(const string_view& name) const NOEXCEPT
{
  for (auto& item : *this)
  {
    if (item.object.name() == name) return &item;
  }
  return nullptr;
}

const Object* Dictionary::get(uint16_t address) const NOEXCEPT
{
  auto it = estd::lower_bound(
    begin(), end(), address, [](const Item& l, uint16_t address) -> bool { return l.address < address; });
  return (it != end() && it->address == address) ? &(it->object) : nullptr;
}

int32_t Dictionary::query(Dictionary::Query& q) const NOEXCEPT
{
  q.item = find(q.object_name);
  if (q.item != nullptr)
  {
    if (q.subobject_name.empty() == false)
    {
      if (q.item->object.otype() == Object::ClassId::Record)
      {
        const Record::Info& info  = static_cast<const Record::Info&>(q.item->object.info());
        auto                finfo = info.find(q.subobject_name);
        // Get type info for this field
        if (finfo != info.end())
        {
          q.info   = &(*finfo);
          q.subIdx = finfo - info.begin() + 1;
          return Error::OK;
        }
      }
      else if (q.item->object.otype() == Object::ClassId::Array)
      {
        const Array::Info& info = static_cast<const Array::Info&>(q.item->object.info());
        q.subIdx                = info.find(q.subobject_name) + 1;
        // All array elements share the same type
        q.info = &info;
        if (q.subIdx != info.nelem) { return Error::OK; }
      }
      return Error::FieldNotFound;
    }
    else
    {
      // Simple info
      q.info = &q.item->object.info();
      return Error::OK;
    }
  }
  return Error::ObjectNotFound;
}
}