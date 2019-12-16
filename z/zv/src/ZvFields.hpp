//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

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

#ifndef ZvFields_HPP
#define ZvFields_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZuPrint.hpp>
#include <zlib/ZuBox.hpp>
#include <zlib/ZuString.hpp>

#include <zlib/ZtEnum.hpp>
#include <zlib/ZtDate.hpp>

namespace ZvFieldType {
  enum _ {
    String = 0,
    Bool,
    Scalar,
    Enum,
    Flags,
    Time
  };
}

namespace ZvFieldFlags {
  enum _ {
    Primary	= 0x01,		// primary key
    Secondary	= 0x02,		// secondary key
    TimeSeries	= 0x04,		// scalar that varies over time
    Synthetic	= 0x08,		// synthetic scalar
    Cumulative	= 0x10,		// first derivative should be logged/graphed
    ColorRAG	= 0x20		// red/amber/green
  };
}

ZuUnionFields(ZvTimeFmt_, csv, fix, iso);
struct ZvTimeNull : public ZuPrintable {
  template <typename S> inline void print(S &) const { }
};
using ZvTimeFmt =
  ZvTimeFmt_<ZtDateFmt::CSV, ZtDateFmt::FIX<-9, ZvTimeNull>, ZtDateFmt::ISO>;

struct ZvFieldFmt {
  ZvFieldFmt() { new (time.new_csv()) ZtDateFmt::CSV{}; }
  ZvFieldFmt(const ZvFieldFmt &) = default;
  ZvFieldFmt &operator =(const ZvFieldFmt &) = default;
  ZvFieldFmt(ZvFieldFmt &&) = default;
  ZvFieldFmt &operator =(ZvFieldFmt &&) = default;
  ~ZvFieldFmt() = default;

  ZuVFmt	scalar;			// scalar format (print only)
  ZvTimeFmt	time;			// date/time format
  char		flagsDelim = '|';	// flags delimiter
};

template <typename T> struct ZvField {
  const char	*id;
  uint32_t	type;		// ZvFieldType
  uint32_t	flags;		// ZvFieldFlags
  void		(*print)(ZmStream &, const T *, const ZvFieldFmt &);
  void		(*scan)(ZuString, T *, const ZvFieldFmt &);
  double	(*scalar)(const T *) = nullptr;
};

#define ZvFieldString_(T, id, flags, member, get, set) \
  { #id, ZvFieldType::String, flags, \
    [](ZmStream &s, const T *o, const ZvFieldFmt &) { s << get(o, member); }, \
    [](ZuString s, T *o, const ZvFieldFmt &) { set(o, member, s); } }
#define ZvFieldBool_(T, id, flags, member, get, set) \
  { #id, ZvFieldType::Bool, flags, \
    [](ZmStream &s, const T *o, const ZvFieldFmt &) { \
      s << (get(o, member) ? '1' : '0'); }, \
    [](ZuString s, T *o, const ZvFieldFmt &) { \
      set(o, member, (s && s.length() == 1 && s[0] == '1')); } }
#define ZvFieldScalar_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::Scalar, flags, \
    [](ZmStream &s, const T_ *o, const ZvFieldFmt &fmt) { \
      s << ZuBoxed(get(o, member)).vfmt(fmt.scalar); }, \
    [](ZuString s, T_ *o, const ZvFieldFmt &) { \
      using V = typename ZuTraits<decltype(get(o, member))>::T; \
      set(o, member, (typename ZuBoxT<V>::T{s})); }, \
    [](const T_ *o) { return (double)get(o, member); } }
#define ZvFieldEnum_(T, id, flags, map, member, get, set) \
  { #id, ZvFieldType::Enum, flags, \
    [](ZmStream &s, const T *o, const ZvFieldFmt &) { \
      s << map::instance()->v2s(get(o, member)); }, \
    [](ZuString s, T *o, const ZvFieldFmt &) { \
      set(o, member, (map::instance()->s2v(s))); } }
#define ZvFieldFlags_(T_, id, flags, map, member, get, set) \
  { #id, ZvFieldType::Flags, flags, \
    [](ZmStream &s, const T_ *o, const ZvFieldFmt &fmt) { \
      map::instance()->print(s, get(o, member), fmt.flagsDelim); }, \
    [](ZuString s, T_ *o, const ZvFieldFmt &fmt) { \
      using V = typename ZuTraits<decltype(get(o, member))>::T; \
      set(o, member, (map::instance()->scan<V>(s, fmt.flagsDelim))); } }
#define ZvFieldTime_(T, id, flags, member, get, set) \
  { #id, ZvFieldType::Time, flags, \
    [](ZmStream &s, const T *o, const ZvFieldFmt &fmt) { \
      switch (fmt.time.type()) { \
	default: \
	case 1: \
	  s << get(o, member).csv(fmt.time.csv()); \
	  break; \
	case 2: \
	  s << get(o, member).fix(fmt.time.fix()); \
	  break; \
	case 3: \
	  s << get(o, member).iso(fmt.time.iso()); \
	  break; \
      } \
    }, \
    [](ZuString s, T *o, const ZvFieldFmt &fmt) { \
      switch (fmt.time.type()) { \
	default: \
	case 1: \
	  set(o, member, (ZtDate{ZtDate::CSV, s})); \
	  break; \
	case 2: \
	  set(o, member, (ZtDate{ZtDate::FIX, s})); \
	  break; \
	case 3: \
	  set(o, member, (ZtDate{s})); \
	  break; \
      } \
    } }

// data member get/set
#define ZvField_Get(o, member) (o->member)
#define ZvField_Set(o, member, v) (o->member = (v))

// data member fields
#define ZvFieldString(T, id, flags) \
	ZvFieldString_(T, id, flags, id, ZvField_Get, ZvField_Set)
#define ZvFieldBool(T, id, flags) \
	ZvFieldBool_(T, id, flags, id, ZvField_Get, ZvField_Set)
#define ZvFieldScalar(T, id, flags) \
	ZvFieldScalar_(T, id, flags, id, ZvField_Get, ZvField_Set)
#define ZvFieldEnum(T, id, flags, map) \
	ZvFieldEnum_(T, id, flags, map, id, ZvField_Get, ZvField_Set)
#define ZvFieldFlags(T, id, flags, map) \
	ZvFieldFlags_(T, id, flags, map, id, ZvField_Get, ZvField_Set)
#define ZvFieldTime(T, id, flags) \
	ZvFieldTime_(T, id, flags, id, ZvField_Get, ZvField_Set)
// alias fields (id different than name of data member)
#define ZvFieldStringAlias(T, id, alias, flags) \
	ZvFieldString_(T, id, flags, alias, ZvField_Get, ZvField_Set)
#define ZvFieldBoolAlias(T, id, alias, flags) \
	ZvFieldBool_(T, id, flags, alias, ZvField_Get, ZvField_Set)
#define ZvFieldScalarAlias(T, id, alias, flags) \
	ZvFieldScalar_(T, id, flags, alias, ZvField_Get, ZvField_Set)
#define ZvFieldEnumAlias(T, id, alias, flags, map) \
	ZvFieldEnum_(T, id, flags, map, alias, ZvField_Get, ZvField_Set)
#define ZvFieldFlagsAlias(T, id, alias, flags, map) \
	ZvFieldFlags_(T, id, flags, map, alias, ZvField_Get, ZvField_Set)
#define ZvFieldTimeAlias(T, id, alias, flags) \
	ZvFieldTime_(T, id, flags, alias, ZvField_Get, ZvField_Set)

// function member get/set
#define ZvField_GetFn(o, member) (o->member())
#define ZvField_SetFn(o, member, v) (o->member(v))

// function member fields
#define ZvFieldStringFn(T, id, flags) \
	ZvFieldString_(T, id, flags, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldBoolFn(T, id, flags) \
	ZvFieldBool_(T, id, flags, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldScalarFn(T, id, flags) \
	ZvFieldScalar_(T, id, flags, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldEnumFn(T, id, flags, map) \
	ZvFieldEnum_(T, id, flags, map, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFlagsFn(T, id, flags, map) \
	ZvFieldFlags_(T, id, flags, map, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldTimeFn(T, id, flags) \
	ZvFieldTime_(T, id, flags, id, ZvField_GetFn, ZvField_SetFn)

// alias function fields (id different than function name, e.g. a nested field)
#define ZvFieldStringAliasFn(T, id, fn, flags) \
	ZvFieldString_(T, id, flags, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldBoolAliasFn(T, id, fn, flags) \
	ZvFieldBool_(T, id, flags, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldScalarAliasFn(T, id, fn, flags) \
	ZvFieldScalar_(T, id, flags, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldEnumAliasFn(T, id, fn, flags, map) \
	ZvFieldEnum_(T, id, flags, map, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFlagsAliasFn(T, id, fn, flags, map) \
	ZvFieldFlags_(T, id, flags, map, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldTimeAliasFn(T, id, fn, flags) \
	ZvFieldTime_(T, id, flags, fn, ZvField_GetFn, ZvField_SetFn)

#define ZvMkField(T, args) \
  ZuPP_Defer(ZvMkField_)()(T, ZuPP_Strip1 ZuPP_Strip2(args))
#define ZvMkField_() ZvMkField__
#define ZvMkField__(T, type, ...) ZvField##type(T, __VA_ARGS__),
#define ZvMkFields(T, ...)  \
  using namespace ZvFieldFlags; \
  return ZvFields<T>{std::initializer_list<ZvField<T>>{ \
    ZuPP_Eval(ZuPP_MapArg(ZvMkField, T, __VA_ARGS__)) \
  } }

template <typename T> using ZvFields = ZuArray<ZvField<T>>;

template <typename Impl> struct ZvFieldTuple : public ZuPrintable {
  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

  template <typename S> void print(S &s_) const {
    ZmStream s{s_};
    auto fields = Impl::fields();
    for (unsigned i = 0, n = fields.length(); i < n; i++) {
      const auto &field = fields[i];
      if (field.flags & ZvFieldFlags::Synthetic) break;
      if (i) s << ' ';
      s << fields[i].id << '=';
      fields[i].print(s);
    }
  }
};

#endif /* ZvFields_HPP */
