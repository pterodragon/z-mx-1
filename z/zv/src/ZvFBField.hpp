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

// FIXME - X Fields

#define ZvFBField_ReadOnly_(U, Type, Fn, ID, ...) \
  struct ZvFBField_Name_(U, Type, ID) : \
      public ZvFieldRd##Type##Fn(U, ID, __VA_ARGS__) { };

#define ZvFBField_Composite_(U, Type, Fn, ID, Flags, SaveFn, LoadFn) \
  struct ZvFBField_Name_(U, Type, ID) : \
      public ZvField##Type##Fn(U, ID, Flags) { \
    using Base = ZvField##Type##Fn(U, ID, Flags); \
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

#define ZvFBFieldString(U, ID, Flags) \
  ZvFBField_Composite_(U, String,, ID, Flags, str, str)
#define ZvFBFieldStringFn(U, ID, Flags) \
  ZvFBField_Composite_(U, String, Fn, ID, Flags, str, str)

#define ZvFBFieldRdString(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, String,, ID, Flags)
#define ZvFBFieldRdStringFn(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, String, Fn, ID, Flags)

#define ZvFBFieldComposite(U, ID, Flags, SaveFn, LoadFn) \
  ZvFBField_Composite_(U, Composite,, ID, Flags, SaveFn, LoadFn)
#define ZvFBFieldCompositeFn(U, ID, Flags, SaveFn, LoadFn) \
  ZvFBField_Composite_(U, Composite, Fn, ID, Flags, SaveFn, LoadFn)

#define ZvFBFieldRdComposite(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Composite,, ID, Flags)
#define ZvFBFieldRdCompositeFn(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Composite, Fn, ID, Flags)

#define ZvFBField_Inline_(U, Fn, ID, Flags, SaveFn, LoadFn) \
  struct ZvFBField_Name_(U, Inline, ID) : \
      public ZvFieldComposite##Fn(U, ID, Flags) { \
    using Base = ZvFieldComposite##Fn(U, ID, Flags); \
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

#define ZvFBFieldInline(U, ID, Flags, SaveFn, LoadFn) \
  ZvFBField_Inline_(U,, ID, Flags, SaveFn, LoadFn)
#define ZvFBFieldInlineFn(U, ID, Flags, SaveFn, LoadFn) \
  ZvFBField_Inline_(U, Fn, ID, Flags, SaveFn, LoadFn)

#define ZvFBField_Int_(U, Type, Fn, ID, Flags) \
  struct ZvFBField_Name_(U, Type, ID) : \
      public ZvField##Type##Fn(U, ID, Flags) { \
    using Base = ZvField##Type##Fn(U, ID, Flags); \
    using T = typename Base::T; \
    using FBB = ZvFBB<U>; \
    using FBS = ZvFBS<U>; \
    enum { Inline = 1 }; \
    static void save(FBB &fbb, const void *o) { fbb.add_##ID(Base::get(o)); } \
    static auto load_(const FBS *o_) { return static_cast<T>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldBool(U, ID, Flags) \
  ZvFBField_Int_(U, Bool,, ID, Flags)
#define ZvFBFieldBoolFn(U, ID, Flags) \
  ZvFBField_Int_(U, Bool, Fn, ID, Flags)
#define ZvFBFieldInt(U, ID, Flags) \
  ZvFBField_Int_(U, Int,, ID, Flags)
#define ZvFBFieldIntFn(U, ID, Flags) \
  ZvFBField_Int_(U, Int, Fn, ID, Flags)

#define ZvFBFieldRdBool(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Bool,, ID, Flags)
#define ZvFBFieldRdBoolFn(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Bool, Fn, ID, Flags)
#define ZvFBFieldRdInt(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Int,, ID, Flags)
#define ZvFBFieldRdIntFn(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Int, Fn, ID, Flags)

#define ZvFBField_Fixed_(U, Type, Fn, ID, Flags, Exponent) \
  struct ZvFBField_Name_(U, Type, ID) : \
      public ZvField##Type##Fn(U, ID, Flags, Exponent) { \
    using Base = ZvField##Type##Fn(U, ID, Flags, Exponent); \
    using T = typename Base::T; \
    using FBB = ZvFBB<U>; \
    using FBS = ZvFBS<U>; \
    enum { Inline = 1 }; \
    static void save(FBB &fbb, const void *o) { fbb.add_##ID(Base::get(o)); } \
    static auto load_(const FBS *o_) { return static_cast<T>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldFloat(U, ID, Flags, Exponent) \
  ZvFBField_Fixed_(U, Float,, ID, Flags, Exponent)
#define ZvFBFieldFloatFn(U, ID, Flags, Exponent) \
  ZvFBField_Fixed_(U, Float, Fn, ID, Flags, Exponent)
#define ZvFBFieldFixed(U, ID, Flags, Exponent) \
  ZvFBField_Fixed_(U, Fixed,, ID, Flags, Exponent)
#define ZvFBFieldFixedFn(U, ID, Flags, Exponent) \
  ZvFBField_Fixed_(U, Fixed, Fn, ID, Flags, Exponent)

#define ZvFBFieldRdFloat(U, ID, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Float,, ID, Flags, Exponent)
#define ZvFBFieldRdFloatFn(U, ID, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Float, Fn, ID, Flags, Exponent)
#define ZvFBFieldRdFixed(U, ID, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Fixed,, ID, Flags, Exponent)
#define ZvFBFieldRdFixedFn(U, ID, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Fixed, Fn, ID, Flags, Exponent)

#define ZvFBField_Decimal_(U, Fn, ID, Flags, Exponent) \
  struct ZvFBField_Name_(U, Decimal, ID) : \
      public ZvFieldDecimal##Fn(U, ID, Flags, Exponent) { \
    using Base = ZvFieldDecimal##Fn(U, ID, Flags, Exponent); \
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

#define ZvFBFieldDecimal(U, ID, Flags, Exponent) \
  ZvFBField_Decimal_(U,, ID, Flags, Exponent)
#define ZvFBFieldDecimalFn(U, ID, Flags, Exponent) \
  ZvFBField_Decimal_(U, Fn, ID, Flags, Exponent)

#define ZvFBFieldRdDecimal(U, ID, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Decimal,, ID, Flags, Exponent)
#define ZvFBFieldRdDecimalFn(U, ID, Flags, Exponent) \
  ZvFBField_ReadOnly_(U, Decimal, Fn, ID, Flags, Exponent)

#define ZvFBFieldHex(U, ID, Flags) \
  ZvFBField_Int_(U, Hex,, ID, Flags)
#define ZvFBFieldHexFn(U, ID, Flags) \
  ZvFBField_Int_(U, Hex, Fn, ID, Flags)

#define ZvFBFieldRdHex(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Hex,, ID, Flags)
#define ZvFBFieldRdHexFn(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Hex, Fn, ID, Flags)

#define ZvFBField_Enum_(U, Fn, ID, Flags, Map) \
  struct ZvFBField_Name_(U, Type, ID) : \
      public ZvFieldEnum##Fn(U, ID, Flags, Map) { \
    using Base = ZvFieldEnum##Fn(U, ID, Flags, Map); \
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

#define ZvFBFieldEnum(U, ID, Flags, Map) \
  ZvFBField_Enum_(U,, ID, Flags, Map)
#define ZvFBFieldEnumFn(U, ID, Flags, Map) \
  ZvFBField_Enum_(U, Fn, ID, Flags, Map)

#define ZvFBFieldRdEnum(U, ID, Flags, Map) \
  ZvFBField_ReadOnly_(U, Enum,, ID, Flags, Map)
#define ZvFBFieldRdEnumFn(U, ID, Flags, Map) \
  ZvFBField_ReadOnly_(U, Enum, Fn, ID, Flags, Map)

#define ZvFBField_Flags_(U, Fn, ID, Flags, Map) \
  struct ZvFBField_Name_(U, Type, ID) : \
      public ZvFieldFlags##Fn(U, ID, Flags, Map) { \
    using Base = ZvFieldFlags##Fn(U, ID, Flags, Map); \
    using T = typename Base::T; \
    using FBB = ZvFBB<U>; \
    using FBS = ZvFBS<U>; \
    enum { Inline = 1 }; \
    static void save(FBB &fbb, const void *o) { fbb.add_##ID(Base::get(o)); } \
    static auto load_(const FBS *o_) { return static_cast<T>(o_->ID()); } \
    static void load(void *o, const FBS *o_) { Base::set(o, load_(o_)); } \
  };

#define ZvFBFieldFlags(U, ID, Flags) \
  ZvFBField_Flags_(U,, ID, Flags)
#define ZvFBFieldFlagsFn(U, ID, Flags) \
  ZvFBField_Flags_(U, Fn, ID, Flags)

#define ZvFBFieldRdFlags(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Flags,, ID, Flags)
#define ZvFBFieldRdFlagsFn(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Flags, Fn, ID, Flags)

#define ZvFBField_Time_(U, Fn, ID, Flags) \
  struct ZvFBField_Name_(U, Type, ID) : \
      public ZvFieldTime##Fn(U, ID, Flags) { \
    using Base = ZvFieldTime##Fn(U, ID, Flags); \
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

#define ZvFBFieldTime(U, ID, Flags) \
  ZvFBField_Time_(U,, ID, Flags)
#define ZvFBFieldTimeFn(U, ID, Flags) \
  ZvFBField_Time_(U, Fn, ID, Flags)

#define ZvFBFieldRdTime(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Time,, ID, Flags)
#define ZvFBFieldRdTimeFn(U, ID, Flags) \
  ZvFBField_ReadOnly_(U, Time, Fn, ID, Flags)

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
