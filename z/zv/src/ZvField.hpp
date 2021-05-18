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

#define ZvFieldXRdString(U, ID, Member, Flags) \
  ZvFieldRdString_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>
#define ZvFieldXRdStringFn(U, ID, Get, Set, Flags) \
  ZvFieldRdString_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>

#define ZvFieldXString(U, ID, Member, Flags) \
  ZvFieldString_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXStringFn(U, ID, Get, Set, Flags) \
  ZvFieldString_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

#define ZvFieldRdString(U, Member, Flags) \
  ZvFieldXRdString(U, Member, Member, Flags)
#define ZvFieldRdStringFn(U, Fn, Flags) \
  ZvFieldXRdStringFn(U, Fn, Fn, Fn, Flags)

#define ZvFieldString(U, Member, Flags) \
  ZvFieldXString(U, Member, Member, Flags)
#define ZvFieldStringFn(U, Fn, Flags) \
  ZvFieldXStringFn(U, Fn, Fn, Fn, Flags)

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

#define ZvFieldXRdComposite(U, ID, Member, Flags) \
  ZvFieldRdComposite_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>
#define ZvFieldXRdCompositeFn(U, ID, Get, Set, Flags) \
  ZvFieldRdComposite_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>

#define ZvFieldXComposite(U, ID, Member, Flags) \
  ZvFieldComposite_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXCompositeFn(U, ID, Get, Set, Flags) \
  ZvFieldComposite_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

#define ZvFieldRdComposite(U, Member, Flags) \
  ZvFieldXRdComposite(U, Member, Member, Flags)
#define ZvFieldRdCompositeFn(U, Fn, Flags) \
  ZvFieldXRdCompositeFn(U, Fn, Fn, Fn, Flags)

#define ZvFieldComposite(U, Member, Flags) \
  ZvFieldXComposite(U, Member, Member, Flags)
#define ZvFieldCompositeFn(U, Fn, Flags) \
  ZvFieldXCompositeFn(U, Fn, Fn, Fn, Flags)

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

#define ZvFieldXRdBool(U, ID, Member, Flags) \
  ZvFieldRdBool_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>
#define ZvFieldXRdBoolFn(U, ID, Get, Set, Flags) \
  ZvFieldRdBool_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>

#define ZvFieldXBool(U, ID, Member, Flags) \
  ZvFieldBool_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXBoolFn(U, ID, Get, Set, Flags) \
  ZvFieldBool_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

#define ZvFieldRdBool(U, Member, Flags) \
  ZvFieldXRdBool(U, Member, Member, Flags)
#define ZvFieldRdBoolFn(U, Fn, Flags) \
  ZvFieldXRdBoolFn(U, Fn, Fn, Fn, Flags)

#define ZvFieldBool(U, Member, Flags) \
  ZvFieldXBool(U, Member, Member, Flags)
#define ZvFieldBoolFn(U, Fn, Flags) \
  ZvFieldXBoolFn(U, Fn, Fn, Fn, Flags)

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
    Base::set(o, ZuBoxT<T>{s});
  }
};

#define ZvFieldXRdInt(U, ID, Member, Flags) \
  ZvFieldRdInt_<ZuFieldXRdData(U, ID, Member),\
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>
#define ZvFieldXRdIntFn(U, ID, Get, Set, Flags) \
  ZvFieldRdInt_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>

#define ZvFieldXInt(U, ID, Member, Flags) \
  ZvFieldInt_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXIntFn(U, ID, Get, Set, Flags) \
  ZvFieldInt_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

#define ZvFieldRdInt(U, Member, Flags) \
  ZvFieldXRdInt(U, Member, Member, Flags)
#define ZvFieldRdIntFn(U, Fn, Flags) \
  ZvFieldXRdIntFn(U, Fn, Fn, Fn, Flags)

#define ZvFieldInt(U, Member, Flags) \
  ZvFieldXInt(U, Member, Member, Flags)
#define ZvFieldIntFn(U, Fn, Flags) \
  ZvFieldXIntFn(U, Fn, Fn, Fn, Flags)

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
    Base::set(o, ZuBoxT<T>{s});
  }
};

#define ZvFieldXRdFloat(U, ID, Member, Flags, Exponent) \
  ZvFieldRdFloat_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly), Exponent>
#define ZvFieldXRdFloatFn(U, ID, Get, Set, Flags, Exponent) \
  ZvFieldRdFloat_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly), Exponent>

#define ZvFieldXFloat(U, ID, Member, Flags, Exponent) \
  ZvFieldFloat_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldXFloatFn(U, ID, Get, Set, Flags, Exponent) \
  ZvFieldFloat_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags), Exponent>

#define ZvFieldRdFloat(U, Member, Flags, Exponent) \
  ZvFieldXRdFloat(U, Member, Member, Flags, Exponent)
#define ZvFieldRdFloatFn(U, Fn, Flags, Exponent) \
  ZvFieldXRdFloatFn(U, Fn, Fn, Fn, Flags, Exponent)

#define ZvFieldFloat(U, Member, Flags, Exponent) \
  ZvFieldXFloat(U, Member, Member, Flags, Exponent)
#define ZvFieldFloatFn(U, Fn, Flags, Exponent) \
  ZvFieldXFloatFn(U, Fn, Fn, Fn, Flags, Exponent)

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

#define ZvFieldXRdFixed(U, ID, Member, Flags, Exponent) \
  ZvFieldRdFixed_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly), Exponent>
#define ZvFieldXRdFixedFn(U, ID, Get, Set, Flags, Exponent) \
  ZvFieldRdFixed_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly), Exponent>

#define ZvFieldXFixed(U, ID, Member, Flags, Exponent) \
  ZvFieldFixed_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldXFixedFn(U, ID, Get, Set, Flags, Exponent) \
  ZvFieldFixed_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags), Exponent>

#define ZvFieldRdFixed(U, Member, Flags, Exponent) \
  ZvFieldXRdFixed(U, Member, Member, Flags, Exponent)
#define ZvFieldRdFixedFn(U, Fn, Flags, Exponent) \
  ZvFieldXRdFixedFn(U, Fn, Fn, Fn, Flags, Exponent)

#define ZvFieldFixed(U, Member, Flags, Exponent) \
  ZvFieldXFixed(U, Member, Member, Flags, Exponent)
#define ZvFieldFixedFn(U, Fn, Flags, Exponent) \
  ZvFieldXFixedFn(U, Fn, Fn, Fn, Flags, Exponent)

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

#define ZvFieldXRdDecimal(U, ID, Member, Flags, Exponent) \
  ZvFieldRdDecimal_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly), Exponent>
#define ZvFieldXRdDecimalFn(U, ID, Get, Set, Flags, Exponent) \
  ZvFieldRdDecimal_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly), Exponent>

#define ZvFieldXDecimal(U, ID, Member, Flags, Exponent) \
  ZvFieldDecimal_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldXDecimalFn(U, ID, Get, Set, Flags, Exponent) \
  ZvFieldDecimal_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags), Exponent>

#define ZvFieldRdDecimal(U, Member, Flags, Exponent) \
  ZvFieldXRdDecimal(U, Member, Member, Flags, Exponent)
#define ZvFieldRdDecimalFn(U, Fn, Flags, Exponent) \
  ZvFieldXRdDecimalFn(U, Fn, Fn, Fn, Flags, Exponent)

#define ZvFieldDecimal(U, Member, Flags, Exponent) \
  ZvFieldXDecimal(U, Member, Member, Flags, Exponent)
#define ZvFieldDecimalFn(U, Fn, Flags, Exponent) \
  ZvFieldXDecimalFn(U, Fn, Fn, Fn, Flags, Exponent)

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
    Base::set(o, ZuBoxT<T>{ZuFmt::Hex<>{}, s});
  }
};

#define ZvFieldXRdHex(U, ID, Member, Flags) \
  ZvFieldRdHex_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>
#define ZvFieldXRdHexFn(U, ID, Get, Set, Flags) \
  ZvFieldRdHex_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>

#define ZvFieldXHex(U, ID, Member, Flags) \
  ZvFieldHex_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXHexFn(U, ID, Get, Set, Flags) \
  ZvFieldHex_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

#define ZvFieldRdHex(U, Member, Flags) \
  ZvFieldXRdHex(U, Member, Member, Flags)
#define ZvFieldRdHexFn(U, Fn, Flags) \
  ZvFieldXRdHexFn(U, Fn, Fn, Fn, Flags)

#define ZvFieldHex(U, Member, Flags) \
  ZvFieldXHex(U, Member, Member, Flags)
#define ZvFieldHexFn(U, Fn, Flags) \
  ZvFieldXHexFn(U, Fn, Fn, Fn, Flags)

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

#define ZvFieldXRdEnum(U, ID, Member, Flags, Map) \
  ZvFieldRdEnum_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly), Map>
#define ZvFieldXRdEnumFn(U, ID, Get, Set, Flags, Map) \
  ZvFieldRdEnum_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly), Map>

#define ZvFieldXEnum(U, ID, Member, Flags, Map) \
  ZvFieldEnum_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags), Map>
#define ZvFieldXEnumFn(U, ID, Get, Set, Flags, Map) \
  ZvFieldEnum_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags), Map>

#define ZvFieldRdEnum(U, Member, Flags, Map) \
  ZvFieldXRdEnum(U, Member, Member, Flags, Map)
#define ZvFieldRdEnumFn(U, Fn, Flags, Map) \
  ZvFieldXRdEnumFn(U, Fn, Fn, Fn, Flags, Map)

#define ZvFieldEnum(U, Member, Flags, Map) \
  ZvFieldXEnum(U, Member, Member, Flags, Map)
#define ZvFieldEnumFn(U, Fn, Flags, Map) \
  ZvFieldXEnumFn(U, Fn, Fn, Fn, Flags, Map)

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

#define ZvFieldXRdFlags(U, ID, Member, Flags, Map) \
  ZvFieldRdFlags_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly), Map>
#define ZvFieldXRdFlagsFn(U, ID, Get, Set, Flags, Map) \
  ZvFieldRdFlags_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly), Map>

#define ZvFieldXFlags(U, ID, Member, Flags, Map) \
  ZvFieldFlags_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags), Map>
#define ZvFieldXFlagsFn(U, ID, Get, Set, Flags, Map) \
  ZvFieldFlags_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags), Map>

#define ZvFieldRdFlags(U, Member, Flags, Map) \
  ZvFieldXRdFlags(U, Member, Member, Flags, Map)
#define ZvFieldRdFlagsFn(U, Fn, Flags, Map) \
  ZvFieldXRdFlagsFn(U, Fn, Fn, Fn, Flags, Map)

#define ZvFieldFlags(U, Member, Flags, Map) \
  ZvFieldXFlags(U, Member, Member, Flags, Map)
#define ZvFieldFlagsFn(U, Fn, Flags, Map) \
  ZvFieldXFlagsFn(U, Fn, Fn, Fn, Flags, Map)

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

#define ZvFieldXRdTime(U, ID, Member, Flags) \
  ZvFieldRdTime_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>
#define ZvFieldXRdTimeFn(U, ID, Get, Set, Flags) \
  ZvFieldRdTime_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags) | ZvFieldFlags::ReadOnly)>

#define ZvFieldXTime(U, ID, Member, Flags) \
  ZvFieldTime_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXTimeFn(U, ID, Get, Set, Flags) \
  ZvFieldTime_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

#define ZvFieldRdTime(U, Member, Flags) \
  ZvFieldXRdTime(U, Member, Member, Flags)
#define ZvFieldRdTimeFn(U, Fn, Flags) \
  ZvFieldXRdTimeFn(U, Fn, Fn, Fn, Flags)

#define ZvFieldTime(U, Member, Flags) \
  ZvFieldXTime(U, Member, Member, Flags)
#define ZvFieldTimeFn(U, Fn, Flags) \
  ZvFieldXTimeFn(U, Fn, Fn, Fn, Flags)

#define ZvField__(U, Type, ...) ZvField##Type(U, __VA_ARGS__)
#define ZvField_(U, Args) ZuPP_Defer(ZvField__)(U, ZuPP_Strip(Args))

ZuTypeList<> ZvFieldList_(...); // default

void *ZvFielded_(...); // default

#define ZvFields(U, ...)  \
  namespace { \
    ZuPP_Eval(ZuPP_MapArg(ZuField_ID, U, __VA_ARGS__)) \
  } \
  U *ZvFielded_(U *); \
  ZuTypeList<ZuPP_Eval(ZuPP_MapArgComma(ZvField_, U, __VA_ARGS__))> \
  ZvFieldList_(U *)

template <typename T>
using ZvFieldList = decltype(ZvFieldList_(ZuDeclVal<ZuDecay<T> *>()));

template <typename T>
using ZvFielded = ZuDeref<decltype(*ZvFielded_(ZuDeclVal<ZuDecay<T> *>()))>;

template <typename Impl> struct ZvFieldPrint : public ZuPrintable {
  const Impl *impl() const { return static_cast<const Impl *>(this); }
  Impl *impl() { return static_cast<Impl *>(this); }

  template <typename T>
  struct Print_Filter { enum { OK = !(T::Flags & ZvFieldFlags::ReadOnly) }; };
  template <typename S> void print(S &s) const {
    using FieldList = ZuTypeGrep<Print_Filter, ZvFieldList<Impl>>;
    thread_local ZvFieldFmt fmt;
    ZuTypeAll<FieldList>::invoke([o = impl(), &s]<typename Field>() {
      if constexpr (ZuTypeIndex<Field, FieldList>::I) s << ' ';
      s << Field::id() << '=';
      Field::print(o, s, fmt);
    });
  }
};

template <typename V, typename FieldList>
struct ZvFieldKey__ {
  template <typename ...Fields>
  struct Mk_ {
    using Tuple = ZuTuple<typename Fields::T...>;
    static auto tuple(const V &v) { return Tuple{Fields::get(&v)...}; }
  };
  using Mk = ZuTypeApply<Mk_, FieldList>;
};
template <typename V> struct ZvFieldKey_ {
  template <typename T>
  struct Filter { enum { OK = T::Flags & ZvFieldFlags::Primary }; };
  using FieldList = ZuTypeGrep<Filter, ZvFieldList<V>>;
  using Mk = typename ZvFieldKey__<V, FieldList>::Mk;
};
template <typename V>
auto ZvFieldKey(const V &v) {
  return ZvFieldKey_<V>::Mk::tuple(v);
}

// run-time introspection

struct ZvVField_Enum_ {
  const char	*(*id)();
  int		(*s2v)(ZuString);
  ZuString	(*v2s)(int);
};
template <typename Map>
struct ZvVField_Enum : public ZvVField_Enum_ {
  ZvVField_Enum() : ZvVField_Enum_{
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
  ZvVField_Flags() : ZvVField_Flags_{
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

template <typename T>
inline void ZvVField_RO_set(void *, T) { }
inline void ZvVField_RO_scan(void *, ZuString, const ZvFieldFmt &) { }
template <typename Field, int = Field::Flags & ZvFieldFlags::ReadOnly>
struct ZvVField_RW {
  template <typename T>
  static auto setFn() { return ZvVField_RO_set<T>; }
  static auto scanFn() { return ZvVField_RO_scan; }
};
template <typename Field>
struct ZvVField_RW<Field, 0> {
  template <typename T>
  static auto setFn() { return [](void *o, T v) { Field::set(o, v); }; }
  static auto scanFn() { return Field::scan; }
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
        ZuIfT<Field::Type == ZvFieldType::String> *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = 0},
      print{Field::template print<ZmStream>},
      scan{ZvVField_RW<Field>::scanFn()},
      getFn{.string = [](const void *o) -> ZuString {
	return Field::get(o);
      }},
      setFn{.string = ZvVField_RW<Field>::template setFn<ZuString>()} { }

  template <typename Field>
  ZvVField(Field,
        ZuIfT<Field::Type == ZvFieldType::Composite> *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = 0},
      print{Field::template print<ZmStream>},
      scan{ZvVField_RW<Field>::scanFn()},
      getFn{.composite = [](const void *o) -> ZmStreamFn {
	using T = typename Field::T;
	const T &v = Field::get(o);
	return {&v, [](const T *o, ZmStream &s) { s << *o; }};
      }},
      setFn{.string = ZvVField_RW<Field>::template setFn<ZuString>()} { }

  template <typename Field>
  ZvVField(Field,
        ZuIfT<
	  Field::Type == ZvFieldType::Bool ||
	  Field::Type == ZvFieldType::Int ||
	  Field::Type == ZvFieldType::Hex> *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = 0},
      print{Field::template print<ZmStream>},
      scan{ZvVField_RW<Field>::scanFn()},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = ZvVField_RW<Field>::template setFn<int64_t>()} { }

  template <typename Field>
  ZvVField(Field,
        ZuIfT<Field::Type == ZvFieldType::Enum> *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.enum_ = []() -> ZvVField_Enum_ * {
	return ZvVField_Enum<typename Field::Map>::instance();
      }},
      print{Field::template print<ZmStream>},
      scan{ZvVField_RW<Field>::scanFn()},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = ZvVField_RW<Field>::template setFn<int64_t>()} { }

  template <typename Field>
  ZvVField(Field,
        ZuIfT<Field::Type == ZvFieldType::Flags> *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.flags = []() -> ZvVField_Flags_ * {
	return ZvVField_Flags<typename Field::Map>::instance();
      }},
      print{Field::template print<ZmStream>},
      scan{ZvVField_RW<Field>::scanFn()},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = ZvVField_RW<Field>::template setFn<int64_t>()} { }

  template <typename Field>
  ZvVField(Field,
        ZuIfT<Field::Type == ZvFieldType::Float> *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = Field::Exponent},
      print{Field::template print<ZmStream>},
      scan{ZvVField_RW<Field>::scanFn()},
      getFn{.float_ = [](const void *o) -> double { return Field::get(o); }},
      setFn{.float_ = ZvVField_RW<Field>::template setFn<double>()} { }

  template <typename Field>
  ZvVField(Field,
        ZuIfT<Field::Type == ZvFieldType::Fixed> *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = Field::Exponent},
      print{Field::template print<ZmStream>},
      scan{ZvVField_RW<Field>::scanFn()},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = ZvVField_RW<Field>::template setFn<int64_t>()} { }

  template <typename Field>
  ZvVField(Field,
        ZuIfT<Field::Type == ZvFieldType::Decimal> *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = Field::Exponent},
      print{Field::template print<ZmStream>},
      scan{ZvVField_RW<Field>::scanFn()},
      getFn{.decimal = [](const void *o) -> ZuDecimal {
	return Field::get(o);
      }},
      setFn{.decimal = ZvVField_RW<Field>::template setFn<ZuDecimal>()} { }

  template <typename Field>
  ZvVField(Field,
        ZuIfT<Field::Type == ZvFieldType::Time> *_ = 0) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = 0},
      print{Field::template print<ZmStream>},
      scan{ZvVField_RW<Field>::scanFn()},
      getFn{.time = [](const void *o) -> ZmTime { return Field::get(o); }},
      setFn{.time = ZvVField_RW<Field>::template setFn<ZmTime>()} { }

  template <unsigned I, typename L>
  ZuIfT<I == ZvFieldType::String> get_(const void *o, L l) {
    l(getFn.string(o));
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvFieldType::Composite> get_(const void *o, L l) {
    l(getFn.composite(o));
  }
  template <unsigned I, typename L>
  ZuIfT<
    I == ZvFieldType::Bool || I == ZvFieldType::Int ||
    I == ZvFieldType::Fixed || I == ZvFieldType::Hex ||
    I == ZvFieldType::Enum || I == ZvFieldType::Flags> get_(
	const void *o, L l) {
    l(getFn.int_(o));
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvFieldType::Float> get_(const void *o, L l) {
    l(getFn.float_(o));
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvFieldType::Decimal> get_(const void *o, L l) {
    l(getFn.decimal(o));
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvFieldType::Time> get_(const void *o, L l) {
    l(getFn.time(o));
  }
  template <typename L> void get(const void *o, L l) {
    ZuSwitch::dispatch<ZvFieldType::N>(type,
	[this, o, l = ZuMv(l)](auto i) mutable { get_<i>(o, ZuMv(l)); });
  }

  template <unsigned I, typename L>
  ZuIfT<
    I == ZvFieldType::String || I == ZvFieldType::Composite> set_(
	void *o, L l) {
    setFn.string(o, l.template operator ()<ZuString>());
  }
  template <unsigned I, typename L>
  ZuIfT<
    I == ZvFieldType::Bool || I == ZvFieldType::Int ||
    I == ZvFieldType::Fixed || I == ZvFieldType::Hex ||
    I == ZvFieldType::Enum || I == ZvFieldType::Flags> set_(
	void *o, L l) {
    setFn.int_(o, l.template operator ()<int64_t>());
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvFieldType::Float> set_(void *o, L l) {
    setFn.float_(o, l.template operator ()<double>());
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvFieldType::Decimal> set_(void *o, L l) {
    setFn.decimal(o, l.template operator ()<ZuDecimal>());
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvFieldType::Time> set_(void *o, L l) {
    setFn.time(o, l.template operator ()<ZmTime>());
  }
  template <typename L>
  void set(void *o, L l) {
    ZuSwitch::dispatch<ZvFieldType::N>(type,
	[this, o, l = ZuMv(l)](auto i) mutable { set_<i>(o, ZuMv(l)); });
  }
};

using ZvVFieldArray = ZuArray<const ZvVField>;

template <typename ...Fields>
struct ZvVFields__ {
  static ZvVFieldArray fields() {
    static const ZvVField fields_[] =
      // std::initializer_list<ZvVField>
    {
      ZvVField{Fields{}}...
    };
    return {&fields_[0], sizeof(fields_) / sizeof(fields_[0])};
  }
};
template <typename FieldList>
inline ZvVFieldArray ZvVFields_() {
  return ZuTypeApply<ZvVFields__, FieldList>::fields();
}
template <typename T>
inline ZvVFieldArray ZvVFields() {
  return ZvVFields_<ZvFieldList<T>>();
}

#endif /* ZvField_HPP */
