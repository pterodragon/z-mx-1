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

namespace ZvField {

namespace Type {
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

namespace Flags {
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
#define ZvFieldMkFlags__(Flag) ZvField::Flags::Flag |
#define ZvFieldMkFlags_(...) (ZuPP_Map(ZvFieldMkFlags__, __VA_ARGS__) 0)
#define ZvFieldMkFlags(Args) ZuPP_Defer(ZvFieldMkFlags_)(ZuPP_Strip(Args))

struct TimeFmt_Null : public ZuPrintable {
  template <typename S> void print(S &) const { }
};
using TimeFmt_FIX = ZtDateFmt::FIX<-9, TimeFmt_Null>;
ZuDeclUnion(TimeFmt,
    (ZtDateFmt::CSV, csv),
    (TimeFmt_FIX, fix),
    (ZtDateFmt::ISO, iso));

struct Fmt {
  Fmt() { new (time.init_csv()) ZtDateFmt::CSV{}; }

  ZuVFmt	scalar;			// scalar format (print only)
  TimeFmt	time;			// date/time format
  char		flagsDelim = '|';	// flags delimiter
};

template <typename Base, unsigned Type_, unsigned Flags_>
struct Field_ : public Base {
  enum { Type = Type_ };
  enum { Flags = Flags_ };
};

template <typename Base, unsigned Flags>
struct RdString_ : public Field_<Base, Type::String, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const Fmt &) {
    s << Base::get(o);
  }
};
template <typename Base, unsigned Flags>
struct String_ : public RdString_<Base, Flags> {
  static void scan(void *o, ZuString s, const Fmt &) {
    Base::set(o, s);
  }
};

#define ZvFieldXRdString(U, ID, Member, Flags_) \
  ZvField::RdString_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>
#define ZvFieldXRdStringFn(U, ID, Get, Set, Flags_) \
  ZvField::RdString_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>

#define ZvFieldXString(U, ID, Member, Flags) \
  ZvField::String_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXStringFn(U, ID, Get, Set, Flags) \
  ZvField::String_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

#define ZvFieldRdString(U, Member, Flags) \
  ZvFieldXRdString(U, Member, Member, Flags)
#define ZvFieldRdStringFn(U, Fn, Flags) \
  ZvFieldXRdStringFn(U, Fn, Fn, Fn, Flags)

#define ZvFieldString(U, Member, Flags) \
  ZvFieldXString(U, Member, Member, Flags)
#define ZvFieldStringFn(U, Fn, Flags) \
  ZvFieldXStringFn(U, Fn, Fn, Fn, Flags)

template <typename Base, unsigned Flags>
struct RdComposite_ : public Field_<Base, Type::Composite, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const Fmt &) {
    s << Base::get(o);
  }
};
template <typename Base, unsigned Flags>
struct Composite_ : public RdComposite_<Base, Flags> {
  static void scan(void *o, ZuString s, const Fmt &) {
    Base::set(o, s);
  }
};

#define ZvFieldXRdComposite(U, ID, Member, Flags_) \
  ZvField::RdComposite_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>
#define ZvFieldXRdCompositeFn(U, ID, Get, Set, Flags_) \
  ZvField::RdComposite_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>

#define ZvFieldXComposite(U, ID, Member, Flags) \
  ZvField::Composite_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXCompositeFn(U, ID, Get, Set, Flags) \
  ZvField::Composite_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

#define ZvFieldRdComposite(U, Member, Flags) \
  ZvFieldXRdComposite(U, Member, Member, Flags)
#define ZvFieldRdCompositeFn(U, Fn, Flags) \
  ZvFieldXRdCompositeFn(U, Fn, Fn, Fn, Flags)

#define ZvFieldComposite(U, Member, Flags) \
  ZvFieldXComposite(U, Member, Member, Flags)
#define ZvFieldCompositeFn(U, Fn, Flags) \
  ZvFieldXCompositeFn(U, Fn, Fn, Fn, Flags)

template <typename Base, unsigned Flags>
struct RdBool_ : public Field_<Base, Type::Bool, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const Fmt &) {
    s << (Base::get(o) ? '1' : '0');
  }
};
template <typename Base, unsigned Flags>
struct Bool_ : public RdBool_<Base, Flags> {
  static void scan(void *o, ZuString s, const Fmt &) {
    Base::set(o, s.length() == 1 && s[0] == '1');
  }
};

#define ZvFieldXRdBool(U, ID, Member, Flags_) \
  ZvField::RdBool_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>
#define ZvFieldXRdBoolFn(U, ID, Get, Set, Flags_) \
  ZvField::RdBool_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>

#define ZvFieldXBool(U, ID, Member, Flags) \
  ZvField::Bool_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXBoolFn(U, ID, Get, Set, Flags) \
  ZvField::Bool_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

#define ZvFieldRdBool(U, Member, Flags) \
  ZvFieldXRdBool(U, Member, Member, Flags)
#define ZvFieldRdBoolFn(U, Fn, Flags) \
  ZvFieldXRdBoolFn(U, Fn, Fn, Fn, Flags)

#define ZvFieldBool(U, Member, Flags) \
  ZvFieldXBool(U, Member, Member, Flags)
#define ZvFieldBoolFn(U, Fn, Flags) \
  ZvFieldXBoolFn(U, Fn, Fn, Fn, Flags)

template <typename Base, unsigned Flags>
struct RdInt_ : public Field_<Base, Type::Int, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const Fmt &fmt) {
    s << ZuBoxed(Base::get(o)).vfmt(fmt.scalar);
  }
};
template <typename Base, unsigned Flags>
struct Int_ : public RdInt_<Base, Flags> {
  static void scan(void *o, ZuString s, const Fmt &) {
    using T = typename Base::T;
    Base::set(o, ZuBoxT<T>{s});
  }
};

#define ZvFieldXRdInt(U, ID, Member, Flags_) \
  ZvField::RdInt_<ZuFieldXRdData(U, ID, Member),\
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>
#define ZvFieldXRdIntFn(U, ID, Get, Set, Flags_) \
  ZvField::RdInt_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>

#define ZvFieldXInt(U, ID, Member, Flags) \
  ZvField::Int_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXIntFn(U, ID, Get, Set, Flags) \
  ZvField::Int_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

#define ZvFieldRdInt(U, Member, Flags) \
  ZvFieldXRdInt(U, Member, Member, Flags)
#define ZvFieldRdIntFn(U, Fn, Flags) \
  ZvFieldXRdIntFn(U, Fn, Fn, Fn, Flags)

#define ZvFieldInt(U, Member, Flags) \
  ZvFieldXInt(U, Member, Member, Flags)
#define ZvFieldIntFn(U, Fn, Flags) \
  ZvFieldXIntFn(U, Fn, Fn, Fn, Flags)

template <typename Base, unsigned Flags, unsigned Exponent_>
struct RdFloat_ : public Field_<Base, Type::Float, Flags> {
  enum { Exponent = Exponent_ };
  template <typename S>
  static void print(const void *o, S &s, const Fmt &fmt) {
    s << ZuBoxed(Base::get(o)).vfmt(fmt.scalar);
  }
};
template <typename Base, unsigned Flags, unsigned Exponent>
struct Float_ : public RdFloat_<Base, Flags, Exponent> {
  static void scan(void *o, ZuString s, const Fmt &) {
    using T = typename Base::T;
    Base::set(o, ZuBoxT<T>{s});
  }
};

#define ZvFieldXRdFloat(U, ID, Member, Flags_, Exponent) \
  ZvField::RdFloat_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly), Exponent>
#define ZvFieldXRdFloatFn(U, ID, Get, Set, Flags_, Exponent) \
  ZvField::RdFloat_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly), Exponent>

#define ZvFieldXFloat(U, ID, Member, Flags, Exponent) \
  ZvField::Float_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldXFloatFn(U, ID, Get, Set, Flags, Exponent) \
  ZvField::Float_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags), Exponent>

#define ZvFieldRdFloat(U, Member, Flags, Exponent) \
  ZvFieldXRdFloat(U, Member, Member, Flags, Exponent)
#define ZvFieldRdFloatFn(U, Fn, Flags, Exponent) \
  ZvFieldXRdFloatFn(U, Fn, Fn, Fn, Flags, Exponent)

#define ZvFieldFloat(U, Member, Flags, Exponent) \
  ZvFieldXFloat(U, Member, Member, Flags, Exponent)
#define ZvFieldFloatFn(U, Fn, Flags, Exponent) \
  ZvFieldXFloatFn(U, Fn, Fn, Fn, Flags, Exponent)

template <typename Base, unsigned Flags, unsigned Exponent_>
struct RdFixed_ : public Field_<Base, Type::Fixed, Flags> {
  enum { Exponent = Exponent_ };
  template <typename S>
  static void print(const void *o, S &s, const Fmt &fmt) {
    s << ZuFixed{Base::get(o), Exponent}.vfmt(fmt.scalar);
  }
};
template <typename Base, unsigned Flags, unsigned Exponent>
struct Fixed_ : public RdFixed_<Base, Flags, Exponent> {
  static void scan(void *o, ZuString s, const Fmt &) {
    Base::set(o, ZuFixed{s, Exponent}.value);
  }
};

#define ZvFieldXRdFixed(U, ID, Member, Flags_, Exponent) \
  ZvField::RdFixed_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly), Exponent>
#define ZvFieldXRdFixedFn(U, ID, Get, Set, Flags_, Exponent) \
  ZvField::RdFixed_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly), Exponent>

#define ZvFieldXFixed(U, ID, Member, Flags, Exponent) \
  ZvField::Fixed_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldXFixedFn(U, ID, Get, Set, Flags, Exponent) \
  ZvField::Fixed_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags), Exponent>

#define ZvFieldRdFixed(U, Member, Flags, Exponent) \
  ZvFieldXRdFixed(U, Member, Member, Flags, Exponent)
#define ZvFieldRdFixedFn(U, Fn, Flags, Exponent) \
  ZvFieldXRdFixedFn(U, Fn, Fn, Fn, Flags, Exponent)

#define ZvFieldFixed(U, Member, Flags, Exponent) \
  ZvFieldXFixed(U, Member, Member, Flags, Exponent)
#define ZvFieldFixedFn(U, Fn, Flags, Exponent) \
  ZvFieldXFixedFn(U, Fn, Fn, Fn, Flags, Exponent)

template <typename Base, unsigned Flags, unsigned Exponent_>
struct RdDecimal_ : public Field_<Base, Type::Decimal, Flags> {
  enum { Exponent = Exponent_ };
  template <typename S>
  static void print(const void *o, S &s, const Fmt &fmt) {
    s << Base::get(o).vfmt(fmt.scalar);
  }
};
template <typename Base, unsigned Flags, unsigned Exponent>
struct Decimal_ : public RdDecimal_<Base, Flags, Exponent> {
  static void scan(void *o, ZuString s, const Fmt &) {
    Base::set(o, s);
  }
};

#define ZvFieldXRdDecimal(U, ID, Member, Flags_, Exponent) \
  ZvField::RdDecimal_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly), Exponent>
#define ZvFieldXRdDecimalFn(U, ID, Get, Set, Flags_, Exponent) \
  ZvField::RdDecimal_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly), Exponent>

#define ZvFieldXDecimal(U, ID, Member, Flags, Exponent) \
  ZvField::Decimal_<ZuFieldXData(U, ID, Member), \
    ZvFieldMkFlags(Flags), Exponent>
#define ZvFieldXDecimalFn(U, ID, Get, Set, Flags, Exponent) \
  ZvField::Decimal_<ZuFieldXFn(U, ID, Get, Set), \
    ZvFieldMkFlags(Flags), Exponent>

#define ZvFieldRdDecimal(U, Member, Flags, Exponent) \
  ZvFieldXRdDecimal(U, Member, Member, Flags, Exponent)
#define ZvFieldRdDecimalFn(U, Fn, Flags, Exponent) \
  ZvFieldXRdDecimalFn(U, Fn, Fn, Fn, Flags, Exponent)

#define ZvFieldDecimal(U, Member, Flags, Exponent) \
  ZvFieldXDecimal(U, Member, Member, Flags, Exponent)
#define ZvFieldDecimalFn(U, Fn, Flags, Exponent) \
  ZvFieldXDecimalFn(U, Fn, Fn, Fn, Flags, Exponent)

template <typename Base, unsigned Flags>
struct RdHex_ : public Field_<Base, Type::Hex, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const Fmt &fmt) {
    s << ZuBoxed(Base::get(o)).vfmt(fmt.scalar).hex();
  }
};
template <typename Base, unsigned Flags>
struct Hex_ : public RdHex_<Base, Flags> {
  static void scan(void *o, ZuString s, const Fmt &) {
    using T = typename Base::T;
    Base::set(o, ZuBoxT<T>{ZuFmt::Hex<>{}, s});
  }
};

#define ZvFieldXRdHex(U, ID, Member, Flags_) \
  ZvField::RdHex_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>
#define ZvFieldXRdHexFn(U, ID, Get, Set, Flags_) \
  ZvField::RdHex_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>

#define ZvFieldXHex(U, ID, Member, Flags) \
  ZvField::Hex_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXHexFn(U, ID, Get, Set, Flags) \
  ZvField::Hex_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

#define ZvFieldRdHex(U, Member, Flags) \
  ZvFieldXRdHex(U, Member, Member, Flags)
#define ZvFieldRdHexFn(U, Fn, Flags) \
  ZvFieldXRdHexFn(U, Fn, Fn, Fn, Flags)

#define ZvFieldHex(U, Member, Flags) \
  ZvFieldXHex(U, Member, Member, Flags)
#define ZvFieldHexFn(U, Fn, Flags) \
  ZvFieldXHexFn(U, Fn, Fn, Fn, Flags)

template <typename Base, unsigned Flags, typename Map_>
struct RdEnum_ : public Field_<Base, Type::Enum, Flags> {
  using Map = Map_;
  template <typename S>
  static void print(const void *o, S &s, const Fmt &) {
    s << Map::instance()->v2s(Base::get(o));
  }
};
template <typename Base, unsigned Flags, typename Map>
struct Enum_ : public RdEnum_<Base, Flags, Map> {
  static void scan(void *o, ZuString s, const Fmt &) {
    Base::set(o, Map::instance()->s2v(s));
  }
};

#define ZvFieldXRdEnum(U, ID, Member, Flags_, Map) \
  ZvField::RdEnum_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly), Map>
#define ZvFieldXRdEnumFn(U, ID, Get, Set, Flags_, Map) \
  ZvField::RdEnum_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly), Map>

#define ZvFieldXEnum(U, ID, Member, Flags, Map) \
  ZvField::Enum_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags), Map>
#define ZvFieldXEnumFn(U, ID, Get, Set, Flags, Map) \
  ZvField::Enum_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags), Map>

#define ZvFieldRdEnum(U, Member, Flags, Map) \
  ZvFieldXRdEnum(U, Member, Member, Flags, Map)
#define ZvFieldRdEnumFn(U, Fn, Flags, Map) \
  ZvFieldXRdEnumFn(U, Fn, Fn, Fn, Flags, Map)

#define ZvFieldEnum(U, Member, Flags, Map) \
  ZvFieldXEnum(U, Member, Member, Flags, Map)
#define ZvFieldEnumFn(U, Fn, Flags, Map) \
  ZvFieldXEnumFn(U, Fn, Fn, Fn, Flags, Map)

template <typename Base, unsigned Flags, typename Map_>
struct RdFlags_ : public Field_<Base, Type::Flags, Flags> {
  using Map = Map_;
  template <typename S>
  static void print(const void *o, S &s, const Fmt &fmt) {
    Map::instance()->print(s, Base::get(o), fmt.flagsDelim);
  }
};
template <typename Base, unsigned Flags, typename Map>
struct Flags_ : public RdFlags_<Base, Flags, Map> {
  static void scan(void *o, ZuString s, const Fmt &fmt) {
    using T = typename Base::T;
    Base::set(o, Map::instance()->template scan<T>(s, fmt.flagsDelim));
  }
};

#define ZvFieldXRdFlags(U, ID, Member, Flags_, Map) \
  ZvField::RdFlags_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly), Map>
#define ZvFieldXRdFlagsFn(U, ID, Get, Set, Flags_, Map) \
  ZvField::RdFlags_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly), Map>

#define ZvFieldXFlags(U, ID, Member, Flags, Map) \
  ZvField::Flags_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags), Map>
#define ZvFieldXFlagsFn(U, ID, Get, Set, Flags, Map) \
  ZvField::Flags_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags), Map>

#define ZvFieldRdFlags(U, Member, Flags, Map) \
  ZvFieldXRdFlags(U, Member, Member, Flags, Map)
#define ZvFieldRdFlagsFn(U, Fn, Flags, Map) \
  ZvFieldXRdFlagsFn(U, Fn, Fn, Fn, Flags, Map)

#define ZvFieldFlags(U, Member, Flags, Map) \
  ZvFieldXFlags(U, Member, Member, Flags, Map)
#define ZvFieldFlagsFn(U, Fn, Flags, Map) \
  ZvFieldXFlagsFn(U, Fn, Fn, Fn, Flags, Map)

template <typename Base, unsigned Flags>
struct RdTime_ : public Field_<Base, Type::Time, Flags> {
  template <typename S>
  static void print(const void *o, S &s, const Fmt &fmt) {
    ZtDate v{Base::get(o)};
    switch (fmt.time.type()) {
      default:
      case TimeFmt::Index<ZtDateFmt::CSV>::I:
	s << v.csv(fmt.time.csv());
	break;
      case TimeFmt::Index<TimeFmt_FIX>::I:
	s << v.fix(fmt.time.fix());
	break;
      case TimeFmt::Index<ZtDateFmt::ISO>::I:
	s << v.iso(fmt.time.iso());
	break;
    }
  }
};
template <typename Base, unsigned Flags>
struct Time_ : public RdTime_<Base, Flags> {
  static void scan(void *o, ZuString s, const Fmt &fmt) {
    switch (fmt.time.type()) {
      default:
      case TimeFmt::Index<ZtDateFmt::CSV>::I:
	Base::set(o, ZtDate{ZtDate::CSV, s});
	break;
      case TimeFmt::Index<TimeFmt_FIX>::I:
	Base::set(o, ZtDate{ZtDate::FIX, s});
	break;
      case TimeFmt::Index<ZtDateFmt::ISO>::I:
	Base::set(o, ZtDate{s});
	break;
    }
  }
};

#define ZvFieldXRdTime(U, ID, Member, Flags_) \
  ZvField::RdTime_<ZuFieldXRdData(U, ID, Member), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>
#define ZvFieldXRdTimeFn(U, ID, Get, Set, Flags_) \
  ZvField::RdTime_<ZuFieldXRdFn(U, ID, Get), \
    (ZvFieldMkFlags(Flags_) | ZvField::Flags::ReadOnly)>

#define ZvFieldXTime(U, ID, Member, Flags) \
  ZvField::Time_<ZuFieldXData(U, ID, Member), ZvFieldMkFlags(Flags)>
#define ZvFieldXTimeFn(U, ID, Get, Set, Flags) \
  ZvField::Time_<ZuFieldXFn(U, ID, Get, Set), ZvFieldMkFlags(Flags)>

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

} // namespace ZvField

ZuTypeList<> ZvFieldList_(...); // default

void *ZvFielded_(...); // default

#define ZvFields(U, ...)  \
  namespace { \
    ZuPP_Eval(ZuPP_MapArg(ZuField_ID, U, __VA_ARGS__)) \
  } \
  U *ZvFielded_(U *); \
  ZuTypeList<ZuPP_Eval(ZuPP_MapArgComma(ZvField_, U, __VA_ARGS__))> \
  ZvFieldList_(U *)

namespace ZvField {

template <typename T>
using List = decltype(ZvFieldList_(ZuDeclVal<ZuDecay<T> *>()));

template <typename T>
using Fielded = ZuDeref<decltype(*ZvFielded_(ZuDeclVal<ZuDecay<T> *>()))>;

template <typename Impl> struct Print : public ZuPrintable {
  const Impl *impl() const { return static_cast<const Impl *>(this); }
  Impl *impl() { return static_cast<Impl *>(this); }

  template <typename T>
  struct Print_Filter { enum { OK = !(T::Flags & Flags::ReadOnly) }; };
  template <typename S> void print(S &s) const {
    using FieldList = ZuTypeGrep<Print_Filter, List<Impl>>;
    thread_local Fmt fmt;
    ZuTypeAll<FieldList>::invoke([o = impl(), &s]<typename Field>() {
      if constexpr (ZuTypeIndex<Field, FieldList>::I) s << ' ';
      s << Field::id() << '=';
      Field::print(o, s, fmt);
    });
  }
};

template <typename V, typename FieldList>
struct Key__ {
  template <typename ...Fields>
  struct Mk_ {
    using Tuple = ZuTuple<typename Fields::T...>;
    static auto tuple(const V &v) { return Tuple{Fields::get(&v)...}; }
  };
  using Mk = ZuTypeApply<Mk_, FieldList>;
};
template <typename V> struct Key_ {
  template <typename T>
  struct Filter { enum { OK = T::Flags & Flags::Primary }; };
  using FieldList = ZuTypeGrep<Filter, List<V>>;
  using Mk = typename Key__<V, FieldList>::Mk;
};
template <typename V>
auto key(const V &v) {
  return Key_<V>::Mk::tuple(v);
}

} // namespace ZvField

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
  void		(*print)(uint64_t, ZmStream &, const ZvField::Fmt &);
  void		(*scan)(uint64_t &, ZuString, const ZvField::Fmt &);
};
template <typename Map>
struct ZvVField_Flags : public ZvVField_Flags_ {
  ZvVField_Flags() : ZvVField_Flags_{
    .id = []() -> const char * { return Map::id(); },
    .print = [](uint64_t v, ZmStream &s, const ZvField::Fmt &fmt) -> void {
      Map::instance()->print(s, v, fmt.flagsDelim);
    },
    .scan = [](uint64_t &v, ZuString s, const ZvField::Fmt &fmt) -> void {
      v = Map::instance()->template scan<uint64_t>(s, fmt.flagsDelim);
    }
  } { }

  static ZvVField_Flags *instance() {
    return ZmSingleton<ZvVField_Flags>::instance();
  }
};

template <typename T>
inline void ZvVField_RO_set(void *, T) { }
inline void ZvVField_RO_scan(void *, ZuString, const ZvField::Fmt &) { }
template <typename Field, int = Field::Flags & ZvField::Flags::ReadOnly>
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
  uint32_t	type;		// ZvField::Type
  uint32_t	flags;		// ZvField::Flags

  int		(*cmp)(const void *, const void *);

  union {
    unsigned		exponent;	// Float|Fixed|Decimal
    ZvVField_Enum_	*(*enum_)();	// Enum
    ZvVField_Flags_	*(*flags)();	// Flags
  } info;

  void		(*print)(const void *, ZmStream &, const ZvField::Fmt &);
  void		(*scan)(void *, ZuString, const ZvField::Fmt &);
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
        ZuIfT<Field::Type == ZvField::Type::String> *_ = 0) :
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
        ZuIfT<Field::Type == ZvField::Type::Composite> *_ = 0) :
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
	  Field::Type == ZvField::Type::Bool ||
	  Field::Type == ZvField::Type::Int ||
	  Field::Type == ZvField::Type::Hex> *_ = 0) :
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
        ZuIfT<Field::Type == ZvField::Type::Enum> *_ = 0) :
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
        ZuIfT<Field::Type == ZvField::Type::Flags> *_ = 0) :
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
        ZuIfT<Field::Type == ZvField::Type::Float> *_ = 0) :
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
        ZuIfT<Field::Type == ZvField::Type::Fixed> *_ = 0) :
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
        ZuIfT<Field::Type == ZvField::Type::Decimal> *_ = 0) :
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
        ZuIfT<Field::Type == ZvField::Type::Time> *_ = 0) :
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
  ZuIfT<I == ZvField::Type::String> get_(const void *o, L l) {
    l(getFn.string(o));
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvField::Type::Composite> get_(const void *o, L l) {
    l(getFn.composite(o));
  }
  template <unsigned I, typename L>
  ZuIfT<
    I == ZvField::Type::Bool || I == ZvField::Type::Int ||
    I == ZvField::Type::Fixed || I == ZvField::Type::Hex ||
    I == ZvField::Type::Enum || I == ZvField::Type::Flags> get_(
	const void *o, L l) {
    l(getFn.int_(o));
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvField::Type::Float> get_(const void *o, L l) {
    l(getFn.float_(o));
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvField::Type::Decimal> get_(const void *o, L l) {
    l(getFn.decimal(o));
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvField::Type::Time> get_(const void *o, L l) {
    l(getFn.time(o));
  }
  template <typename L> void get(const void *o, L l) {
    ZuSwitch::dispatch<ZvField::Type::N>(type,
	[this, o, l = ZuMv(l)](auto i) mutable { get_<i>(o, ZuMv(l)); });
  }

  template <unsigned I, typename L>
  ZuIfT<
    I == ZvField::Type::String || I == ZvField::Type::Composite> set_(
	void *o, L l) {
    setFn.string(o, l.template operator ()<ZuString>());
  }
  template <unsigned I, typename L>
  ZuIfT<
    I == ZvField::Type::Bool || I == ZvField::Type::Int ||
    I == ZvField::Type::Fixed || I == ZvField::Type::Hex ||
    I == ZvField::Type::Enum || I == ZvField::Type::Flags> set_(
	void *o, L l) {
    setFn.int_(o, l.template operator ()<int64_t>());
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvField::Type::Float> set_(void *o, L l) {
    setFn.float_(o, l.template operator ()<double>());
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvField::Type::Decimal> set_(void *o, L l) {
    setFn.decimal(o, l.template operator ()<ZuDecimal>());
  }
  template <unsigned I, typename L>
  ZuIfT<I == ZvField::Type::Time> set_(void *o, L l) {
    setFn.time(o, l.template operator ()<ZmTime>());
  }
  template <typename L>
  void set(void *o, L l) {
    ZuSwitch::dispatch<ZvField::Type::N>(type,
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
  return ZvVFields_<ZvField::List<T>>();
}

#endif /* ZvField_HPP */
