#pragma once

#include <Arduino.h>
#include <initializer_list>

enum Mode
{
  SMOOTH,
  STEPPED,
  RANDOM
};

enum PtrType
{
  NONE,
  FAR,
  NEAR
};

template <typename T>
struct PointerDescriptor
{
  PtrType type;
  const T *ptr;
};

template <typename T>
struct ArrayDescriptor
{
  byte len;
  PointerDescriptor<T> des;

  ArrayDescriptor<T> operator=(ArrayDescriptor<T> inst)
  {
    len = inst.len;
    des.type = inst.des.type;
    des.ptr = inst.des.ptr;
    return *this;
  }

  const T operator[](size_t index) const
  {
    if (des.type == FAR)
    {
      T *item = (T *)malloc(sizeof(T));
      memcpy_P(item, &des.ptr[index], sizeof(T));
      T val = *item;
      free(item);
      return val;
    }
    else
    {
      return des.ptr[index];
    }
  }
};

struct StringDescriptor
{
  PointerDescriptor<char> des;

private:
  class PrintableSD : public Printable
  {
    friend class StringDescriptor;
    const StringDescriptor *_sd;

    PrintableSD(const StringDescriptor *sd) : _sd(sd){};

  public:
    virtual size_t printTo(Print &p) const
    {
      switch (_sd->des.type)
      {
      case FAR:
        return p.print((__FlashStringHelper *)_sd->des.ptr);
      case NEAR:
        return p.print(_sd->des.ptr);
      default:
        return 0;
      }
    }
  };

public:
  PrintableSD getPrintable() const
  {
    return PrintableSD(this);
  };
};

#define DESCRIBE_ARRAY(type, x) \
  {                             \
    sizeof(x) / sizeof(*x),     \
        type,                   \
        x                       \
  }
struct Gamut
{
  struct Key
  {
    uint16_t ledIndex;
    uint32_t rgb;
  };

  StringDescriptor name;
  ArrayDescriptor<Key> keys;
};