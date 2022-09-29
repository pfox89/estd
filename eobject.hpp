#pragma once

/// \file eobject.hpp
/// Types to help build an object dictionary, which allows reflection on properties.
/// In particular an exposed variable has a type, name, max length, and set/get functionality, and is searchable at
/// runtime This code takes care to do as much as possible constexpr, and allows most FieldInfo table values to be
/// stored in flash on MCUs DataView is used as a view to a type-erased data buffer which can be used to get/set
/// parameters

#include "array.hpp"
#include "estring.hpp"
#include "span.hpp"
#include <cassert>
#include <iterator>

#if defined(__cpp_exceptions)
#include <stdexcept>
#define CHECK_CAST(...) \
  if (false == (__VA_ARGS__)) throw std::bad_cast()
#define CHECK_BOUNDS(WHAT, ...) \
  if (false == (__VA_ARGS__)) throw std::out_of_range(WHAT)
#define CHECK_PTR(PTR) \
  if (PTR == nullptr) throw std::invalid_argument("nullptr")
#else
#define CHECK_CAST(...)         assert(__VA_ARGS__)
#define CHECK_BOUNDS(WHAT, ...) assert((__VA_ARGS__ && WHAT))
#define CHECK_PTR(PTR)          assert(PTR != nullptr)
#endif

#define ERR_ODV3_OBJECT_IS_READ_ONLY_     ((int32_t)0xC09B0004L)
#define ERR_ODV3_OBJECT_DOES_NOT_EXIST_   ((int32_t)0xC09B0005L)
#define ERR_ODV3_GENERAL_PARAMETER_INCOMPATIBILITY_ ((int32_t)0xC09B0008L)
#define ERR_ODV3_DATATYPE_DOES_NOT_MATCH_ ((int32_t)0xC09B000AL)
#define ERR_ODV3_DATATYPE_LENGTH_IS_TOO_LONG_ ((int32_t)0xC09B000BL)
#define ERR_ODV3_DATATYPE_LENGTH_IS_TOO_SHORT_ ((int32_t)0xC09B000CL)
#define ERR_ODV3_SUBINDEX_DOES_NOT_EXIST_ ((int32_t)0xC09B000DL)
#define ERR_ODV3_RANGE_OF_PARAMETER_EXCEEDED_ ((int32_t)0xC09B000EL)
#define ERR_ODV3_VALUE_OF_PARAMETER_WRITTEN_TOO_HIGH_ ((int32_t)0xC09B000FL)
#define ERR_ODV3_VALUE_OF_PARAMETER_WRITTEN_TOO_LOW_ ((int32_t)0xC09B0010L)
#define ERR_ODV3_OBJECT_IS_WRITE_ONLY_ ((int32_t)0xC09B0003L)

namespace eobject
{
using estd::string_buffer;
using estd::string_view;

/// \brief Defines errors that can occur when getting/setting a FieldInfo
enum Error : int32_t
{
  OK             = 0,
  DataTypeError  = ERR_ODV3_DATATYPE_DOES_NOT_MATCH_ ,
  ParamTooLong   = ERR_ODV3_DATATYPE_LENGTH_IS_TOO_LONG_ ,
  ParamTooShort  = ERR_ODV3_DATATYPE_LENGTH_IS_TOO_SHORT_ ,
  ValueTooHigh   = ERR_ODV3_VALUE_OF_PARAMETER_WRITTEN_TOO_HIGH_ ,
  ValueTooLow    = ERR_ODV3_VALUE_OF_PARAMETER_WRITTEN_TOO_LOW_  ,
  ObjectNotFound = ERR_ODV3_OBJECT_DOES_NOT_EXIST_ ,
  FieldNotFound  = ERR_ODV3_SUBINDEX_DOES_NOT_EXIST_ ,
  ReadOnly       = ERR_ODV3_OBJECT_IS_READ_ONLY_ ,
  WriteOnly      = ERR_ODV3_OBJECT_IS_WRITE_ONLY_,
  UnableToSet    = ERR_ODV3_GENERAL_PARAMETER_INCOMPATIBILITY_ 
};

/// \brief Get string describing error
string_view to_string(Error error) NOEXCEPT;

/// \brief Numbers to identify native data types
enum class DataType : uint8_t
{
  Invalid   = 0x0,
  U8        = 0x1,
  U16       = 0x2,
  U32       = 0x3,
  I8        = 0x4,
  I16       = 0x5,
  I32       = 0x6,
  String    = 0x8,
  BinString = 0x9,
  Record    = 0xA
};

/// \brief Get string describing type
string_view to_string(DataType type) NOEXCEPT;

/// \brief Lookup native datatype ID by type
template<class T>
struct Type_
{
};
template<>
struct Type_<uint8_t>
{
  static constexpr DataType id     = DataType::U8;
  static constexpr uint8_t  length = 1;
};
template<>
struct Type_<uint16_t>
{
  static constexpr DataType id     = DataType::U16;
  static constexpr uint8_t  length = 1;
};
template<>
struct Type_<uint32_t>
{
  static constexpr DataType id     = DataType::U32;
  static constexpr uint8_t  length = 1;
};
template<>
struct Type_<int8_t>
{
  static constexpr DataType id     = DataType::I8;
  static constexpr uint8_t  length = 1;
};
template<>
struct Type_<int16_t>
{
  static constexpr DataType id     = DataType::I16;
  static constexpr uint8_t  length = 1;
};
template<>
struct Type_<int32_t>
{
  static constexpr DataType id     = DataType::I32;
  static constexpr uint8_t  length = 1;
};
template<>
struct Type_<string_view>
{
  static constexpr DataType id = DataType::String;
};

template<uint8_t N>
struct Type_<estd::static_string_buffer<N>>
{
  static constexpr DataType id     = DataType::String;
  static constexpr uint8_t  length = N;
};


/// \brief Interface to Object class, which can contain inspectable properties
struct Object
{
  typedef int32_t (*GetFunctionType)(const Object&, uint8_t idx, void*, size_t);
  /// \brief Common type of set functions
  typedef int32_t (*SetFunctionType)(const Object&, uint8_t idx, const void*, size_t);

  struct detail
  {
    /// \brief Check that data is correct type and is in specified range
    /// \remarks This is a standalone function in order to make its signature more closely match other set functions,
    ///          which reduces code size and execution time
    template<class T>
    static int32_t check(T min, T max, const void* data, size_t size) NOEXCEPT
    {
      if (size > sizeof(T)) { return Error::ParamTooLong; }
      else if (size < sizeof(T))
      {
        return Error::ParamTooShort;
      }
      else if (data == nullptr)
      {
        return Error::DataTypeError;
      }
      else
      {
        /// \brief If range is empty, no range check is required
        if (min != max)
        {
          auto value = *static_cast<const T*>(data);
          if (value < min) return Error::ValueTooLow;
          else if (value > max)
            return Error::ValueTooHigh;
        }
      }
      return Error::OK;
    }

    template<class DataClass, class T, T DataClass::*member, T min = T(), T max = T()>
    static int32_t set_default(const Object& object, uint8_t, const void* data, size_t size) NOEXCEPT
    {
      auto e = check<T>(min, max, data, size);
      if (e != Error::OK) return e;
      (static_cast<DataClass*>(const_cast<void*>(object.data()))->*member) = *static_cast<const T*>(data);
      return Error::OK;
    }

    template<class T, T min = T(), T max = T()>
    static int32_t set_variable(const Object& object, uint8_t, const void* data, size_t size) NOEXCEPT
    {
      auto e = check<T>(min, max, data, size);
      if (e != Error::OK) return e;
      (*static_cast<T*>(const_cast<void*>(object.data()))) = *static_cast<const T*>(data);
      return Error::OK;
    }

    template<size_t Length>
    static int32_t set_string_variable(const Object& object, uint8_t, const void* data, size_t size) NOEXCEPT
    {
      if (size > Length) return Error::ParamTooLong;
      // If string is full length, last char should always be null
      else if(size == Length && static_cast<const char*>(data)[size-1] != '\0') return Error::ParamTooLong;

      char* dest_data = static_cast<char*>(const_cast<void*>(object.data()));
      // Copy data to string
      memcpy(dest_data, data, size);
      // Zero remaining string
      if(size < Length)
        memset(dest_data + size, 0, Length - size);
      return Error::OK;
    }

    template<class T, int32_t (*setf)(T), T min = T(), T max = T()>
    static int32_t set_wrapper(const Object&, uint8_t, const void* data, size_t size) NOEXCEPT
    {
      auto e = check<T>(min, max, data, size);
      if (e != Error::OK) return e;
      return setf(*static_cast<const T*>(data));
    }

    static int32_t set_readonly(const Object&, uint8_t, const void*, size_t) NOEXCEPT
    {
      return Error::ReadOnly;
    }


  };

  template<SetFunctionType f1, SetFunctionType f2>
  static int32_t set_chain(const Object& object, uint8_t subIdx, const void* data, size_t size) NOEXCEPT
  {
    int32_t ret = f1(object, subIdx, data, size);
    if(ret == Error::OK) ret = f2(object, subIdx, data, size);
    return ret;
  }

  /// \brief Identify category of object
  enum class ClassId : uint8_t
  {
    Invalid  = 0x0, ///< Invalid/unassigned
    Variable = 0x1, ///< Simple variable (contains single value)
    Array    = 0x2, ///< Array contains multiple named members with same type
    Record   = 0x3  ///< Record contains multiple named members with varying types
  };

  /// \brief Represents how access is controlled to this object
  enum class Permissions : uint8_t
  {
    FactoryHidden = 0, ///< Read/write in factory mode, otherwise read-only, hidden
    FactoryConfig = 1, ///< Read/write in factory mode, otherwise read-only
    Hidden        = 2, ///< Read/write, hidden
    UserConfig    = 3, ///< Read/write to all users
    Info          = 4, ///< Read only to all users
    Status        = 5, ///< Read only, dynamic to all users
    Dynamic       = 6, ///< Fully dynamic object
  };

  /// \brief Struct to store object metadata
  struct Info
  {
    typedef Object::SetFunctionType SetFunctionType;
    ClassId         otype;       ///< Object class
    DataType        type;        ///< Type of this object
    uint8_t         nelem;       ///< Maximum number of children
    Permissions     perm;        ///< Attributes associated with this object
    uint16_t        data_offset;
    uint16_t        data_size;
    SetFunctionType set_function;
  };

  /// \brief Represents typed range metadata, defining allowed range of values for this variable
  template<class T>
  struct TRange
  {
    T min;
    T max;
    /// \brief Check if range is not empty
    constexpr bool valid() const NOEXCEPT { return max != min; }
  };

  /// \brief Packages ranges of different types together for storage in a generic type
  union RangeInfo
  {
    TRange<int8_t>   i8;
    TRange<int16_t>  i16;
    TRange<int32_t>  i32;
    TRange<uint8_t>  u8;
    TRange<uint16_t> u16;
    TRange<uint32_t> u32;


    /// \brief Construct empty RangeInfo with empty range
    constexpr RangeInfo()
      : u32{}
    {}

    constexpr RangeInfo(const TRange<int8_t>& in)
      : i8{ in }
    {}
    constexpr RangeInfo(const TRange<int16_t>& in)
      : i16{ in }
    {}
    constexpr RangeInfo(const TRange<int32_t>& in)
      : i32{ in }
    {}

    constexpr RangeInfo(const TRange<uint8_t>& in)
      : u8{ in }
    {}
    constexpr RangeInfo(const TRange<uint16_t>& in)
      : u16{ in }
    {}
    constexpr RangeInfo(const TRange<uint32_t>& in)
      : u32{ in }
    {}

    __FORCEINLINE constexpr const TRange<int8_t>&   get_range(Type_<int8_t>) const { return i8; }
    __FORCEINLINE constexpr const TRange<int16_t>&  get_range(Type_<int16_t>) const { return i16; }
    __FORCEINLINE constexpr const TRange<int32_t>&  get_range(Type_<int32_t>) const { return i32; }
    __FORCEINLINE constexpr const TRange<uint8_t>&  get_range(Type_<uint8_t>) const { return u8; }
    __FORCEINLINE constexpr const TRange<uint16_t>& get_range(Type_<uint16_t>) const { return u16; }
    __FORCEINLINE constexpr const TRange<uint32_t>& get_range(Type_<uint32_t>) const { return u32; }

    template<class T>
    constexpr T min() const
    {
      return get_range(Type_<T>{}).min;
    }
    template<class T>
    constexpr T max() const
    {
      return get_range(Type_<T>{}).max;
    }

    template<class T>
    constexpr const TRange<T>& get() const
    {
      return get_range(Type_<T>{});
    }

    const void* min(size_t) const NOEXCEPT
    {
      return &u8.min;
    }
    const void* max(size_t size) const NOEXCEPT
    {
      return &u8.min + size;
    }
  };


  /// \brief Get name of object
  const string_view& name() const { return name_; }

  /// \brief Get object metadata
  const Info& info() const NOEXCEPT { return *info_; }
  /// \brief Get object category
  ClassId otype() const { return info_->otype; }
  /// \brief Get data type
  DataType type() const { return info_->type; }
  /// \brief Get number of elements contained in object
  uint8_t count() const { return info_->nelem; }

  /// \brief Set value in object
  int32_t set(uint8_t subIdx, const void* data, size_t size) const NOEXCEPT
  {
    return info_->set_function(*this, subIdx, data, size);
  }

  template<class T>
  int32_t set(uint8_t subIdx, const T& data)
  {
    return set(subIdx, &data, sizeof(T));
  }

  /// \brief Get value from object
  int32_t get(uint8_t subIdx, void* buffer, size_t size) const NOEXCEPT;

  template<class T>
  int32_t get(uint8_t subIdx, T& data)
  {
    return get(subIdx, &data, sizeof(T));
  }

  /// \brief Get entire object as binary data
  int32_t get(void* buffer, size_t size) const NOEXCEPT
  {
    if(data_ == nullptr) return Error::WriteOnly;

    int32_t data_size = info_->data_size;
    if (data_size < size) { memcpy(buffer, static_cast<const uint8_t*>(data_) + info_->data_offset, data_size); }
    return data_size;
  }

  __FORCEINLINE const void* data(uint16_t offset) const { return static_cast<const uint8_t*>(data_) + offset; }
  __FORCEINLINE const void* data() const { return data_ == nullptr ? nullptr : static_cast<const uint8_t*>(data_) + info_->data_offset; }

  size_t size() const { return info_->data_size; }
  struct FieldInfo
  {
    const Info*        info;
    const string_view* name;
    uint16_t           offset;
    uint16_t           size;

    bool valid() const { return name != nullptr; }
  };

  FieldInfo info(uint8_t subIdx) const NOEXCEPT;

  /// \brief Iterator type representing one field in this record, binding a metadata iterator to the object
  template<bool IsConst = false>
  struct Field
  {
    typedef std::conditional_t<IsConst, const Object&, Object&> ObjectReferenceType;

    ObjectReferenceType object;
    uint8_t             id;

    FieldInfo info() const NOEXCEPT { return object.info(id); }

    int32_t get_to(void* buf, size_t size) const NOEXCEPT { return object.get(id, buf, size); }

    template<class T>
    int32_t get_to(T& result) const NOEXCEPT
    {
      return get_to(id, &result, sizeof(result));
    }

    template<bool WasConst, class = std::enable_if_t<IsConst && !WasConst>>
    Field(Field<WasConst> f) NOEXCEPT
      : object(f.obj)
      , id(f.id)
    {}

    Field(ObjectReferenceType objIn, uint8_t idIn) NOEXCEPT
      : object(objIn)
      , id(idIn)
    {}

    template<bool IsConst_ = IsConst, class = std::enable_if_t<!IsConst_>>
    int32_t set_from(const void* buf, size_t size) NOEXCEPT
    {
      return object.set(id, buf, size);
    }

    template<class T, bool IsConst_ = IsConst, class = std::enable_if_t<!IsConst_>>
    int32_t set_from(const T& data) NOEXCEPT
    {
      return object.set(id, &data, sizeof(T));
    }

    Field& operator++() NOEXCEPT
    {
      ++id;
      return *this;
    }
    bool   operator!=(const Field& other) const { return id != other.id; }
    Field& operator*() { return *this; }
  };

  typedef Field<false> iterator;
  typedef Field<true>  const_iterator;

  iterator begin() { return iterator(*this, 0); }
  iterator end() { return iterator(*this, info_->nelem); }

  const_iterator begin() const { return const_iterator(*this, 0); }
  const_iterator end() const { return const_iterator(*this, info_->nelem); }

  constexpr Object(string_view name, const Info* info, const void* data)
    : name_(name)
    , info_(info)
    , data_(data)
  {}

  constexpr Object()              = default;
  constexpr Object(Object&&)      = default;
  constexpr Object(const Object&) = default;
  constexpr Object& operator=(const Object&) = default;

private:
  string_view name_ = string_view();
  const Info* info_ = nullptr;
  const void* data_ = nullptr;
};

/// \brief Get string describing object category
string_view to_string(Object::ClassId type) NOEXCEPT;

struct Variable
{
  struct Info : Object::Info
  {
    /// \brief Range of variable (default, min, max)
    Object::RangeInfo range;

    constexpr Info()
      : Object::Info{ Object::ClassId::Variable, DataType::Invalid, 1, Object::Permissions{}, 0, 0, nullptr }
      , range{}
    {}

    /// \brief Create simple info
    template<class T>
    constexpr Info(Object::Permissions perm, uint16_t offset, SetFunctionType setf, T min, T max)
      : Object::Info{ Object::ClassId::Variable, Type_<T>::id, 1, perm, offset, sizeof(T), setf }
      , range{ Object::TRange<T>{ min, max } }
    {}

    constexpr Info(Object::Permissions perm, DataType type, SetFunctionType setf, uint16_t length)
      : Object::Info{ Object::ClassId::Variable, type, 1, perm, 0, length, setf }
      , range{}
    {}
  };

  template<class T, T min = T(), T max = T()>
  constexpr Info static make_info(
    Object::Permissions perm, Object::Info::SetFunctionType setf = Object::detail::set_variable<T, min, max>) NOEXCEPT
  {
    return Info(perm, 0, setf, min, max);
  }

  template<uint16_t Length>
  constexpr Info static make_string_info(Object::Permissions perm, 
    Object::Info::SetFunctionType setf = Object::detail::set_string_variable<Length>) NOEXCEPT
  {
    return Info(perm, DataType::String, setf, Length);
  }

  template<uint16_t Length>
  constexpr Info static make_binstring_info(Object::Permissions perm, 
    Object::Info::SetFunctionType setf = Object::detail::set_string_variable<Length>) NOEXCEPT
  {
    return Info(perm, DataType::BinString, setf, Length);
  }
};

struct Array
{
  struct detail
  {
    /// Helper function to get data from standalone array
    template< class T, uint8_t Count>
    static int32_t get_data(const Object& obj, uint8_t subIdx, void* data, size_t size) NOEXCEPT
    {
      if (subIdx == 0)
      {
        *static_cast<uint8_t*>(data) = Count;
        return sizeof(uint8_t);
      }
      else if (subIdx > Count)
        return static_cast<int>(Error::FieldNotFound);
      if (sizeof(T) > size) return static_cast<int>(Error::ParamTooShort);

      *static_cast<T*>(data) = (static_cast<const T*>(obj.data()))[subIdx - 1];
      return sizeof(T);
    }

    /// Helper function to set data to standalone array
    template< class T, uint8_t Count, T min = T(), T max = T()>
    static int32_t set_data(T (&object)[Count], uint8_t subIdx, const void* data, size_t size) NOEXCEPT
    {
      if (subIdx == 0) return Error::ReadOnly;
      else if (subIdx > Count)
        return Error::FieldNotFound;

      auto v = Object::detail::check<T>(data, min, max);
      if (v.error != Error::OK) return v.error;
      object[subIdx - 1] = v.value;
      return Error::OK;
    }

    /// Helper function to get data from array contained in DataClass (struct)
    template<class DataClass, class T, uint8_t Count, T (DataClass::*member)[Count]>
    static int32_t get_data(const Object& obj, uint8_t subIdx, void* data, size_t size) NOEXCEPT
    {
      if (subIdx == 0)
      {
        *static_cast<uint8_t*>(data) = Count;
        return sizeof(uint8_t);
      }
      else if (subIdx > Count)
        return static_cast<int>(Error::FieldNotFound);
      if (sizeof(T) > size) return static_cast<int>(Error::ParamTooShort);

      *static_cast<T*>(data) = (static_cast<const DataClass&>(obj.data()).*member)[subIdx - 1];
      return sizeof(T);
    }

    /// Helper function to set data to array contained in DataClass (struct)
    template<class DataClass, class T, uint8_t Count, T (DataClass::*member)[Count], T min = T(), T max = T()>
    static int32_t set_data(DataClass& object, uint8_t subIdx, const void* data, size_t size) NOEXCEPT
    {
      if (subIdx == 0) return Error::ReadOnly;
      else if (subIdx > Count)
        return Error::FieldNotFound;

      auto v = Object::detail::check<T>(data, min, max);
      if (v.error != Error::OK) return v.error;
      (static_cast<DataClass&>(object.data).*member)[subIdx - 1] = v.value;
      return Error::OK;
    }
  };

  struct Info : Object::Info
  {
    /// \brief Range of variable (default, min, max)
    Object::RangeInfo range;
    string_view       names[1]; ///< Pointer to array of subobject names

    typedef const string_view* iterator;

    /// \brief Find field by name
    /// \returns index to field, or index past end of array
    uint8_t find(string_view name) const
    {
      auto it = names;
      for (; it != names + nelem; ++it)
      {
        if (*it == name) return it - names;
      }
      return nelem;
    }

    /// \brief Get name of field by index
    string_view operator[](uint8_t id) const { return id < nelem ? names[id] : string_view{}; }

    /// \brief Construct array metadata
    template<class DataClass, class T, uint8_t Count>
    constexpr Info(Object::Permissions perm,
                   uint16_t          offset,
                   T (DataClass::*member)[Count],
                   const string_view name,
                   SetFunctionType   setf_in,
                   T                 min,
                   T                 max)
      : Object::Info{ Object::ClassId::Array, Type_<T>::id, Count, perm, offset, sizeof(T) * Count, setf_in }
      , range{ Object::TRange<T>{min, max} }
      , names{ name }
    {}

    template<class T, uint8_t Count>
    constexpr Info(Object::Permissions perm,
                   T (&)[Count],
                   const string_view name,
                   SetFunctionType   setf_in,
                   T                 min,
                   T                 max)
      : Object::Info{ Object::ClassId::Array, Type_<T>::id, Count, perm, 0, sizeof(T) * Count, setf_in }
      , range{ Object::TRange<T>{min, max} }
      , names{ name }
    {}

    iterator begin() const { return names; }
    iterator end() const { return names + nelem; }
  };

  /// \brief Subclass for building a flat array of field names
  template<uint8_t Count>
  struct TInfo : Info
  {
    static constexpr uint8_t fields_n_count = Count > 0 ? Count : 1;
    string_view              names_n[fields_n_count];

    /// Specialization to init array constexpr
    template<class DataClass, class T, uint8_t first, uint8_t... Ns>
    constexpr TInfo(Object::Permissions perm,
                    uint16_t offset,
                    T (DataClass::*member)[Count],
                    string_view(&&names_in)[Count],
                    SetFunctionType setf_in,
                    T               min,
                    T max,
                    std::integer_sequence<uint8_t, first, Ns...>)
      : Info{ perm,offset, member,  names_in[0], setf_in, min, max }
      , names_n{ names_in[Ns]... }
    {}

    /// Specialization to init array constexpr
    template<class T, uint8_t first, uint8_t... Ns>
    constexpr TInfo(Object::Permissions perm,
                    T (&array)[Count],
                    string_view(&&names_in)[Count],
                    SetFunctionType setf_in,
                    T               min,
                    T max,
                    std::integer_sequence<uint8_t, first, Ns...>)
      : Info{ perm, array,  names_in[0], setf_in, min, max }
      , names_n{ names_in[Ns]... }
    {}

    /// Specialization for a single field
    template<class DataClass, class T, uint8_t first>
    constexpr TInfo(Object::Permissions perm,
                    uint16_t offset,
                    T (DataClass::*member)[Count],
                    string_view(&&names_in)[Count],
                    SetFunctionType setf_in,
                    T               min,
                    T max,
                    std::integer_sequence<uint8_t, first>)
      : Info{ perm, offset,  member, names_in[0], setf_in, min, max }
      , names_n{}
    {}

    /// Specialization for a single field
    template<class T, uint8_t first>
    constexpr TInfo(Object::Permissions perm,
                   T (&array)[Count],
                    string_view(&&names_in)[Count],
                    SetFunctionType setf_in,
                    T               min,
                    T max,
                    std::integer_sequence<uint8_t, first>)
      : Info{ perm, array, names_in[0], setf_in, min, max }
      , names_n{}
    {}

    /// Create array located within DataClass structure
    template<class DataClass, class T>
    constexpr TInfo(Object::Permissions perm,
                    uint16_t offset,
                    T (DataClass::*member)[Count],
                    string_view(&&names_in)[Count],
                    SetFunctionType setf_in,
                    T               min,
                    T               max)
      : TInfo(perm, offset, member,  std::move(names_in), setf_in, min, max, std::make_integer_sequence<uint8_t, Count>{})
    {}

    /// Create standalone array
    template<class T>
    constexpr TInfo(Object::Permissions perm,
                    T (&array)[Count],
                    string_view(&&names_in)[Count],
                    SetFunctionType setf_in,
                    T               min,
                    T               max)
      : TInfo(perm, array,  std::move(names_in), setf_in, min, max, std::make_integer_sequence<uint8_t, Count>{})
    {}

    /// Constructor to create array info without field names
    template<class DataClass, class T>
    constexpr TInfo(Object::Permissions perm,
                    uint16_t offset,
                    T (DataClass::*member)[Count],
                    SetFunctionType setf_in,
                    T               min,
                    T               max)
      : Info(perm, offset, member, string_view{}, setf_in, min, max)
      , names_n{}
    {}

    /// Constructor to create array info without field names
    template<class T>
    constexpr TInfo(Object::Permissions perm,
                    T (&array)[Count],
                    SetFunctionType setf_in,
                    T               min,
                    T               max)
      : Info(perm, array, string_view{}, setf_in, min, max)
      , names_n{}
    {}
  };

  /// \brief Make metadata structure for array
  template<uint8_t Count, class DataClass, class T>
  static constexpr TInfo<Count> make_info(Object::Permissions perm,
                                          uint16_t            offset,
                                          T (DataClass::*member)[Count],
                                          string_view(&&names_in)[Count],
                                          Object::Info::SetFunctionType setf_in, T min=T(), T max=T())
  {
    return TInfo<Count>(perm, offset, member, std::move(names_in), setf_in, min, max);
  }

   /// \brief Make metadata structure for array
  template<class T, uint8_t Count>
  static constexpr TInfo<Count> make_info(Object::Permissions perm,
                                          T (&array)[Count],
                                          string_view(&&names_in)[Count],
                                          Object::Info::SetFunctionType setf_in, T min=T(), T max=T())
  {
    return TInfo<Count>(perm, array, std::move(names_in), setf_in, min, max);
  }

  /// \brief Make metadata structure for array without names
  template<uint8_t Count, class DataClass, class T>
  static constexpr TInfo<Count> make_info(Object::Permissions perm,
                                          uint16_t            offset,
                                          T (DataClass::*member)[Count],
                                          Object::Info::SetFunctionType setf_in, T min=T(), T max=T())
  {
    return TInfo<Count>(perm, offset, member, setf_in, min, max);
  }

    /// \brief Make metadata structure for array without names
  template<class T, uint8_t Count>
  static constexpr TInfo<Count> make_info(Object::Permissions perm,
                                           T (&array)[Count],
                                          Object::Info::SetFunctionType setf_in, T min=T(), T max=T())
  {
    return TInfo<Count>(perm, array, setf_in, min, max);
  }

};

struct Record
{
  struct detail
  {
    static int32_t set_data(const Object& obj, uint8_t subIdx, const void* data, size_t size) NOEXCEPT
    {
      if (obj.info().otype != Object::ClassId::Record || subIdx > obj.info().nelem) return Error::FieldNotFound;
      if (subIdx == 0) return Error::ReadOnly;

      return static_cast<const Record::Info&>(obj.info()).fields[subIdx - 1].set_function(obj, subIdx, data, size);
    }

  };

  struct FieldInfo : Variable::Info
  {
    /// \brief Name of this field
    string_view name;

    constexpr FieldInfo() NOEXCEPT
      : Info()
      , name()
    {}

    /// \brief Create Field metadata definition
    template<class T>
    constexpr FieldInfo(
      Object::Permissions perm, string_view nameIn, uint16_t offset, SetFunctionType setf, T min, T max)
      : Variable::Info{ perm, offset, setf, min, max }
      , name(nameIn)
    {}
  };

  template<uint8_t Count = uint8_t(0), uint16_t Offset = uint8_t(0), uint16_t Size = uint8_t(0)>
  struct FieldList
  {

    const FieldInfo fields_arr[Count + 1];

    constexpr FieldList() NOEXCEPT {}

    template<uint16_t OldOffset, uint16_t OldSize, uint8_t... Ns>
    constexpr FieldList(const FieldList<uint8_t(Count - 1), OldOffset, OldSize>& info,
                        const FieldInfo&                                         last,
                        std::integer_sequence<uint8_t, Ns...>) NOEXCEPT
      : /*data_offset(info.data_offset), data_size(info.data_size), */ fields_arr{ info.fields_arr[Ns]..., last }
    {}

    template<uint16_t OldOffset, uint16_t OldSize>
    constexpr FieldList(const FieldList<uint8_t(Count - 1), OldOffset, OldSize>& info, const FieldInfo& last) NOEXCEPT
      : FieldList(info, last, std::make_integer_sequence<uint8_t, Count - 1>{})
    {}

    template<class DataClass,
             class T,
             T DataClass::*member,
             size_t        offset,
             T             min,
             T             max,
             uint16_t      NewOffset = Count == 0 ? offset : Offset,
             uint16_t      NewSize   = Count == 0 ? sizeof(T) : Size + sizeof(T)>
    constexpr FieldList<Count + 1, NewOffset, NewSize> field(
      Object::Permissions           perm,
      string_view                   name,
      Object::Info::SetFunctionType setf = Object::detail::set_default<DataClass, T, member, min, max>) NOEXCEPT
    {
      static_assert(Count == 0 || offset == Size + Offset,
                    "Gap in structure! Make sure all fields are defined in order and none are missing");

      return FieldList<Count + 1, NewOffset, NewSize>(
        *this, FieldInfo{ perm, name, static_cast<uint16_t>(offset), setf, min, max });
    }


    template<class T,
             int32_t (*setf)(T),
             size_t   offset,
             T        min,
             T        max,
             uint16_t NewOffset = Count == 0 ? offset : Offset,
             uint16_t NewSize   = Count == 0 ? sizeof(T) : Size + sizeof(T)>
    constexpr FieldList<Count + 1, NewOffset, NewSize> setter_field(Object::Permissions perm, string_view name) NOEXCEPT
    {
      static_assert(Count == 0 || offset == Size + Offset,
                    "Gap in structure! Make sure all fields are defined in order and none are missing");

      return FieldList<Count + 1, NewOffset, NewSize>(
        *this,
        FieldInfo{
          perm, name, static_cast<uint16_t>(offset), &Object::detail::set_wrapper<T, setf, min, max>, min, max });
    }
  };

  /// \brief Record metadata object
  struct Info : Object::Info
  {
    const FieldInfo fields[1]; ///< Properties contained in this object

    typedef const FieldInfo* iterator;
    /// \brief Get iterator to metadata of first field in this record
    iterator begin() const { return fields; }
    /// \brief Get iterator to metadata of last field in this record
    iterator end() const { return fields + nelem; }

    /// \brief Find field metadata by name
    iterator find(string_view name) const;

    constexpr Info(
      Object::Permissions perm, const FieldInfo& field, uint8_t count, uint16_t start_offset, uint16_t size)
      : Object::Info{ Object::ClassId::Record, DataType::Record, count, perm, start_offset, size, detail::set_data }
      , fields{ field }
    {}

    constexpr Info(
      Object::Permissions perm, const FieldInfo& field, uint8_t count, uint16_t start_offset, uint16_t size, SetFunctionType setf)
      : Object::Info{ Object::ClassId::Record, DataType::Record, count, perm, start_offset, size, setf }
      , fields{ field }
    {}
  };

  template<uint8_t Count>
  struct TInfo : Info
  {
    static constexpr uint8_t fields_n_count = Count > 0 ? Count : 1;
    const FieldInfo          fields_n[fields_n_count];

    /// Specialization to init array constexpr
    template<uint16_t Offset, uint16_t Size, uint8_t first, uint8_t... Ns>
    constexpr TInfo(Object::Permissions              perm,
                    FieldList<Count, Offset, Size>&& fields,
                    SetFunctionType setf,
                    std::integer_sequence<uint8_t, first, Ns...>)
      : Info{ perm, fields.fields_arr[0], Count, Offset, Size, setf }
      , fields_n{ fields.fields_arr[Ns]... }
    {}

    /// Specialization for a single field
    template<uint8_t first, uint16_t Offset, uint16_t Size>
    constexpr TInfo(Object::Permissions              perm,
                    FieldList<Count, Offset, Size>&& fields,
                    SetFunctionType setf,
                    std::integer_sequence<uint8_t, first>)
      : Info{ perm, fields.fields_arr[0], Count, Offset, Size, setf }
      , fields_n{}
    {}

    /// Constructor that creates full info structure
    template<uint16_t Offset, uint16_t Size>
    constexpr TInfo(Object::Permissions perm, FieldList<Count, Offset, Size>&& fields, SetFunctionType setf=detail::set_data)
      : TInfo(perm, std::move(fields), setf, std::make_integer_sequence<uint8_t, Count>{})
    {}

  };

  static constexpr FieldList<0> fields() { return FieldList<0>{}; }

  template<uint8_t Count, uint16_t Offset, uint16_t Size>
  static constexpr TInfo<Count> make_info(Object::Permissions perm, FieldList<Count, Offset, Size>&& fields, Info::SetFunctionType setf=detail::set_data)
  {
    return TInfo<Count>(perm, std::move(fields), setf);
  }

};

/// \brief Object Dictionary stores index of objects by address
struct Dictionary
{
  /// \brief Structure holds indexed information about item in dictionary
  struct Item
  {
    uint16_t address     = 0;
    uint16_t pdo_mapping = 0;
    Object   object      = Object();
  };

  /// \brief Structure holds query information for searching the dictionary using strings
  struct Query
  {
    estd::string_view   object_name;
    estd::string_view   subobject_name;
    const Item*         item;
    const Object::Info* info;
    int16_t             subIdx;

    /// \brief Check that charaacter is object separator
    static bool issep(char C) NOEXCEPT { return C == '.' || C == ':' || C == '/'; }

    /// \brief construct query from string
    Query(estd::string_view& str)
      : object_name(estd::next_token(str, issep))
      , subobject_name(estd::next_token(str, issep))
      , info(nullptr)
      , subIdx(-1)
    {
      estd::trim_prefix(object_name, estd::isspace);
      estd::trim_suffix(subobject_name, estd::isspace);
    }
  };

  typedef const Item* pointer;

  /// \brief Constructor builds index of objects, optionally at compile-time
  constexpr Dictionary(uint16_t count_in, const Item item)
    : count(count_in)
    , items{ item }
  {}

  /// \brief Get iterator to first object in dictionary
  pointer begin() const NOEXCEPT { return items; }
  /// \brief Get iterator past last object in dictionary
  pointer end() const NOEXCEPT { return items + count; }

  /// \brief Get object by address
  /// \remarks Uses binary search on a sorted list
  /// \returns pointer to object found, or nullptr if no object found at this address
  const Object* get(uint16_t address) const NOEXCEPT;

  /// \brief Write value to object in dicitionary
  int32_t write(uint16_t address, uint8_t subIdx, const void* data, size_t size) const
  {
    const Object* o = get(address);
    if (o == nullptr) return Error::ObjectNotFound;
    return o->set(subIdx, data, size);
  }

  /// \brief Read value from object in dictionary
  int32_t read(uint16_t address, uint8_t subIdx, void* data, size_t size) const
  {
    const Object* o = get(address);
    if (o == nullptr) return static_cast<int>(Error::ObjectNotFound);
    return o->get(subIdx, data, size);
  }

  /// \brief Find object by name
  const Item* find(const string_view& name) const NOEXCEPT;

  /// \brief Get object/subobject from dictionary based on string
  int32_t query(Query& q) const NOEXCEPT;

  size_t count;
  Item   items[1];
};

/// \brief Class for constructing a constexxpr dictionary that includes storage for the dictionary
template<uint16_t Count>
struct TDictionary : Dictionary
{
  static constexpr uint16_t items_n_count = Count > 0 ? Count : 1;

  Item items_n[items_n_count];

  /// \brief Compares addresses of objects in index
  constexpr static bool compare_address(const Dictionary::Item& l, const Dictionary::Item& r) NOEXCEPT
  {
    return l.address < r.address;
  }

  /// \brief Compares names of objects in index
  constexpr static bool compare_name(const Dictionary::Item& l, const Dictionary::Item& r) NOEXCEPT
  {
    return l.object.name() < r.object.name();
  }

  template<uint16_t first, uint16_t... Ns>
  constexpr TDictionary(estd::array<Dictionary::Item, Count>&& objects,
                        std::integer_sequence<uint16_t, first, Ns...>) NOEXCEPT
    : Dictionary(Count, objects[0])
    , items_n{ objects[Ns]... }
  {}

  template<uint16_t first>
  constexpr TDictionary(estd::array<Dictionary::Item, Count>&& objects, std::integer_sequence<uint16_t, first>) NOEXCEPT
    : Dictionary(Count, objects[0])
    , items_n{}
  {}

  /// \brief Constructor builds index of objects, optionally at compile-time
  constexpr TDictionary(estd::array<Dictionary::Item, Count>&& objects) NOEXCEPT
    : TDictionary(estd::sort(std::move(objects), compare_address), std::make_integer_sequence<uint16_t, Count>{})
  {}
};

template<class... Ts>
static constexpr TDictionary<sizeof...(Ts)> make_dictionary(Ts... args) NOEXCEPT
{
  constexpr size_t size = sizeof...(Ts);
  return TDictionary<size>(estd::array<Dictionary::Item, size>{ args... });
}

}
