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

// flat object introspection - flatbuffers extensions

#ifndef ZvFBField_HPP
#define ZvFBField_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/Zfb.hpp>

#include <zlib/ZvField.hpp>

void *ZvFBB_(...); // default
void *ZvFBS_(...); // default

template <typename T>
using ZvFBB = ZuDecay<decltype(*ZvFBB_(ZuDeclVal<T *>()))>;
template <typename T>
using ZvFBS = ZuDecay<decltype(*ZvFBS_(ZuDeclVal<T *>()))>;

#define ZvFBField_Name_(U, Type, ID, ...) U##_##ID##_FB
#define ZvFBField_Name(U, Args) ZuPP_Defer(ZvFBField_Name_)(U, ZuPP_Strip(Args))

#define ZvFBField_ReadOnly_(U, Type, ID, Base) \
  struct ZvFBField_Name_(U, Type, ID) : public Base { };

#define ZvFBField_Composite_(U, Type, ID, Base_, SaveFn, LoadFn) \
  struct ZvFBField_Name_(U, Type, ID) : public Base_ { \
    using Base = Base_; \
    using T = typename Base::T; \
    using FBB = ZvFBB<U>; \
    using FBS = ZvFBS<U>; \
    enum { Inline = 0 }; \
    static Zfb::Offset<void> save(Zfb::Builder &fbb, const void *o) { \
      using namespace Zfb::Save; \
      return SaveFn(fbb, Base::get(o)).Union(); \
    } \
    static void save(FBB &fbb, Zfb::Offset<void> offset) { \
      fbb.add_##ID(offset.o); \
    } \
    static auto load_(const FBS *o_) { \
      using namespace Zfb::Load; \
      return LoadFn(o_->ID()); \
    } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldXString(U, ID, Member, Flags) \
  ZvFBField_Composite_(U, String, ID, \
      ZvFieldXString(U, ID, Member, Flags), str, str)
#define ZvFBFieldXStringFn(U, ID, Get, Set, Flags) \
  ZvFBField_Composite_(U, String, ID, \
      ZvFieldXStringFn(U, ID, Get, Set, Flags), str, str)

#define ZvFBFieldXRdString(U, ID, Member, Flags) \
  ZvFBField_ReadOnly_(U, String, ID, \
      ZvFieldXRdString(U, ID, Member, Flags))
#define ZvFBFieldXRdStringFn(U, ID, Get, Flags) \
  ZvFBField_ReadOnly_(U, String, ID, \
      ZvFieldXRdStringFn(U, ID, Get, Flags))

#define ZvFBFieldRdString(U, Member, Flags) \
  ZvFBFieldXRdString(U, Member, Member, Flags)
#define ZvFBFieldRdStringFn(U, Fn, Flags) \
  ZvFBFieldXRdStringFn(U, Fn, Fn, Flags)

#define ZvFBFieldString(U, Member, Flags) \
  ZvFBFieldXString(U, Member, Member, Flags)
#define ZvFBFieldStringFn(U, Fn, Flags) \
  ZvFBFieldXStringFn(U, Fn, Fn, Fn, Flags)

#define ZvFBFieldXComposite(U, ID, Member, Flags, SaveFn, LoadFn) \
  ZvFBField_Composite_(U, Composite, ID, \
      ZvFieldXComposite(U, ID, Member, Flags), SaveFn, LoadFn)
#define ZvFBFieldXCompositeFn(U, ID, Get, Set, Flags, SaveFn, LoadFn) \
  ZvFBField_Composite_(U, Composite, ID, \
      ZvFieldXCompositeFn(U, ID, Get, Set, Flags), SaveFn, LoadFn)

#define ZvFBFieldXRdComposite(U, ID, Member, Flags) \
  ZvFBField_ReadOnly_(U, Composite, ID, \
      ZvFieldXRdComposite(U, ID, Member, Flags))
#define ZvFBFieldXRdCompositeFn(U, ID, Get, Flags) \
  ZvFBField_ReadOnly_(U, Composite, ID, \
      ZvFieldXRdCompositeFn(U, ID, Get, Flags))

#define ZvFBFieldRdComposite(U, Member, Flags) \
  ZvFBFieldXRdComposite(U, Member, Member, Flags)
#define ZvFBFieldRdCompositeFn(U, Fn, Flags) \
  ZvFBFieldXRdCompositeFn(U, Fn, Fn, Flags)

#define ZvFBFieldComposite(U, Member, Flags, SaveFn, LoadFn) \
  ZvFBFieldXComposite(U, Member, Member, Flags, SaveFn, LoadFn)
#define ZvFBFieldCompositeFn(U, Fn, Flags, SaveFn, LoadFn) \
  ZvFBFieldXCompositeFn(U, Fn, Fn, Fn, Flags, SaveFn, LoadFn)

#define ZvFBField_Inline_(U, ID, Base_, SaveFn, LoadFn) \
  struct ZvFBField_Name_(U, Inline, ID) : public Base_ { \
    using Base = Base_; \
    using FBB = ZvFBB<U>; \
    using FBS = ZvFBS<U>; \
    enum { Inline = 1 }; \
    static void save(FBB &fbb, const void *o) { \
      using namespace Zfb::Save; \
      auto v = SaveFn(Base::get(o)); \
      fbb.add_##ID(&v); \
    } \
    static auto load_(const FBS *o_) { \
      using namespace Zfb::Load; \
      auto v = o_->ID(); \
      return LoadFn(v); \
    } \
    static void load(void *o, const FBS *o_) { \
      Base::set(o, load_(o_)); \
    } \
  };

#define ZvFBFieldXInline(U, ID, Member, Flags, SaveFn, LoadFn) \
  ZvFBField_Inline_(U, ID, \
      ZvFieldXComposite(U, ID, Member, Flags), SaveFn, LoadFn)
#define ZvFBFieldXInlineFn(U, ID, Get, Set, Flags, SaveFn, LoadFn) \
  ZvFBField_Inline_(U, ID, \
      ZvFieldXCompositeFn(U, ID, Get, Set, Flags), SaveFn, LoadFn)

#define ZvFBFieldInline(U, Member, Flags, SaveFn, LoadFn) \
  ZvFBFieldXInline(U, Member, Member, Flags, SaveFn, LoadFn)
#define ZvFBFieldInlineFn(U, Fn, Flags, SaveFn, LoadFn) \
  ZvFBFieldXInlineFn(U, Fn, Fn, Fn, Flags, SaveFn, LoadFn)

#define ZvFBField_Int_(U, Type, ID, Base_) \
  struct ZvFBField_Name_(U, Type, ID) : public Base_ { \
    using Base = Base_; \
    using T = typename Base::T; \
    using FBB = ZvFBB<U>; \
    using FBS = ZvFBS<U>; \
    enum { Inline = 1 }; \
    static void save(FBB &fbb, const void *o) { fbb.add_##ID(Base::get(o)); } \
    static auto load_(const FBS *o_) { return static_cast<T>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldXBool(U, ID, Member, Flags) \
  ZvFBField_Int_(U, Bool, ID, ZvFieldXBool(U, ID, Member, Flags))
#define ZvFBFieldXBoolFn(U, ID, Get, Set, Flags) \
  ZvFBField_Int_(U, Bool, ID, ZvFieldXBoolFn(U, ID, Get, Set, Flags))

#define ZvFBFieldXRdBool(U, ID, Member, Flags) \
  ZvFBField_ReadOnly_(U, Bool, ID, \
      ZvFieldXRdBool(U, ID, Member, Flags))
#define ZvFBFieldXRdBoolFn(U, ID, Get, Flags) \
  ZvFBField_ReadOnly_(U, Bool, ID, \
      ZvFieldXRdBoolFn(U, ID, Get, Flags))

#define ZvFBFieldBool(U, Member, Flags) \
  ZvFBFieldXBool(U, Member, Member, Flags)
#define ZvFBFieldBoolFn(U, Fn, Flags) \
  ZvFBFieldXBoolFn(U, Fn, Fn, Fn, Flags)

#define ZvFBFieldRdBool(U, Member, Flags) \
  ZvFBFieldXRdBool(U, Member, Member, Flags)
#define ZvFBFieldRdBoolFn(U, Fn, Flags) \
  ZvFBFieldXRdBoolFn(U, Fn, Fn, Flags)

#define ZvFBFieldXInt(U, ID, Member, Flags) \
  ZvFBField_Int_(U, Int, ID, ZvFieldXInt(U, ID, Member, Flags))
#define ZvFBFieldXIntFn(U, ID, Get, Set, Flags) \
  ZvFBField_Int_(U, Int, ID, ZvFieldXIntFn(U, ID, Get, Set, Flags))

#define ZvFBFieldXRdInt(U, ID, Member, Flags) \
  ZvFBField_ReadOnly_(U, Int, ID, \
      ZvFieldXRdInt(U, ID, Member, Flags))
#define ZvFBFieldXRdIntFn(U, ID, Get, Flags) \
  ZvFBField_ReadOnly_(U, Int, ID, \
      ZvFieldXRdIntFn(U, ID, Get, Flags))

#define ZvFBFieldInt(U, Member, Flags) \
  ZvFBFieldXInt(U, Member, Member, Flags)
#define ZvFBFieldIntFn(U, Fn, Flags) \
  ZvFBFieldXIntFn(U, Fn, Fn, Fn, Flags)

#define ZvFBFieldRdInt(U, Member, Flags) \
  ZvFBFieldXRdInt(U, Member, Member, Flags)
#define ZvFBFieldRdIntFn(U, Fn, Flags) \
  ZvFBFieldXRdIntFn(U, Fn, Fn, Flags)

#define ZvFBField_Fixed_(U, Type, ID, Base_) \
  struct ZvFBField_Name_(U, Type, ID) : public Base_ { \
    using Base = Base_; \
    using T = typename Base::T; \
    using FBB = ZvFBB<U>; \
    using FBS = ZvFBS<U>; \
    enum { Inline = 1 }; \
    static void save(FBB &fbb, const void *o) { fbb.add_##ID(Base::get(o)); } \
    static auto load_(const FBS *o_) { return static_cast<T>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldXFloat(U, ID, Member, Flags, Exponent) \
  ZvFBField_Fixed_(U, Float, ID, \
      ZvFieldXFloat(U, ID, Member, Flags, Exponent))
#define ZvFBFieldXFloatFn(U, ID, Get, Set, Flags, Exponent) \
  ZvFBField_Fixed_(U, Float, ID, \
      ZvFieldXFloatFn(U, ID, Get, Set, Flags, Exponent))

#define ZvFBFieldXRdFloat(U, ID, Member, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Float, ID, \
      ZvFieldXRdFloat(U, ID, Member, Flags, Exponent))
#define ZvFBFieldXRdFloatFn(U, ID, Get, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Float, ID, \
      ZvFieldXRdFloatFn(U, ID, Get, Flags, Exponent))

#define ZvFBFieldFloat(U, Member, Flags, Exponent) \
  ZvFBFieldXFloat(U, Member, Member, Flags, Exponent)
#define ZvFBFieldFloatFn(U, Fn, Flags, Exponent) \
  ZvFBFieldXFloatFn(U, Fn, Fn, Fn, Flags, Exponent)

#define ZvFBFieldRdFloat(U, Member, Flags, Exponent) \
  ZvFBFieldXRdFloat(U, Member, Member, Flags, Exponent)
#define ZvFBFieldRdFloatFn(U, Fn, Flags, Exponent) \
  ZvFBFieldXRdFloatFn(U, Fn, Fn, Flags, Exponent)

#define ZvFBFieldXFixed(U, ID, Member, Flags, Exponent) \
  ZvFBField_Fixed_(U, Fixed, ID, \
      ZvFieldXFixed(U, ID, Member, Flags, Exponent))
#define ZvFBFieldXFixedFn(U, ID, Get, Set, Flags, Exponent) \
  ZvFBField_Fixed_(U, Fixed, ID, \
      ZvFieldXFixedFn(U, ID, Get, Set, Flags, Exponent))

#define ZvFBFieldXRdFixed(U, ID, Member, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Fixed, ID, \
      ZvFieldXRdFixed(U, ID, Member, Flags, Exponent))
#define ZvFBFieldXRdFixedFn(U, ID, Get, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Fixed, ID, \
      ZvFieldXRdFixedFn(U, ID, Get, Flags, Exponent))

#define ZvFBFieldFixed(U, Member, Flags, Exponent) \
  ZvFBFieldXFixed(U, Member, Member, Flags, Exponent)
#define ZvFBFieldFixedFn(U, Fn, Flags, Exponent) \
  ZvFBFieldXFixedFn(U, Fn, Fn, Fn, Flags, Exponent)

#define ZvFBFieldRdFixed(U, Member, Flags, Exponent) \
  ZvFBFieldXRdFixed(U, Member, Member, Flags, Exponent)
#define ZvFBFieldRdFixedFn(U, Fn, Flags, Exponent) \
  ZvFBFieldXRdFixedFn(U, Fn, Fn, Flags, Exponent)

#define ZvFBField_Decimal_(U, ID, Base_) \
  struct ZvFBField_Name_(U, Decimal, ID) : public Base_ { \
    using Base = Base_; \
    using FBB = ZvFBB<U>; \
    using FBS = ZvFBS<U>; \
    enum { Inline = 1 }; \
    static void save(FBB &fbb, const void *o) { \
      using namespace Zfb::Save; \
      auto v = decimal(Base::get(o)); \
      fbb.add_##ID(&v); \
    } \
    static auto load_(const FBS *o_) { \
      using namespace Zfb::Load; \
      return decimal(o_->ID()); \
    } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldXDecimal(U, ID, Member, Flags, Exponent) \
  ZvFBField_Decimal_(U, ID, \
      ZvFieldXDecimal(U, ID, Member, Flags, Exponent))
#define ZvFBFieldXDecimalFn(U, ID, Get, Set, Flags, Exponent) \
  ZvFBField_Decimal_(U, ID, \
      ZvFieldXDecimalFn(U, ID, Get, Set, Flags, Exponent))

#define ZvFBFieldXRdDecimal(U, ID, Member, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Decimal, ID, \
      ZvFieldXRdDecimal(U, ID, Member, Flags, Exponent))
#define ZvFBFieldXRdDecimalFn(U, ID, Get, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Decimal, ID, \
      ZvFieldXRdDecimalFn(U, ID, Get, Flags, Exponent))

#define ZvFBFieldDecimal(U, Member, Flags, Exponent) \
  ZvFBFieldXDecimal(U, Member, Member, Flags, Exponent)
#define ZvFBFieldDecimalFn(U, Fn, Flags, Exponent) \
  ZvFBFieldXDecimalFn(U, Fn, Fn, Fn, Flags, Exponent)

#define ZvFBFieldRdDecimal(U, Member, Flags, Exponent) \
  ZvFBFieldXRdDecimal(U, Member, Member, Flags, Exponent)
#define ZvFBFieldRdDecimalFn(U, Fn, Flags, Exponent) \
  ZvFBFieldXRdDecimalFn(U, Fn, Fn, Flags, Exponent)

#define ZvFBFieldXHex(U, ID, Member, Flags) \
  ZvFBField_Int_(U, Hex, ID, ZvFieldXHex(U, ID, Member, Flags))
#define ZvFBFieldXHexFn(U, ID, Get, Set, Flags) \
  ZvFBField_Int_(U, Hex, ID, ZvFieldXHexFn(U, ID, Get, Set, Flags))

#define ZvFBFieldXRdHex(U, ID, Member, Flags) \
  ZvFBField_ReadOnly_(U, Hex, ID, \
      ZvFieldXRdHex(U, ID, Member, Flags))
#define ZvFBFieldXRdHexFn(U, ID, Get, Flags) \
  ZvFBField_ReadOnly_(U, Hex, ID, \
      ZvFieldXRdHexFn(U, ID, Get, Flags))

#define ZvFBFieldHex(U, Member, Flags) \
  ZvFBFieldXHex(U, Member, Member, Flags)
#define ZvFBFieldHexFn(U, Fn, Flags) \
  ZvFBFieldXHexFn(U, Fn, Fn, Fn, Flags)

#define ZvFBFieldRdHex(U, Member, Flags) \
  ZvFBFieldXRdHex(U, Member, Member, Flags)
#define ZvFBFieldRdHexFn(U, Fn, Flags) \
  ZvFBFieldXRdHexFn(U, Fn, Fn, Flags)

#define ZvFBField_Enum_(U, ID, Base_, Map) \
  struct ZvFBField_Name_(U, Enum, ID) : Base_ { \
    using Base = Base_; \
    using T = typename Base::T; \
    using FBB = ZvFBB<U>; \
    using FBS = ZvFBS<U>; \
    enum { Inline = 1 }; \
    static void save(FBB &fbb, const void *o) { \
      fbb.add_##ID(static_cast<Map::FBS>(Base::get(o))); \
    } \
    static auto load_(const FBS *o_) { return static_cast<T>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldXEnum(U, ID, Member, Flags, Map) \
  ZvFBField_Enum_(U, ID, \
      ZvFieldXEnum(U, ID, Member, Flags, Map), Map)
#define ZvFBFieldXEnumFn(U, ID, Get, Set, Flags, Map) \
  ZvFBField_Enum_(U, ID, \
      ZvFieldXEnumFn(U, ID, Get, Set, Flags, Map), Map)

#define ZvFBFieldXRdEnum(U, ID, Member, Flags, Map) \
  ZvFBField_ReadOnly_(U, Enum, ID, \
      ZvFieldXRdEnum(U, ID, Member, Flags, Map))
#define ZvFBFieldXRdEnumFn(U, ID, Get, Flags, Map) \
  ZvFBField_ReadOnly_(U, Enum, ID, \
      ZvFieldXRdEnumFn(U, ID, Get, Flags, Map))

#define ZvFBFieldEnum(U, Member, Flags, Map) \
  ZvFBFieldXEnum(U, Member, Member, Flags, Map)
#define ZvFBFieldEnumFn(U, Fn, Flags, Map) \
  ZvFBFieldXEnumFn(U, Fn, Fn, Fn, Flags, Map)

#define ZvFBFieldRdEnum(U, Member, Flags, Map) \
  ZvFBFieldXRdEnum(U, Member, Member, Flags, Map)
#define ZvFBFieldRdEnumFn(U, Fn, Flags, Map) \
  ZvFBFieldXRdEnumFn(U, Fn, Fn, Flags, Map)

#define ZvFBField_Flags_(U, ID, Base_) \
  struct ZvFBField_Name_(U, Flags, ID) : public Base_ { \
    using Base = Base_; \
    using T = typename Base::T; \
    using FBB = ZvFBB<U>; \
    using FBS = ZvFBS<U>; \
    enum { Inline = 1 }; \
    static void save(FBB &fbb, const void *o) { fbb.add_##ID(Base::get(o)); } \
    static auto load_(const FBS *o_) { return static_cast<T>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldXFlags(U, ID, Member, Flags, Map) \
  ZvFBField_Flags_(U, ID, \
      ZvFieldXFlags(U, ID, Member, Flags, Map))
#define ZvFBFieldXFlagsFn(U, ID, Get, Set, Flags, Map) \
  ZvFBField_Flags_(U, ID, \
      ZvFieldXFlagsFn(U, ID, Get, Set, Flags, Map))

#define ZvFBFieldXRdFlags(U, ID, Member, Flags_, Map) \
  ZvFBField_ReadOnly_(U, Flags, ID, \
      ZvFieldXRdFlags(U, ID, Member, Flags_, Map))
#define ZvFBFieldXRdFlagsFn(U, ID, Get, Flags_, Map) \
  ZvFBField_ReadOnly_(U, Flags, ID, \
      ZvFieldXRdFlagsFn(U, ID, Get, Flags_, Map))

#define ZvFBFieldFlags(U, Member, Flags, Map) \
  ZvFBFieldXFlags(U, Member, Member, Flags, Map)
#define ZvFBFieldFlagsFn(U, Fn, Flags, Map) \
  ZvFBFieldXFlagsFn(U, Fn, Fn, Fn, Flags, Map)

#define ZvFBFieldRdFlags(U, Member, Flags, Map) \
  ZvFBFieldXRdFlags(U, Member, Member, Flags, Map)
#define ZvFBFieldRdFlagsFn(U, Fn, Flags, Map) \
  ZvFBFieldXRdFlagsFn(U, Fn, Fn, Flags, Map)

#define ZvFBField_Time_(U, ID, Base_) \
  struct ZvFBField_Name_(U, Time, ID) : public Base_ { \
    using Base = Base_; \
    using FBB = ZvFBB<U>; \
    using FBS = ZvFBS<U>; \
    enum { Inline = 1 }; \
    static void save(FBB &fbb, const void *o) { \
      using namespace Zfb::Save; \
      auto v = dateTime(Base::get(o)); \
      fbb.add_##ID(&v); \
    } \
    static auto load_(const FBS *o_) { \
      using namespace Zfb::Load; \
      return dateTime(o_->ID()); \
    } \
    static void load(void *o, const FBS *o_) { \
      Base::set(o, load_(o_)); \
    } \
  };

#define ZvFBFieldXTime(U, ID, Member, Flags) \
  ZvFBField_Time_(U, ID, \
      ZvFieldXTime(U, ID, Member, Flags))
#define ZvFBFieldXTimeFn(U, ID, Get, Set, Flags) \
  ZvFBField_Time_(U, ID, \
      ZvFieldXTimeFn(U, ID, Get, Set, Flags))

#define ZvFBFieldXRdTime(U, ID, Member, Flags) \
  ZvFBField_ReadOnly_(U, Time, ID, \
      ZvFieldXRdTime(U, ID, Member, Flags))
#define ZvFBFieldXRdTimeFn(U, ID, Get, Flags) \
  ZvFBField_ReadOnly_(U, Time, ID, \
      ZvFieldXRdTimeFn(U, ID, Get, Flags))

#define ZvFBFieldTime(U, Member, Flags) \
  ZvFBFieldXTime(U, Member, Member, Flags)
#define ZvFBFieldTimeFn(U, Fn, Flags) \
  ZvFBFieldXTimeFn(U, Fn, Fn, Fn, Flags)

#define ZvFBFieldRdTime(U, Member, Flags) \
  ZvFBFieldXRdTime(U, Member, Member, Flags)
#define ZvFBFieldRdTimeFn(U, Fn, Flags) \
  ZvFBFieldXRdTimeFn(U, Fn, Fn, Flags)

#define ZvFBField_Decl_(U, Type, ...) ZvFBField##Type(U, __VA_ARGS__)
#define ZvFBField_Decl(U, Args) \
  ZuPP_Defer(ZvFBField_Decl_)(U, ZuPP_Strip(Args))

template <typename T> ZuTypeList<> ZvFBFieldList_(T *); // default

#define ZvFBFields(U, ...)  \
  fbs::U##Builder *ZvFBB_(U *); \
  fbs::U *ZvFBS_(U *); \
  namespace { \
    ZuPP_Eval(ZuPP_MapArg(ZuField_ID, U, __VA_ARGS__)) \
    ZuPP_Eval(ZuPP_MapArg(ZvFBField_Decl, U, __VA_ARGS__)) \
    using U##_Fields = \
      ZuTypeList<ZuPP_Eval(ZuPP_MapArgComma(ZvFBField_Name, U, __VA_ARGS__))>; \
  } \
  U *ZvFielded_(U *); \
  U##_Fields ZvFBFieldList_(U *); \
  U##_Fields ZvFieldList_(U *)

namespace ZvFB {

namespace Type = ZvField::Type;
namespace Flags = ZvField::Flags;

template <typename T>
using List = decltype(ZvFBFieldList_(ZuDeclVal<T *>()));

namespace Save {

template <typename T> using Offset = Zfb::Offset<T>;

template <typename Field> struct HasOffset {
  enum { OK =
    Field::Type == Type::String ||
    (Field::Type == Type::Composite && !Field::Inline)
  };
};
template <
  typename OffsetFieldList, typename Field, bool = HasOffset<Field>::OK>
struct SaveField {
  template <typename FBB>
  static void save(FBB &fbb, const void *o, const Offset<void> *) {
    Field::save(fbb, o);
  }
};
template <typename OffsetFieldList, typename Field>
struct SaveField<OffsetFieldList, Field, true> {
  template <typename FBB>
  static void save(FBB &fbb, const void *, const Offset<void> *offsets) {
    using OffsetIndex = ZuTypeIndex<Field, OffsetFieldList>;
    Field::save(fbb, offsets[OffsetIndex::I]);
  }
};
template <typename T, typename FieldList>
struct SaveFieldList {
  using FBB = ZvFBB<T>;
  using FBS = ZvFBS<T>;
  static Zfb::Offset<FBS> save(Zfb::Builder &fbb_, const void *o) {
    using OffsetFieldList = ZuTypeGrep<HasOffset, FieldList>;
    Offset<void> offsets[OffsetFieldList::N];
    ZuTypeAll<OffsetFieldList>::invoke(
	[o, &fbb_, offsets = &offsets[0]]<typename Field>() {
	  using OffsetIndex = ZuTypeIndex<Field, OffsetFieldList>;
	  offsets[OffsetIndex::I] = Field::save(fbb_, o).Union();
	});
    FBB fbb{fbb_};
    ZuTypeAll<FieldList>::invoke(
	[o, &fbb, offsets = &offsets[0]]<typename Field>() {
	  SaveField<OffsetFieldList, Field>::save(fbb, o, offsets);
	});
    return fbb.Finish();
  }
};

} // namespace Save

template <typename T>
struct Table_ {
  using FBB = ZvFBB<T>;
  using FBS = ZvFBS<T>;
  using FieldList = List<T>;

  template <typename U>
  struct AllFilter { enum { OK = !(U::Flags & Flags::ReadOnly) }; };
  using AllFields = ZuTypeGrep<AllFilter, FieldList>;

  template <typename U>
  struct UpdatedFilter { enum {
    OK = U::Flags & (Flags::Primary | Flags::Update) }; };
  using UpdatedFields = ZuTypeGrep<UpdatedFilter, AllFields>;

  template <typename U>
  struct CtorFilter { enum { OK = U::Flags & Flags::Ctor_ }; };
  using CtorFields_ = ZuTypeGrep<CtorFilter, AllFields>;
  template <typename U>
  struct CtorIndex { enum { I = Flags::CtorIndex(U::Flags) }; };
  using CtorFields = ZuTypeSort<CtorIndex, CtorFields_>;

  template <typename U>
  struct InitFilter { enum { OK = !(U::Flags & Flags::Ctor_) }; };
  using InitFields = ZuTypeGrep<InitFilter, AllFields>;

  template <typename U>
  struct KeyFilter { enum { OK = U::Flags & Flags::Primary }; };
  using KeyFields = ZuTypeGrep<KeyFilter, AllFields>;

  static Zfb::Offset<FBS> save(Zfb::Builder &fbb, const void *o) {
    using namespace Save;
    return SaveFieldList<T, AllFields>::save(fbb, o);
  }
  static Zfb::Offset<FBS> saveUpdate(Zfb::Builder &fbb, const void *o) {
    using namespace Save;
    return SaveFieldList<T, UpdatedFields>::save(fbb, o);
  }
  static void load(void *o, const FBS *o_) {
    ZuTypeAll<AllFields>::invoke(
	[o, o_]<typename Field>() { Field::load(o, o_); });
  }
  static void loadUpdate(void *o, const FBS *o_) {
    ZuTypeAll<UpdatedFields>::invoke(
	[o, o_]<typename Field>() { Field::load(o, o_); });
  }

  template <typename ...Fields>
  struct Load_Ctor_ : public T {
    Load_Ctor_() = default;
    Load_Ctor_(const FBS *o_) : T{Fields::load_(o_)...} { }
  };
  using Load_Ctor = ZuTypeApply<Load_Ctor_, CtorFields>;
  struct Load : public Load_Ctor {
    Load() = default;
    Load(const FBS *o_) : Load_Ctor{o_} {
      ZuTypeAll<InitFields>::invoke(
	  [this, o_]<typename Field>() {
	    Field::load(static_cast<T *>(this), o_);
	  });
    }
  };

  template <typename ...Fields>
  struct Key {
    using Tuple = ZuTuple<typename Fields::T...>;
    static auto tuple(const FBS *o_) { return Tuple{Fields::load_(o_)...}; }
  };
  static auto key(const FBS *o_) {
    using Mk = ZuTypeApply<Key, KeyFields>;
    return Mk::tuple(o_);
  }
};
template <typename T>
using Table = Table_<ZvField::Fielded<T>>;

template <typename T>
inline auto save(Zfb::Builder &fbb, const T &o) {
  return Table<T>::save(fbb, &o);
}
template <typename T>
inline auto saveUpdate(Zfb::Builder &fbb, const T &o) {
  return Table<T>::saveUpdate(fbb, &o);
}

template <typename T>
inline void load(T &o, const ZvFBS<T> *o_) {
  Table<T>::load(&o, o_);
}
template <typename T>
inline void loadUpdate(T &o, const ZvFBS<T> *o_) {
  Table<T>::loadUpdate(&o, o_);
}

template <typename T> using Load = typename Table<T>::Load;

template <typename T>
inline auto key(const ZvFBS<T> *o_) {
  return Table<T>::key(o_);
}

} // namespace ZvFB

#endif /* ZvFBField_HPP */
