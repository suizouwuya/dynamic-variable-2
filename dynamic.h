#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <algorithm>
#include <list>
#include <vector>
#include <mutex>

/**
 * @author Andrey Bezborodov <andrey@wavecon.ru>
 * @brief Dynamic type implementation
 */
class dynamic {
public:
  //! Stored type
  enum class Type {
    //! Stored type is undefined
    Undefined,
    //! Signed integer value
    SignedInt,
    //! Unsigned integer value
    UnsignedInt,
    //! Floating point
    Float,
    //! String value
    String,
    //! Binary data
    Binary,
  };
public:

  dynamic() {__init__();}

  dynamic(std::int8_t Value)    {__init__(); *this = Value;}
  dynamic(std::int16_t Value)   {__init__(); *this = Value;}
  dynamic(std::int32_t Value)   {__init__(); *this = Value;}
  dynamic(std::int64_t Value)   {__init__(); *this = Value;}

  dynamic(std::uint8_t Value)   {__init__(); *this = Value;}
  dynamic(std::uint16_t Value)  {__init__(); *this = Value;}
  dynamic(std::uint32_t Value)  {__init__(); *this = Value;}
  dynamic(std::uint64_t Value)  {__init__(); *this = Value;}

  dynamic(bool Value)   {__init__(); *this = Value;}
  dynamic(float Value)  {__init__(); *this = Value;}
  dynamic(double Value) {__init__(); *this = Value;}

  dynamic(const char * Value)                       {__init__(); *this = Value;}
  dynamic(const std::string & Value)                {__init__(); *this = Value;}
  dynamic(const std::vector<std::uint8_t> & Value)  {__init__(); *this = Value;}
  dynamic(const   dynamic & Value)                  {__init__(); *this = Value;}


public:
  void operator = (std::int8_t Value)   {storeNumeric<std::int8_t>(Value, Type::SignedInt);}
  void operator = (std::int16_t Value)  {storeNumeric<std::int16_t>(Value, Type::SignedInt);}
  void operator = (std::int32_t Value)  {storeNumeric<std::int32_t>(Value, Type::SignedInt);}
  void operator = (std::int64_t Value)  {storeNumeric<std::int64_t>(Value, Type::SignedInt);}

  void operator = (std::uint8_t Value)  {storeNumeric<std::uint8_t>(Value, Type::UnsignedInt);}
  void operator = (std::uint16_t Value) {storeNumeric<std::uint16_t>(Value, Type::UnsignedInt);}
  void operator = (std::uint32_t Value) {storeNumeric<std::uint32_t>(Value, Type::UnsignedInt);}
  void operator = (std::uint64_t Value) {storeNumeric<std::uint64_t>(Value, Type::UnsignedInt);}

  void operator = (bool Value)          {storeNumeric<bool>(Value, Type::SignedInt);}
  void operator = (float Value)         {storeNumeric<float>(Value, Type::Float);}
  void operator = (double Value)        {storeNumeric<double>(Value, Type::Float);}
  void operator = (const char * Value)  {
    std::size_t Size = std::strlen(Value);
    if (!Size) return;
    resize(Size);
    value_rw().assign(Value, Value + Size);
    setType(Type::String);
  }
  void operator = (const std::string & Value) {
    if (Value.empty()) return;
    resize(Value.size());
    std::copy(Value.begin(), Value.end(), value_rw().begin());
    setType(Type::String);
  }
  void operator = (const std::vector<std::uint8_t> & Value) {
    if (Value.empty()) return;
    resize(Value.size());
    copy(Value.begin(), Value.end(), value_rw().begin());
    setType(Type::Binary);
  }
  void operator = (const   dynamic & Value) {
    resize(Value.size());
    std::copy(Value.value().begin(), Value.value().end(), value_rw().begin());
    _StoredType = Value._StoredType;
  }

public:
  operator std::int8_t()  const   {return asNumeric<std::int8_t>();}
  operator std::int16_t() const   {return asNumeric<std::int16_t>();}
  operator std::int32_t() const   {return asNumeric<std::int32_t>();}
  operator std::int64_t() const   {return asNumeric<std::int64_t>();}

  operator std::uint8_t() const   {return asNumeric<std::uint8_t>();}
  operator std::uint16_t() const  {return asNumeric<std::uint16_t>();}
  operator std::uint32_t() const  {return asNumeric<std::uint32_t>();}
  operator std::uint64_t() const  {return asNumeric<std::uint64_t>();}

  operator bool() const {
    switch (type()) {
      case Type::UnsignedInt: case Type::SignedInt: case Type::Float: return asNumeric<bool>();
      case Type::String: {
        std::string Value = *this;
        std::transform(Value.begin(), Value.end(), Value.begin(), ::tolower);
        if (      Value == "true"   || Value == "yes" || Value == "1" || Value == "on"   || Value == "enabled"  ) return true;
        else if ( Value == "false"  || Value == "no"  || Value == "0" || Value == "off"  || Value == "disabled" ) return false;
        else throw std::bad_cast();
      }
      default: throw std::bad_cast();
    }
  }
  operator float() const  {return asNumeric<float>();}
  operator double() const {return asNumeric<double>();}
  operator std::string() const {
    switch (type()) {
      case Type::Undefined:           return std::string();
      case Type::SignedInt:
        switch (size()) {
          case sizeof(std::int8_t):   return std::to_string(cast<std::int8_t>());
          case sizeof(std::int16_t):  return std::to_string(cast<std::int16_t>());
          case sizeof(std::int32_t):  return std::to_string(cast<std::int32_t>());
          case sizeof(std::int64_t):  return std::to_string(cast<std::int64_t>());
          default: throw std::bad_cast();
        }
      case Type::UnsignedInt:
        switch (size()) {
          case sizeof(std::uint8_t):   return std::to_string(cast<std::uint8_t>());
          case sizeof(std::uint16_t):  return std::to_string(cast<std::uint16_t>());
          case sizeof(std::uint32_t):  return std::to_string(cast<std::uint32_t>());
          case sizeof(std::uint64_t):  return std::to_string(cast<std::uint64_t>());
          default: throw std::bad_cast();
        }
      case Type::Float:
        switch (size()) {
          case sizeof(float):   return std::to_string(cast<float>());
          case sizeof(double):  return std::to_string(cast<double>());
          default: throw std::bad_cast();
        }
      case Type::String:
      case Type::Binary: {
        std::string Result;
        Result.resize(size());
        std::copy(value().begin(), value().end(), Result.begin());
        return Result;
      }
      default: throw std::bad_cast();
    }
  }
  operator std::vector<std::uint8_t>() const {
    return value();
  }

  template <typename T>
  T cast() const {
    return static_cast<T>(*this);
  }

public:
  bool operator == (std::int8_t Second) const   {return Second == cast<std::int8_t>();}
  bool operator == (std::int16_t Second) const  {return Second == cast<std::int16_t>();}
  bool operator == (std::int32_t Second) const  {return Second == cast<std::int32_t>();}
  bool operator == (std::int64_t Second) const  {return Second == cast<std::int64_t>();}

  bool operator == (std::uint8_t Second) const  {return Second == cast<std::uint8_t>();}
  bool operator == (std::uint16_t Second) const {return Second == cast<std::uint16_t>();}
  bool operator == (std::uint32_t Second) const {return Second == cast<std::uint32_t>();}
  bool operator == (std::uint64_t Second) const {return Second == cast<std::uint64_t>();}

  bool operator == (bool Second) const          {return Second == cast<bool>();}
  bool operator == (float Second) const         {return Second == cast<float>();}
  bool operator == (double Second) const        {return Second == cast<double>();}
  bool operator == (const char * Second) const  {return cast<std::string>() == Second;}
  bool operator == (const std::string & Second) const {return Second == cast<std::string>();}

  bool operator == (const dynamic & Second) const {
    Type    T = getMajorType(*this, Second);
    size_t  S = std::max(size(), Second.size());
    switch (T) {
      case Type::SignedInt:
        switch (S) {
          case sizeof(std::int8_t):   return (cast<std::int8_t>() == Second.cast<std::int8_t>());
          case sizeof(std::int16_t):  return (cast<std::int16_t>() == Second.cast<std::int16_t>());
          case sizeof(std::int32_t):  return (cast<std::int32_t>() == Second.cast<std::int32_t>());
          case sizeof(std::int64_t):  return (cast<std::int64_t>() == Second.cast<std::int64_t>());
          default: throw std::bad_cast();
        }
      case Type::UnsignedInt:
        switch (S) {
          case sizeof(std::uint8_t):   return (cast<std::uint8_t>() == Second.cast<std::uint8_t>());
          case sizeof(std::uint16_t):  return (cast<std::uint16_t>() == Second.cast<std::uint16_t>());
          case sizeof(std::uint32_t):  return (cast<std::uint32_t>() == Second.cast<std::uint32_t>());
          case sizeof(std::uint64_t):  return (cast<std::uint64_t>() == Second.cast<std::uint64_t>());
          default: throw std::bad_cast();
        }
      case Type::Float:
        switch (S) {
          case sizeof(float):   return (cast<float>() == Second.cast<float>());
          case sizeof(double):  return (cast<double>() == Second.cast<double>());
          default: throw std::bad_cast();
        }
      case Type::String: return (cast<std::string>() == Second.cast<std::string>());
      case Type::Binary:
        if (size() != Second.size()) return false;
        return !std::memcmp(data(), Second.data(), S);
      default:
        throw std::bad_cast();
    }
  }

public:
  bool operator != (std::int8_t Second) const     {return Second != cast<std::int8_t>();}
  bool operator != (std::int16_t Second) const    {return Second != cast<std::int8_t>();}
  bool operator != (std::int32_t Second) const    {return Second != cast<std::int8_t>();}
  bool operator != (std::int64_t Second) const    {return Second != cast<std::int8_t>();}

  bool operator != (std::uint8_t Second) const    {return Second != cast<std::uint8_t>();}
  bool operator != (std::uint16_t Second) const   {return Second != cast<std::uint16_t>();}
  bool operator != (std::uint32_t Second) const   {return Second != cast<std::uint32_t>();}
  bool operator != (std::uint64_t Second) const   {return Second != cast<std::uint64_t>();}

  bool operator != (bool Second) const    {return Second != cast<bool>();}
  bool operator != (float Second) const   {return Second != cast<float>();}
  bool operator != (double Second) const  {return Second != cast<double>();}

  bool operator != (const std::string & Second) const {return Second != cast<std::string>();}
  bool operator != (const char * Second) const        {return cast<std::string>() != Second;}

public:
  bool operator < (std::int8_t Second) const          {return cast<std::int8_t>() < Second;}
  bool operator < (std::int16_t Second) const         {return cast<std::int16_t>() < Second;}
  bool operator < (std::int32_t Second) const         {return cast<std::int32_t>() < Second;}
  bool operator < (std::int64_t Second) const         {return cast<std::int64_t>() < Second;}
  bool operator < (std::uint8_t Second) const         {return cast<std::uint8_t>() < Second;}
  bool operator < (std::uint16_t Second) const        {return cast<std::uint16_t>() < Second;}
  bool operator < (std::uint32_t Second) const        {return cast<std::uint32_t>() < Second;}
  bool operator < (std::uint64_t Second) const        {return cast<std::uint64_t>() < Second;}
  bool operator < (const std::string & Second) const  {return cast<std::string>() < Second;}
  bool operator < (const char * Second) const         {return cast<std::string>() < Second;}

  bool operator < (const dynamic& Second) const { return false; }

public:
  bool operator > (const dynamic& Second) const { return false; }

public:
  dynamic operator + (std::int8_t Second) const {
    switch (_StoredType) {
      case Type::SignedInt:
        switch (size()) {
          case sizeof(std::int8_t):   return (cast<std::int8_t>() + Second);
          case sizeof(std::int16_t):  return (cast<std::int16_t>() + Second);
          case sizeof(std::int32_t):  return (cast<std::int32_t>() + Second);
          case sizeof(std::int64_t):  return (cast<std::int64_t>() + Second);
          default: throw std::bad_cast();
        }

      // - Not implemented yet
      default: throw std::bad_exception();
    }
  }

  void operator += (std::int8_t Second) {
    switch (type()) {
      case Type::Undefined: *this = Second; return;
      case Type::SignedInt:
        switch (size()) {
          case sizeof(std::int8_t):   *this = (cast<std::int8_t>() + Second);   return;
          case sizeof(std::int16_t):  *this = (cast<std::int16_t>() + Second);  return;
          case sizeof(std::int32_t):  *this = (cast<std::int32_t>() + Second);  return;
          case sizeof(std::int64_t):  *this = (cast<std::int64_t>() + Second);  return;
          default: throw std::bad_cast();
        }
      case Type::UnsignedInt:
        switch (size()) {
          case sizeof(std::uint8_t):  *this = (cast<std::uint8_t>() + Second);  return;
          case sizeof(std::uint16_t): *this = (cast<std::uint16_t>() + Second); return;
          case sizeof(std::uint32_t): *this = (cast<std::uint32_t>() + Second); return;
          case sizeof(std::uint64_t): *this = (cast<std::uint64_t>() + Second); return;
          default: throw std::bad_cast();
        }
      case Type::Float:
        switch (size()) {
          case sizeof(float):   *this = (cast<float>() + Second);   return;
          case sizeof(double):  *this = (cast<double>() + Second);  return;
          default: throw std::bad_cast();
        }
      case Type::String: {

      }
      case Type::Binary: {

      }
    }
  }

  dynamic operator + (std::int16_t Second) const {
    return dynamic();
  }

public:
  //! Get size of raw data
  std::size_t size() const {
    return value().size();
  }

  //! Access raw data buffer
  const std::uint8_t * data() const {
    return value().data();
  }

  //! Access raw data buffer
  std::uint8_t * data() {
    return &value_rw()[0];
  }

  //! Access raw data
  const std::vector<std::uint8_t> & value() const {
    return _Value;
  }

  //! Access raw data
  std::vector<std::uint8_t> & value() {
    return _Value;
  }



  //! Clear value
  void clear() {
    value_rw().clear();
    setType(Type::Undefined);
  }

  //! Copy all raw data to buffer
  void copyTo(std::uint8_t * Buffer, std::size_t BufferSize) const {
    if (BufferSize < size()) throw std::overflow_error("Buffer to small");
    std::copy(value().begin(), value().end(), Buffer);
  }

  //! Copy given ammount of raw data to buffer
  void copyTo(std::uint8_t * Buffer, std::size_t BufferSize, std::size_t Count) const {
    if (BufferSize < size()) throw std::overflow_error("Buffer to small");
    if (Count > size()) throw std::length_error("Requested count greater than data size");
    std::copy(value().begin(), value().begin() + Count, Buffer);
  }

  //! Get stored type
  Type type() const {
    return _StoredType;
  }

  //! Get major type for two given dynamic variables
  static Type getMajorType(const dynamic & T0, const dynamic & T1) {
    return Type::Undefined;
  }

  //! Resize container to fit new size
  void resize(std::size_t NewSize) {
    value_rw().resize(NewSize);
    if (value().capacity() > NewSize) std::vector<std::uint8_t>().swap(value_rw());
  }

private:
  //! Init function
  void __init__() {
    setType(Type::Undefined);
  }

  //! Access to value container in R/W mode
  std::vector<std::uint8_t> & value_rw() {
    return _Value;
  }

  //! Get stored type as numeric type
  template <typename T>
  T asNumeric() const {
    switch (type()) {
      // --- Cast from undefined
      case Type::Undefined: throw std::bad_cast();
      // --- Cast from signed integer
      case Type::SignedInt: {
        switch (size()) {
          case sizeof(std::int8_t):   return *reinterpret_cast<const std::int8_t*>(data());
          case sizeof(std::int16_t):  return *reinterpret_cast<const std::int16_t*>(data());
          case sizeof(std::int32_t):  return *reinterpret_cast<const std::int16_t*>(data());
          case sizeof(std::int64_t):  return *reinterpret_cast<const std::int16_t*>(data());
          default: throw std::bad_cast();
        }
      }; break;
      // --- Cast from unsigned integer
      case Type::UnsignedInt: {
        switch (size()) {
          case sizeof(std::uint8_t):    return *reinterpret_cast<const std::uint8_t*>(data());
          case sizeof(std::uint16_t):   return *reinterpret_cast<const std::uint16_t*>(data());
          case sizeof(std::uint32_t):   return *reinterpret_cast<const std::uint32_t*>(data());
          case sizeof(std::uint64_t):   return *reinterpret_cast<const std::uint64_t*>(data());
          default: throw std::bad_cast();
        }
      }; break;
      // --- Cast from float
      case Type::Float: {
        // - Determine correct float type
        if (size() == sizeof(float))        return *reinterpret_cast<const float*>(data());
        else if (size() == sizeof(double))  return *reinterpret_cast<const double*>(data());
        else throw std::bad_cast();
      }; break;
      // --- Cast from string
      case Type::String: {
        std::istringstream iss(std::string(reinterpret_cast<const char*>(data()), size()));
        T Result = 0;
        iss >> Result;
        return Result;
      }; break;
      // --- Cast from binary data
      case Type::Binary: {
        if (size() > sizeof(T)) throw std::bad_cast();
        T Result = 0;
        copyTo(reinterpret_cast<std::uint8_t*>(&Result), sizeof(T), std::min(sizeof(T), size()));
        return Result;
      }; break;
    }
    throw std::bad_cast();
  }

  //! Store numeric value
  template <typename T>
  void storeNumeric(const T Value, Type StoredType) {
    resize(sizeof(T));
    value_rw().assign(reinterpret_cast<const std::uint8_t*>(&Value), reinterpret_cast<const std::uint8_t*>(&Value) + sizeof(T));
    setType(StoredType);
  }

public:
  //! Set type
  void setType(Type StoredType) {
    _StoredType = StoredType;
  }
private:
  //! Raw data buffer
  std::vector<std::uint8_t> _Value;

  //! Stored type
  Type                      _StoredType;

  //! Mutex
  std::mutex                _Mutex;
};

