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

// heap-allocated dynamic array class

#ifndef ZtArray_HPP
#define ZtArray_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZtLib_HPP
#include <zlib/ZtLib.hpp>
#endif

#include <initializer_list>

#include <stdlib.h>
#include <string.h>

#include <zlib/ZuIfT.hpp>
#include <zlib/ZuInt.hpp>
#include <zlib/ZuNull.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuArrayFn.hpp>
#include <zlib/ZuUTF.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuBox.hpp>
#include <zlib/ZuEquivChar.hpp>

#include <zlib/ZmAssert.hpp>

#include <zlib/ZtPlatform.hpp>
#include <zlib/ZtIconv.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800 4348)
#endif

template <typename T, class Cmp> class ZtArray;

template <typename T> struct ZtArray_ { };

template <typename T_> struct ZtArray_Char2 { using T = ZuNull; };
template <> struct ZtArray_Char2<char> { using T = wchar_t; };
template <> struct ZtArray_Char2<wchar_t> { using T = char; };

template <typename T_, class Cmp_ = ZuCmp<T_> >
class ZtArray : public ZtArray_<T_>, public ZuArrayFn<T_, Cmp_> {
public:
  using T = T_;
  using Cmp = Cmp_;
  using Ops = ZuArrayFn<T, Cmp>;

  enum Copy_ { Copy };
  enum Move_ { Move };

private:
  using Char = T;
  using Char2 = typename ZtArray_Char2<T>::T;

  // from same type ZtArray
  template <typename U, typename V = T> struct IsZtArray {
    enum { OK = ZuConversion<ZtArray_<V>, U>::Base };
  };
  template <typename U, typename R = void>
  struct MatchZtArray : public ZuIfT<IsZtArray<U>::OK, R> { };

  // from string literal with same char
  template <typename U, typename V = Char> struct IsStrLiteral {
    enum { OK = ZuTraits<U>::IsCString &&
      ZuTraits<U>::IsArray && ZuTraits<U>::IsPrimitive &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchStrLiteral : public ZuIfT<IsStrLiteral<U>::OK, R> { };

  // from some other string with same char (including string literals)
  template <typename U, typename V = Char> struct IsAnyString {
    enum { OK =
      !IsZtArray<U>::OK &&
      (ZuTraits<U>::IsArray || ZuTraits<U>::IsString) &&
      ZuEquivChar<typename ZuTraits<U>::Elem, V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchAnyString : public ZuIfT<IsAnyString<U>::OK, R> { };

  // from some other string with same char (other than a string literal)
  template <typename U, typename V = Char> struct IsString {
    enum { OK =
      !IsStrLiteral<U>::OK &&
      IsAnyString<U>::OK };
  };
  template <typename U, typename R = void>
  struct MatchString : public ZuIfT<IsString<U>::OK, R> { };

  // from another array type with convertible element type (not a string)
  template <typename U, typename V = T> struct IsArray {
    enum { OK =
      !IsZtArray<U>::OK &&
      !IsAnyString<U>::OK &&
      !ZuConversion<U, V>::Same &&
      ZuTraits<U>::IsArray &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Exists };
  };
  template <typename U, typename R = void>
  struct MatchArray : public ZuIfT<IsArray<U>::OK, R> { };

  // from another array type with same element type (not a string)
  template <typename U, typename V = T> struct IsSameArray {
    enum { OK =
      !IsZtArray<U>::OK &&
      !ZuConversion<U, V>::Same &&
      !ZuTraits<U>::IsString && ZuTraits<U>::IsArray &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchSameArray : public ZuIfT<IsSameArray<U>::OK, R> { };

  // from another array type with convertible element type (not a string)
  template <typename U, typename V = T> struct IsDiffArray {
    enum { OK =
      !IsZtArray<U>::OK &&
      !ZuConversion<U, V>::Same &&
      !ZuTraits<U>::IsString && ZuTraits<U>::IsArray &&
      !ZuConversion<typename ZuTraits<U>::Elem, V>::Same &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Exists };
  };
  template <typename U, typename R = void>
  struct MatchDiffArray : public ZuIfT<IsDiffArray<U>::OK, R> { };

  // from char2 string (requires conversion)
  template <typename U, typename V = Char2> struct IsChar2String {
    enum { OK = !ZuConversion<ZuNull, V>::Same && ZuTraits<U>::IsString &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchChar2String : public ZuIfT<IsChar2String<U>::OK, R> { };

  // from individual char2 (requires conversion)
  template <typename U, typename V = Char2> struct IsChar2 {
    enum { OK = !ZuConversion<ZuNull, V>::Same &&
      ZuConversion<U, V>::Same &&
      !ZuConversion<U, wchar_t>::Same };
  };
  template <typename U, typename R = void>
  struct MatchChar2 : public ZuIfT<IsChar2<U>::OK, R> { };

  // from printable type (if this is a char array)
  template <typename U, typename V = Char> struct IsPrint {
    enum { OK = ZuEquivChar<char, V>::Same &&
      ZuPrint<U>::OK && !ZuPrint<U>::String };
  };
  template <typename U, typename R = void>
  struct MatchPrint : public ZuIfT<IsPrint<U>::OK, R> { };
  template <typename U, typename V = Char> struct IsPDelegate {
    enum { OK = ZuEquivChar<char, V>::Same && ZuPrint<U>::Delegate };
  };
  template <typename U, typename R = void>
  struct MatchPDelegate : public ZuIfT<IsPDelegate<U>::OK, R> { };
  template <typename U, typename V = Char> struct IsPBuffer {
    enum { OK = ZuEquivChar<char, V>::Same && ZuPrint<U>::Buffer };
  };
  template <typename U, typename R = void>
  struct MatchPBuffer : public ZuIfT<IsPBuffer<U>::OK, R> { };

  // from real primitive types other than chars (if this is a char array)
  template <typename U, typename V = T> struct IsReal {
    enum { OK = ZuEquivChar<char, V>::Same && !ZuEquivChar<U, V>::Same &&
      ZuTraits<U>::IsReal && ZuTraits<U>::IsPrimitive &&
      !ZuTraits<U>::IsArray };
  };
  template <typename U, typename R = void>
  struct MatchReal : public ZuIfT<IsReal<U>::OK, R> { };

  // from individual element
  template <typename U, typename V = T> struct IsElem {
    enum { OK = ZuConversion<U, V>::Same ||
      (!IsZtArray<U>::OK &&
       !IsString<U>::OK &&
       !IsArray<U>::OK &&
       !IsChar2<U>::OK &&
       !IsPrint<U>::OK &&
       !IsReal<U>::OK &&
       ZuConversion<U, V>::Exists) };
  };
  template <typename U, typename R = void>
  struct MatchElem : public ZuIfT<IsElem<U>::OK, R> { };

  // from individual element, for push()
  template <typename U, typename V = T> struct IsPushElem {
    enum { OK = ZuConversion<U, V>::Same ||
      (!IsZtArray<U>::OK &&
       !IsArray<U>::OK &&
       ZuConversion<U, V>::Exists) };
  };
  template <typename U, typename R = void>
  struct MatchPushElem : public ZuIfT<IsPushElem<U>::OK, R> { };

  // an unsigned|int|size_t parameter to the constructor is a buffer size
  template <typename U, typename V = T> struct IsCtorSize {
    enum { OK =
      ZuConversion<U, unsigned>::Same ||
      ZuConversion<U, int>::Same ||
      ZuConversion<U, size_t>::Same };
  };
  template <typename U, typename R = void>
  struct MatchCtorSize : public ZuIfT<IsCtorSize<U>::OK, R> { };

  // construction from individual element
  template <typename U, typename V = T, typename W = Char2> struct IsCtorElem {
    enum { OK =
      !IsZtArray<U>::OK &&
      !IsString<U>::OK &&
      !IsArray<U>::OK &&
      !IsChar2<U>::OK &&
      !IsPrint<U>::OK &&
      !IsReal<U>::OK &&
      !IsCtorSize<U>::OK &&
      ZuConversion<U, V>::Exists };
  };
  template <typename U, typename R = void>
  struct MatchCtorElem : public ZuIfT<IsCtorElem<U>::OK, R> { };

public:
  ZuInline ZtArray() { null_(); }
private:
  enum NoInit_ { NoInit };
  ZuInline ZtArray(NoInit_ _) { }
public:
  ZuInline ZtArray(const ZtArray &a) { ctor(a); }
  ZuInline ZtArray(ZtArray &&a) noexcept {
    if (!a.owned())
      shadow_(a.m_data, a.length());
    else {
      own_(a.m_data, a.length(), a.size(), a.mallocd());
      a.owned(false);
    }
  }
  ZuInline ZtArray(std::initializer_list<T> a) {
    copy_(a.begin(), a.size());
  }

  template <typename A> ZuInline ZtArray(A &&a) { ctor(ZuFwd<A>(a)); }

private:
  template <typename A_> struct Fwd_ZtArray {
    using A = typename ZuDecay<A_>::T;

    static void ctor_(ZtArray *this_, const A &a) {
      this_->copy_(a.m_data, a.length());
    }
    static void ctor_(ZtArray *this_, A &&a) {
      this_->move_(a.m_data, a.length());
    }

    static void assign_(ZtArray *this_, const A &a) {
      uint32_t oldLength = 0;
      T *oldData = this_->free_1(oldLength);
      this_->copy_(a.m_data, a.length());
      this_->free_2(oldData, oldLength);
    }
    static void assign_(ZtArray *this_, A &&a) {
      uint32_t oldLength = 0;
      T *oldData = this_->free_1(oldLength);
      this_->move_(a.m_data, a.length());
      this_->free_2(oldData, oldLength);
    }

    static ZtArray add_(const ZtArray *this_, const A &a) {
      return this_->add(a.m_data, a.length());
    }
    static ZtArray add_(const ZtArray *this_, A &&a) {
      return this_->add_mv(a.m_data, a.length());
    }

    static void splice_(ZtArray *this_,
	ZtArray *removed, int offset, int length, const A &a) {
      if (this_ == &a) {
	ZtArray a_ = a;
	this_->splice_cp_(removed, offset, length, a_.m_data, a_.length());
      } else
	this_->splice_cp_(removed, offset, length, a.m_data, a.length());
    }
    static void splice_(ZtArray *this_,
	ZtArray *removed, int offset, int length, A &&a) {
      this_->splice_mv_(removed, offset, length, a.m_data, a.length());
    }
  };
  template <typename A_> struct Fwd_Array {
    using A = typename ZuDecay<A_>::T;
    using Elem = typename ZuArrayT<A>::Elem;

    static void ctor_(ZtArray *this_, const A &a_) {
      ZuArray<const Elem> a(a_);
      this_->copy_(a.data(), a.length());
    }
    static void ctor_(ZtArray *this_, A &&a_) {
      ZuArray<Elem> a(a_);
      this_->move_(a.data(), a.length());
    }

    static void assign_(ZtArray *this_, const A &a_) {
      ZuArray<const Elem> a(a_);
      uint32_t oldLength = 0;
      T *oldData = this_->free_1(oldLength);
      this_->copy_(a.data(), a.length());
      this_->free_2(oldData, oldLength);
    }
    static void assign_(ZtArray *this_, A &&a_) {
      ZuArray<Elem> a(a_);
      uint32_t oldLength = 0;
      T *oldData = this_->free_1(oldLength);
      this_->move_(a.data(), a.length());
      this_->free_2(oldData, oldLength);
    }

    static ZtArray add_(const ZtArray *this_, const A &a_) {
      ZuArray<const Elem> a(a_);
      return this_->add(a.data(), a.length());
    }
    static ZtArray add_(const ZtArray *this_, A &&a_) {
      ZuArray<Elem> a(a_);
      return this_->add_mv(a.data(), a.length());
    }

    static void splice_(ZtArray *this_,
	ZtArray *removed, int offset, int length, const A &a_) {
      ZuArray<const Elem> a(a_);
      this_->splice_cp_(removed, offset, length, a.data(), a.length());
    }
    static void splice_(ZtArray *this_,
	ZtArray *removed, int offset, int length, A &&a_) {
      ZuArray<Elem> a(a_);
      this_->splice_mv_(removed, offset, length, a.data(), a.length());
    }
  };

  template <typename A> ZuInline typename MatchZtArray<A>::T ctor(A &&a)
    { Fwd_ZtArray<A>::ctor_(this, ZuFwd<A>(a)); }
  template <typename A> ZuInline typename MatchArray<A>::T ctor(A &&a)
    { Fwd_Array<A>::ctor_(this, ZuFwd<A>(a)); }

  template <typename S> ZuInline typename MatchStrLiteral<S>::T ctor(S &&s_)
    { ZuArray<const T> s(s_); shadow_(s.data(), s.length()); }
  template <typename S> ZuInline typename MatchString<S>::T ctor(S &&s_)
    { ZuArray<const T> s(s_); copy_(s.data(), s.length()); }

  template <typename S> ZuInline typename MatchChar2String<S>::T ctor(S &&s_) {
    ZuArray<const Char2> s(s_);
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (!o) { null_(); return; }
    alloc_(o, 0);
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(m_data, o), s));
  }
  template <typename C> typename MatchChar2<C>::T ctor(C c) {
    ZuArray<const Char2> s{&c, 1};
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (!o) { null_(); return; }
    alloc_(o, 0);
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(m_data, o), s));
  }

  template <typename P> ZuInline typename MatchPDelegate<P>::T ctor(const P &p)
    { null_(); ZuPrint<P>::print(*this, p); }
  template <typename P> ZuInline typename MatchPBuffer<P>::T ctor(const P &p) {
    unsigned o = ZuPrint<P>::length(p);
    if (!o) { null_(); return; }
    alloc_(o, 0);
    length_(ZuPrint<P>::print(reinterpret_cast<char *>(m_data), o, p));
  }

  template <typename V> ZuInline typename MatchCtorSize<V>::T ctor(V size) {
    if (!size) { null_(); return; }
    alloc_(size, 0);
  }

  template <typename R> ZuInline typename MatchCtorElem<R>::T ctor(R &&r) {
    unsigned z = grow_(0, 1);
    m_data = (T *)::malloc(z * sizeof(T));
    size_owned(z, 1);
    length_mallocd(1, 1);
    this->initItem(m_data, ZuFwd<R>(r));
  }

private:
  template <typename A> typename MatchZtArray<A>::T copy(const A &a) {
    copy_(a.m_data, a.length());
  }
  template <typename A> typename MatchArray<A>::T copy(A &&a_) {
    ZuArrayT<A> a(a_);
    copy_(a.data(), a.length());
  }

  template <typename S>
  ZuInline typename MatchAnyString<S>::T copy(S &&s) { ctor(ZuFwd<S>(s)); }
  template <typename S>
  ZuInline typename MatchChar2String<S>::T copy(S &&s) { ctor(ZuFwd<S>(s)); }
  template <typename C>
  ZuInline typename MatchChar2<C>::T copy(C c) { ctor(c); }
  template <typename R>
  ZuInline typename MatchElem<R>::T copy(R &&r) { ctor(ZuFwd<R>(r)); }

public:
  ZuInline ZtArray &operator =(const ZtArray &a)
    { assign(a); return *this; }
  ZuInline ZtArray &operator =(ZtArray &&a) noexcept {
    free_();
    new (this) ZtArray(ZuMv(a));
    return *this;
  }

  template <typename A>
  ZuInline ZtArray &operator =(A &&a) { assign(ZuFwd<A>(a)); return *this; }

  ZuInline ZtArray &operator =(std::initializer_list<T> a) {
    uint32_t oldLength = 0;
    T *oldData = free_1(oldLength);
    copy_(a.begin(), a.size());
    free_2(oldData, oldLength);
    return *this;
  }

private:
  template <typename A> typename MatchZtArray<A>::T assign(A &&a) {
    Fwd_ZtArray<A>::assign_(this, ZuFwd<A>(a));
  }
  template <typename A> typename MatchArray<A>::T assign(A &&a) {
    Fwd_Array<A>::assign_(this, ZuFwd<A>(a));
  }

  template <typename S> typename MatchStrLiteral<S>::T assign(S &&s_) {
    ZuArray<const T> s(s_);
    free_();
    shadow_(s.data(), s.length());
  }
  template <typename S> typename MatchString<S>::T assign(S &&s_) {
    ZuArray<const T> s(s_);
    uint32_t oldLength = 0;
    T *oldData = free_1(oldLength);
    copy_(s.data(), s.length());
    free_2(oldData, oldLength);
  }

  template <typename S> typename MatchChar2String<S>::T assign(S &&s_) {
    ZuArray<const Char2> s(s_);
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (!o) { null(); return; }
    if (!owned() || size() < o) size(o);
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(m_data, o), s));
  }
  template <typename C> typename MatchChar2<C>::T assign(C c) {
    ZuArray<const Char2> s{&c, 1};
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (!o) { null(); return; }
    if (!owned() || size() < o) size(o);
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(m_data, o), s));
  }

  template <typename P>
  ZuInline typename MatchPDelegate<P>::T assign(const P &p) {
    ZuPrint<P>::print(*this, p);
  }
  template <typename P>
  ZuInline typename MatchPBuffer<P>::T assign(const P &p) {
    unsigned o = ZuPrint<P>::length(p);
    if (!o) { null(); return; }
    if (!owned() || size() < o) size(o);
    length_(ZuPrint<P>::print(reinterpret_cast<char *>(m_data), o, p));
  }

  template <typename V> ZuInline typename MatchReal<V>::T assign(V v) {
    assign(ZuBoxed(v));
  }

  template <typename I> ZuInline typename MatchElem<I>::T assign(I &&i) {
    free_();
    ctor(ZuFwd<I>(i));
  }

public:
  template <typename A> ZtArray &operator -=(A &&a)
    { shadow(ZuFwd<A>(a)); return *this; }

private:
  template <typename A> typename MatchZtArray<A>::T shadow(const A &a) {
    if (this == &a) return;
    free_();
    shadow_(a.m_data, a.length());
  }
  template <typename A>
  typename MatchSameArray<A>::T shadow(A &&a_) {
    ZuArrayT<A> a(a_);
    free_();
    shadow_(a.data(), a.length());
  }

public:
  template <typename S>
  ZtArray(S &&s_, ZtIconv *iconv, typename ZuIsString<S>::T *_ = 0) {
    ZuArrayT<S> s(s_);
    convert_(s, iconv);
  }
  ZtArray(const Char *data, unsigned length, ZtIconv *iconv) {
    ZuArray<const Char> s(data, length);
    convert_(s, iconv);
  }
  ZtArray(const Char2 *data, unsigned length, ZtIconv *iconv) {
    ZuArray<const Char2> s(data, length);
    convert_(s, iconv);
  }

  ZtArray(unsigned length, unsigned size,
      bool initItems = !ZuTraits<T>::IsPrimitive) {
    if (!size) { null_(); return; }
    alloc_(size, length);
    if (initItems) this->initItems(m_data, length);
  }
  explicit ZtArray(const T *data, unsigned length) {
    if (!length) { null_(); return; }
    shadow_(data, length);
  }
  explicit ZtArray(Copy_ _, const T *data, unsigned length) {
    if (!length) { null_(); return; }
    copy_(data, length);
  }
  explicit ZtArray(Move_ _, T *data, unsigned length) {
    if (!length) { null_(); return; }
    move_(data, length);
  }
  explicit ZtArray(const T *data, unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    own_(data, length, size, true);
  }
  explicit ZtArray(
      const T *data, unsigned length, unsigned size, bool mallocd) {
    if (!size) { null_(); return; }
    own_(data, length, size, mallocd);
  }

  ~ZtArray() { free_(); }

// re-initializers

  void init() { free_(); init_(); }
  void init_() { null_(); }

  template <typename A> void init(A &&a) { assign(ZuFwd<A>(a)); }
  template <typename A> void init_(A &&a) { ctor(ZuFwd<A>(a)); }

  void init(
      unsigned length, unsigned size,
      bool initItems = !ZuTraits<T>::IsPrimitive) {
    if (this->size() < size || initItems) {
      free_();
      alloc_(size, length);
    } else
      length_(length);
    if (initItems) this->initItems(m_data, length);
  }
  void init_(
      unsigned length, unsigned size,
      bool initItems = !ZuTraits<T>::IsPrimitive) {
    if (!size) { null_(); return; }
    alloc_(size, length);
    if (initItems) this->initItems(m_data, length);
  }
  void init(const T *data, unsigned length) {
    free_();
    init_(data, length);
  }
  void init_(const T *data, unsigned length) {
    if (!length) { null_(); return; }
    shadow_(data, length);
  }
  void init(Copy_ _, const T *data, unsigned length) {
    uint32_t oldLength = 0;
    T *oldData = free_1(oldLength);
    init_(_, data, length);
    free_2(oldData, oldLength);
  }
  void init(Move_ _, T *data, unsigned length) {
    uint32_t oldLength = 0;
    T *oldData = free_1(oldLength);
    init_(_, data, length);
    free_2(oldData, oldLength);
  }
  void init_(Copy_ _, const T *data, unsigned length) {
    if (!length) { null_(); return; }
    copy_(data, length);
  }
  void init_(Move_ _, T *data, unsigned length) {
    if (!length) { null_(); return; }
    move_(data, length);
  }
  void init(const T *data, unsigned length, unsigned size) {
    free_();
    init_(data, length, size);
  }
  void init_(const T *data, unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    own_(data, length, size, true);
  }
  void init(
      const T *data, unsigned length, unsigned size, bool mallocd) {
    free_();
    init_(data, length, size, mallocd);
  }
  void init_(
      const T *data, unsigned length, unsigned size, bool mallocd) {
    if (!size) { null_(); return; }
    own_(data, length, size, mallocd);
  }

// internal initializers / finalizer

private:
  void null_() {
    m_data = 0;
    size_owned(0, 0);
    length_mallocd(0, 0);
  }

  void own_(
      const T *data, unsigned length, unsigned size, bool mallocd) {
    ZmAssert(size >= length);
    if (!size) {
      if (data && mallocd) ::free((void *)data);
      null_();
      return;
    }
    m_data = (T *)data;
    size_owned(size, 1);
    length_mallocd(length, mallocd);
  }

  void shadow_(const T *data, unsigned length) {
    if (!length) { null_(); return; }
    m_data = (T *)data;
    size_owned(length, 0);
    length_mallocd(length, 0);
  }

  void alloc_(unsigned size, unsigned length) {
    if (!size) { null_(); return; }
    m_data = (T *)::malloc(size * sizeof(T));
    size_owned(size, 1);
    length_mallocd(length, 1);
  }

  template <typename S> void copy_(const S *data, unsigned length) {
    if (!length) { null_(); return; }
    m_data = (T *)::malloc(length * sizeof(T));
    if (length) this->copyItems(m_data, data, length);
    size_owned(length, 1);
    length_mallocd(length, 1);
  }

  template <typename S> void move_(S *data, unsigned length) {
    if (!length) { null_(); return; }
    m_data = (T *)::malloc(length * sizeof(T));
    if (length) this->moveItems(m_data, data, length);
    size_owned(length, 1);
    length_mallocd(length, 1);
  }

  template <typename S> void convert_(const S &s, ZtIconv *iconv);

  void free_() {
    if (m_data && owned()) {
      this->destroyItems(m_data, length());
      if (mallocd()) ::free(m_data);
    }
  }
  T *free_1(uint32_t &length_mallocd) {
    if (!m_data || !owned()) return 0;
    length_mallocd = m_length_mallocd;
    return m_data;
  }
  void free_2(T *data, uint32_t length_mallocd) {
    if (data) {
      this->destroyItems(data, length_mallocd & ~(1U<<31U));
      if (length_mallocd>>31U) ::free(data);
    }
  }

public:
// truncation (to minimum size)

  void truncate() {
    size(length());
    unsigned n = length();
    if (!m_data || size() <= n) return;
    T *newData = (T *)::malloc(n * sizeof(T));
    this->moveItems(newData, m_data, n);
    free_();
    m_data = newData;
    mallocd(1);
    size_owned(length(), 1);
  }

// array / ptr operators

  ZuInline T &item(int i) { return m_data[i]; }
  ZuInline const T &item(int i) const { return m_data[i]; }

  ZuInline T &operator [](int i) { return m_data[i]; }
  ZuInline const T &operator [](int i) const { return m_data[i]; }

  ZuInline operator T *() { return m_data; }
  ZuInline operator const T *() const { return m_data; }

// accessors

  using iterator = T *;
  using const_iterator = const T *;
  ZuInline const T *begin() const { return m_data; }
  ZuInline const T *end() const { return &m_data[length()]; }
  T *begin() {
    return const_cast<T *>(static_cast<const ZtArray &>(*this).begin());
  }
  T *end() {
    return const_cast<T *>(static_cast<const ZtArray &>(*this).end());
  }

  ZuInline T *data() { return m_data; }
  ZuInline const T *data() const { return m_data; }

  ZuInline unsigned length() const { return m_length_mallocd & ~(1U<<31U); }
  ZuInline unsigned size() const { return m_size_owned & ~(1U<<31U); }

  ZuInline bool mallocd() const { return m_length_mallocd>>31U; }
  ZuInline bool owned() const { return m_size_owned>>31U; }

private:
  ZuInline void length_(unsigned v) {
    m_length_mallocd = (m_length_mallocd & (1U<<31U)) | (uint32_t)v;
  }
  ZuInline void mallocd(bool v) {
    m_length_mallocd = (m_length_mallocd & ~(1U<<31U)) | (((uint32_t)v)<<31U);
  }
  ZuInline void length_mallocd(unsigned l, bool m) {
    m_length_mallocd = l | (((uint32_t)m)<<31U);
  }
  ZuInline void size_(unsigned v) {
    m_size_owned = (m_size_owned & (1U<<31U)) | (uint32_t)v;
  }
  ZuInline void owned(bool v) {
    m_size_owned = (m_size_owned & ~(1U<<31U)) | (((uint32_t)v)<<31U);
  }
  ZuInline void size_owned(unsigned z, bool o) {
    m_size_owned = z | (((uint32_t)o)<<31U);
  }

public:
  ZuInline T *data(bool move) {
    if (move) owned(0);
    return m_data;
  }

// reset to null array

  void null() {
    free_();
    null_();
  }

// reset without freeing

  void clear() {
    if (length()) length(0);
  }

// set length

  ZuInline void length(unsigned length) {
    if (!owned() || length > size()) size(length);
    if constexpr (!ZuTraits<T>::IsPrimitive) {
      unsigned n = this->length();
      if (length > n) {
	this->initItems(m_data + n, length - n);
      } else if (length < n) {
	this->destroyItems(m_data + length, n - length);
      }
    }
    length_(length);
  }
  void length(unsigned length, bool initItems) {
    if (!owned() || length > size()) size(length);
    if (initItems) {
      unsigned n = this->length();
      if (length > n) {
	this->initItems(m_data + n, length - n);
      } else if (length < n) {
	this->destroyItems(m_data + length, n - length);
      }
    }
    length_(length);
  }

// set size

  T *ensure(unsigned z) {
    if (ZuLikely(z <= size())) return m_data;
    return size(z);
  }
  T *size(unsigned z) {
    if (!z) { null(); return 0; }
    if (owned() && z == size()) return m_data;
    T *newData = (T *)::malloc(z * sizeof(T));
    unsigned n = z;
    if (n > length()) n = length();
    if (m_data) {
      if (n) this->moveItems(newData, m_data, n);
      free_();
    } else
      n = 0;
    m_data = newData;
    size_owned(z, 1);
    length_mallocd(n, 1);
    return newData;
  }

// hash()

  ZuInline uint32_t hash() const { return Ops::hash(m_data, length()); }

// buffer access

  ZuInline auto buf() {
    return ZuArray{data(), size()};
  }
  ZuInline auto cbuf() const {
    return ZuArray{data(), length()};
  }

// comparison

  ZuInline bool operator !() const { return !length(); }

  template <typename A>
  ZuInline typename MatchZtArray<A, int>::T cmp(const A &a) const {
    if (this == &a) return 0;
    return cmp(a.m_data, a.length());
  }
  template <typename A>
  ZuInline typename MatchArray<A, int>::T cmp(A &&a_) const {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    return cmp(a.data(), a.length());
  }
  template <typename S>
  ZuInline typename MatchAnyString<S, int>::T cmp(S &&s_) const {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    return cmp(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename MatchChar2String<S, int>::T cmp(const S &s) const {
    return cmp(ZtArray{s});
  }

  int cmp(const T *a, unsigned n) const {
    if (!a) return !!m_data;
    if (!m_data) return -1;
    unsigned l = length();
    if (int i = Ops::cmp(m_data, a, l < n ? l : n)) return i;
    return l - n;
  }

  template <typename A>
  ZuInline typename MatchZtArray<A, bool>::T less(const A &a) const {
    if (this == &a) return true;
    return less(a.m_data, a.length());
  }
  template <typename A>
  ZuInline typename MatchArray<A, bool>::T less(A &&a_) const {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    return less(a.data(), a.length());
  }
  template <typename S>
  ZuInline typename MatchAnyString<S, bool>::T less(S &&s_) const {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    return less(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename MatchChar2String<S, bool>::T less(const S &s) const {
    return less(ZtArray{s});
  }

  ZuInline bool less(const T *a, unsigned n) const {
    if (!a) return false;
    if (!m_data) return true;
    unsigned l = length();
    if (int i = Ops::cmp(m_data, a, l < n ? l : n)) return i < 0;
    return l < n;
  }

  template <typename A>
  ZuInline typename MatchZtArray<A, bool>::T greater(const A &a) const {
    if (this == &a) return true;
    return greater(a.m_data, a.length());
  }
  template <typename A>
  ZuInline typename MatchArray<A, bool>::T greater(A &&a_) const {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    return greater(a.data(), a.length());
  }
  template <typename S>
  ZuInline typename MatchAnyString<S, bool>::T greater(S &&s_) const {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    return greater(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename MatchChar2String<S, bool>::T greater(const S &s) const {
    return greater(ZtArray{s});
  }

  ZuInline bool greater(const T *a, unsigned n) const {
    if (!m_data) return false;
    if (!a) return true;
    unsigned l = length();
    if (int i = Ops::cmp(m_data, a, l < n ? l : n)) return i > 0;
    return l > n;
  }

  template <typename A>
  ZuInline typename MatchZtArray<A, bool>::T equals(const A &a) const {
    if (this == &a) return true;
    return equals(a.m_data, a.length());
  }
  template <typename A>
  ZuInline typename MatchArray<A, bool>::T equals(A &&a_) const {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    return equals(a.data(), a.length());
  }
  template <typename S>
  ZuInline typename MatchAnyString<S, bool>::T equals(S &&s_) const {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    return equals(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename MatchChar2String<S, bool>::T equals(const S &s) const {
    return equals(ZtArray{s});
  }

  ZuInline bool equals(const T *a, unsigned n) const {
    if (!a) return !m_data;
    if (!m_data) return false;
    if (length() != n) return false;
    return Ops::equals(m_data, a, n);
  }

  template <typename A>
  ZuInline bool operator ==(const A &a) const { return equals(a); }
  template <typename A>
  ZuInline bool operator !=(const A &a) const { return !equals(a); }
  template <typename A>
  ZuInline bool operator >(const A &a) const { return greater(a); }
  template <typename A>
  ZuInline bool operator >=(const A &a) const { return !less(a); }
  template <typename A>
  ZuInline bool operator <(const A &a) const { return less(a); }
  template <typename A>
  ZuInline bool operator <=(const A &a) const { return !greater(a); }

// +, += operators

  template <typename A>
  ZuInline ZtArray<T, Cmp> operator +(const A &a) const { return add(a); }

private:
  template <typename A>
  ZuInline typename MatchZtArray<A, ZtArray<T, Cmp> >::T add(A &&a) const {
    return Fwd_ZtArray<A>::add_(this, ZuFwd<A>(a));
  }
  template <typename A>
  ZuInline typename MatchArray<A, ZtArray<T, Cmp> >::T add(A &&a) const {
    return Fwd_Array<A>::add_(this, ZuFwd<A>(a));
  }

  template <typename S>
  ZuInline typename MatchAnyString<S, ZtArray<T, Cmp> >::T add(S &&s_) const
    { ZuArrayT<S> s(ZuFwd<S>(s_)); return add(s.data(), s.length()); }
  template <typename S>
  ZuInline typename MatchChar2String<S, ZtArray<T, Cmp> >::T add(S &&s) const
    { return add(ZtArray{ZuFwd<S>(s)}); }
  template <typename C>
  ZuInline typename MatchChar2<C, ZtArray<T, Cmp> >::T add(C c) const
    { return add(ZtArray(c)); }

  template <typename P>
  ZuInline typename MatchPDelegate<P, ZtArray<T, Cmp> >::T add(P &&p) const
    { return add(ZtArray(ZuFwd<P>(p))); }
  template <typename P>
  ZuInline typename MatchPBuffer<P, ZtArray<T, Cmp> >::T add(P &&p) const
    { return add(ZtArray(ZuFwd<P>(p))); }

  template <typename R>
  ZuInline typename MatchElem<R, ZtArray<T, Cmp> >::T add(R &&r) const {
    unsigned n = length();
    unsigned z = grow_(n, n + 1);
    T *newData = (T *)::malloc(z * sizeof(T));
    if (n) this->copyItems(newData, m_data, n);
    this->initItem(newData + n, ZuFwd<R>(r));
    return ZtArray<T, Cmp>(newData, n + 1, z);
  }

  ZtArray<T, Cmp> add(const T *data, unsigned length) const {
    unsigned n = this->length();
    unsigned z = n + length;
    if (ZuUnlikely(!z)) return ZtArray<T, Cmp>();
    T *newData = (T *)::malloc(z * sizeof(T));
    if (n) this->copyItems(newData, m_data, n);
    if (length) this->copyItems(newData + n, data, length);
    return ZtArray<T, Cmp>(newData, z, z);
  }
  ZtArray<T, Cmp> add_mv(T *data, unsigned length) const {
    unsigned n = this->length();
    unsigned z = n + length;
    if (ZuUnlikely(!z)) return ZtArray<T, Cmp>();
    T *newData = (T *)::malloc(z * sizeof(T));
    if (n) this->copyItems(newData, m_data, n);
    if (length) this->moveItems(newData + n, data, length);
    return ZtArray<T, Cmp>(newData, z, z);
  }

public:
  template <typename A> ZuInline ZtArray &operator +=(A &&a)
    { append_(ZuFwd<A>(a)); return *this; }
  template <typename A> ZuInline ZtArray &operator <<(A &&a)
    { append_(ZuFwd<A>(a)); return *this; }

private:
  template <typename A>
  ZuInline typename MatchZtArray<A>::T append_(A &&a) {
    Fwd_ZtArray<A>::splice_(this, 0, length(), 0, ZuFwd<A>(a));
  }
  template <typename A>
  ZuInline typename MatchArray<A>::T append_(A &&a) {
    Fwd_Array<A>::splice_(this, 0, length(), 0, ZuFwd<A>(a));
  }

  template <typename S> ZuInline typename MatchAnyString<S>::T append_(S &&s_) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    splice_cp_(0, length(), 0, s.data(), s.length());
  }

  template <typename S>
  ZuInline typename MatchChar2String<S>::T append_(S &&s)
    { append_(ZtArray{ZuFwd<S>(s)}); }
  template <typename C>
  ZuInline typename MatchChar2<C>::T append_(C c)
    { append_(ZtArray(c)); }

  template <typename P>
  ZuInline typename MatchPDelegate<P>::T append_(const P &p) {
    ZuPrint<P>::print(*this, p);
  }
  template <typename P>
  typename MatchPBuffer<P>::T append_(const P &p) {
    unsigned n = length();
    unsigned o = ZuPrint<P>::length(p);
    if (!owned() || size() < n + o) size(n + o);
    length(n + ZuPrint<P>::print(reinterpret_cast<char *>(m_data) + n, o, p));
  }

  template <typename V> ZuInline typename MatchReal<V>::T append_(V v) {
    append_(ZuBoxed(v));
  }

  template <typename I> ZuInline typename MatchElem<I>::T append_(I &&i) {
    this->initItem(push(), ZuFwd<I>(i));
  }

public:
  ZuInline void append(const T *data, unsigned length) {
    if (data) splice_cp_(0, this->length(), 0, data, length);
  }
  ZuInline void append_mv(T *data, unsigned length) {
    if (data) splice_mv_(0, this->length(), 0, data, length);
  }

// splice()

public:
  template <typename A>
  ZuInline void splice(
      ZtArray &removed, int offset, int length, A &&replace) {
    splice_(&removed, offset, length, ZuFwd<A>(replace));
  }

  template <typename A>
  ZuInline void splice(int offset, int length, A &&replace) {
    splice_(nullptr, offset, length, ZuFwd<A>(replace));
  }

  ZuInline void splice(ZtArray &removed, int offset, int length) {
    splice_del_(&removed, offset, length);
  }

  ZuInline void splice(int offset, int length) {
    splice_del_(nullptr, offset, length);
  }

private:
  template <typename A>
  typename MatchZtArray<A>::T splice_(
      ZtArray *removed, int offset, int length, A &&a) {
    Fwd_ZtArray<A>::splice_(this, removed, offset, length, ZuFwd<A>(a));
  }
  template <typename A>
  ZuInline typename MatchArray<A>::T splice_(
      ZtArray *removed, int offset, int length, A &&a) {
    Fwd_Array<A>::splice_(this, removed, offset, length, ZuFwd<A>(a));
  }

  template <typename S>
  ZuInline typename MatchAnyString<S>::T splice_(
      ZtArray *removed, int offset, int length, const S &s_) {
    ZuArrayT<S> s(s_);
    splice_cp_(removed, offset, length, s.data(), s.length());
  }

  template <typename S>
  ZuInline typename MatchChar2String<S>::T splice_(
      ZtArray *removed, int offset, int length, const S &s) {
    splice_(removed, offset, length, ZtArray(s));
  }
  template <typename C>
  ZuInline typename MatchChar2<C>::T splice_(
      ZtArray *removed, int offset, int length, C c) {
    splice_(removed, offset, length, ZtArray(c));
  }

  template <typename R>
  ZuInline typename MatchElem<R>::T splice_(
      ZtArray *removed, int offset, int length, R &&r_) {
    T r{ZuFwd<R>(r_)};
    splice_mv_(removed, offset, length, &r, 1);
  }

public:
  template <typename R>
  ZuInline void splice(
      ZtArray &removed, int offset, int length,
      const R *replace, unsigned rlength) {
    splice_cp_(&removed, offset, length, replace, rlength);
  }
  template <typename R>
  ZuInline void splice_mv(
      ZtArray &removed, int offset, int length,
      R *replace, unsigned rlength) {
    splice_mv_(&removed, offset, length, replace, rlength);
  }

  template <typename R>
  ZuInline void splice(
      int offset, int length, const R *replace, unsigned rlength) {
    splice_cp_(nullptr, offset, length, replace, rlength);
  }
  template <typename R>
  ZuInline void splice_mv(
      int offset, int length, R *replace, unsigned rlength) {
    splice_mv_(nullptr, offset, length, replace, rlength);
  }

  template <typename A> ZuInline typename MatchZtArray<A>::T push(A &&a)
    { Fwd_ZtArray<A>::splice_(this, nullptr, length(), 0, ZuFwd<A>(a)); }
  template <typename A> ZuInline typename MatchArray<A>::T push(A &&a)
    { Fwd_Array<A>::splice_(this, nullptr, length(), 0, ZuFwd<A>(a)); }

  void *push() {
    unsigned n = length();
    unsigned z = size();
    if (!owned() || n + 1 > z) {
      z = grow_(z, n + 1);
      T *newData = (T *)::malloc(z * sizeof(T));
      this->moveItems(newData, m_data, n);
      free_();
      m_data = newData;
      size_owned(z, 1);
      length_mallocd(n + 1, 1);
    } else
      length_(n + 1);
    return (void *)(m_data + n);
  }
  template <typename I>
  ZuInline typename MatchPushElem<I, T *>::T push(I &&i) {
    auto ptr = push();
    if (ZuLikely(ptr)) this->initItem(ptr, ZuFwd<I>(i));
    return static_cast<T *>(ptr);
  }
  T pop() {
    unsigned n = length();
    if (!n) return ZuCmp<T>::null();
    T v;
    if (ZuUnlikely(!owned())) {
      v = m_data[--n];
    } else {
      v = ZuMv(m_data[--n]);
      this->destroyItem(m_data + n);
    }
    length_(n);
    return v;
  }
  T shift() {
    unsigned n = length();
    if (!n) return ZuCmp<T>::null();
    T v;
    if (ZuUnlikely(!owned())) {
      v = m_data[0];
      ++m_data;
      --n;
    } else {
      v = ZuMv(m_data[0]);
      this->destroyItem(m_data);
      this->moveItems(m_data, m_data + 1, --n);
    }
    length_(n);
    return v;
  }

  template <typename A> typename MatchZtArray<A>::T unshift(A &&a)
    { Fwd_ZtArray<A>::splice_(this, nullptr, 0, 0, ZuFwd<A>(a)); }
  template <typename A> typename MatchArray<A>::T unshift(A &&a)
    { Fwd_Array<A>::splice_(this, nullptr, 0, 0, ZuFwd<A>(a)); }

  void *unshift() {
    unsigned n = length();
    unsigned z = size();
    if (!owned() || n + 1 > z) {
      z = grow_(z, n + 1);
      T *newData = (T *)::malloc(z * sizeof(T));
      this->moveItems(newData + 1, m_data, n);
      free_();
      m_data = newData;
      size_owned(z, 1);
      length_mallocd(n + 1, 1);
    } else {
      this->moveItems(m_data + 1, m_data, n);
      length_(n + 1);
    }
    return (void *)m_data;
  }
  template <typename I> void unshift(I &&i) {
    this->initItem(unshift(), ZuFwd<I>(i));
  }

private:
  void splice_del_(
      ZtArray *removed,
      int offset,
      int length) {
    unsigned n = this->length();
    unsigned z = size();
    if (offset < 0) { if ((offset += n) < 0) offset = 0; }
    if (length < 0) { if ((length += (n - offset)) < 0) length = 0; }

    if (offset > (int)n) {
      if (removed) removed->clear();
      if (!owned() || offset > (int)z) {
	z = grow_(z, offset);
	size(z);
      }
      this->initItems(m_data + n, offset - n);
      length_(offset);
      return;
    }

    if (offset + length > (int)n) length = n - offset;

    int l = n - length;

    if (l > 0 && (!owned() || l > (int)z)) {
      z = grow_(z, l);
      if (removed) removed->init(Move, m_data + offset, length);
      T *newData = (T *)::malloc(z * sizeof(T));
      this->moveItems(newData, m_data, offset);
      if (offset + length < (int)n)
	this->moveItems(
	    newData + offset,
	    m_data + offset + length,
	    n - (offset + length));
      free_();
      m_data = newData;
      size_owned(z, 1);
      length_mallocd(l, 1);
      return;
    }

    if (removed) removed->init(Move, m_data + offset, length);
    this->destroyItems(m_data + offset, length);
    if (l > 0) {
      if (offset + length < (int)n)
	this->moveItems(
	    m_data + offset,
	    m_data + offset + length,
	    n - (offset + length));
    }
    length_(l);
  }

  template <typename R>
  void splice_cp_(
      ZtArray *removed,
      int offset,
      int length,
      const R *replace,
      unsigned rlength) {
    unsigned n = this->length();
    unsigned z = size();
    if (offset < 0) { if ((offset += n) < 0) offset = 0; }
    if (length < 0) { if ((length += (n - offset)) < 0) length = 0; }

    if (offset > (int)n) {
      if (removed) removed->clear();
      if (!owned() || offset + (int)rlength > (int)z) {
	z = grow_(z, offset + rlength);
	size(z);
      }
      this->initItems(m_data + n, offset - n);
      this->copyItems(m_data + offset, replace, rlength);
      length_(offset + rlength);
      return;
    }

    if (offset + length > (int)n) length = n - offset;

    int l = n + rlength - length;

    if (l > 0 && (!owned() || l > (int)z)) {
      z = grow_(z, l);
      if (removed) removed->init(Move, m_data + offset, length);
      T *newData = (T *)::malloc(z * sizeof(T));
      this->moveItems(newData, m_data, offset);
      this->copyItems(newData + offset, replace, rlength);
      if (offset + length < (int)n)
	this->moveItems(
	    newData + offset + rlength,
	    m_data + offset + length,
	    n - (offset + length));
      free_();
      m_data = newData;
      size_owned(z, 1);
      length_mallocd(l, 1);
      return;
    }

    if (removed) removed->init(Move, m_data + offset, length);
    this->destroyItems(m_data + offset, length);
    if (l > 0) {
      if ((int)rlength != length && offset + length < (int)n)
	this->moveItems(
	    m_data + offset + rlength,
	    m_data + offset + length,
	    n - (offset + length));
      this->copyItems(m_data + offset, replace, rlength);
    }
    length_(l);
  }

  template <typename R>
  void splice_mv_(
      ZtArray *removed,
      int offset,
      int length,
      R *replace,
      unsigned rlength) {
    unsigned n = this->length();
    unsigned z = size();
    if (offset < 0) { if ((offset += n) < 0) offset = 0; }
    if (length < 0) { if ((length += (n - offset)) < 0) length = 0; }

    if (offset > (int)n) {
      if (removed) removed->clear();
      if (!owned() || offset + (int)rlength > (int)z) {
	z = grow_(z, offset + rlength);
	size(z);
      }
      this->initItems(m_data + n, offset - n);
      this->moveItems(m_data + offset, replace, rlength);
      length_(offset + rlength);
      return;
    }

    if (offset + length > (int)n) length = n - offset;

    int l = n + rlength - length;

    if (l > 0 && (!owned() || l > (int)z)) {
      z = grow_(z, l);
      if (removed) removed->init(Move, m_data + offset, length);
      T *newData = (T *)::malloc(z * sizeof(T));
      this->moveItems(newData, m_data, offset);
      this->moveItems(newData + offset, replace, rlength);
      if ((int)rlength != length && offset + length < (int)n)
	this->moveItems(
	    newData + offset + rlength,
	    m_data + offset + length,
	    n - (offset + length));
      free_();
      m_data = newData;
      size_owned(z, 1);
      length_mallocd(l, 1);
      return;
    }

    if (removed) removed->init(Move, m_data + offset, length);
    this->destroyItems(m_data + offset, length);
    if (l > 0) {
      if ((int)rlength != length && offset + length < (int)n)
	this->moveItems(
	    m_data + offset + rlength,
	    m_data + offset + length,
	    n - (offset + length));
      this->moveItems(m_data + offset, replace, rlength);
    }
    length_(l);
  }

// iterate

public:
  template <typename Fn> void iterate(Fn fn) {
    unsigned n = length();
    for (unsigned i = 0; i < n; i++) fn(m_data[i]);
  }

// grep

  // l(item) -> bool // item is spliced out if true
  template <typename L> void grep(L l) {
    for (unsigned i = 0, n = length(); i < n; i++) {
      if (l(m_data[i])) {
	splice_del_(0, i, 1);
	--i, --n;
      }
    }
  }

// growth algorithm

  void grow(unsigned length) {
    unsigned o = size();
    if (ZuUnlikely(length > o)) size(grow_(o, length));
    this->length(length);
  }
  void grow(unsigned length, bool initItems) {
    unsigned o = size();
    if (ZuUnlikely(length > o)) size(grow_(o, length));
    this->length(length, initItems);
  }
private:
  ZuInline static unsigned grow_(unsigned o, unsigned n) {
    return ZtPlatform::grow(o * sizeof(T), n * sizeof(T)) / sizeof(T);
  }

public:
  // traits

  struct Traits : public ZuBaseTraits<ZtArray> {
    using Elem = T;
    enum {
      IsArray = 1, IsPrimitive = 0,
      IsString =
	ZuConversion<char, T>::Same ||
	ZuConversion<wchar_t, T>::Same,
      IsWString = ZuConversion<wchar_t, T>::Same
    };
    template <typename U = ZtArray>
    static typename ZuNotConst<U, T *>::T data(U &a) { return a.data(); }
    static const T *data(const ZtArray &a) { return a.data(); }
    static unsigned length(const ZtArray &a) { return a.length(); }
  };
  friend Traits ZuTraitsType(ZtArray *);

private:
  uint32_t		m_size_owned;	// allocated size and owned flag
  uint32_t		m_length_mallocd;// initialized length and malloc'd flag
  T			*m_data;	// data buffer
};

template <typename T, class Cmp>
template <typename S>
ZuInline void ZtArray<T, Cmp>::convert_(const S &s, ZtIconv *iconv) {
  null_();
  iconv->convert(*this, s);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// generic printing
template <class Cmp>
struct ZuPrint<ZtArray<char, Cmp> > : public ZuPrintString { };

#endif /* ZtArray_HPP */
