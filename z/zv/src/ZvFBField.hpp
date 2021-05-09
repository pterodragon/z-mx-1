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

template <typename T> void *ZvFBB_(T *);
template <typename T> void *ZvFBS_(T *);

template <typename T> using ZvFBB = decltype(*ZvFBB_(ZuDeclVal<T *>()));
template <typename T> using ZvFBS = decltype(*ZvFBS_(ZuDeclVal<T *>()));

#define ZvFBField_Name_(T, Type, ID, ...) T##_##ID##_FB
#define ZvFBField_Name(T, Args) ZuPP_Defer(ZvFBField_Name_)(T, ZuPP_Strip(Args))

#define ZvFBField_Composite(T, Type, Fn, ID, Flags, SaveFn, LoadFn) \
  struct ZvFBField_Name_(T, Type, ID) : \
      public ZvField##Type##Fn(T, ID, Flags) { \
    using Base = ZvField##Type(T, ID, Flags); \
    using FBB = ZvFBB<T>; \
    using FBS = ZvFBS<T>; \
    static Zfb::Offset<void> save(Zfb::Builder &fbb, const void *o) { \
      using namespace Zfb::Save; \
      return SaveFn(fbb, Base::get(o)); \
    } \
    static void save(FBB &fbb, Zfb::Offset<void> offset) { \
      fbb.add_##ID(Zfb::Offset<ZvFBS<typename Base::T>>{offset.o}); \
    } \
    static auto load_(const FBS *o_) { \
      using namespace Zfb::Load; \
      return LoadFn(o_->ID()); \
    } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldString(T, ID, Flags) \
  ZvFBField_Composite(T, String,, ID, Flags, str, str)
#define ZvFBFieldStringFn(T, ID, Flags) \
  ZvFBField_Composite(T, String, Fn, ID, Flags, str, str)

#define ZvFBFieldComposite(T, Type, ID, Flags, SaveFn, LoadFn) \
  ZvFBField_Composite(T, Composite,, ID, Flags, SaveFn, LoadFn)
#define ZvFBFieldCompositeFn(T, Type, ID, Flags, SaveFn, LoadFn) \
  ZvFBField_Composite(T, Composite, Fn, ID, Flags, SaveFn, LoadFn)

#define ZvFBField_Primitive(T, Type, Fn, ID, Flags) \
  struct ZvFBField_Name_(T, Type, ID) : \
      public ZvField##Type##Fn(T, ID, Flags) { \
    using Base = ZvField##Type(T, ID, Flags); \
    using V = typename Base::T; \
    using FBB = ZvFBB<T>; \
    using FBS = ZvFBS<T>; \
    static void save(FBB &fbb, const void *o) { fbb.add_##ID(Base::get(o)); } \
    static auto load_(const FBS *o_) { return static_cast<V>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldInt(T, ID, Flags) \
  ZvFBField_Primitive(T, Bool,, ID, Flags)
#define ZvFBFieldIntFn(T, ID, Flags) \
  ZvFBField_Primitive(T, Bool, Fn, ID, Flags)
#define ZvFBFieldFloat(T, ID, Flags) \
  ZvFBField_Primitive(T, Float,, ID, Flags)
#define ZvFBFieldFloatFn(T, ID, Flags) \
  ZvFBField_Primitive(T, Float, Fn, ID, Flags)
#define ZvFBFieldFixed(T, ID, Flags) \
  ZvFBField_Primitive(T, Fixed,, ID, Flags)
#define ZvFBFieldFixedFn(T, ID, Flags) \
  ZvFBField_Primitive(T, Fixed, Fn, ID, Flags)

#define ZvFBField_Decimal(T, Fn, ID, Flags) \
  struct ZvFBField_Name_(T, Decimal, ID) : \
      public ZvFieldDecimal##Fn(T, ID, Flags) { \
    using Base = ZvFieldDecimal(T, ID, Flags); \
    using FBB = ZvFBB<T>; \
    using FBS = ZvFBS<T>; \
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

#define ZvFBFieldDecimal(T, ID, Flags) \
  ZvFBField_Decimal(T,, ID, Flags)
#define ZvFBFieldDecimalFn(T, ID, Flags) \
  ZvFBField_Decimal(T, Fn, ID, Flags)

#define ZvFBFieldHex(T, ID, Flags) \
  ZvFBField_Primitive(T, Hex,, ID, Flags)
#define ZvFBFieldHexFn(T, ID, Flags) \
  ZvFBField_Primitive(T, Hex, Fn, ID, Flags)

#define ZvFBField_Enum(T, Fn, ID, Flags, Map) \
  struct ZvFBField_Name_(T, Type, ID) : \
      public ZvFieldEnum##Fn(T, ID, Flags, Map) { \
    using Base = ZvFieldEnum(T, ID, Flags, Map); \
    using V = typename Base::T; \
    using FBB = ZvFBB<T>; \
    using FBS = ZvFBS<T>; \
    static void save(FBB &fbb, const void *o) { \
      fbb.add_##ID(static_cast<Map::FBS>(Base::get(o))); \
    } \
    static auto load_(const FBS *o_) { return static_cast<V>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldEnum(T, ID, Flags) \
  ZvFBField_Enum(T,, ID, Flags)
#define ZvFBFieldEnumFn(T, ID, Flags) \
  ZvFBField_Enum(T, Fn, ID, Flags)

#define ZvFBField_Flags(T, Fn, ID, Flags, Map) \
  struct ZvFBField_Name_(T, Type, ID) : \
      public ZvFieldFlags##Fn(T, ID, Flags, Map) { \
    using Base = ZvFieldFlags(T, ID, Flags, Map); \
    using V = typename Base::T; \
    using FBB = ZvFBB<T>; \
    using FBS = ZvFBS<T>; \
    static void save(FBB &fbb, const void *o) { fbb.add_##ID(Base::get(o)); } \
    static auto load_(const FBS *o_) { return static_cast<V>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldFlags(T, ID, Flags) \
  ZvFBField_Flags(T,, ID, Flags)
#define ZvFBFieldFlagsFn(T, ID, Flags) \
  ZvFBField_Flags(T, Fn, ID, Flags)

#define ZvFBField_Time(T, Fn, ID, Flags) \
  struct ZvFBField_Name_(T, Type, ID) : \
      public ZvFieldTime##Fn(T, ID, Flags) { \
    using Base = ZvFieldTime(T, ID, Flags); \
    using FBB = ZvFBB<T>; \
    using FBS = ZvFBS<T>; \
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

#define ZvFBFieldTime(T, ID, Time) \
  ZvFBField_Time(T,, ID, Time)
#define ZvFBFieldTimeFn(T, ID, Time) \
  ZvFBField_Time(T, Fn, ID, Time)

#define ZvFBField_Decl_(T, Type, ...) ZvFBField##Type(T, __VA_ARGS__)
#define ZvFBField_Decl(T, Args) ZuPP_Defer(ZvFBField_Decl_)(T, ZuPP_Strip(Args))

template <typename T> ZuTypeList<> ZvFBFieldList_(T *); // default

#define ZvFBFields(U, ...)  \
  fbs::U##Builder *ZvFBB_(U *); \
  fbs::U *ZvFBS_(U *); \
  namespace { \
    ZuPP_Eval(ZuPP_MapArg(ZuField_ID, U, __VA_ARGS__)) \
    ZuPP_Eval(ZuPP_MapArg(ZvFBField_Decl, U, __VA_ARGS__)) \
    using U##_Fields = \
      ZuTypeList<ZuPP_Eval(ZuPP_MapArgComma(ZvFBField_Name, U, __VA_ARGS__))> \
  } \
  U##_Fields ZvFBFieldList_(U *); \
  U##_Fields ZvFieldList_(U *);

template <typename T>
using ZvFBFieldList = decltype(ZvFBFieldList_(ZuDeclVal<T *>()));

namespace ZvFB_Save {

template <typename T> using Offset = Zfb::Offset<T>;

template <typename Field> struct HasOffset {
  enum { OK =
    Field::Type == ZvFieldType::String ||
    Field::Type == ZvFieldType::Composite ||
    Field::Type == ZvFieldType::Decimal ||
    Field::Type == ZvFieldType::Time };
};
template <
  typename OffsetFieldList, typename Field, bool = HasOffset<Field>::OK>
struct SaveField {
  template <typename FBB>
  static void save(const void *o, FBB &fbb, const Offset<void> *) {
    Field::save(fbb, o);
  }
};
template <typename OffsetFieldList, typename Field>
struct SaveField<OffsetFieldList, Field, true> {
  template <typename FBB>
  static void save(const void *, FBB &fbb, const Offset<void> *offsets) {
    using OffsetIndex = Field::OffsetIndex<OffsetFieldList>;
    Field::save(fbb, offsets[OffsetIndex]);
  }
};
template <typename T, typename FieldList>
struct SaveFieldList {
  using FBB = ZvFBB<T>;
  using FBS = ZvFBS<T>;
  static Zfb::Offset<FBS> save(const void *o, Zfb::Builder &fbb) {
    using OffsetFieldList = ZuTypeGrep<HasOffset, FieldList>::T;
    Offset<void> offsets[OffsetFieldList::N];
    ZuTypeAll<OffsetFieldList>::invoke(
	[o, &fbb, offsets = &offsets[0]]<typename Field>() {
	  using OffsetIndex = Field::OffsetIndex<OffsetFieldList>;
	  offsets[OffsetIndex::I] = Field::save(o, fbb).Union();
	});
    FBB b{fbb};
    ZuTypeAll<FieldList>::invoke(
	[o, &b, offsets = &offsets[0]]<typename Field>() {
	  SaveField<OffsetFieldList, Field>::save(o, b, offsets);
	});
    return b.Finish();
  }
};

} // namespace ZvFB_Save

template <typename T>
struct ZvFB {
  using FBB = ZvFBB<T>;
  using FBS = ZvFBS<T>;
  using FieldList = ZfbFieldList<T>;

  template <typename T>
  struct NotSynOK { enum { OK = !(T::Flags & ZvFieldFlags::Synthetic) }; };
  using AllFields = typename ZuTypeGrep<NotSynOK, FieldList>::T;

  template <typename T>
  struct UpdateOK { enum {
    OK = (T::Flags & (ZvFieldFlags::Primary | ZvFieldFlags::Update)) }; };
  using UpdatedFields = typename ZuTypeGrep<UpdateOK, AllFields>::T;

  template <typename T>
  struct CtorOK { enum { OK = T::Flags & ZvFieldFlags::Ctor_ }; };
  using CtorFields_ = typename ZuTypeGrep<CtorOK, AllFields>::T;
  template <typename T>
  struct CtorIndex { enum { I = ZvFieldFlags::CtorIndex(T::Flags) }; };
  using CtorFields = typename ZuTypeSort<CtorIndex, CtorFields_>::T;

  template <typename T>
  struct InitOK { enum { OK = !(T::Flags & ZvFieldFlags::Ctor) }; };
  using InitFields = typename ZuTypeGrep<InitOK, AllFields>::T;

  static Zfb::Offset<FBS> save(const void *o, Zfb::Builder &fbb) const {
    using namespace ZvFB_Save;
    return SaveFieldList<T, AllFields>::save(o, fbb);
  }
  static Zfb::Offset<FBS> saveUpdate(const void *o, Zfb::Builder &fbb) const {
    using namespace ZvFB_Save;
    return SaveFieldList<T, UpdatedFields>::save(o, fbb);
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
    Load_Ctor_(const FBS *o_) : T{ Fields::load_(o_)... } { }
  };
  using Load_Ctor = typename ZuTypeApply<Load_Ctor_, CtorFields>::T;
  template <typename ...Fields>
  struct Load_Init_ : public Load_Ctor {
    Load_Init_() = default;
    Load_Init_(const FBS *o_) : Load_Ctor{o_} {
      ZuTypeAll<Fields>::invoke(
	  [this, o_]<typename Field>() {
	    Field::load(static_cast<T *>(this), o_);
	  });
    }
  };
  using Load = typename ZuTypeApply<Load_Init_, InitFields>::T;
};

#endif /* ZvFBField_HPP */
