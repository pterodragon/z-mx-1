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
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuCan.hpp>
#include <zlib/ZuNormChar.hpp>
#include <zlib/ZuUTF.hpp>

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

  ZuArrayN__() : m_length{0} { }

  ZuArrayN__(unsigned length) : m_length{length} { }

  uint32_t	m_length;
};

template <typename T_> struct ZuArrayN_Char2 { using T = ZuNull; };
template <> struct ZuArrayN_Char2<char> { using T = wchar_t; };
template <> struct ZuArrayN_Char2<wchar_t> { using T = char; };

template <typename T_, unsigned N_, typename Cmp_, typename ArrayN>
class ZuArrayN_ : public ZuArrayN__<T_>, public ZuArrayFn<T_, Cmp_> {
  ZuConversionFriend;

  ZuArrayN_(const ZuArrayN_ &);
  ZuArrayN_ &operator =(const ZuArrayN_ &);
  bool operator *() const;

  using Base = ZuArrayN__<T_>;
  using Base::m_length;

public:
  using T = T_;
  enum { N = N_ };
  using Char2 = typename ZuArrayN_Char2<T>::T;
  using Cmp = Cmp_;

  enum Nop_ { Nop };

protected:
  template <typename T> using NormChar = ZuNormChar<T>;

  // from some string with same char (including string literals)
  template <typename U, typename V = T> struct IsString {
    using W = typename NormChar<typename ZuTraits<U>::Elem>::T;
    using X = typename NormChar<V>::T;
    enum { OK =
      (ZuTraits<U>::IsArray || ZuTraits<U>::IsString) &&
      ZuConversion<W, X>::Same };
  };
  template <typename U, typename R = void>
  struct MatchString : public ZuIfT<IsString<U>::OK, R> { };

  // from any array type with convertible element type (not a string)
  template <typename U, typename V = T> struct IsArray {
    enum { OK =
      !IsString<U>::OK &&
      !ZuConversion<U, V>::Same &&
      ZuTraits<U>::IsArray &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Exists };
  };
  template <typename U, typename R = void>
  struct MatchArray : public ZuIfT<IsArray<U>::OK, R> { };

  // from char2 string (requires conversion)
  template <typename U, typename V = Char2> struct IsChar2String {
    enum { OK = !ZuConversion<ZuNull, V>::Same && ZuTraits<U>::IsString &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchChar2String : public ZuIfT<IsChar2String<U>::OK, R> { };

  // from individual char2 (requires conversion, char->wchar_t only)
  template <typename U, typename V = Char2> struct IsChar2 {
    enum { OK = !ZuConversion<ZuNull, V>::Same &&
      ZuConversion<U, V>::Same &&
      !ZuConversion<U, wchar_t>::Same };
  };
  template <typename U, typename R = void>
  struct MatchChar2 : public ZuIfT<IsChar2<U>::OK, R> { };

  // from printable type (if this is a char array)
  template <typename U, typename V = T> struct IsPDelegate {
    enum { OK = ZuConversion<char, V>::Same && ZuPrint<U>::Delegate };
  };
  template <typename U, typename R = void>
  struct MatchPDelegate : public ZuIfT<IsPDelegate<U>::OK, R> { };
  template <typename U, typename V = T> struct IsPBuffer {
    enum { OK = ZuConversion<char, V>::Same && ZuPrint<U>::Buffer };
  };
  template <typename U, typename R = void>
  struct MatchPBuffer : public ZuIfT<IsPBuffer<U>::OK, R> { };

  // from real primitive types other than chars (if this is a char string)
  template <typename U, typename V = T> struct IsReal {
    enum { OK = ZuConversion<char, V>::Same &&
      !ZuConversion<U, V>::Same &&
      ZuTraits<U>::IsReal && ZuTraits<U>::IsPrimitive &&
      !ZuTraits<U>::IsArray };
  };
  template <typename U, typename R = void>
  struct MatchReal : public ZuIfT<IsReal<U>::OK, R> { };

  // from individual element
  template <typename U, typename V = T> struct IsElem {
    enum { OK = ZuConversion<U, V>::Same ||
      (!IsString<U>::OK &&
       !IsArray<U>::OK &&
       !IsChar2String<U>::OK &&
       !IsChar2<U>::OK &&
       !IsPDelegate<U>::OK &&
       !IsPBuffer<U>::OK &&
       !IsReal<U>::OK &&
       ZuConversion<U, V>::Exists) };
  };
  template <typename U, typename R = void>
  struct MatchElem : public ZuIfT<IsElem<U>::OK, R> { };

  ZuInline ~ZuArrayN_() { dtor(); }

  ZuInline ZuArrayN_() { }

  ZuInline ZuArrayN_(Nop_ _) : Base(Base::Nop) { }

  ZuInline ZuArrayN_(unsigned length, bool initItems) : Base(length) {
    if (m_length > N) m_length = N;
    if (initItems) this->initItems(data(), m_length);
  }

  ZuInline void dtor() {
    this->destroyItems(data(), m_length);
  }

  template <typename A>
  typename ZuConvertible<A, T>::T init(const A *a, unsigned length) {
    if (ZuUnlikely(length > N)) length = N;
    if (ZuUnlikely(!a))
      length = 0;
    else if (ZuLikely(length))
      this->copyItems(data(), a, length);
    m_length = length;
  }
  template <typename A>
  typename ZuConvertible<A, T>::T init_mv(A *a, unsigned length) {
    if (ZuUnlikely(length > N)) length = N;
    if (ZuUnlikely(!a))
      length = 0;
    else if (ZuLikely(length))
      this->moveItems(data(), a, length);
    m_length = length;
  }

  template <typename S> ZuInline typename MatchString<S>::T init(S &&s_) {
    ZuArray<const T> s(s_);
    init(s.data(), s.length());
  }

  template <typename A> ZuInline typename MatchArray<A>::T init(A &&a) {
    ZuMvCp<A>::mvcp(ZuFwd<A>(a),
      [this](auto &&a_) {
	using Array = typename ZuDecay<decltype(a_)>::T;
	using Elem = typename ZuTraits<Array>::Elem;
	ZuArray<Elem> a(a_);
	this->init_mv(a.data(), a.length());
      },
      [this](const auto &a_) {
	using Array = typename ZuDecay<decltype(a_)>::T;
	using Elem = typename ZuTraits<Array>::Elem;
	ZuArray<typename ZuConst<Elem>::T> a(a_);
	this->init(a.data(), a.length());
      });
  }

  template <typename S> ZuInline typename MatchChar2String<S>::T init(S &&s) {
    data()[m_length = ZuUTF<T, Char2>::cvt({data(), N}, s)] = 0;
  }
  template <typename C> ZuInline typename MatchChar2<C>::T init(C c) {
    data()[m_length = ZuUTF<T, Char2>::cvt({data(), N}, {&c, 1})] = 0;
  }

  template <typename P> typename MatchPDelegate<P>::T init(const P &p) {
    ZuPrint<P>::print(*static_cast<ArrayN *>(this), p);
  }
  template <typename P> typename MatchPBuffer<P>::T init(const P &p) {
    unsigned length = ZuPrint<P>::length(p);
    if (length > N)
      m_length = 0;
    else
      m_length = ZuPrint<P>::print(data(), length, p);
  }

  template <typename V> ZuInline typename MatchReal<V>::T init(V v) {
    init(ZuBoxed(v));
  }

  template <typename E>
  ZuInline typename MatchElem<E>::T init(E &&e) {
    this->initItem(data(), ZuFwd<E>(e));
    m_length = 1;
  }

  template <typename A>
  typename ZuConvertible<A, T>::T append_(const A *a, unsigned length) {
    if (m_length + length > N)
      length = N - m_length;
    if (a && length) this->copyItems(data() + m_length, a, length);
    m_length += length;
  }
  template <typename A>
  typename ZuConvertible<A, T>::T append_mv_(A *a, unsigned length) {
    if (m_length + length > N)
      length = N - m_length;
    if (a && length) this->moveItems(data() + m_length, a, length);
    m_length += length;
  }

  template <typename S> typename MatchString<S>::T append_(S &&s_) {
    ZuArray<typename ZuConst<T>::T> s(s_);
    this->append_(s.data(), s.length());
  }

  template <typename A> typename MatchArray<A>::T append_(A &&a) {
    ZuMvCp<A>::mvcp(ZuFwd<A>(a),
      [this](auto &&a_) {
	using Array = typename ZuDecay<decltype(a_)>::T;
	using Elem = typename ZuTraits<Array>::Elem;
	ZuArray<Elem> a(a_);
	this->append_mv_(a.data(), a.length());
      },
      [this](const auto &a_) {
	using Array = typename ZuDecay<decltype(a_)>::T;
	using Elem = typename ZuTraits<Array>::Elem;
	ZuArray<typename ZuConst<Elem>::T> a(a_);
	this->append_(a.data(), a.length());
      });
  }

  template <typename S>
  ZuInline typename MatchChar2String<S>::T append_(S &&s) {
    if (m_length < N)
      data()[m_length = ZuUTF<T, Char2>::cvt(
	  {data() + m_length, N - m_length}, s)] = 0;
  }

  template <typename C> ZuInline typename MatchChar2<C>::T append_(C c) {
    if (m_length < N)
      data()[m_length += ZuUTF<T, Char2>::cvt(
	  {data() + m_length, N - m_length}, {&c, 1})] = 0;
  }

  template <typename P>
  typename MatchPDelegate<P>::T append_(const P &p) {
    ZuPrint<P>::print(*static_cast<ArrayN *>(this), p);
  }
  template <typename P>
  typename MatchPBuffer<P>::T append_(const P &p) {
    unsigned length = ZuPrint<P>::length(p);
    if (m_length + length > N) return;
    m_length += ZuPrint<P>::print(data() + m_length, length, p);
  }

  template <typename V> ZuInline typename MatchReal<V>::T append_(V v) {
    append_(ZuBoxed(v));
  }

  template <typename E> typename MatchElem<E>::T append_(E &&e) {
    if (m_length >= N) return;
    this->initItem(data() + m_length, ZuFwd<E>(e));
    ++m_length;
  }

public:
// array/ptr operators

  ZuInline T &operator [](int i) { return data()[i]; }
  ZuInline const T &operator [](int i) const { return data()[i]; }

// accessors

  ZuInline const T *begin() const { return data(); }
  ZuInline const T *end() const { return data() + m_length; }

  ZuInline static constexpr unsigned size() { return N; }
  ZuInline unsigned length() const { return m_length; }

// reset to null

  ZuInline void clear() { null(); }
  ZuInline void null() { m_length = 0; }

// set length

  template <bool InitItems = !ZuTraits<T>::IsPrimitive>
  void length(unsigned length) {
    if (length > N) length = N;
    if constexpr (InitItems) {
      if (length > m_length) {
	this->initItems(data() + m_length, length - m_length);
      } else if (length < m_length) {
	this->destroyItems(data() + length, m_length - length);
      }
    }
    m_length = length;
  }

// push/pop/shift/unshift

  void *push() {
    if (m_length >= N) return nullptr;
    return &(data()[m_length++]);
  }
  template <typename I> T *push(I &&i) {
    auto ptr = push();
    if (ZuLikely(ptr)) this->initItem(ptr, ZuFwd<I>(i));
    return static_cast<T *>(ptr);
  }
  T pop() {
    if (!m_length) return Cmp::null();
    T t = ZuMv(data()[--m_length]);
    this->destroyItem(data() + m_length);
    return t;
  }
  T shift() {
    if (!m_length) return Cmp::null();
    T t = ZuMv(data()[0]);
    this->destroyItem(data());
    this->moveItems(data(), data() + 1, --m_length);
    return t;
  }
  template <typename I> T *unshift(I &&i) {
    if (m_length >= N) return 0;
    this->moveItems(data() + 1, data(), m_length++);
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
  template <typename U> struct IsVoid {
    enum { OK = ZuConversion<void, U>::Same };
  };
  template <typename U, typename R = void>
  struct MatchVoid : public ZuIfT<IsVoid<U>::OK, R> { };

  template <typename U, typename V = T> struct IsAppend {
    enum { OK = ZuArrayN_CanAppend<U, V>::OK };
  };
  template <typename U, typename R = void>
  struct MatchAppend : public ZuIfT<IsAppend<U>::OK, R> { };

  template <typename U, typename V = T> struct IsSplice {
    enum { OK = !IsVoid<U>::OK && !IsAppend<U>::OK };
  };
  template <typename U, typename R = void>
  struct MatchSplice : public ZuIfT<IsSplice<U>::OK, R> { };

  template <typename U>
  void splice_(int offset, int length, U *removed) {
    if (ZuUnlikely(!length)) return;
    if (offset < 0) { if ((offset += m_length) < 0) offset = 0; }
    if (offset >= (int)N) return;
    if (length < 0) { if ((length += (m_length - offset)) <= 0) return; }
    if (offset + length > (int)N)
      if (!(length = N - offset)) return;
    if (offset > (int)m_length) {
      this->initItems(data() + m_length, offset - m_length);
      m_length = offset;
      return;
    }
    if (offset + length > (int)m_length)
      if (!(length = m_length - offset)) return;
    if (removed) splice__(data() + offset, length, removed);
    this->destroyItems(data() + offset, length);
    this->moveItems(
	data() + offset,
	data() + offset + length,
	m_length - (offset + length));
    m_length -= length;
  }
  template <typename U>
  typename MatchVoid<U>::T splice__(const T *, unsigned, U *) { }
  template <typename U>
  typename MatchAppend<U>::T
      splice__(const T *data, unsigned length, U *removed) {
    removed->append_(data, length);
  }
  template <typename U>
  typename MatchSplice<U>::T
      splice__(const T *data, unsigned length, U *removed) {
    for (unsigned i = 0; i < length; i++) *removed << data[i];
  }

// comparison

public:
  ZuInline bool operator !() const { return !m_length; }
  ZuOpBool

protected:
  ZuInline bool same(const ZuArrayN_ &a) const { return this == &a; }
  template <typename A> ZuInline bool same(const A &a) const { return false; }

private:
  ZuInline auto array_() const {
    return ZuArray<typename ZuConst<T>::T>{data(), length()};
  }
public:
  template <typename A>
  ZuInline int cmp(const A &a) const {
    if (same(a)) return 0;
    return array_().cmp(a);
  }
  template <typename A>
  ZuInline bool less(const A &a) const {
    return !same(a) && array_().less(a);
  }
  template <typename A>
  ZuInline bool greater(const A &a) const {
    return !same(a) && array_().greater(a);
  }
  template <typename A>
  ZuInline bool equals(const A &a) const {
    return same(a) || array_().equals(a);
  }

// hash

  ZuInline uint32_t hash() const {
    return ZuHash<ArrayN>::hash(*static_cast<const ArrayN *>(this));
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

namespace Zu_ {
template <typename T_, unsigned N_, class Cmp_ = ZuCmp<T_> >
class ArrayN : public ZuArrayN_<T_, N_, Cmp_, ArrayN<T_, N_, Cmp_> > {
  ZuAssert(N_ > 0);
  ZuAssert(N_ < (1U<<16) - 1U);

public:
  using T = T_;
  enum { N = N_ };
  using Cmp = Cmp_;
  using Base = ZuArrayN_<T, N, Cmp, ArrayN>;
  enum Move_ { Move };

private:
  // an unsigned|int|size_t parameter to the constructor is a buffer length
  template <typename U> struct IsCtorLength {
    enum { OK =
      ZuConversion<U, unsigned>::Same ||
      ZuConversion<U, int>::Same ||
      ZuConversion<U, size_t>::Same };
  };
  template <typename U, typename R = void>
  struct MatchCtorLength : public ZuIfT<IsCtorLength<U>::OK, R> { };

  // constructor arg
  template <typename U> struct IsCtorArg {
    enum { OK =
      !IsCtorLength<U>::OK &&
      !ZuConversion<Base, U>::Base };
  };
  template <typename U, typename R = void>
  struct MatchCtorArg : public ZuIfT<IsCtorArg<U>::OK, R> { };

public:
  ZuInline ArrayN() { }

  ZuInline ArrayN(const ArrayN &a) : Base(Base::Nop) {
    this->init(a.data(), a.length());
  }
  ZuInline ArrayN &operator =(const ArrayN &a) {
    if (this != &a) {
      this->dtor();
      this->init(a.data(), a.length());
    }
    return *this;
  }

  ZuInline ArrayN(ArrayN &&a) : Base(Base::Nop) {
    this->init_mv(a.data(), a.length());
  }
  ZuInline ArrayN &operator =(ArrayN &&a) {
    if (this != &a) {
      this->dtor();
      this->init_mv(a.data(), a.length());
    }
    return *this;
  }

  ZuInline ArrayN(std::initializer_list<T> a) : Base(Base::Nop) {
    this->init(a.begin(), a.size());
  }

  // miscellaneous types handled by base class
  template <typename A>
  ZuInline ArrayN(A &&a,
      typename MatchCtorArg<A>::T *_ = 0) : Base(Base::Nop) {
    this->init(ZuFwd<A>(a));
  }
  template <typename A>
  ZuInline ArrayN &operator =(A &&a) {
    this->dtor();
    this->init(ZuFwd<A>(a));
    return *this;
  }
  template <typename A>
  ZuInline ArrayN &operator +=(A &&a) {
    this->append_(ZuFwd<A>(a));
    return *this;
  }
  template <typename A>
  ZuInline ArrayN &operator <<(A &&a) {
    this->append_(ZuFwd<A>(a));
    return *this;
  }

  // length
  template <typename L>
  ZuInline ArrayN(L l, bool initItems = !ZuTraits<T>::IsPrimitive,
      typename MatchCtorLength<L>::T *_ = 0) : Base(l, initItems) { }

  // arrays as ptr, length
  template <typename A>
  ZuInline ArrayN(const A *a, unsigned length,
    typename ZuConvertible<A, T>::T *_ = 0) : Base(Base::Nop) {
    this->init(a, length);
  }
  ZuInline ArrayN(Move_ _, T *a, unsigned length) : Base(Base::Nop) {
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
} // namespace Zu_
template <typename T, unsigned N, typename Cmp = ZuCmp<T>>
using ZuArrayN = Zu_::ArrayN<T, N, Cmp>;

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

// STL structured binding cruft
#include <type_traits>
namespace std {
  template <class> struct tuple_size;
  template <typename T, unsigned N, typename Cmp>
  struct tuple_size<ZuArrayN<T, N, Cmp>> :
  public integral_constant<std::size_t, N> { };

  template <size_t, typename> struct tuple_element;
  template <size_t I, typename T, unsigned N, typename Cmp>
  struct tuple_element<I, ZuArrayN<T, N, Cmp>> {
    using type = T;
  };
}
namespace Zu_ {
  using size_t = std::size_t;
  namespace {
    template <size_t I, typename T>
    using tuple_element_t = typename std::tuple_element<I, T>::type;
  }
  template <size_t I, typename T, unsigned N, typename Cmp>
  constexpr tuple_element_t<I, ArrayN<T, N, Cmp>> &
  get(ArrayN<T, N, Cmp> &a) noexcept { return a[I]; }
  template <size_t I, typename T, unsigned N, typename Cmp>
  constexpr const tuple_element_t<I, ArrayN<T, N, Cmp>> &
  get(const ArrayN<T, N, Cmp> &a) noexcept { return a[I]; }
  template <size_t I, typename T, unsigned N, typename Cmp>
  constexpr tuple_element_t<I, ArrayN<T, N, Cmp>> &&
  get(ArrayN<T, N, Cmp> &&a) noexcept {
    return static_cast<tuple_element_t<I, ArrayN<T, N, Cmp>> &&>(a[I]);
  }
  template <size_t I, typename T, unsigned N, typename Cmp>
  constexpr const tuple_element_t<I, ArrayN<T, N, Cmp>> &&
  get(const ArrayN<T, N, Cmp> &&a) noexcept {
    return static_cast<const tuple_element_t<I, ArrayN<T, N, Cmp>> &&>(
	a[I]);
  }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuArrayN_HPP */
