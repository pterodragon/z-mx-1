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

#define ZvFBField_Composite(T, Type, ID, Flags, SaveFn, LoadFn) \
  struct T##_##ID##_FB : public ZvField##Type(T, ID, Flags) { \
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

#define ZvFBField_Primitive(T, Type, ID, Flags) \
  struct T##_##ID##_FB : public ZvField##Type(T, ID, Flags) { \
    using Base = ZvField##Type(T, ID, Flags); \
    using V = typename Base::T; \
    using FBB = ZvFBB<T>; \
    using FBS = ZvFBS<T>; \
    static void save(FBB &fbb, const void *o) { fbb.add_##ID(Base::get(o)); } \
    static auto load_(const FBS *o_) { return static_cast<V>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldString(T, ID, Flags) \
  ZvFBField_Composite(T, String, ID, Flags, str, str)

#define ZvFBFieldComposite(T, ID, Flags, SaveFn, LoadFn) \
  ZvFBField_Composite(T, Composite, ID, Flags, SaveFn, LoadFn)

#define ZvFBFieldInt(T, ID, Flags) ZvFBField_Primitive(T, Bool, ID, Flags)
#define ZvFBFieldFloat(T, ID, Flags) ZvFBField_Primitive(T, Float, ID, Flags)
#define ZvFBFieldFixed(T, ID, Flags) ZvFBField_Primitive(T, Fixed, ID, Flags)

#define ZvFBFieldDecimal(T, ID, Flags) \
  struct T##_##ID##_FB : public ZvFieldDecimal(T, ID, Flags) { \
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

#define ZvFBFieldHex(T, ID, Flags) ZvFBField_Primitive(T, Hex, ID, Flags)

#define ZvFBFieldEnum(T, ID, Flags, Map) \
  struct T##_##ID##_FB : public ZvFieldEnum(T, ID, Flags, Map) { \
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

#define ZvFBFieldFlags(T, ID, Flags, Map) \
  struct T##_##ID##_FB : public ZvFieldFlags(T, ID, Flags, Map) { \
    using Base = ZvFieldFlags(T, ID, Flags, Map); \
    using V = typename Base::T; \
    using FBB = ZvFBB<T>; \
    using FBS = ZvFBS<T>; \
    static void save(FBB &fbb, const void *o) { fbb.add_##ID(Base::get(o)); } \
    static auto load_(const FBS *o_) { return static_cast<V>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldTime(T, ID, Flags) \
  struct T##_##ID##_FB : public ZvFieldTime(T, ID, Flags) { \
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

#define ZvFBField_Decl_(T, Type, ...) ZvFBField##Type(T, __VA_ARGS__)
#define ZvFBField_Decl(T, Args) ZuPP_Defer(ZvFBField_Decl_)(T, ZuPP_Strip(Args))

#define ZvFBField_Name_(T, Type, ID, ...) T##_##ID##_FB
#define ZvFBField_Name(T, Args) ZuPP_Defer(ZvFBField_Name_)(T, ZuPP_Strip(Args))

template <typename T> ZuTypeList<> ZvFBFieldList_(T *); // default

#define ZvFBFieldDef(T_, ...)  \
  fbs::T_##Builder *ZvFBB_(T_ *); \
  fbs::T_ *ZvFBS_(T_ *); \
  namespace { \
    ZuPP_Eval(ZuPP_MapArg(ZuField_ID, T_, __VA_ARGS__)) \
    ZuPP_Eval(ZuPP_MapArg(ZvFBField_Decl, T_, __VA_ARGS__)) \
    using T_##_Fields = \
      ZuTypeList<ZuPP_Eval(ZuPP_MapArgComma(ZvFBField_Name, T_, __VA_ARGS__))> \
  } \
  T_##_Fields ZvFBFieldList_(T_ *); \
  T_##_Fields ZvFieldList_(T_ *);

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
template <typename Field> struct IsUpdated {
  enum { OK = (Field::Flags & ZvFieldFlags::Update) };
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
  struct NotSyn { enum { OK = !(T::Flags & ZvFieldFlags::Synthetic) }; };
  using All = typename ZuTypeGrep<NotSyn, FieldList>::T;

  template <typename T>
  struct Updated_ { enum {
    OK = (T::Flags & (ZvFieldFlags::Primary | ZvFieldFlags::Update)) }; };
  using Updated = typename ZuTypeGrep<Updated_, All>::T;

  static Zfb::Offset<FBS> save(const void *o, Zfb::Builder &fbb) const {
    using namespace ZvFB_Save;
    return SaveFieldList<T, All>::save(o, fbb);
  }
  static Zfb::Offset<FBS> saveUpdate(const void *o, Zfb::Builder &fbb) const {
    using namespace ZvFB_Save;
    return SaveFieldList<T, Updated>::save(o, fbb);
  }
  static void load(void *o, const FBS *o_) {
    ZuTypeAll<All>::invoke(
	[o, o_]<typename Field>() { Field::load(o, o_); });
  }
  static void loadUpdate(void *o, const FBS *o_) {
    ZuTypeAll<Updated>::invoke(
	[o, o_]<typename Field>() { Field::load(o, o_); });
  }

  template <typename ...Fields>
  struct Load_ : public T {
    Load_() = default;
    Load_(const Load_ &) = default;
    Load_(Load_ &&) = default;
    Load_(const FBS *o_) : T{ Fields::load_(o_)... } { }
  };
  using Load = typename ZuTypeApply<Load_, All>::T;
};

#endif /* ZvFBField_HPP */
