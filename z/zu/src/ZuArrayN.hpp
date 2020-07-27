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

// fixed-size arrays for use in structs and passing by value
//
// * cached length

#ifndef ZuArrayN_HPP
#define ZuArrayN_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <initializer_list>

#include <zlib/ZuAssert.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuArray.hpp>
#include <zlib/ZuArrayFn.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuString.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuCan.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800 4996 4348)
#endif

ZuCan(append, ZuArrayN_CanAppend_);
template <typename R, typename T> struct ZuArrayN_CanAppend {
  enum _ { OK =
    ZuArrayN_CanAppend_<R, void (R::*)(const T *, int)>::OK ||
    ZuArrayN_CanAppend_<R, void (R::*)(const T *, unsigned)>::OK ||
    ZuArrayN_CanAppend_<R, void (R::*)(const T *, size_t)>::OK ||
    ZuArrayN_CanAppend_<R, R &(R::*)(const T *, int)>::OK ||
    ZuArrayN_CanAppend_<R, R &(R::*)(const T *, unsigned)>::OK ||
    ZuArrayN_CanAppend_<R, R &(R::*)(const T *, size_t)>::OK
  };
};
template <typename T> struct ZuArrayN_CanAppend<void, T> {
  enum _ { OK = 0 };
};

struct ZuArrayN___ { };

template <typename T> struct ZuArrayN__ : public ZuArrayN___ {
protected:
  using Nop_ = enum { Nop };
  ZuArrayN__(Nop_ _) { }

  ZuArrayN__(unsigned size) :
    m_size(size), m_length(0) { }

  ZuArrayN__(unsigned size, unsigned length) :
    m_size(size), m_length(length) { }

  uint32_t	m_size;
  uint32_t	m_length;
};

template <typename T_, typename Cmp_, typename ArrayN>
class ZuArrayN_ : public ZuArrayN__<T_>, public ZuArrayFn<T_, Cmp_> {
  ZuArrayN_();
  ZuArrayN_(const ZuArrayN_ &);
  ZuArrayN_ &operator =(const ZuArrayN_ &);
  bool operator *() const;

  using Base = ZuArrayN__<T_>;

public:
  using T = T_;
  using Cmp = Cmp_;

  enum Nop_ { Nop };

private:
  template <typename U, typename R = void, typename V = T,
    bool B = ZuPrint<typename ZuDecay<U>::T>::Delegate>
  struct MatchPDelegate;
  template <typename U, typename R>
  struct MatchPDelegate<U, R, char, true> { using T = R; };
  template <typename U, typename R = void, typename V = T,
    bool B = ZuPrint<typename ZuDecay<U>::T>::Buffer>
  struct MatchPBuffer;
  template <typename U, typename R>
  struct MatchPBuffer<U, R, char, true> { using T = R; };

  template <typename U, typename R = void,
    typename V = T,
    bool S = ZuTraits<U>::IsString,
    bool W = ZuTraits<U>::IsWString> struct ToString;
  template <typename U, typename R>
  struct ToString<U, R, char, true, false> { using T = R; };
  template <typename U, typename R>
  struct ToString<U, R, wchar_t, true, true> { using T = R; };

protected:
  ZuInline ~ZuArrayN_() { dtor(); }

  ZuInline ZuArrayN_(unsigned size) : Base(size) { }

  ZuInline ZuArrayN_(Nop_ _) : Base(Base::Nop) { }

  ZuInline ZuArrayN_(unsigned size, unsigned length, bool initItems) :
      Base(size, length) {
    if (Base::m_length > Base::m_size) Base::m_length = Base::m_size;
    if (initItems) this->initItems(data(), Base::m_length);
  }

  ZuInline void dtor() {
    this->destroyItems(data(), Base::m_length);
  }

  template <typename A>
  typename ZuConvertible<A, T>::T init(const A *a, unsigned length) {
    if (ZuUnlikely(length > Base::m_size)) length = Base::m_size;
    if (ZuUnlikely(!a))
      length = 0;
    else if (ZuLikely(length))
      this->copyItems(data(), a, length);
    Base::m_length = length;
  }
  template <typename A>
  typename ZuConvertible<A, T>::T init_mv(A *a, unsigned length) {
    if (ZuUnlikely(length > Base::m_size)) length = Base::m_size;
    if (ZuUnlikely(!a))
      length = 0;
    else if (ZuLikely(length))
      this->moveItems(data(), a, length);
    Base::m_length = length;
  }
  template <typename E>
  ZuInline typename ZuConvertible<E, T>::T init(E &&e) {
    this->initItem(data(), ZuFwd<E>(e));
    Base::m_length = 1;
  }

  template <typename P> typename MatchPDelegate<P>::T init(const P &p) {
    ZuPrint<P>::print(*static_cast<ArrayN *>(this), p);
  }
  template <typename P> typename MatchPBuffer<P>::T init(const P &p) {
    unsigned length = ZuPrint<P>::length(p);
    if (length > Base::m_size)
      Base::m_length = 0;
    else
      Base::m_length = ZuPrint<P>::print(data(), length, p);
  }

  template <typename A>
  typename ZuConvertible<A, T>::T append_(const A *a, unsigned length) {
    if (Base::m_length + length > Base::m_size)
      length = Base::m_size - Base::m_length;
    if (a && length) this->copyItems(data() + Base::m_length, a, length);
    Base::m_length += length;
  }
  template <typename A>
  typename ZuConvertible<A, T>::T append_mv_(A *a, unsigned length) {
    if (Base::m_length + length > Base::m_size)
      length = Base::m_size - Base::m_length;
    if (a && length) this->moveItems(data() + Base::m_length, a, length);
    Base::m_length += length;
  }
  template <typename E>
  typename ZuConvertible<E, T>::T append_(E &&e) {
    if (Base::m_length >= Base::m_size) return;
    this->initItem(data() + Base::m_length, ZuFwd<E>(e));
    ++Base::m_length;
  }

  template <typename P>
  typename MatchPDelegate<P>::T append_(const P &p) {
    ZuPrint<P>::print(*static_cast<ArrayN *>(this), p);
  }
  template <typename P>
  typename MatchPBuffer<P>::T append_(const P &p) {
    unsigned length = ZuPrint<P>::length(p);
    if (Base::m_length + length > Base::m_size) return;
    Base::m_length += ZuPrint<P>::print(data() + Base::m_length, length, p);
  }

public:
// array/ptr operators

  ZuInline T &operator [](int i) { return data()[i]; }
  ZuInline const T &operator [](int i) const { return data()[i]; }

// accessors

  ZuInline const T *begin() const { return data(); }
  ZuInline const T *end() const { return data() + Base::m_length; }

  ZuInline unsigned size() const { return Base::m_size; }
  ZuInline unsigned length() const { return Base::m_length; }

// reset to null

  ZuInline void null() { Base::m_length = 0; }

// set length

  void length(
      unsigned length, bool initItems = !ZuTraits<T>::IsPrimitive) {
    if (length > Base::m_size) length = Base::m_size;
    if (initItems) {
      if (length > Base::m_length) {
	this->initItems(data() + Base::m_length, length - Base::m_length);
      } else if (length < Base::m_length) {
	this->destroyItems(data() + length, Base::m_length - length);
      }
    }
    Base::m_length = length;
  }

// push/pop/shift/unshift

  template <typename I> T *push(I &&i) {
    if (Base::m_length >= Base::m_size) return 0;
    T *ptr = &(data()[Base::m_length++]);
    this->initItem((void *)ptr, ZuFwd<I>(i));
    return ptr;
  }
  T pop() {
    if (!Base::m_length) return Cmp::null();
    T t = ZuMv(data()[--Base::m_length]);
    this->destroyItem(data() + Base::m_length);
    return t;
  }
  T shift() {
    if (!Base::m_length) return Cmp::null();
    T t = ZuMv(data()[0]);
    this->destroyItem(data());
    this->moveItems(data(), data() + 1, --Base::m_length);
    return t;
  }
  template <typename I> T *unshift(I &&i) {
    if (Base::m_length >= Base::m_size) return 0;
    this->moveItems(data() + 1, data(), Base::m_length++);
    T *ptr = data();
    this->initItem((void *)ptr, ZuFwd<I>(i));
    return ptr;
  }

  void splice(int offset, int length) {
    splice_(offset, length, (void *)0);
  }
  template <typename U>
  void splice(int offset, int length, U &removed) {
    splice_(offset, length, &removed);
  }

private:
  template <typename U, typename R = void,
    bool B = ZuConversion<void, U>::Same> struct SpliceVoid;
  template <typename U, typename R>
  struct SpliceVoid<U, R, true> { using T = R; };

  template <typename U, typename R = void, typename V = T,
    bool B = ZuArrayN_CanAppend<U, V>::OK> struct SpliceAppend;
  template <typename U, typename R>
  struct SpliceAppend<U, R, T, true> { using T = R; };

  template <typename U, typename R = void, typename V = T,
    bool B =
      !ZuConversion<void, U>::Same &&
      !ZuArrayN_CanAppend<U, V>::OK> struct SpliceDefault;
  template <typename U, typename R>
  struct SpliceDefault<U, R, T, true> { using T = R; };

  template <typename U>
  void splice_(int offset, int length, U *removed) {
    if (ZuUnlikely(!length)) return;
    if (offset < 0) { if ((offset += Base::m_length) < 0) offset = 0; }
    if (offset >= (int)Base::m_size) return;
    if (length < 0) { if ((length += (Base::m_length - offset)) <= 0) return; }
    if (offset + length > (int)Base::m_size)
      if (!(length = Base::m_size - offset)) return;
    if (offset > (int)Base::m_length) {
      this->initItems(data() + Base::m_length, offset - Base::m_length);
      Base::m_length = offset;
      return;
    }
    if (offset + length > (int)Base::m_length)
      if (!(length = Base::m_length - offset)) return;
    if (removed) splice__(data() + offset, length, removed);
    this->destroyItems(data() + offset, length);
    this->moveItems(
	data() + offset,
	data() + offset + length,
	Base::m_length - (offset + length));
    Base::m_length -= length;
  }
  template <typename U>
  typename SpliceVoid<U>::T
      splice__(const T *, unsigned, U *) { }
  template <typename U>
  typename SpliceAppend<U>::T
      splice__(const T *data, unsigned length, U *removed) {
    removed->append_(data, length);
  }
  template <typename U>
  typename SpliceDefault<U>::T
      splice__(const T *data, unsigned length, U *removed) {
    for (unsigned i = 0; i < length; i++) *removed << data[i];
  }

// comparison

  ZuInline bool operator !() const { return !Base::m_length; }
  ZuOpBool

protected:
  ZuInline bool same(const ZuArrayN_ &a) const { return this == &a; }
  template <typename A> ZuInline bool same(const A &a) const { return false; }

public:
  template <typename A>
  ZuInline int cmp(const A &a) const {
    if (same(a)) return 0;
    return ZuArray{data(), length()}.cmp(a);
  }
  template <typename A>
  ZuInline bool less(const A &a) const {
    return !same(a) && ZuArray{data(), length()}.less(a);
  }
  template <typename A>
  ZuInline bool greater(const A &a) const {
    return !same(a) && ZuArray{data(), length()}.greater(a);
  }
  template <typename A>
  ZuInline bool equals(const A &a) const {
    return same(a) || ZuArray{data(), length()}.equals(a);
  }

// hash

  ZuInline uint32_t hash() const {
    return ZuHash<ArrayN>::hash(*static_cast<const ArrayN *>(this));
  }

// conversions
 
  template <typename S>
  ZuInline typename ToString<S, S>::T as() const {
    return ZuTraits<S>::make(data(), Base::m_length);
  }

private:
  struct Align : public Base { T m_data[1]; };

public:
  ZuInline T *data() {
    return reinterpret_cast<Align *>(this)->m_data;
  }
  ZuInline const T *data() const {
    return reinterpret_cast<const Align *>(this)->m_data;
  }
};

// ZuArrayN<T, N> can be cast and used as ZuArrayN<T>

template <typename T_, unsigned N_ = 1, class Cmp_ = ZuCmp<T_> >
class ZuArrayN : public ZuArrayN_<T_, Cmp_, ZuArrayN<T_, N_, Cmp_> > {
  ZuAssert(N_ < (1U<<16) - 1U);

public:
  using T = T_;
  using Cmp = Cmp_;
  using Base = ZuArrayN_<T, Cmp, ZuArrayN>;
  enum { N = N_ };
  enum Move_ { Move };

private:
  template <typename U, typename R = void, typename V = T,
    bool A = ZuConversion<ZuArrayN___, U>::Base> struct MatchArrayN;
  template <typename U, typename R>
  struct MatchArrayN<U, R, T, true> { using T = R; };

  template <typename U, typename R = void, typename V = T,
    bool A = !ZuConversion<ZuArrayN___, U>::Base &&
      (!ZuTraits<U>::IsString || ZuTraits<U>::IsWString) &&
      ZuTraits<U>::IsArray> struct MatchArray;
  template <typename U, typename R>
  struct MatchArray<U, R, T, true> { using T = R; };

  template <typename U, typename R = void, typename V = T,
    bool B = ZuPrint<U>::OK && !ZuPrint<U>::String> struct MatchPrint;
  template <typename U, typename R>
  struct MatchPrint<U, R, char, true> { using T = R; };
  template <typename U, typename R = void, typename V = T,
    bool B = ZuTraits<U>::IsBoxed> struct MatchBoxed { };
  template <typename U, typename R>
  struct MatchBoxed<U, R, char, true> { using T = R; };

  template <typename U, typename R = void, typename V = T,
    bool S = ZuTraits<U>::IsString && !ZuTraits<U>::IsWString
    > struct MatchString;
  template <typename U, typename R>
  struct MatchString<U, R, char, true> { using T = R; };

  template <typename U, typename R = void, typename V = T,
    bool E =
      (!ZuConversion<char, V>::Same ||
       ((!ZuPrint<U>::OK || // avoid MatchPrint ambiguity
	 ZuPrint<U>::String) &&
	(!ZuTraits<U>::IsString || // avoid MatchString ambiguity
	 ZuTraits<U>::IsWString))) &&
      !ZuTraits<U>::IsArray && // avoid MatchArray ambiguity
      ZuConversion<U, V>::Exists
    > struct MatchElem;
  template <typename U, typename R>
  struct MatchElem<U, R, T, true> { using T = R; };

  template <typename U, typename R = void, typename V = T,
    bool E = ZuConversion<U, unsigned>::Same ||
	     ZuConversion<U, int>::Same ||
	     ZuConversion<U, size_t>::Same
    > struct CtorLength;
  template <typename U, typename R>
  struct CtorLength<U, R, T, true> { using T = R; };

  template <typename U, typename R = void, typename V = T,
    bool E =
      (!ZuConversion<char, V>::Same ||
       ((!ZuPrint<U>::OK || // avoid MatchPrint ambiguity
	 ZuPrint<U>::String) &&
	(!ZuTraits<U>::IsString || // avoid MatchString ambiguity
	 ZuTraits<U>::IsWString))) &&
      !ZuTraits<U>::IsArray && // avoid MatchArray ambiguity
      !ZuConversion<U, unsigned>::Same && // avoid CtorLength ambiguity
      !ZuConversion<U, int>::Same &&      // ''
      !ZuConversion<U, size_t>::Same &&   // ''
      ZuConversion<U, V>::Exists
    > struct CtorElem;
  template <typename U, typename R>
  struct CtorElem<U, R, T, true> { using T = R; };

public:
  ZuInline ZuArrayN() : Base(N) { }

  ZuInline ZuArrayN(const ZuArrayN &a) : Base(Base::Nop) {
    Base::m_size = N;
    this->init(a.data(), a.length());
  }
  ZuInline ZuArrayN &operator =(const ZuArrayN &a) {
    if (this != &a) {
      this->dtor();
      this->init(a.data(), a.length());
    }
    return *this;
  }

  ZuInline ZuArrayN(ZuArrayN &&a) : Base(Base::Nop) {
    Base::m_size = N;
    this->init_mv(a.data(), a.length());
  }
  ZuInline ZuArrayN &operator =(ZuArrayN &&a) {
    if (this != &a) {
      this->dtor();
      this->init_mv(a.data(), a.length());
    }
    return *this;
  }

  ZuInline ZuArrayN(std::initializer_list<T> a) : Base(Base::Nop) {
    Base::m_size = N;
    this->init(a.begin(), a.size());
  }

  // ZuArrayN types
  template <typename A>
  ZuInline ZuArrayN(A &&a, typename MatchArrayN<A>::T *_ = 0) :
      Base(Base::Nop) {
    Base::m_size = N;
    ZuMvCp<A>::mvcp(ZuFwd<A>(a),
      [this](auto &&a) { this->init_mv(a.data(), a.length()); },
      [this](const auto &a) { this->init(a.data(), a.length()); });
  }
  template <typename A>
  ZuInline typename MatchArrayN<A, ZuArrayN &>::T operator =(A &&a) {
    this->dtor();
    ZuMvCp<A>::mvcp(ZuFwd<A>(a),
      [this](auto &&a) { this->init_mv(a.data(), a.length()); },
      [this](const auto &a) { this->init(a.data(), a.length()); });
    return *this;
  }
  template <typename A>
  ZuInline typename MatchArrayN<A, ZuArrayN &>::T operator +=(A &&a) {
    ZuMvCp<A>::mvcp(ZuFwd<A>(a),
      [this](auto &&a) { this->append_mv_(a.data(), a.length()); },
      [this](const auto &a) { this->append_(a.data(), a.length()); });
    return *this;
  }
  template <typename A>
  ZuInline typename MatchArrayN<A, ZuArrayN &>::T operator <<(A &&a) {
    ZuMvCp<A>::mvcp(ZuFwd<A>(a),
      [this](auto &&a) { this->append_mv_(a.data(), a.length()); },
      [this](const auto &a) { this->append_(a.data(), a.length()); });
    return *this;
  }

  // array types (other than ZuArrayN<>)
  template <typename A>
  ZuInline ZuArrayN(A &&a, typename MatchArray<A>::T *_ = 0) :
      Base(Base::Nop) {
    Base::m_size = N;
    ZuMvCp<A>::mvcp(ZuFwd<A>(a),
      [this](auto &&a_) {
	using Elem = typename ZuTraits<decltype(a_)>::Elem;
	ZuArray<Elem> a(a_);
	this->init_mv(const_cast<Elem *>(a.data()), a.length());
      },
      [this](const auto &a_) {
	using Elem = typename ZuTraits<decltype(a_)>::Elem;
	ZuArray<Elem> a(a_);
	this->init(a.data(), a.length());
      });
  }
  template <typename A>
  ZuInline typename MatchArray<A, ZuArrayN &>::T operator =(A &&a) {
    this->dtor();
    ZuMvCp<A>::mvcp(ZuFwd<A>(a),
      [this](auto &&a_) {
	using Elem = typename ZuTraits<decltype(a_)>::Elem;
	ZuArray<Elem> a(a_);
	this->init_mv(const_cast<Elem *>(a.data()), a.length());
      },
      [this](const auto &a_) {
	using Elem = typename ZuTraits<decltype(a_)>::Elem;
	ZuArray<Elem> a(a_);
	this->init(a.data(), a.length());
      });
    return *this;
  }
  template <typename A>
  typename MatchArray<A, ZuArrayN &>::T operator +=(A &&a) {
    return operator <<(ZuFwd<A>(a));
  }
  template <typename A>
  typename MatchArray<A, ZuArrayN &>::T operator <<(A &&a) {
    ZuMvCp<A>::mvcp(ZuFwd<A>(a),
      [this](auto &&a_) {
	using Elem = typename ZuTraits<decltype(a_)>::Elem;
	ZuArray<Elem> a(a_);
	this->append_mv_(const_cast<Elem *>(a.data()), a.length());
      },
      [this](const auto &a_) {
	using Elem = typename ZuTraits<decltype(a_)>::Elem;
	ZuArray<Elem> a(a_);
	this->append_(a.data(), a.length());
      });
    return *this;
  }

  // printable types (if this is a char array)
  template <typename P>
  ZuInline ZuArrayN(const P &p, typename MatchPrint<P>::T *_ = 0) :
      Base(Base::Nop) {
    Base::m_size = N;
    this->init(p);
  }
  template <typename P>
  ZuInline typename MatchPrint<P, ZuArrayN &>::T operator =(const P &p) {
    // this->dtor();
    this->init(p);
    return *this;
  }
  template <typename P>
  ZuInline typename MatchPrint<P, ZuArrayN &>::T operator +=(const P &p) {
    this->append_(p);
    return *this;
  }
  template <typename P>
  ZuInline typename MatchPrint<P, ZuArrayN &>::T operator <<(const P &p) {
    this->append_(p);
    return *this;
  }
  template <typename B>
  ZuInline typename MatchBoxed<B, ZuArrayN &>::T
      print(const B &b, int precision = -1, int comma = 0, int pad = -1) {
    unsigned length = b.length(precision, comma);
    if (Base::m_length + length > Base::m_size) return;
    Base::m_length +=
      b.print(this->data() + Base::m_length, length, precision, comma, pad);
    return *this;
  }

  // string types (if this is a char array)
  template <typename S>
  ZuInline ZuArrayN(S &&s_, typename MatchString<S>::T *_ = 0) :
      Base(Base::Nop) {
    ZuString s(ZuFwd<S>(s_));
    Base::m_size = N;
    this->init(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename MatchString<S, ZuArrayN &>::T operator =(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    // this->dtor();
    this->init(s.data(), s.length());
    return *this;
  }
  template <typename S>
  ZuInline typename MatchString<S, ZuArrayN &>::T operator +=(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    this->append_(s.data(), s.length());
    return *this;
  }
  template <typename S>
  ZuInline typename MatchString<S, ZuArrayN &>::T operator <<(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    this->append_(s.data(), s.length());
    return *this;
  }

  // length
  template <typename L>
  ZuInline ZuArrayN(L l, bool initItems = !ZuTraits<T>::IsPrimitive,
      typename CtorLength<L>::T *_ = 0) : Base(N, l, initItems) { }

private:
  enum Size_ { Size };
  template <typename Z>
  ZuInline ZuArrayN(Size_, Z z,
      typename CtorLength<Z>::T *_ = 0) : Base(z, 0, 0) { }

public:
  // element types 
  template <typename E>
  ZuInline ZuArrayN(E &&e, typename CtorElem<E>::T *_ = 0) :
      Base(Base::Nop) {
    Base::m_size = N;
    this->init(ZuFwd<E>(e));
  }
  template <typename E>
  ZuInline typename MatchElem<E, ZuArrayN &>::T operator =(E &&e) {
    this->dtor();
    this->init(ZuFwd<E>(e));
    return *this;
  }
  template <typename E>
  ZuInline typename MatchElem<E, ZuArrayN &>::T operator +=(E &&e) {
    this->append_(ZuFwd<E>(e));
    return *this;
  }
  template <typename E>
  ZuInline typename MatchElem<E, ZuArrayN &>::T operator <<(E &&e) {
    this->append_(ZuFwd<E>(e));
    return *this;
  }

  // arrays as ptr, length
  template <typename A>
  ZuInline ZuArrayN(const A *a, unsigned length,
    typename ZuConvertible<A, T>::T *_ = 0) :
      Base(Base::Nop) {
    Base::m_size = N;
    this->init(a, length);
  }
  ZuInline ZuArrayN(Move_ _, T *a, unsigned length) : Base(Base::Nop) {
    Base::m_size = N;
    this->init_mv(a, length);
  }

  // append()
  template <typename A>
  ZuInline typename ZuConvertible<A, T>::T append(const A *a, unsigned length) {
    this->append_(a, length);
  }
  template <typename A>
  ZuInline typename ZuConvertible<A, T>::T append_mv(A *a, unsigned length) {
    this->append_mv_(a, length);
  }

  template <typename A>
  ZuInline bool operator ==(const A &a) const { return this->equals(a); }
  template <typename A>
  ZuInline bool operator !=(const A &a) const { return !this->equals(a); }
  template <typename A>
  ZuInline bool operator >(const A &a) const { return this->greater(a); }
  template <typename A>
  ZuInline bool operator >=(const A &a) const { return !this->less(a); }
  template <typename A>
  ZuInline bool operator <(const A &a) const { return this->less(a); }
  template <typename A>
  ZuInline bool operator <=(const A &a) const { return !this->greater(a); }

private:
  char	*m_data[N * sizeof(T)];
};

template <typename T_, unsigned N, typename Cmp>
struct ZuTraits<ZuArrayN<T_, N, Cmp> > :
    public ZuGenericTraits<ZuArrayN<T_, N, Cmp> > {
  using T = ZuArrayN<T_, N, Cmp>;
  using Elem = T_;
  enum {
    IsArray = 1, IsPrimitive = 0,
    IsPOD = ZuTraits<T>::IsPOD,
    IsString =
      ZuConversion<char, Elem>::Same ||
      ZuConversion<wchar_t, Elem>::Same,
    IsWString = ZuConversion<wchar_t, Elem>::Same,
    IsComparable = 1, IsHashable = 1
  };
  ZuInline static const Elem *data(const T &a) { return a.data(); }
  ZuInline static unsigned length(const T &a) { return a.length(); }
};

// generic printing
template <unsigned N>
struct ZuPrint<ZuArrayN<char, N> > : public ZuPrintString { };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuArrayN_HPP */
