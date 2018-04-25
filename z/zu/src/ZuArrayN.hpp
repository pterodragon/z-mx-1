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

// fixed-size arrays for use in structs and passing by value
//
// * cached length

#ifndef ZuArrayN_HPP
#define ZuArrayN_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <initializer_list>

#include <ZuAssert.hpp>
#include <ZuTraits.hpp>
#include <ZuConversion.hpp>
#include <ZuIfT.hpp>
#include <ZuArray.hpp>
#include <ZuArrayFn.hpp>
#include <ZuCmp.hpp>
#include <ZuString.hpp>
#include <ZuPrint.hpp>
#include <ZuCan.hpp>

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
  typedef enum { Nop } Nop_;
  inline ZuArrayN__(Nop_ _) { }

  inline ZuArrayN__(unsigned size) :
    m_size(size), m_length(0) { }

  inline ZuArrayN__(unsigned size, unsigned length) :
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

  typedef ZuArrayN__<T_> Base;

public:
  typedef T_ T;
  typedef Cmp_ Cmp;
  typedef ZuArrayFn<T, Cmp> Ops;

private:
  template <typename U, typename R = void, typename V = T,
    bool B = ZuPrint<U>::Delegate> struct FromPDelegate;
  template <typename U, typename R>
  struct FromPDelegate<U, R, char, true> { typedef R T; };
  template <typename U, typename R = void, typename V = T,
    bool B = ZuPrint<U>::Buffer> struct FromPBuffer;
  template <typename U, typename R>
  struct FromPBuffer<U, R, char, true> { typedef R T; };

  template <typename U, typename R = void,
    typename V = T,
    bool S = ZuTraits<U>::IsString,
    bool W = ZuTraits<U>::IsWString> struct ToString;
  template <typename U, typename R>
  struct ToString<U, R, char, true, false> { typedef R T; };
  template <typename U, typename R>
  struct ToString<U, R, wchar_t, true, true> { typedef R T; };

protected:
  ZuInline ~ZuArrayN_() {
    this->destroyItems(data(), Base::m_length);
  }

  ZuInline ZuArrayN_(unsigned size) : Base(size) { }

  typedef enum { Nop } Nop_;
  ZuInline ZuArrayN_(Nop_ _) : Base(Base::Nop) { }

  ZuInline ZuArrayN_(unsigned size, unsigned length, bool initItems) :
      Base(size, length) {
    if (Base::m_length > Base::m_size) Base::m_length = Base::m_size;
    if (initItems) this->initItems(data(), Base::m_length);
  }

  template <typename A>
  inline typename ZuConvertible<A, T>::T init(const A *a, unsigned length) {
    if (ZuUnlikely(length > Base::m_size)) length = Base::m_size;
    if (ZuUnlikely(!a))
      length = 0;
    else if (ZuLikely(length))
      this->copyItems(data(), a, length);
    Base::m_length = length;
  }
  template <typename E>
  ZuInline typename ZuConvertible<E, T>::T init(E &&e) {
    this->initItem(data(), ZuFwd<E>(e));
    Base::m_length = 1;
  }

  template <typename P> inline typename FromPDelegate<P>::T init(const P &p) {
    ZuPrint<P>::print(*static_cast<ArrayN *>(this), p);
  }
  template <typename P> inline typename FromPBuffer<P>::T init(const P &p) {
    unsigned length = ZuPrint<P>::length(p);
    if (length > Base::m_size)
      Base::m_length = 0;
    else
      Base::m_length = ZuPrint<P>::print(data(), length, p);
  }

  template <typename A>
  inline typename ZuConvertible<A, T>::T append_(const A *a, unsigned length) {
    if (Base::m_length + length > Base::m_size)
      length = Base::m_size - Base::m_length;
    if (a && length) this->copyItems(data() + Base::m_length, a, length);
    Base::m_length += length;
  }
  template <typename E>
  inline typename ZuConvertible<E, T>::T append_(E &&e) {
    if (Base::m_length >= Base::m_size) return;
    this->initItem(data() + Base::m_length, ZuFwd<E>(e));
    ++Base::m_length;
  }

  template <typename P>
  inline typename FromPDelegate<P>::T append_(const P &p) {
    ZuPrint<P>::print(*static_cast<ArrayN *>(this), p);
  }
  template <typename P>
  inline typename FromPBuffer<P>::T append_(const P &p) {
    unsigned length = ZuPrint<P>::length(p);
    if (Base::m_length + length > Base::m_size) return;
    Base::m_length += ZuPrint<P>::print(data() + Base::m_length, length, p);
  }

public:
// array/ptr operators

  ZuInline T &operator [](int i) { return data()[i]; }
  ZuInline const T &operator [](int i) const { return data()[i]; }

// accessors

  ZuInline unsigned length() const { return Base::m_length; }

// reset to null string

  ZuInline void null() { Base::m_length = 0; }

// set length

  inline void length(
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

  template <typename I> inline T *push(I &&i) {
    if (Base::m_length >= Base::m_size) return 0;
    T *ptr = &(data()[Base::m_length++]);
    this->initItem((void *)ptr, ZuFwd<I>(i));
    return ptr;
  }
  inline T pop() {
    if (!Base::m_length) return Cmp::null();
    T t = ZuMv(data()[--Base::m_length]);
    this->destroyItem(data() + Base::m_length);
    return t;
  }
  inline T shift() {
    if (!Base::m_length) return Cmp::null();
    T t = ZuMv(data()[0]);
    this->destroyItem(data());
    this->moveItems(data(), data() + 1, --Base::m_length);
    return t;
  }
  template <typename I> inline T *unshift(I &&i) {
    if (Base::m_length >= Base::m_size) return 0;
    this->moveItems(data() + 1, data(), Base::m_length++);
    T *ptr = data();
    this->initItem((void *)ptr, ZuFwd<I>(i));
    return ptr;
  }

  inline void splice(int offset, int length) {
    splice_(offset, length, (void *)0);
  }
  template <typename U>
  inline void splice(int offset, int length, U &removed) {
    splice_(offset, length, &removed);
  }

private:
  template <typename U, typename R = void,
    bool B = ZuConversion<void, U>::Same> struct SpliceVoid;
  template <typename U, typename R>
  struct SpliceVoid<U, R, true> { typedef R T; };

  template <typename U, typename R = void, typename V = T,
    bool B = ZuArrayN_CanAppend<U, V>::OK> struct SpliceAppend;
  template <typename U, typename R>
  struct SpliceAppend<U, R, T, true> { typedef R T; };

  template <typename U, typename R = void, typename V = T,
    bool B =
      !ZuConversion<void, U>::Same &&
      !ZuArrayN_CanAppend<U, V>::OK> struct SpliceDefault;
  template <typename U, typename R>
  struct SpliceDefault<U, R, T, true> { typedef R T; };

  template <typename U>
  inline void splice_(int offset, int length, U *removed) {
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
  inline typename SpliceVoid<U>::T
      splice__(const T *, unsigned, U *) { }
  template <typename U>
  inline typename SpliceAppend<U>::T
      splice__(const T *data, unsigned length, U *removed) {
    removed->append_(data, length);
  }
  template <typename U>
  inline typename SpliceDefault<U>::T
      splice__(const T *data, unsigned length, U *removed) {
    for (unsigned i = 0; i < length; i++) *removed << data[i];
  }

// comparison

  ZuInline bool operator !() const { return !Base::m_length; }

protected:
  ZuInline bool same(const ZuArrayN_ &a) const { return this == &a; }
  template <typename A> ZuInline bool same(const A &a) const { return false; }

public:
  template <typename A>
  ZuInline int cmp(const A &a) const {
    if (same(a)) return 0;
    return ZuCmp<ArrayN>::cmp(*static_cast<const ArrayN *>(this), a);
  }

  template <typename A>
  ZuInline bool equals(const A &a) const {
    if (same(a)) return true;
    return ZuCmp<ArrayN>::equals(*static_cast<const ArrayN *>(this), a);
  }

  template <typename A>
  ZuInline bool operator ==(const A &a) const { return equals(a); }
  template <typename A>
  ZuInline bool operator !=(const A &a) const { return !equals(a); }
  template <typename A>
  ZuInline bool operator >(const A &a) const { return cmp(a) > 0; }
  template <typename A>
  ZuInline bool operator >=(const A &a) const { return cmp(a) >= 0; }
  template <typename A>
  ZuInline bool operator <(const A &a) const { return cmp(a) < 0; }
  template <typename A>
  ZuInline bool operator <=(const A &a) const { return cmp(a) <= 0; }

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
  ZuInline T *data() { return ((Align *)this)->m_data; }
  ZuInline const T *data() const { return ((const Align *)this)->m_data; }
};

// ZuArrayN<T, N> can be cast and used as ZuArrayN<T>

template <typename T_, unsigned N_ = 1, class Cmp_ = ZuCmp<T_> >
class ZuArrayN : public ZuArrayN_<T_, Cmp_, ZuArrayN<T_, N_, Cmp_> > {
  ZuAssert(N_ < (1U<<16) - 1U);
  struct Private { };

public:
  typedef T_ T;
  typedef Cmp_ Cmp;
  typedef ZuArrayN_<T, Cmp, ZuArrayN> Base;
  enum { N = N_ };

private:
  template <typename U, typename R = void, typename V = T,
    bool A = ZuConversion<ZuArrayN___, U>::Base> struct FromArrayN;
  template <typename U, typename R>
  struct FromArrayN<U, R, T, true> { typedef R T; };

  template <typename U, typename R = void, typename V = T,
    bool A = !ZuConversion<ZuArrayN___, U>::Base &&
      (!ZuTraits<U>::IsString || ZuTraits<U>::IsWString) &&
      ZuTraits<U>::IsArray> struct FromArray;
  template <typename U, typename R>
  struct FromArray<U, R, T, true> { typedef R T; };

  template <typename U, typename R = void, typename V = T,
    bool B = ZuPrint<U>::OK && !ZuPrint<U>::String> struct FromPrint;
  template <typename U, typename R>
  struct FromPrint<U, R, char, true> { typedef R T; };
  template <typename U, typename R = void, typename V = T,
    bool B = ZuTraits<U>::IsBoxed> struct FromBoxed { };
  template <typename U, typename R>
  struct FromBoxed<U, R, char, true> { typedef R T; };

  template <typename U, typename R = void, typename V = T,
    bool S = ZuTraits<U>::IsString && !ZuTraits<U>::IsWString
    > struct FromString;
  template <typename U, typename R>
  struct FromString<U, R, char, true> { typedef R T; };

  template <typename U, typename R = void, typename V = T,
    bool E =
      (!ZuConversion<char, V>::Same ||
       ((!ZuPrint<U>::OK || // avoid FromPrint ambiguity
	 ZuPrint<U>::String) &&
	(!ZuTraits<U>::IsString || // avoid FromString ambiguity
	 ZuTraits<U>::IsWString))) &&
      !ZuTraits<U>::IsArray && // avoid FromArray ambiguity
      ZuConversion<U, V>::Exists
    > struct FromElem;
  template <typename U, typename R>
  struct FromElem<U, R, T, true> { typedef R T; };

  template <typename U, typename R = Private, typename V = T,
    bool E = ZuConversion<U, unsigned>::Same ||
	     ZuConversion<U, int>::Same ||
	     ZuConversion<U, size_t>::Same
    > struct CtorLength;
  template <typename U, typename R>
  struct CtorLength<U, R, T, true> { typedef R T; };

  template <typename U, typename R = Private, typename V = T,
    bool E =
      (!ZuConversion<char, V>::Same ||
       ((!ZuPrint<U>::OK || // avoid FromPrint ambiguity
	 ZuPrint<U>::String) &&
	(!ZuTraits<U>::IsString || // avoid FromString ambiguity
	 ZuTraits<U>::IsWString))) &&
      !ZuTraits<U>::IsArray && // avoid FromArray ambiguity
      !ZuConversion<U, unsigned>::Same && // avoid CtorLength ambiguity
      !ZuConversion<U, int>::Same &&      // ''
      !ZuConversion<U, size_t>::Same &&   // ''
      ZuConversion<U, V>::Exists
    > struct CtorElem;
  template <typename U, typename R>
  struct CtorElem<U, R, T, true> { typedef R T; };

public:
  ZuInline ZuArrayN() : Base(N) { }

  ZuInline ZuArrayN(const ZuArrayN &a) : Base(Base::Nop) {
    Base::m_size = N;
    this->init(a.data(), a.length());
  }
  ZuInline ZuArrayN &operator =(const ZuArrayN &a) {
    if (this != &a) this->init(a.data(), a.length());
    return *this;
  }

  ZuInline ZuArrayN(std::initializer_list<T> a) : Base(Base::Nop) {
    Base::m_size = N;
    this->init(a.begin(), a.size());
  }

  // ZuArrayN types
  template <typename A>
  ZuInline ZuArrayN(const A &a, typename FromArrayN<A, Private>::T *_ = 0) :
      Base(Base::Nop) {
    Base::m_size = N;
    this->init(a.data(), a.length());
  }
  template <typename A>
  ZuInline typename FromArrayN<A, ZuArrayN &>::T operator =(const A &a) {
    this->init(a.data(), a.length());
    return *this;
  }
  template <typename A>
  ZuInline typename FromArrayN<A, ZuArrayN &>::T operator +=(const A &a) {
    this->append_(a.data(), a.length());
    return *this;
  }
  template <typename A>
  ZuInline typename FromArrayN<A, ZuArrayN &>::T operator <<(const A &a) {
    this->append_(a.data(), a.length());
    return *this;
  }

  // array types (other than ZuArrayN<>)
  template <typename A>
  ZuInline ZuArrayN(A &&a_, typename FromArray<A, Private>::T *_ = 0) :
      Base(Base::Nop) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    Base::m_size = N;
    this->init(a.data(), a.length());
  }
  template <typename A>
  ZuInline typename FromArray<A, ZuArrayN &>::T operator =(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    this->init(a.data(), a.length());
    return *this;
  }
  template <typename A>
  ZuInline typename FromArray<A, ZuArrayN &>::T operator +=(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    this->append_(a.data(), a.length());
    return *this;
  }
  template <typename A>
  ZuInline typename FromArray<A, ZuArrayN &>::T operator <<(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    this->append_(a.data(), a.length());
    return *this;
  }

  // printable types (if this is a char array)
  template <typename P>
  ZuInline ZuArrayN(const P &p, typename FromPrint<P, Private>::T *_ = 0) :
      Base(Base::Nop) {
    Base::m_size = N;
    this->init(p);
  }
  template <typename P>
  ZuInline typename FromPrint<P, ZuArrayN &>::T operator =(const P &p) {
    this->init(p);
    return *this;
  }
  template <typename P>
  ZuInline typename FromPrint<P, ZuArrayN &>::T operator +=(const P &p) {
    this->append_(p);
    return *this;
  }
  template <typename P>
  ZuInline typename FromPrint<P, ZuArrayN &>::T operator <<(const P &p) {
    this->append_(p);
    return *this;
  }
  template <typename B>
  ZuInline typename FromBoxed<B, ZuArrayN &>::T
      print(const B &b, int precision = -1, int comma = 0, int pad = -1) {
    unsigned length = b.length(precision, comma);
    if (Base::m_length + length > Base::m_size) return;
    Base::m_length +=
      b.print(this->data() + Base::m_length, length, precision, comma, pad);
    return *this;
  }

  // string types (if this is a char array)
  template <typename S>
  ZuInline ZuArrayN(S &&s_, typename FromString<S, Private>::T *_ = 0) :
      Base(Base::Nop) {
    ZuString s(ZuFwd<S>(s_));
    Base::m_size = N;
    this->init(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename FromString<S, ZuArrayN &>::T operator =(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    this->init(s.data(), s.length());
    return *this;
  }
  template <typename S>
  ZuInline typename FromString<S, ZuArrayN &>::T operator +=(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    this->append_(s.data(), s.length());
    return *this;
  }
  template <typename S>
  ZuInline typename FromString<S, ZuArrayN &>::T operator <<(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    this->append_(s.data(), s.length());
    return *this;
  }

  // length
  template <typename L>
  ZuInline ZuArrayN(L l, bool initItems = !ZuTraits<T>::IsPrimitive,
      typename CtorLength<L>::T *_ = 0) : Base(N, l, initItems) { }

private:
  template <typename> friend struct ZuAlloca;
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
  ZuInline typename FromElem<E, ZuArrayN &>::T operator =(E &&e) {
    this->init(ZuFwd<E>(e));
    return *this;
  }
  template <typename E>
  ZuInline typename FromElem<E, ZuArrayN &>::T operator +=(E &&e) {
    this->append_(ZuFwd<E>(e));
    return *this;
  }
  template <typename E>
  ZuInline typename FromElem<E, ZuArrayN &>::T operator <<(E &&e) {
    this->append_(ZuFwd<E>(e));
    return *this;
  }

  // arrays as ptr, length
  template <typename A>
  ZuInline ZuArrayN(const A *a, unsigned length,
    typename ZuConvertible<A, T, Private>::T *_ = 0) :
      Base(Base::Nop) {
    Base::m_size = N;
    this->init(a, length);
  }

  // append()
  template <typename A>
  ZuInline typename ZuConvertible<A, T>::T append(const A *a, unsigned length) {
    this->append_(a, length);
  }

private:
  T	m_data[N];
};

template <typename T_, unsigned N, typename Cmp>
struct ZuTraits<ZuArrayN<T_, N, Cmp> > :
    public ZuGenericTraits<ZuArrayN<T_, N, Cmp> > {
  typedef ZuArrayN<T_, N, Cmp> T;
  typedef T_ Elem;
  enum {
    IsArray = 1, IsPrimitive = 0,
    IsPOD = ZuTraits<T>::IsPOD,
    IsString =
      ZuConversion<char, Elem>::Same ||
      ZuConversion<wchar_t, Elem>::Same,
    IsWString = ZuConversion<wchar_t, Elem>::Same,
    IsHashable = 1, IsComparable = 1
  };
  inline static T make(const T *data, unsigned length) {
    if (!data) return T();
    return T(data, length);
  }
  inline static const Elem *data(const T &a) { return a.data(); }
  inline static unsigned length(const T &a) { return a.length(); }
};

// generic printing
template <unsigned N> struct ZuPrint<ZuArrayN<char, N> > :
  public ZuPrintString<ZuArrayN<char, N> > { };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuArrayN_HPP */
