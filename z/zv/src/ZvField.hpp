//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// flat object introspection
// - print/scan (CSV, etc.)
// - ORM
// - data series

#ifndef ZvField_HPP
#define ZvField_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZuPrint.hpp>
#include <zlib/ZuBox.hpp>
#include <zlib/ZuString.hpp>
#include <zlib/ZuFixed.hpp>
#include <zlib/ZuDecimal.hpp>
#include <zlib/ZuField.hpp>

#include <zlib/ZmStream.hpp>
#include <zlib/ZmTime.hpp>

#include <zlib/ZtEnum.hpp>
#include <zlib/ZtDate.hpp>

namespace ZvFieldType {
  enum _ {
    String = 0,		// a contiguous UTF-8 string
    Composite,		// composite type
    Bool,		// an integral type, interpreted as bool
    Int,		// an integral type <= 64bits
    Float,		// floating point type, interpreted as double
    Fixed,		// ZuFixedVal
    Decimal,		// ZuDecimal
    Hex,		// an integral type printed in hex
    Enum,		// an integral type interpreted via ZtEnum
    Flags,		// an integral type interpreted via ZtEnumFlags
    Time,		// ZmTime
    N
  };
}

namespace ZvFieldFlags {
  enum {
    Primary	= 0x00001,	// primary key
    Secondary	= 0x00002,	// secondary key
    ReadOnly	= 0x00004,	// read-only
    Update	= 0x00008,	// include in updates
    Ctor_	= 0x00010,	// constructor parameter
    Series	= 0x00020,	// data series
      Index	= 0x00040,	// - index (e.g. time stamp)
      Delta	= 0x00080,	// - first derivative
      Delta2	= 0x00100,	// - second derivative
  };
  enum {
    CtorShift	= 12		// bit-shift for constructor parameter index
  };
  // parameter index -> flags
  inline constexpr unsigned Ctor(unsigned i) {
    return Ctor_ | (i<<CtorShift);
  }
  // flags -> parameter index
  inline constexpr unsigned CtorIndex(unsigned flags) {
    return flags>>CtorShift;
  }
}
#define ZvFieldMkFlags__(Flag) ZvFieldFlags::Flag |
#define ZvFieldMkFlags_(...) (ZuPP_Map(ZvFieldMkFlags__, __VA_ARGS__) 0)
#define ZvFieldMkFlags(Args) ZuPP_Defer(ZvFieldMkFlags_)(ZuPP_Strip(Args))

struct ZvTimeFmt_Null : public ZuPrintable {
  template <typename S> void print(S &) const { }
};
using ZvTimeFmt_FIX = ZtDateFmt::FIX<-9, ZvTimeFmt_Null>;
ZuDeclUnion(ZvTimeFmt,
    (ZtDateFmt::CSV, csv),
    (ZvTimeFmt_FIX, fix),
    (ZtDateFmt::ISO, iso));

struct ZvFieldFmt {
  ZvFieldFmt() { new (time.init_csv()) ZtDateFmt::CSV{}; }

  ZuVFmt	scalar;			// scalar format (print only)
  ZvTimeFmt	time;			// date/time format
  char		flagsDelim = '|';	// flags delimiter
};

template <typename Base, unsigned Type_, unsigned Flags_>
struct ZvField_ : public Base {
  enum { Type = Type_ };
  enum { Flags = Flags_ };
};

template <typename Base, unsigned Flags>
struct ZvFieldRdString_ : public ZvField_<Base, ZvFieldType::String, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &) {
    s << Base::get(o);
  }
};
template <typename Base, unsigned Flags>
struct ZvFieldString_ : public ZvFieldRdString_<Base, Flags> {
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, s);
  }
};
#define ZvFieldRdString(U, ID, Flags) \
  ZvFieldRdString_<ZuFieldRdData(U, ID), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>
#define ZvFieldRdStringFn(U, ID, Flags) \
  ZvFieldRdString_<ZuFieldRdFn(U, ID), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>
#define ZvFieldString(U, ID, Flags) \
  ZvFieldString_<ZuFieldData(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldStringFn(U, ID, Flags) \
  ZvFieldString_<ZuFieldFn(U, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags>
struct ZvFieldRdComposite_ :
    public ZvField_<Base, ZvFieldType::Composite, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &) {
    s << Base::get(o);
  }
};
template <typename Base, unsigned Flags>
struct ZvFieldComposite_ : public ZvFieldRdComposite_<Base, Flags> {
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, s);
  }
};
#define ZvFieldRdComposite(U, ID, Flags) \
  ZvFieldRdComposite_<ZuFieldRdData(U, ID), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>
#define ZvFieldRdCompositeFn(U, ID, Flags) \
  ZvFieldRdComposite_<ZuFieldRdFn(U, ID), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>
#define ZvFieldComposite(U, ID, Flags) \
  ZvFieldComposite_<ZuFieldData(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldCompositeFn(U, ID, Flags) \
  ZvFieldComposite_<ZuFieldFn(U, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags>
struct ZvFieldRdBool_ : public ZvField_<Base, ZvFieldType::Bool, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &) {
    s << (Base::get(o) ? '1' : '0');
  }
};
template <typename Base, unsigned Flags>
struct ZvFieldBool_ : public ZvFieldRdBool_<Base, Flags> {
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, s.length() == 1 && s[0] == '1');
  }
};
#define ZvFieldRdBool(U, ID, Flags) \
  ZvFieldRdBool_<ZuFieldRdData(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldRdBoolFn(U, ID, Flags) \
  ZvFieldRdBool_<ZuFieldRdFn(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldBool(U, ID, Flags) \
  ZvFieldBool_<ZuFieldData(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldBoolFn(U, ID, Flags) \
  ZvFieldBool_<ZuFieldFn(U, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags>
struct ZvFieldRdInt_ : public ZvField_<Base, ZvFieldType::Int, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    s << ZuBoxed(Base::get(o)).vfmt(fmt.scalar);
  }
};
template <typename Base, unsigned Flags>
struct ZvFieldInt_ : public ZvFieldRdInt_<Base, Flags> {
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    using T = typename Base::T;
    Base::set(o, typename ZuBoxT<T>::T{s});
  }
};
#define ZvFieldRdInt(U, ID, Flags) \
  ZvFieldRdInt_<ZuFieldRdData(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldRdIntFn(U, ID, Flags) \
  ZvFieldRdInt_<ZuFieldRdFn(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldInt(U, ID, Flags) \
  ZvFieldInt_<ZuFieldData(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldIntFn(U, ID, Flags) \
  ZvFieldInt_<ZuFieldFn(U, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags, unsigned Exponent_>
struct ZvFieldRdFloat_ : public ZvField_<Base, ZvFieldType::Float, Flags> {
  enum { Exponent = Exponent_ };
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    s << ZuBoxed(Base::get(o)).vfmt(fmt.scalar);
  }
};
template <typename Base, unsigned Flags, unsigned Exponent>
struct ZvFieldFloat_ : public ZvFieldRdFloat_<Base, Flags, Exponent> {
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    using T = typename Base::T;
    Base::set(o, typename ZuBoxT<T>::T{s});
  }
};
#define ZvFieldRdFloat(U, ID, Flags, Exponent) \
  ZvFieldRdFloat_<ZuFieldRdData(U, ID), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldRdFloatFn(U, ID, Flags, Exponent) \
  ZvFieldRdFloat_<ZuFieldRdFn(U, ID), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldFloat(U, ID, Flags, Exponent) \
  ZvFieldFloat_<ZuFieldData(U, ID), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldFloatFn(U, ID, Flags, Exponent) \
  ZvFieldFloat_<ZuFieldFn(U, ID), ZvFieldMkFlags(Flags), Exponent>

template <typename Base, unsigned Flags, unsigned Exponent_>
struct ZvFieldRdFixed_ : public ZvField_<Base, ZvFieldType::Fixed, Flags> {
  enum { Exponent = Exponent_ };
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    s << ZuFixed{Base::get(o), Exponent}.vfmt(fmt.scalar);
  }
};
template <typename Base, unsigned Flags, unsigned Exponent>
struct ZvFieldFixed_ : public ZvFieldRdFixed_<Base, Flags, Exponent> {
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, ZuFixed{s, Exponent}.value);
  }
};
#define ZvFieldRdFixed(U, ID, Flags, Exponent) \
  ZvFieldRdFixed_<ZuFieldRdData(U, ID), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldRdFixedFn(U, ID, Flags, Exponent) \
  ZvFieldRdFixed_<ZuFieldRdFn(U, ID), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldFixed(U, ID, Flags, Exponent) \
  ZvFieldFixed_<ZuFieldData(U, ID), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldFixedFn(U, ID, Flags, Exponent) \
  ZvFieldFixed_<ZuFieldFn(U, ID), ZvFieldMkFlags(Flags), Exponent>

template <typename Base, unsigned Flags, unsigned Exponent_>
struct ZvFieldRdDecimal_ : public ZvField_<Base, ZvFieldType::Decimal, Flags> {
  enum { Exponent = Exponent_ };
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    s << Base::get(o).vfmt(fmt.scalar);
  }
};
template <typename Base, unsigned Flags, unsigned Exponent>
struct ZvFieldDecimal_ : public ZvFieldRdDecimal_<Base, Flags, Exponent> {
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, s);
  }
};
#define ZvFieldRdDecimal(U, ID, Flags, Exponent) \
  ZvFieldRdDecimal_<ZuFieldRdData(U, ID), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldRdDecimalFn(U, ID, Flags, Exponent) \
  ZvFieldRdDecimal_<ZuFieldRdFn(U, ID), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldDecimal(U, ID, Flags, Exponent) \
  ZvFieldDecimal_<ZuFieldData(U, ID), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldDecimalFn(U, ID, Flags, Exponent) \
  ZvFieldDecimal_<ZuFieldFn(U, ID), ZvFieldMkFlags(Flags), Exponent>

template <typename Base, unsigned Flags>
struct ZvFieldRdHex_ : public ZvField_<Base, ZvFieldType::Hex, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    s << ZuBoxed(Base::get(o)).vfmt(fmt.scalar).hex();
  }
};
template <typename Base, unsigned Flags>
struct ZvFieldHex_ : public ZvFieldRdHex_<Base, Flags> {
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    using T = typename Base::T;
    Base::set(o, typename ZuBoxT<T>::T{ZuFmt::Hex<>{}, s});
  }
};
#define ZvFieldRdHex(U, ID, Flags) \
  ZvFieldRdHex_<ZuFieldRdData(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldRdHexFn(U, ID, Flags) \
  ZvFieldRdHex_<ZuFieldRdFn(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldHex(U, ID, Flags) \
  ZvFieldHex_<ZuFieldData(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldHexFn(U, ID, Flags) \
  ZvFieldHex_<ZuFieldFn(U, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags, typename Map_>
struct ZvFieldRdEnum_ : public ZvField_<Base, ZvFieldType::Enum, Flags> {
  using Map = Map_;
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &) {
    s << Map::instance()->v2s(Base::get(o));
  }
};
template <typename Base, unsigned Flags, typename Map>
struct ZvFieldEnum_ : public ZvFieldRdEnum_<Base, Flags, Map> {
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, Map::instance()->s2v(s));
  }
};
#define ZvFieldRdEnum(U, ID, Flags, Map) \
  ZvFieldRdEnum_<ZuFieldRdData(U, ID), ZvFieldMkFlags(Flags), Map>
#define ZvFieldRdEnumFn(U, ID, Flags, Map) \
  ZvFieldRdEnum_<ZuFieldRdFn(U, ID), ZvFieldMkFlags(Flags), Map>
#define ZvFieldEnum(U, ID, Flags, Map) \
  ZvFieldEnum_<ZuFieldData(U, ID), ZvFieldMkFlags(Flags), Map>
#define ZvFieldEnumFn(U, ID, Flags, Map) \
  ZvFieldEnum_<ZuFieldFn(U, ID), ZvFieldMkFlags(Flags), Map>

template <typename Base, unsigned Flags, typename Map_>
struct ZvFieldRdFlags_ : public ZvField_<Base, ZvFieldType::Flags, Flags> {
  using Map = Map_;
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    Map::instance()->print(s, Base::get(o), fmt.flagsDelim);
  }
};
template <typename Base, unsigned Flags, typename Map>
struct ZvFieldFlags_ : public ZvFieldRdFlags_<Base, Flags, Map> {
  static void scan(void *o, ZuString s, const ZvFieldFmt &fmt) {
    using T = typename Base::T;
    Base::set(o, Map::instance()->template scan<T>(s, fmt.flagsDelim));
  }
};
#define ZvFieldRdFlags(U, ID, Flags, Map) \
  ZvFieldRdFlags_<ZuFieldRdData(U, ID), ZvFieldMkFlags(Flags), Map>
#define ZvFieldRdFlagsFn(U, ID, Flags, Map) \
  ZvFieldRdFlags_<ZuFieldRdFn(U, ID), ZvFieldMkFlags(Flags), Map>
#define ZvFieldFlags(U, ID, Flags, Map) \
  ZvFieldFlags_<ZuFieldData(U, ID), ZvFieldMkFlags(Flags), Map>
#define ZvFieldFlagsFn(U, ID, Flags, Map) \
  ZvFieldFlags_<ZuFieldFn(U, ID), ZvFieldMkFlags(Flags), Map>

template <typename Base, unsigned Flags>
struct ZvFieldRdTime_ : public ZvField_<Base, ZvFieldType::Time, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    ZtDate v{Base::get(o)};
    switch (fmt.time.type()) {
      default:
      case ZvTimeFmt::Index<ZtDateFmt::CSV>::I:
	s << v.csv(fmt.time.csv());
	break;
      case ZvTimeFmt::Index<ZvTimeFmt_FIX>::I:
	s << v.fix(fmt.time.fix());
	break;
      case ZvTimeFmt::Index<ZtDateFmt::ISO>::I:
	s << v.iso(fmt.time.iso());
	break;
    }
  }
};
template <typename Base, unsigned Flags>
struct ZvFieldTime_ : public ZvFieldRdTime_<Base, Flags> {
  static void scan(void *o, ZuString s, const ZvFieldFmt &fmt) {
    switch (fmt.time.type()) {
      default:
      case ZvTimeFmt::Index<ZtDateFmt::CSV>::I:
	Base::set(o, ZtDate{ZtDate::CSV, s});
	break;
      case ZvTimeFmt::Index<ZvTimeFmt_FIX>::I:
	Base::set(o, ZtDate{ZtDate::FIX, s});
	break;
      case ZvTimeFmt::Index<ZtDateFmt::ISO>::I:
	Base::set(o, ZtDate{s});
	break;
    }
  }
};
#define ZvFieldRdTime(U, ID, Flags) \
  ZvFieldRdTime_<ZuFieldRdData(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldRdTimeFn(U, ID, Flags) \
  ZvFieldRdTime_<ZuFieldRdFn(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldTime(U, ID, Flags) \
  ZvFieldTime_<ZuFieldData(U, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldTimeFn(U, ID, Flags) \
  ZvFieldTime_<ZuFieldFn(U, ID), ZvFieldMkFlags(Flags)>

#define ZvField__(U, Type, ...) ZvField##Type(U, __VA_ARGS__)
#define ZvField_(U, Args) ZuPP_Defer(ZvField__)(U, ZuPP_Strip(Args))

template <typename T> ZuTypeList<> ZvFieldList_(T *); // default

#define ZvFields(U, ...)  \
  namespace { \
    ZuPP_Eval(ZuPP_MapArg(ZuField_ID, U, __VA_ARGS__)) \
  } \
  ZuTypeList<ZuPP_Eval(ZuPP_MapArgComma(ZvField_, U, __VA_ARGS__))> \
  ZvFieldList_(U *);

template <typename T>
using ZvFieldList = decltype(ZvFieldList_(ZuDeclVal<T *>()));

template <typename Impl> struct ZvFieldPrint : public ZuPrintable {
  const Impl *impl() const { return static_cast<const Impl *>(this); }
  Impl *impl() { return static_cast<Impl *>(this); }

  template <typename T>
  struct Print_Filter { enum { OK = !(T::Flags & ZvFieldFlags::ReadOnly) }; };
  template <typename S> void print(S &s) const {
    using FieldList = typename ZuTypeGrep<Print_Filter, ZvFieldList<Impl>>::T;
    thread_local ZvFieldFmt fmt;
    ZuTypeAll<FieldList>::invoke([o = impl(), &s]<typename Field>() {
      if constexpr (ZuTypeIndex<Field, FieldList>::I) s << ' ';
      s << Field::id() << '=';
      Field::print(o, s, fmt);
    });
  }
};

template <typename V, typename FieldList>
struct ZvFieldTuple {
  template <typename ...Fields>
  struct Mk_ {
    using T = ZuTuple<typename Fields::T...>;
    static auto mk(const V &v) { return T{ Fields::get(&v)... }; }
  };
  using Mk = typename ZuTypeApply<Mk_, FieldList>::T;
};

template <typename V> struct ZvFieldKey_ {
  template <typename T>
  struct Filter { enum { OK = T::Flags & ZvFieldFlags::Primary }; };
  using FieldList = typename ZuTypeGrep<Filter, ZvFieldList<V>>::T;
  using Mk = typename ZvFieldTuple<V, FieldList>::Mk;
};
template <typename V>
auto ZvFieldKey(const V &v) {
  return ZvFieldKey_<V>::Mk::mk(v);
}

// run-time introspection

struct ZvVField_Enum_ {
  const char	*(*id)();
  int		(*s2v)(ZuString);
  ZuString	(*v2s)(int);
};
template <typename Map>
struct ZvVField_Enum : public ZvVField_Enum_ {
  ZvVField_Enum(Map) : ZvVField_Enum_{
    .id = []() -> const char * { return Map::id(); },
    .s2v = [](ZuString s) -> int {
      return Map::instance()->s2v(s);
    },
    .v2s = [](int i) -> ZuString {
      return Map::instance()->v2s(i);
    }
  } { }

  static ZvVField_Enum *instance() {
    return ZmSingleton<ZvVField_Enum>::instance();
  }
};

struct ZvVField_Flags_ {
  const char	*(*id)();
  void		(*print)(uint64_t, ZmStream &, const ZvFieldFmt &);
  void		(*scan)(uint64_t &, ZuString, const ZvFieldFmt &);
};
template <typename Map>
struct ZvVField_Flags : public ZvVField_Flags_ {
  ZvVField_Flags(Map) : ZvVField_Flags_{
    .id = []() -> const char * { return Map::id(); },
    .print = [](uint64_t v, ZmStream &s, const ZvFieldFmt &fmt) -> void {
      Map::instance()->print(s, v, fmt.flagsDelim);
    },
    .scan = [](uint64_t &v, ZuString s, const ZvFieldFmt &fmt) -> void {
      v = Map::instance()->template scan<uint64_t>(s, fmt.flagsDelim);
    }
  } { }

  static ZvVField_Flags *instance() {
    return ZmSingleton<ZvVField_Flags>::instance();
  }
};

template <typename Field, bool = Field::Flags & ZvFieldFlags::ReadOnly>
struct ZvVField_Set {
  template <typename T>
  static void set(void *o, T &&v) { Field::set(o, ZuFwd<T>(v)); }
};
template <typename Field>
struct ZvVField_Set<Field, true> {
  template <typename T>
  static void set(void *, T &&) { }
};
struct ZvVField {
  const char	*id;
  uint32_t	type;		// ZvFieldType
  uint32_t	flags;		// ZvFieldFlags

  int		(*cmp)(const void *, const void *);

  union {
    unsigned		exponent;	// Float|Fixed|Decimal
    ZvVField_Enum_	*(*enum_)();	// Enum
    ZvVField_Flags_	*(*flags)();	// Flags
  } info;

  void		(*print)(const void *, ZmStream &, const ZvFieldFmt &);
  void		(*scan)(void *, ZuString, const ZvFieldFmt &);
  union {
    ZuString	(*string)(const void *);	// String
    ZmStreamFn	(*composite)(const void *);	// Composite
    int64_t	(*int_)(const void *);		// Bool|Int|Fixed|Hex|Enum|Flags
    double	(*float_)(const void *);	// Float
    ZuDecimal	(*decimal)(const void *);	// Decimal
    ZmTime	(*time)(const void *);		// Time
  } getFn;
  union {
    void	(*string)(void *, ZuString);	// String|Composite
    void	(*int_)(void *, int64_t);	// Bool|Int|Fixed|Hex|Enum|Flags
    void	(*float_)(void *, double);	// Float
    void	(*decimal)(void *, ZuDecimal);	// Decimal
    void	(*time)(void *, ZmTime);	// Time
  } setFn;

  template <typename Field>
  ZvVField(Field,
        typename ZuIfT<Field::Type == ZvFieldType::String>::T *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = 0},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.string = [](const void *o) -> ZuString {
	return Field::get(o);
      }},
      setFn{.string = [](void *o, ZuString s) {
	ZvVField_Set<Field>::set(o, s);
      }} { }

  template <typename Field>
  ZvVField(Field,
        typename ZuIfT<Field::Type == ZvFieldType::Composite>::T *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = 0},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.composite = [](const void *o) -> ZmStreamFn {
	using T = typename Field::T;
	const T &v = Field::get(o);
	return {&v, [](const T *o, ZmStream &s) { s << *o; }};
      }},
      setFn{.stream = [](void *o, ZuString s) {
	ZvVField_Set<Field>::set(o, s);
      }} { }

  template <typename Field>
  ZvVField(Field,
        typename ZuIfT<
	  Field::Type == ZvFieldType::Bool ||
	  Field::Type == ZvFieldType::Int ||
	  Field::Type == ZvFieldType::Hex>::T *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = 0},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = [](void *o, int64_t v) {
	ZvVField_Set<Field>::set(o, v);
      }} { }

  template <typename Field>
  ZvVField(Field,
        typename ZuIfT<Field::Type == ZvFieldType::Enum>::T *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.enum_ = []() {
	return ZvVField_Enum<typename Field::Map>::instance();
      }},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = [](void *o, int64_t v) {
	ZvVField_Set<Field>::set(o, v);
      }} { }

  template <typename Field>
  ZvVField(Field,
        typename ZuIfT<Field::Type == ZvFieldType::Flags>::T *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.flags = []() {
	return ZvVField_Flags<typename Field::Map>::instance();
      }},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = [](void *o, int64_t v) {
	ZvVField_Set<Field>::set(o, v);
      }} { }

  template <typename Field>
  ZvVField(Field,
        typename ZuIfT<Field::Type == ZvFieldType::Float>::T *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = Field::Exponent},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.float_ = [](const void *o) -> double { return Field::get(o); }},
      setFn{.float_ = [](void *o, double v) {
	ZvVField_Set<Field>::set(o, v);
      }} { }

  template <typename Field>
  ZvVField(Field,
        typename ZuIfT<Field::Type == ZvFieldType::Fixed>::T *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = Field::Exponent},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = [](void *o, int64_t v) {
	ZvVField_Set<Field>::set(o, v);
      }} { }

  template <typename Field>
  ZvVField(Field,
        typename ZuIfT<Field::Type == ZvFieldType::Decimal>::T *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = Field::Exponent},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.decimal = [](const void *o) -> ZuDecimal {
	return Field::get(o);
      }},
      setFn{.decimal = [](void *o, ZuDecimal v) {
	ZvVField_Set<Field>::set(o, v);
      }} { }

  template <typename Field>
  ZvVField(Field,
        typename ZuIfT<Field::Type == ZvFieldType::Time>::T *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = 0},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.time = [](const void *o) -> ZmTime { return Field::get(o); }},
      setFn{.time = [](void *o, ZmTime v) {
	ZvVField_Set<Field>::set(o, v);
      }} { }

  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::String>::T get_(const void *o, L l) {
    l(getFn.string(o));
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Composite>::T get_(const void *o, L l) {
    l(getFn.composite(o));
  }
  template <unsigned I, typename L>
  typename ZuIfT<
    I == ZvFieldType::Bool || I == ZvFieldType::Int ||
    I == ZvFieldType::Fixed || I == ZvFieldType::Hex ||
    I == ZvFieldType::Enum || I == ZvFieldType::Flags>::T get_(
	const void *o, L l) {
    l(getFn.int_(o));
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Float>::T get_(const void *o, L l) {
    l(getFn.float_(o));
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Decimal>::T get_(const void *o, L l) {
    l(getFn.decimal(o));
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Time>::T get_(const void *o, L l) {
    l(getFn.time(o));
  }
  template <typename L> void get(const void *o, L l) {
    ZuSwitch::dispatch<ZvFieldType::N>(type,
	[this, o, l = ZuMv(l)](auto i) mutable { get_<i>(o, ZuMv(l)); });
  }

  template <unsigned I, typename L>
  typename ZuIfT<
    I == ZvFieldType::String || I == ZvFieldType::Composite>::T set_(
	void *o, L l) {
    setFn.string(o, l.template operator ()<ZuString>());
  }
  template <unsigned I, typename L>
  typename ZuIfT<
    I == ZvFieldType::Bool || I == ZvFieldType::Int ||
    I == ZvFieldType::Fixed || I == ZvFieldType::Hex ||
    I == ZvFieldType::Enum || I == ZvFieldType::Flags>::T set_(
	void *o, L l) {
    setFn.int_(o, l.template operator ()<int64_t>());
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Float>::T set_(void *o, L l) {
    setFn.float_(o, l.template operator ()<double>());
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Decimal>::T set_(void *o, L l) {
    setFn.decimal(o, l.template operator ()<ZuDecimal>());
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Time>::T set_(void *o, L l) {
    setFn.time(o, l.template operator ()<ZmTime>());
  }
  template <typename L>
  void set(void *o, L l) {
    ZuSwitch::dispatch<ZvFieldType::N>(type,
	[this, o, l = ZuMv(l)](auto i) mutable { set_<i>(o, ZuMv(l)); });
  }
};

using ZvVFieldArray = ZuArray<ZvVField>;

template <typename ...Fields>
struct ZvVFields__ {
  const ZvVFieldArray fields() {
    static ZvVField fields_[] =
      // std::initializer_list<ZvVField>
    {
      ZvVField{Fields{}}...
    };
    return {&fields_[0], sizeof(fields_) / sizeof(fields_[0])};
  }
};
template <typename FieldList>
inline const ZvVFieldArray ZvVFields_() {
  using Mk = typename ZuTypeApply<ZvVFields__, FieldList>::T;
  return Mk::fields();
}
template <typename T>
inline const ZvVFieldArray ZvVFields() {
  return ZvVFields_<ZvFieldList<T>>();
}

#endif /* ZvField_HPP */
