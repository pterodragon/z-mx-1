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

// run-time flat object introspection
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

#include <zlib/ZtEnum.hpp>
#include <zlib/ZtDate.hpp>

namespace ZvFieldType {
  enum _ {
    String = 0,
    Stream,
    Bool,
    Int,
    Float,
    Fixed,
    Decimal,
    Hex,
    Enum,
    Flags,
    Time,
    N
  };
}

namespace ZvFieldFlags {
  enum _ {
    Primary	= 0x01,		// primary key
    Secondary	= 0x02,		// secondary key
    Synthetic	= 0x04,		// synthetic (derived)
    Series	= 0x08,		// data series
      Index	= 0x10,		// - index (e.g. time stamp)
      Delta	= 0x20,		// - first derivative
      Delta2	= 0x40		// - second derivative
  };
}

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
  ZvFieldFmt(const ZvFieldFmt &) = default;
  ZvFieldFmt &operator =(const ZvFieldFmt &) = default;
  ZvFieldFmt(ZvFieldFmt &&) = default;
  ZvFieldFmt &operator =(ZvFieldFmt &&) = default;
  ~ZvFieldFmt() = default;

  ZuVFmt	scalar;			// scalar format (print only)
  ZvTimeFmt	time;			// date/time format
  char		flagsDelim = '|';	// flags delimiter
};

struct ZvField_Enum_ : public ZuObject {
  virtual ~ZvField_Enum_() { }
  virtual void print(ZmStream &, int64_t) = 0;
  virtual int64_t scan(ZuString) = 0;
};
template <typename Map>
struct ZvField_Enum : public ZvField_Enum_ {
  static ZvField_Enum *instance() {
    return ZmSingleton<ZvField_Enum>::instance();
  }
  ZvField_Enum() : map{Map::instance()} { }
  void print(ZmStream &s, int64_t i) { s << map->v2s(i); }
  int64_t scan(ZuString s) { return map->s2v(s); }
  ZuRef<Map>	map;
};

struct ZvField_Flags_ : public ZuObject {
  virtual ~ZvField_Flags_() { }
  virtual void print(ZmStream &, int64_t, char delim) = 0;
  virtual int64_t scan(ZuString, char delim) = 0;
};
template <typename Map>
struct ZvField_Flags : public ZvField_Flags_ {
  static ZvField_Flags *instance() {
    return ZmSingleton<ZvField_Flags>::instance();
  }
  ZvField_Flags() : map{Map::instance()} { }
  void print(ZmStream &s, int64_t i, char delim) { map->print(s, i, delim); }
  int64_t scan(ZuString s, char delim) {
    return map->template scan<int64_t>(s, delim);
  }
  ZuRef<Map>	map;
};

struct ZvField {
  const char	*id;
  uint32_t	type;		// ZvFieldType
  uint32_t	flags;		// ZvFieldFlags

  int		(*cmp)(const void *, const void *);

  union {
    unsigned		exponent;	// Float|Fixed|Decimal
    ZvField_Enum_	*(*enum_)();	// Enum
    ZvField_Flags_	*(*flags)();	// Flags
  } info;

  void		(*print)(const void *, ZmStream &, const ZvFieldFmt *);
  union {
    ZuString	(*string)(const void *);	// String
    ZmStreamFn	(*stream)(const void *);	// Stream
    int64_t	(*int_)(const void *);		// Bool|Int|Fixed|Hex|Enum|Flags
    double	(*float_)(const void *);	// Float
    ZuDecimal	(*decimal)(const void *);	// Decimal
    ZmTime	(*time)(const void *);		// Time
  } getFn;

  void		(*scan)(void *, ZuString, const ZvFieldFmt *);
  union {
    void	(*string)(void *, ZuString);	// String|Stream
    void	(*int_)(void *, int64_t);	// Bool|Int|Fixed|Hex|Enum|Flags
    void	(*float_)(void *, double);	// Float
    void	(*decimal)(void *, ZuDecimal);	// Decimal
    void	(*time)(void *, ZmTime);	// Time
  } setFn;

  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::String>::T get_(const void *ptr, L l) {
    l(getFn.string(ptr));
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Stream>::T get_(const void *ptr, L l) {
    l(getFn.stream(ptr));
  }
  template <unsigned I, typename L>
  typename ZuIfT<
    I == ZvFieldType::Bool || I == ZvFieldType::Int ||
    I == ZvFieldType::Fixed || I == ZvFieldType::Hex ||
    I == ZvFieldType::Enum || I == ZvFieldType::Flags>::T get_(
	const void *ptr, L l) {
    l(getFn.int_(ptr));
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Float>::T get_(const void *ptr, L l) {
    l(getFn.float_(ptr));
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Decimal>::T get_(const void *ptr, L l) {
    l(getFn.decimal(ptr));
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Time>::T get_(const void *ptr, L l) {
    l(getFn.time(ptr));
  }
  template <typename L> void get(const void *ptr, L l) {
    ZuSwitch::dispatch<ZvFieldType::N>(type,
	[ptr, l = ZuMv(l)](auto i) mutable { get_<i>(ptr, ZuMv(l)); });
  }

  template <unsigned I, typename L>
  typename ZuIfT<
    I == ZvFieldType::String || I == ZvFieldType::Stream>::T set_(
	void *ptr, L l) {
    setFn.string(ptr, l.template operator ()<ZuString>());
  }
  template <unsigned I, typename L>
  typename ZuIfT<
    I == ZvFieldType::Bool || I == ZvFieldType::Int ||
    I == ZvFieldType::Fixed || I == ZvFieldType::Hex ||
    I == ZvFieldType::Enum || I == ZvFieldType::Flags>::T set_(
	void *ptr, L l) {
    setFn.int_(ptr, l.template operator ()<int64_t>());
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Float>::T set_(void *ptr, L l) {
    setFn.float_(ptr, l.template operator ()<double>());
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Decimal>::T set_(void *ptr, L l) {
    setFn.decimal(ptr, l.template operator ()<ZuDecimal>());
  }
  template <unsigned I, typename L>
  typename ZuIfT<I == ZvFieldType::Time>::T set_(void *ptr, L l) {
    setFn.time(ptr, l.template operator ()<ZmTime>());
  }
  template <typename L>
  void set(void *ptr, L l) {
    ZuSwitch::dispatch<ZvFieldType::N>(type,
	[ptr, l = ZuMv(l)](auto i) mutable { set_<i>(ptr, ZuMv(l)); });
  }
};

#define ZvFieldCmp_(T_, member, get) \
    [](const void *o1, const void *o2) { \
      return ZuCmp<decltype(get(T_, o1, member))>::cmp( \
	  get(T_, o1, member), get(T_, o2, member)); }

#define ZvFieldString_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::String, flags, \
    ZvFieldCmp_(T_, member, get), \
    { .exponent = 0 }, \
    [](const void *o, ZmStream &s, const ZvFieldFmt *) { \
      s << get(T_, o, member); \
    }, { \
      .string = [](const void *o) -> ZuString { return get(T_, o, member); } \
    }, [](void *o, ZuString s, const ZvFieldFmt *) { \
      set(T_, o, member, s); \
    }, { \
      .string = [](void *o, ZuString v) { set(T_, o, member, v); } \
    } }
#define ZvFieldStream_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::Stream, flags, \
    ZvFieldCmp_(T_, member, get), \
    { .exponent = 0 }, \
    [](const void *o, ZmStream &s, const ZvFieldFmt *) { \
      s << get(T_, o, member); \
    }, { \
      .stream = [](const void *o) -> ZmStreamFn { \
	using V = typename ZuDecay<decltype(get(T_, o, member))>::T; \
	const V &v = get(T_, o, member); \
	return {&v, [](const V *ptr, ZmStream &s) { s << *ptr; }}; \
      } \
    }, [](void *o, ZuString s, const ZvFieldFmt *) { \
      set(T_, o, member, s); \
    }, { \
      .string = [](void *o, ZuString v) { set(T_, o, member, v); } \
    } }
#define ZvFieldBool_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::Bool, flags, \
    ZvFieldCmp_(T_, member, get), \
    { .exponent = 0 }, \
    [](const void *o, ZmStream &s, const ZvFieldFmt *) { \
      s << (get(T_, o, member) ? '1' : '0'); \
    }, { \
      .int_ = [](const void *o) -> int64_t { return get(T_, o, member); } \
    }, [](void *o, ZuString s, const ZvFieldFmt *) { \
      set(T_, o, member, (s && s.length() == 1 && s[0] == '1')); \
    }, { \
      .int_ = [](void *o, int64_t v) { set(T_, o, member, v); } \
    } }
#define ZvFieldInt_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::Int, flags, \
    ZvFieldCmp_(T_, member, get), \
    { .exponent = 0 }, \
    [](const void *o, ZmStream &s, const ZvFieldFmt *fmt) { \
      s << ZuBoxed(get(T_, o, member)).vfmt(fmt->scalar); \
    }, { \
      .int_ = [](const void *o) -> int64_t { return get(T_, o, member); } \
    }, [](void *o, ZuString s, const ZvFieldFmt *) { \
      using V = typename ZuDecay<decltype(get(T_, o, member))>::T; \
      set(T_, o, member, (typename ZuBoxT<V>::T{s})); \
    }, { \
      .int_ = [](void *o, int64_t v) { set(T_, o, member, v); } \
    } }
#define ZvFieldFloat_(T_, id, flags, exponent_, member, get, set) \
  { #id, ZvFieldType::Float, flags, \
    ZvFieldCmp_(T_, member, get), \
    { .exponent = exponent_ }, \
    [](const void *o, ZmStream &s, const ZvFieldFmt *fmt) { \
      s << ZuBoxed(get(T_, o, member)).vfmt(fmt->scalar); \
    }, { \
      .float_ = [](const void *o) -> double { return get(T_, o, member); } \
    }, [](void *o, ZuString s, const ZvFieldFmt *) { \
      using V = typename ZuDecay<decltype(get(T_, o, member))>::T; \
      set(T_, o, member, (typename ZuBoxT<V>::T{s})); \
    }, { \
      .float_ = [](void *o, double v) { set(T_, o, member, v); } \
    } }
#define ZvFieldFixed_(T_, id, flags, exponent_, member, get, set) \
  { #id, ZvFieldType::Fixed, flags, \
    ZvFieldCmp_(T_, member, get), \
    { .exponent = exponent_ }, \
    [](const void *o, ZmStream &s, const ZvFieldFmt *fmt) { \
      s << ZuFixed{get(T_, o, member), exponent_}.vfmt(fmt->scalar); \
    }, { \
      .int_ = [](const void *o) -> int64_t { return get(T_, o, member); } \
    }, [](void *o, ZuString s, const ZvFieldFmt *) { \
      set(T_, o, member, (ZuFixed{s, exponent_}.value)); \
    }, { \
      .int_ = [](void *o, int64_t v) { set(T_, o, member, v); } \
    } }
#define ZvFieldDecimal_(T_, id, flags, exponent_, member, get, set) \
  { #id, ZvFieldType::Decimal, flags, \
    ZvFieldCmp_(T_, member, get), \
    { .exponent = exponent_ }, \
    [](const void *o, ZmStream &s, const ZvFieldFmt *fmt) { \
      s << get(T_, o, member).vfmt(fmt->scalar); \
    }, { \
      .decimal = [](const void *o) -> ZuDecimal { return get(T_, o, member); } \
    }, [](void *o, ZuString s, const ZvFieldFmt *) { \
      set(T_, o, member, s); \
    }, { \
      .decimal = [](void *o, ZuDecimal v) { set(T_, o, member, v); } \
    } }
#define ZvFieldHex_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::Hex, flags, \
    ZvFieldCmp_(T_, member, get), \
    { .exponent = 0 }, \
    [](const void *o, ZmStream &s, const ZvFieldFmt *) { \
      s << ZuBoxed(get(T_, o, member)).hex(); \
    }, { \
      .int_ = [](const void *o) -> int64_t { return get(T_, o, member); } \
    }, [](void *o, ZuString s, const ZvFieldFmt *) { \
      using V = typename ZuDecay<decltype(get(T_, o, member))>::T; \
      set(T_, o, member, (typename ZuBoxT<V>::T{ZuFmt::Hex<>{}, s})); \
    }, { \
      .int_ = [](void *o, int64_t v) { set(T_, o, member, v); } \
    } }
#define ZvFieldEnum_(T_, id, flags, map, member, get, set) \
  { #id, ZvFieldType::Enum, flags, \
    ZvFieldCmp_(T_, member, get), { \
      .enum_ = []() -> ZvField_Enum_ * { \
	return ZvField_Enum<map>::instance(); \
      } \
    }, [](const void *o, ZmStream &s, const ZvFieldFmt *) { \
      ZvField_Enum<map>::instance()->print(s, get(T_, o, member)); \
    }, { \
      .int_ = [](const void *o) -> int64_t { return get(T_, o, member); } \
    }, [](void *o, ZuString s, const ZvFieldFmt *) { \
      set(T_, o, member, ZvField_Enum<map>::instance()->scan(s)); \
    }, { \
      .int_ = [](void *o, int64_t v) { set(T_, o, member, v); } \
    } }
#define ZvFieldFlags_(T_, id, flags_, map, member, get, set) \
  { #id, ZvFieldType::Flags, flags_, \
    ZvFieldCmp_(T_, member, get), { \
      .flags = []() -> ZvField_Flags_ * { \
	return ZvField_Flags<map>::instance(); \
      } \
    }, [](const void *o, ZmStream &s, const ZvFieldFmt *fmt) { \
      ZvField_Flags<map>::instance()->print( \
	  s, get(T_, o, member), fmt->flagsDelim); \
    }, { \
      .int_ = [](const void *o) -> int64_t { return get(T_, o, member); } \
    }, [](void *o, ZuString s, const ZvFieldFmt *fmt) { \
      set(T_, o, member, \
	  ZvField_Flags<map>::instance()->scan(s, fmt->flagsDelim)); \
    }, { \
      .int_ = [](void *o, int64_t v) { set(T_, o, member, v); } \
    } }
#define ZvFieldTime_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::Time, flags, \
    ZvFieldCmp_(T_, member, get), \
    { .exponent = 0 }, \
    [](const void *o, ZmStream &s, const ZvFieldFmt *fmt) { \
      switch (fmt->time.type()) { \
	default: \
	case ZvTimeFmt::Index<ZtDateFmt::CSV>::I: \
	  s << get(T_, o, member).csv(fmt->time.csv()); \
	  break; \
	case ZvTimeFmt::Index<ZvTimeFmt_FIX>::I: \
	  s << get(T_, o, member).fix(fmt->time.fix()); \
	  break; \
	case ZvTimeFmt::Index<ZtDateFmt::ISO>::I: \
	  s << get(T_, o, member).iso(fmt->time.iso()); \
	  break; \
      } \
    }, { \
      .time = [](const void *o) -> ZmTime { return get(T_, o, member); } \
    }, [](void *o, ZuString s, const ZvFieldFmt *fmt) { \
      switch (fmt->time.type()) { \
	default: \
	case ZvTimeFmt::Index<ZtDateFmt::CSV>::I: \
	  set(T_, o, member, (ZtDate{ZtDate::CSV, s})); \
	  break; \
	case ZvTimeFmt::Index<ZvTimeFmt_FIX>::I: \
	  set(T_, o, member, (ZtDate{ZtDate::FIX, s})); \
	  break; \
	case ZvTimeFmt::Index<ZtDateFmt::ISO>::I: \
	  set(T_, o, member, (ZtDate{s})); \
	  break; \
      } \
    }, { \
      .time = [](void *o, ZmTime v) { set(T_, o, member, v); } \
    } }

// data member get/set
#define ZvField_Get(T_, o, member) (static_cast<const T_ *>(o)->member)
#define ZvField_Set(T_, o, member, v) (static_cast<T_ *>(o)->member = (v))

// data member fields
#define ZvFieldString(T, id, flags) \
	ZvFieldString_(T, id, flags, id, ZvField_Get, ZvField_Set)
#define ZvFieldStream(T, id, flags) \
	ZvFieldStream_(T, id, flags, id, ZvField_Get, ZvField_Set)
#define ZvFieldBool(T, id, flags) \
	ZvFieldBool_(T, id, flags, id, ZvField_Get, ZvField_Set)
#define ZvFieldInt(T, id, flags) \
	ZvFieldInt_(T, id, flags, id, ZvField_Get, ZvField_Set)
#define ZvFieldFloat(T, id, flags, exponent) \
	ZvFieldFloat_(T, id, flags, exponent, id, ZvField_Get, ZvField_Set)
#define ZvFieldFixed(T, id, flags, exponent) \
	ZvFieldFixed_(T, id, flags, exponent, id, ZvField_Get, ZvField_Set)
#define ZvFieldDecimal(T, id, flags, exponent) \
	ZvFieldDecimal_(T, id, flags, exponent, id, ZvField_Get, ZvField_Set)
#define ZvFieldHex(T, id, flags) \
	ZvFieldHex_(T, id, flags, id, ZvField_Get, ZvField_Set)
#define ZvFieldEnum(T, id, flags, map) \
	ZvFieldEnum_(T, id, flags, map, id, ZvField_Get, ZvField_Set)
#define ZvFieldFlags(T, id, flags, map) \
	ZvFieldFlags_(T, id, flags, map, id, ZvField_Get, ZvField_Set)
#define ZvFieldTime(T, id, flags) \
	ZvFieldTime_(T, id, flags, id, ZvField_Get, ZvField_Set)
// alias fields (id different than name of data member)
#define ZvFieldStringAlias(T, id, alias, flags) \
	ZvFieldString_(T, id, flags, alias, ZvField_Get, ZvField_Set)
#define ZvFieldStreamAlias(T, id, alias, flags) \
	ZvFieldStream_(T, id, flags, alias, ZvField_Get, ZvField_Set)
#define ZvFieldBoolAlias(T, id, alias, flags) \
	ZvFieldBool_(T, id, flags, alias, ZvField_Get, ZvField_Set)
#define ZvFieldIntAlias(T, id, alias, flags) \
	ZvFieldInt_(T, id, flags, alias, ZvField_Get, ZvField_Set)
#define ZvFieldFloatAlias(T, id, alias, flags, exponent) \
	ZvFieldFloat_(T, id, flags, exponent, alias, ZvField_Get, ZvField_Set)
#define ZvFieldFixedAlias(T, id, alias, flags, exponent) \
	ZvFieldFixed_(T, id, flags, exponent, alias, ZvField_Get, ZvField_Set)
#define ZvFieldDecimalAlias(T, id, alias, flags, exponent) \
	ZvFieldDecimal_(T, id, flags, exponent, alias, ZvField_Get, ZvField_Set)
#define ZvFieldHexAlias(T, id, alias, flags) \
	ZvFieldHex_(T, id, flags, alias, ZvField_Get, ZvField_Set)
#define ZvFieldEnumAlias(T, id, alias, flags, map) \
	ZvFieldEnum_(T, id, flags, map, alias, ZvField_Get, ZvField_Set)
#define ZvFieldFlagsAlias(T, id, alias, flags, map) \
	ZvFieldFlags_(T, id, flags, map, alias, ZvField_Get, ZvField_Set)
#define ZvFieldTimeAlias(T, id, alias, flags) \
	ZvFieldTime_(T, id, flags, alias, ZvField_Get, ZvField_Set)

// function member get/set
#define ZvField_GetFn(T_, o, member) (static_cast<const T_ *>(o)->member())
#define ZvField_SetFn(T_, o, member, v) (static_cast<T_ *>(o)->member(v))

// function member fields
#define ZvFieldStringFn(T, id, flags) \
	ZvFieldString_(T, id, flags, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldStreamFn(T, id, flags) \
	ZvFieldStream_(T, id, flags, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldBoolFn(T, id, flags) \
	ZvFieldBool_(T, id, flags, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldIntFn(T, id, flags) \
	ZvFieldInt_(T, id, flags, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFloatFn(T, id, flags, exponent) \
	ZvFieldFloat_(T, id, flags, exponent, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFixedFn(T, id, flags, exponent) \
	ZvFieldFixed_(T, id, flags, exponent, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldDecimalFn(T, id, flags, exponent) \
	ZvFieldDecimal_(T, id, flags, exponent, id, \
	    ZvField_GetFn, ZvField_SetFn)
#define ZvFieldHexFn(T, id, flags) \
	ZvFieldHex_(T, id, flags, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldEnumFn(T, id, flags, map) \
	ZvFieldEnum_(T, id, flags, map, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFlagsFn(T, id, flags, map) \
	ZvFieldFlags_(T, id, flags, map, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldTimeFn(T, id, flags) \
	ZvFieldTime_(T, id, flags, id, ZvField_GetFn, ZvField_SetFn)

// alias function fields (id different than function name, e.g. a nested field)
#define ZvFieldStringAliasFn(T, id, fn, flags) \
	ZvFieldString_(T, id, flags, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldStreamAliasFn(T, id, fn, flags) \
	ZvFieldStream_(T, id, flags, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldBoolAliasFn(T, id, fn, flags) \
	ZvFieldBool_(T, id, flags, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldIntAliasFn(T, id, fn, flags) \
	ZvFieldInt_(T, id, flags, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFloatAliasFn(T, id, fn, flags, exponent) \
	ZvFieldFloat_(T, id, flags, exponent, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFixedAliasFn(T, id, fn, flags, exponent) \
	ZvFieldFixed_(T, id, flags, exponent, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldDecimalAliasFn(T, id, fn, flags, exponent) \
	ZvFieldDecimal_(T, id, flags, exponent, fn, \
	    ZvField_GetFn, ZvField_SetFn)
#define ZvFieldHexAliasFn(T, id, fn, flags) \
	ZvFieldHex_(T, id, flags, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldEnumAliasFn(T, id, fn, flags, map) \
	ZvFieldEnum_(T, id, flags, map, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFlagsAliasFn(T, id, fn, flags, map) \
	ZvFieldFlags_(T, id, flags, map, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldTimeAliasFn(T, id, fn, flags) \
	ZvFieldTime_(T, id, flags, fn, ZvField_GetFn, ZvField_SetFn)

using ZvFieldArray = ZuArray<const ZvField>;

#define ZvField_(T, args) \
  ZuPP_Defer(ZvField__)()(T, ZuPP_Strip(args))
// FIXME - direct to ZvField___ ?
#define ZvField__() ZvField___
#define ZvField___(T, type, ...) ZvField##type(T, __VA_ARGS__)
#define ZvFields(T, ...)  \
  using namespace ZvFieldFlags; \
  static ZvField fields_[] = { \
    ZuPP_Eval(ZuPP_MapArgComma(ZvField_, T, __VA_ARGS__)) \
  }; \
  return ZvFieldArray{&fields_[0], sizeof(fields_) / sizeof(fields_[0])}

template <typename Impl> struct ZvFieldTuple : public ZuPrintable {
  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

  template <typename S> void print(S &s_) const {
    ZmStream s{s_};
    auto fields = Impl::fields();
    thread_local ZvFieldFmt fmt;
    for (unsigned i = 0, n = fields.length(); i < n; i++) {
      const auto &field = fields[i];
      if (field.flags & ZvFieldFlags::Synthetic) break;
      if (i) s << ' ';
      s << fields[i].id << '=';
      fields[i].print(impl(), s, &fmt);
    }
  }
};

#endif /* ZvField_HPP */
