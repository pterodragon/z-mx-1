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

// ZtEnum wrapper, used by configuration and CSV

#ifndef ZvEnum_HPP
#define ZvEnum_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#include <ZmObject.hpp>
#include <ZmRef.hpp>

#include <ZtString.hpp>
#include <ZtEnum.hpp>

#include <ZvError.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

template <typename T> class ZvEnum;

class ZvAPI ZvInvalidEnum : public ZvError {
public:
  // inline ZvInvalidEnum() { }
  template <typename Key, typename Value>
  inline ZvInvalidEnum(Key &&key, Value &&value) :
    m_key(ZuFwd<Key>(key)), m_value(ZuFwd<Value>(value)) { }

#if 0
  inline ZvInvalidEnum(const ZvInvalidEnum &i) :
    m_key(i.m_key), m_value(i.m_value) { }
  inline ZvInvalidEnum &operator =(const ZvInvalidEnum &i) {
    if (this != &i) m_key = i.m_key, m_value = i.m_value;
    return *this;
  }
  inline ZvInvalidEnum(ZvInvalidEnum &&i) :
    m_key(ZuMv(i.m_key)), m_value(ZuMv(i.m_value)) { }
  inline ZvInvalidEnum &operator =(ZvInvalidEnum &&i) {
    m_key = ZuMv(i.m_key), m_value = ZuMv(i.m_value);
    return *this;
  }
#endif

  inline const ZtString &key() const { return m_key; }
  inline const ZtString &value() const { return m_value; }

private:
  ZtString			m_key;
  ZtString			m_value;
};
template <typename T> class ZvInvalidEnumT : public ZvInvalidEnum {
public:
  template <typename Key, typename Value>
  inline ZvInvalidEnumT(
      Key &&key, Value &&value, const ZvEnum<T> *enum_) :
    ZvInvalidEnum(ZuFwd<Key>(key), ZuFwd<Value>(value)), m_enum(enum_) { }

  void print_(ZmStream &s) const;

  // inline const ZvEnum<T> *enum_() const { return m_enum; }

private:
  const ZvEnum<T>	*m_enum;
};

template <typename T> class ZvEnum : public T {
template <typename> friend class ZvInvalidEnumT;

public:
  inline static ZvEnum *instance() {
    return static_cast<ZvEnum *>(T::instance());
  }

  inline ZtEnum s2v(ZuString key, ZuString s,
      int def = ZtEnum(), bool noThrow = false) const {
    if (ZuUnlikely(!s)) return def;
    ZtEnum v = T::s2v(s);
    if (ZuLikely(*v)) return v;
    if (noThrow) return def;
    throw ZvInvalidEnumT<T>(key, s, this);
  }

  inline const char *v2s(ZuString key, ZtEnum v) const {
    const char *s = T::v2s(v);
    if (ZuLikely(s)) return s;
    throw ZvInvalidEnumT<T>(key, v, this);
  }

private:
  inline typename T::V2S &v2s_() { return this->m_v2s; }
public:
  class Iterator;
friend class Iterator;
  class Iterator : private T::V2S::ReadIterator {
  public:
    inline Iterator(const ZvEnum *e) :
      T::V2S::ReadIterator(e->v2s_()) { }
    inline ZtEnum iterate() { return this->iterateKey(); }
  };

private:
  template <typename Key, typename Value>
  inline void errorMessage(ZmStream &s, Key &&key, Value &&value) const {
    s << ZuFwd<Key>(key) << ": \"" << ZuFwd<Value>(value) <<
      "\" did not match { ";
    {
      typename T::S2V::ReadIterator i(this->m_s2v);
      ZuPair<const char *, ZtEnum> kv = i.iterate();
      if (kv.p1()) {
	s << kv.p1() << " = " << kv.p2();
	while (kv = i.iterate(), kv.p1()) {
	  s << ", ";
	  s << kv.p1() << " = " << kv.p2();
	}
      }
    }
    s << " }";
  }
};

template <typename T>
void ZvInvalidEnumT<T>::print_(ZmStream &s) const
{
  m_enum->errorMessage(s, this->key(), this->value());
}

template <typename T> class ZvFlags : public ZvEnum<T> {
public:
  inline static ZvFlags *instance() {
    return static_cast<ZvFlags *>(T::instance());
  }

  template <typename S, typename Flags> inline unsigned print(
      ZuString key, S &s, const Flags &v, char delim = '|') const {
    if (!v) return 0;
    if (unsigned n = T::print(s, v, delim)) return n;
    throw ZvInvalidEnumT<T>(
	key, ZuBoxed(v).hex(), static_cast<const ZvEnum<T> *>(this));
    // not reached
  }

  template <typename Flags>
  inline Flags scan(ZuString key, ZuString s, char delim = '|') const {
    if (!s) return 0;
    if (Flags v = T::template scan<Flags>(s, delim)) return v;
    throw ZvInvalidEnumT<T>(
	key, s, static_cast<const ZvEnum<T> *>(this));
    // not reached
  }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZvEnum_HPP */
