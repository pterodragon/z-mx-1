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

#include <zlib/ZtEnum.hpp>
#include <zlib/ZtDate.hpp>

namespace ZvFieldType {
  enum _ {
    String = 0,
    Bool,
    Int,
    Float,
    Fixed,
    Hex,
    Enum,
    Flags,
    Time
  };
}

namespace ZvFieldFlags {
  enum _ {
    Synthetic	= 0x01,		// synthetic
    Series	= 0x04,		// data series
      Index	= 0x08,		// - index (e.g. time stamp)
      Delta	= 0x10,		// - first derivative
      Delta2	= 0x20		// - second derivative
  };
}

struct ZvTimeNull : public ZuPrintable {
  template <typename S> void print(S &) const { }
};
ZuDeclUnion(ZvTimeFmt,
    (ZtDateFmt::CSV, csv),
    ((ZtDateFmt::FIX<-9, ZvTimeNull>), fix),
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

struct ZvField {
  const char	*id;
  uint32_t	type;		// ZvFieldType
  uint32_t	flags;		// ZvFieldFlags
  int		(*cmp)(const void *, const void *);
  void		(*print)(ZmStream &, const void *, const ZvFieldFmt &);
  void		(*scan)(ZuString, void *, const ZvFieldFmt &);
  ZuFixed	(*scalar)(const void *) = nullptr;
};

#define ZvFieldCmp_(T_, member, get) \
    [](const void *o1, const void *o2) { \
      return ZuCmp<decltype(get(static_cast<const T_ *>(o1), member))>::cmp( \
	  get(o1, member), get(static_cast<const T_ *>(o2), member)); }

#define ZvFieldString_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::String, flags, \
    ZvFieldCmp_(T_, member, get), \
    [](ZmStream &s, const void *o, const ZvFieldFmt &) { \
      s << get(static_cast<const T_ *>(o), member); }, \
    [](ZuString s, void *o, const ZvFieldFmt &) { \
      set(static_cast<T_ *>(o), member, s); } }
#define ZvFieldBool_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::Bool, flags, \
    ZvFieldCmp_(T_, member, get), \
    [](ZmStream &s, const void *o, const ZvFieldFmt &) { \
      s << (get(static_cast<const T_ *>(o), member) ? '1' : '0'); }, \
    [](ZuString s, void *o, const ZvFieldFmt &) { \
      set(static_cast<T_ *>(o), member, \
	  (s && s.length() == 1 && s[0] == '1')); } }
#define ZvFieldInt_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::Int, flags, \
    ZvFieldCmp_(T_, member, get), \
    [](ZmStream &s, const T_ *o, const ZvFieldFmt &fmt) { \
      s << ZuBoxed(get( \
	    static_cast<const T_ *>(o), member)).vfmt(fmt.scalar); }, \
    [](ZuString s, T_ *o, const ZvFieldFmt &) { \
      using V = typename ZuDecay<decltype( \
	  get(static_cast<const T_ *>(o), member))>::T; \
      set(static_cast<T_ *>(o), member, (typename ZuBoxT<V>::T{s})); }, \
    [](const T_ *o) { \
      return ZuFixed{get(static_cast<const T_ *>(o), member), 0}; } }
#define ZvFieldFloat_(T_, id, flags, exponent, member, get, set) \
  { #id, ZvFieldType::Float, flags, \
    ZvFieldCmp_(T_, member, get), \
    [](ZmStream &s, const T_ *o, const ZvFieldFmt &fmt) { \
      s << ZuBoxed(get( \
	    static_cast<const T_ *>(o), member)).vfmt(fmt.scalar); }, \
    [](ZuString s, T_ *o, const ZvFieldFmt &) { \
      using V = typename ZuDecay<decltype( \
	  get(static_cast<const T_ *>(o), member))>::T; \
      set(static_cast<T_ *>(o), member, (typename ZuBoxT<V>::T{s})); }, \
    [](const T_ *o) { \
      return ZuFixed{get(static_cast<const T_ *>(o), member), exponent}; } }
#define ZvFieldFixed_(T_, id, flags, exponent, member, get, set) \
  { #id, ZvFieldType::Fixed, flags, \
    ZvFieldCmp_(T_, member, get), \
    [](ZmStream &s, const T_ *o, const ZvFieldFmt &fmt) { \
      s << ZuFixed{get(static_cast<const T_ *>(o), member), exponent}. \
	vfmt(fmt.scalar); }, \
    [](ZuString s, T_ *o, const ZvFieldFmt &) { \
      set(static_cast<T_ *>(o), member, ZuFixed{s, exponent}.value); }, \
    [](const T_ *o) { \
      return ZuFixed{get(static_cast<const T_ *>(o), member), exponent}; } }
#define ZvFieldHex_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::Hex, flags, \
    ZvFieldCmp_(T_, member, get), \
    [](ZmStream &s, const T_ *o, const ZvFieldFmt &fmt) { \
      s << ZuBoxed(get(static_cast<const T_ *>(o), member)).hex(); }, \
    [](ZuString s, T_ *o, const ZvFieldFmt &) { \
      using V = typename ZuDecay<decltype( \
	  get(static_cast<const T_ *>(o), member))>::T; \
      set(static_cast<T_ *>(o), member, \
	  (typename ZuBoxT<V>::T{ZuFmt::Hex<>{}, s})); } }
#define ZvFieldEnum_(T_, id, flags, map, member, get, set) \
  { #id, ZvFieldType::Enum, flags, \
    ZvFieldCmp_(T_, member, get), \
    [](ZmStream &s, const void *o, const ZvFieldFmt &) { \
      s << map::instance()->v2s(get(static_cast<const T_ *>(o), member)); }, \
    [](ZuString s, void *o, const ZvFieldFmt &) { \
      set(static_cast<T_ *>(o), member, (map::instance()->s2v(s))); } }
#define ZvFieldFlags_(T_, id, flags, map, member, get, set) \
  { #id, ZvFieldType::Flags, flags, \
    ZvFieldCmp_(T_, member, get), \
    [](ZmStream &s, const void *o, const ZvFieldFmt &fmt) { \
      map::instance()->print(s, get( \
	    static_cast<const T_ *>(o), member), fmt.flagsDelim); }, \
    [](ZuString s, void *o, const ZvFieldFmt &fmt) { \
      using V = typename ZuDecay<decltype( \
	  get(static_cast<const T_ *>(o), member))>::T; \
      set(static_cast<T_ *>(o), member, \
	  (map::instance()->scan<V>(s, fmt.flagsDelim))); } }
#define ZvFieldTime_(T_, id, flags, member, get, set) \
  { #id, ZvFieldType::Time, flags, \
    ZvFieldCmp_(T_, member, get), \
    [](ZmStream &s, const void *o, const ZvFieldFmt &fmt) { \
      switch (fmt.time.type()) { \
	default: \
	case 1: \
	  s << get(static_cast<const T_ *>(o), member).csv(fmt.time.csv()); \
	  break; \
	case 2: \
	  s << get(static_cast<const T_ *>(o), member).fix(fmt.time.fix()); \
	  break; \
	case 3: \
	  s << get(static_cast<const T_ *>(o), member).iso(fmt.time.iso()); \
	  break; \
      } \
    }, \
    [](ZuString s, void *o, const ZvFieldFmt &fmt) { \
      switch (fmt.time.type()) { \
	default: \
	case 1: \
	  set(static_cast<T_ *>(o), member, (ZtDate{ZtDate::CSV, s})); \
	  break; \
	case 2: \
	  set(static_cast<T_ *>(o), member, (ZtDate{ZtDate::FIX, s})); \
	  break; \
	case 3: \
	  set(static_cast<T_ *>(o), member, (ZtDate{s})); \
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
#define ZvFieldInt(T, id, flags, exponent) \
	ZvFieldInt_(T, id, flags, exponent, id, ZvField_Get, ZvField_Set)
#define ZvFieldFloat(T, id, flags, exponent) \
	ZvFieldFloat_(T, id, flags, exponent, id, ZvField_Get, ZvField_Set)
#define ZvFieldFixed(T, id, flags, exponent) \
	ZvFieldFixed_(T, id, flags, exponent, id, ZvField_Get, ZvField_Set)
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
#define ZvFieldIntAlias(T, id, alias, flags, exponent) \
	ZvFieldInt_(T, id, flags, exponent, alias, ZvField_Get, ZvField_Set)
#define ZvFieldFloatAlias(T, id, alias, flags, exponent) \
	ZvFieldFloat_(T, id, flags, exponent, alias, ZvField_Get, ZvField_Set)
#define ZvFieldFixedAlias(T, id, alias, flags, exponent) \
	ZvFieldFixed_(T, id, flags, exponent, alias, ZvField_Get, ZvField_Set)
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
#define ZvFieldIntFn(T, id, flags, exponent) \
	ZvFieldInt_(T, id, flags, exponent, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFloatFn(T, id, flags, exponent) \
	ZvFieldFloat_(T, id, flags, exponent, id, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFixedFn(T, id, flags, exponent) \
	ZvFieldFixed_(T, id, flags, exponent, id, ZvField_GetFn, ZvField_SetFn)
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
#define ZvFieldIntAliasFn(T, id, fn, flags, exponent) \
	ZvFieldInt_(T, id, flags, exponent, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFloatAliasFn(T, id, fn, flags, exponent) \
	ZvFieldFloat_(T, id, flags, exponent, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFixedAliasFn(T, id, fn, flags, exponent) \
	ZvFieldFixed_(T, id, flags, exponent, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldEnumAliasFn(T, id, fn, flags, map) \
	ZvFieldEnum_(T, id, flags, map, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldFlagsAliasFn(T, id, fn, flags, map) \
	ZvFieldFlags_(T, id, flags, map, fn, ZvField_GetFn, ZvField_SetFn)
#define ZvFieldTimeAliasFn(T, id, fn, flags) \
	ZvFieldTime_(T, id, flags, fn, ZvField_GetFn, ZvField_SetFn)

#define ZvMkField(T, args) \
  ZuPP_Defer(ZvMkField_)()(T, ZuPP_Strip(args))
#define ZvMkField_() ZvMkField__
#define ZvMkField__(T, type, ...) ZvField##type(T, __VA_ARGS__)
#define ZvMkFields(T, ...)  \
  using namespace ZvFieldFlags; \
  static ZvField fields_[] = { \
    ZuPP_Eval(ZuPP_MapArgComma(ZvMkField, T, __VA_ARGS__)) \
  }; \
  return ZvFields{&fields_[0], sizeof(fields_) / sizeof(fields_[0])} \

using ZvFields = ZuArray<ZvField>;

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
      fields[i].print(s, impl(), fmt);
    }
  }
};

#endif /* ZvField_HPP */
