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
    Fixed,		// ZuFixed
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
    Synthetic	= 0x00004,	// synthetic (derived)
    Update	= 0x00008,	// include in updates
    Ctor_	= 0x00010,	// passed to constructor
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
#define ZvFieldMkFlags__(Flag) ZvFieldFlags::##Flag |
#define ZvFieldMkFlags_(...) \
  (ZuPP_Eval(ZuPP_Map(ZvFieldMkFlags__, __VA_ARGS__)) 0)
#define ZvFieldMkFlags(Args) ZuPP_Defer(ZvFieldMkFlags_)(T, ZuPP_Strip(Args))

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

template <typename Base, unsigned Flags_>
struct ZvFieldString_ : public ZvField_<Base, ZvFieldType::String, Flags_> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &) {
    s << Base::get(o);
  }
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, s);
  }
};
#define ZvFieldString(T, ID, Flags) \
  ZvFieldString_<ZuFieldData(T, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldStringFn(T, ID, Flags) \
  ZvFieldString_<ZuFieldFn(T, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags_>
struct ZvFieldComposite_ : public ZvField_<Base, ZvFieldType::Composite, Flags_> {
  static ZmStreamFn get(const void *o) {
    using T = typename Base::T;
    const T &v = Base::get(o);
    return {&v, [](const T *p, ZmStream &s) { s << *p; }};
  }
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &) {
    s << Base::get(o);
  }
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, s);
  }
};
#define ZvFieldComposite(T, ID, Flags) \
  ZvFieldComposite_<ZuFieldData(T, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldCompositeFn(T, ID, Flags) \
  ZvFieldComposite_<ZuFieldFn(T, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags_>
struct ZvFieldBool_ : public ZvField_<Base, ZvFieldType::Bool, Flags_> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &) {
    s << (Base::get(o) ? '1' : '0');
  }
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, s.length() == 1 && s[0] == '1');
  }
};
#define ZvFieldBool(T, ID, Flags) \
  ZvFieldBool_<ZuFieldData(T, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldBoolFn(T, ID, Flags) \
  ZvFieldBool_<ZuFieldFn(T, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags_>
struct ZvFieldInt_ : public ZvField_<Base, ZvFieldType::Int, Flags_> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    s << ZuBoxed(Base::get(o)).vfmt(fmt.scalar);
  }
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    using T = typename Base::T;
    Base::set(o, typename ZuBoxT<T>::T{s});
  }
};
#define ZvFieldInt(T, ID, Flags) \
  ZvFieldInt_<ZuFieldData(T, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldIntFn(T, ID, Flags) \
  ZvFieldInt_<ZuFieldFn(T, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags_>
struct ZvFieldFloat_ : public ZvField_<Base, ZvFieldType::Float, Flags_> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    s << ZuBoxed(Base::get(o)).vfmt(fmt.scalar);
  }
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    using T = typename Base::T;
    Base::set(o, typename ZuBoxT<T>::T{s});
  }
};
#define ZvFieldFloat(T, ID, Flags) \
  ZvFieldFloat_<ZuFieldData(T, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldFloatFn(T, ID, Flags) \
  ZvFieldFloat_<ZuFieldFn(T, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags_, unsigned Exponent_>
struct ZvFieldFixed_ : public ZvField_<Base, ZvFieldType::Fixed, Flags_> {
  enum { Exponent = Exponent_ };
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    s << ZuFixed{Base::get(o), Exponent}.vfmt(fmt.scalar);
  }
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, ZuFixed{s, Exponent}.value);
  }
};
#define ZvFieldFixed(T, ID, Flags, Exponent) \
  ZvFieldFixed_<ZuFieldData(T, ID), Flags, Exponent>
#define ZvFieldFixedFn(T, ID, Flags, Exponent) \
  ZvFieldFixed_<ZuFieldFn(T, ID), Flags, Exponent>

template <typename Base, unsigned Flags_>
struct ZvFieldDecimal_ : public ZvField_<Base, ZvFieldType::Decimal, Flags_> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    s << Base::get(o).vfmt(fmt.scalar);
  }
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, s);
  }
};
#define ZvFieldDecimal(T, ID, Flags) \
  ZvFieldDecimal_<ZuFieldData(T, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldDecimalFn(T, ID, Flags) \
  ZvFieldDecimal_<ZuFieldFn(T, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags_>
struct ZvFieldHex_ : public ZvField_<Base, ZvFieldType::Hex, Flags_> {
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    s << ZuBoxed(Base::get(o)).vfmt(fmt.scalar).hex();
  }
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    using T = typename Base::T;
    Base::set(o, typename ZuBoxT<T>::T{ZuFmt::Hex<>{}, s});
  }
};
#define ZvFieldHex(T, ID, Flags) \
  ZvFieldHex_<ZuFieldData(T, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldHexFn(T, ID, Flags) \
  ZvFieldHex_<ZuFieldFn(T, ID), ZvFieldMkFlags(Flags)>

template <typename Base, unsigned Flags_, typename Map_>
struct ZvFieldEnum_ : public ZvField_<Base, ZvFieldType::Enum, Flags_> {
  using Map = Map_;
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &) {
    s << Map::instance()->v2s(Base::get(o));
  }
  static void scan(void *o, ZuString s, const ZvFieldFmt &) {
    Base::set(o, Map::instance()->s2v(s));
  }
};
#define ZvFieldEnum(T, ID, Flags, Map) \
  ZvFieldEnum_<ZuFieldData(T, ID), Flags, Map>
#define ZvFieldEnumFn(T, ID, Flags, Map) \
  ZvFieldEnum_<ZuFieldFn(T, ID), Flags, Map>

template <typename Base, unsigned Flags_, typename Map_>
struct ZvFieldFlags_ : public ZvField_<Base, ZvFieldType::Flags, Flags_> {
  using Map = Map_;
  template <typename S>
  static void print(const void *o, S &s, const ZvFieldFmt &fmt) {
    Map::instance()->print(s, Base::get(o), fmt.flagsDelim);
  }
  static void scan(void *o, ZuString s, const ZvFieldFmt &fmt) {
    using T = typename Base::T;
    Base::set(o, Map::instance()->template scan<T>(s, fmt.flagsDelim));
  }
};
#define ZvFieldFlags(T, ID, Flags, Map) \
  ZvFieldFlags_<ZuFieldData(T, ID), Flags, Map>
#define ZvFieldFlagsFn(T, ID, Flags, Map) \
  ZvFieldFlags_<ZuFieldFn(T, ID), Flags, Map>

template <typename Base, unsigned Flags_>
struct ZvFieldTime_ : public ZvField_<Base, ZvFieldType::Time, Flags_> {
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
#define ZvFieldTime(T, ID, Flags) \
  ZvFieldTime_<ZuFieldData(T, ID), ZvFieldMkFlags(Flags)>
#define ZvFieldTimeFn(T, ID, Flags) \
  ZvFieldTime_<ZuFieldFn(T, ID), ZvFieldMkFlags(Flags)>

#define ZvField__(T, Type, ...) ZvField##Type(T, __VA_ARGS__)
#define ZvField_(T, Args) ZuPP_Defer(ZvField__)(T, ZuPP_Strip(Args))

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

  template <typename S> void print(S &s) const {
    template <typename T>
    struct Filter { enum { OK = !(T::Flags & ZvFieldFlags::Synthetic) }; };
    using FieldList = typename ZuTypeGrep<Filter, ZvFieldList<Impl>>::T;
    thread_local ZvFieldFmt fmt;
    ZuTypeAll<FieldList>::invoke([ptr = impl(), &s]<typename Field>() {
      if constexpr (ZuTypeIndex<Field, FieldList>::I) s << ' ';
      s << Field::id() << '=';
      Field::print(ptr, s, fmt);
    });
  }
};

template <typename V, typename FieldList>
struct ZvFieldTuple {
  template <typename ...Fields>
  struct Mk_ {
    using T = ZuTuple<typename Fields::T...>;
    static T mk(const V &v) { return { Fields::get(&v)... }; }
  };
  using Mk = typename ZuTypeApply<Mk_, FieldList>::T;
};

template <typename V> struct ZvFieldKey_ {
  template <typename T>
  struct PrimaryOK { enum { OK = T::Flags & ZvFieldFlags::Primary }; };
  using FieldList = typename ZuTypeGrep<PrimaryOK, ZvFieldList<V>>::T;
  using Mk = typename ZuFieldTuple<V, FieldList>::Mk;
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
struct ZvVField_Enum : public ZvField_Enum_ {
  ZvVField_Enum(Map) : ZvField_Enum_{
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
struct ZvVField_Flags : public ZvField_Flags_ {
  ZvVField_Flags(Map) : ZvField_Flags_{
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

  template <
    typename Field,
    typename = ZuIfT<Field::Type == ZvFieldType::String>::T>
  ZvVField(Field) :
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
	Field::set(o, s);
      }} { }

  template <
    typename Field,
    typename = ZuIfT<Field::Type == ZvFieldType::Composite>::T>
  ZvVField(Field) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = 0},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.composite = [](const void *o) -> ZmStreamFn {
	using T = typename Field::T;
	const T &v = Field::get(o);
	return {&v, [](const T *ptr, ZmStream &s) { s << *ptr; }};
      }},
      setFn{.stream = [](void *o, ZuString s) {
	Field::set(o, s);
      }} { }

  template <
    typename Field,
    typename = ZuIfT<
      Field::Type == ZvFieldType::Bool ||
      Field::Type == ZvFieldType::Int ||
      Field::Type == ZvFieldType::Hex>::T>
  ZvVField(Field) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = 0},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = [](void *o, int64_t v) { Field::set(o, v); }} { }

      Field::Type == ZvFieldType::Flags

  template <
    typename Field,
    typename = ZuIfT<Field::Type == ZvFieldType::Enum>::T>
  ZvVField(Field) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.enum_ = []() { return ZvVField_Enum<Field::Map>::instance(); }},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = [](void *o, int64_t v) { Field::set(o, v); }} { }

  template <
    typename Field,
    typename = ZuIfT<Field::Type == ZvFieldType::Flags>::T>
  ZvVField(Field) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.flags = []() { return ZvVField_Flags<Field::Map>::instance(); }},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = [](void *o, int64_t v) { Field::set(o, v); }} { }

  template <
    typename Field,
    typename = ZuIfT<Field::Type == ZvFieldType::Float>::T>
  ZvVField(Field) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = Field::Exponent},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.float_ = [](const void *o) -> double { return Field::get(o); }},
      setFn{.float_ = [](void *o, double v) { Field::set(o, v); }} { }

  template <
    typename Field,
    typename = ZuIfT<Field::Type == ZvFieldType::Fixed>::T>
  ZvVField(Field) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = Field::Exponent},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.int_ = [](const void *o) -> int64_t { return Field::get(o); }},
      setFn{.int_ = [](void *o, int64_t v) { Field::set(o, v); }} { }

  template <
    typename Field,
    typename = ZuIfT<Field::Type == ZvFieldType::Decimal>::T>
  ZvVField(Field) :
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
	Field::set(o, v);
      }} { }

  template <
    typename Field,
    typename = ZuIfT<Field::Type == ZvFieldType::Time>::T>
  ZvVField(Field) :
      id{Field::id()}, type{Field::Type}, flags{Field::Flags},
      cmp{[](const void *o1, const void *o2) {
	return ZuCmp<typename Field::T>::cmp(Field::get(o1), Field::get(o2));
      }},
      info{.exponent = 0},
      print{Field::template print<ZmStream>}, scan{Field::scan},
      getFn{.time = [](const void *o) -> ZmTime { return Field::get(o); }},
      setFn{.time = [](void *o, ZmTime v) { Field::set(o, v); }} { }

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
	[o, l = ZuMv(l)](auto i) mutable { get_<i>(o, ZuMv(l)); });
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
	[o, l = ZuMv(l)](auto i) mutable { set_<i>(o, ZuMv(l)); });
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
  return ZvVFields_<ZvFieldList<T>>::fields();
}

#endif /* ZvField_HPP */
