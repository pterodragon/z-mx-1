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

// reference-counted POD structures

#ifndef ZuPOD_HPP
#define ZuPOD_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#include <ZuInt.hpp>

// generic run-time access to a reference-counted POD type

template <typename Base>
class ZuAnyPOD_ : public Base {
public:
  template <typename ...Args>
  ZuInline ZuAnyPOD_(unsigned size, Args &&... args) :
    Base(ZuFwd<Args>(args)...), m_size(size) { }
  virtual ~ZuAnyPOD_() { }

  ZuInline const void *ptr() const { return &m_data; }
  ZuInline void *ptr() { return &m_data; }

  template <typename T> ZuInline const T &as() const {
    const T *ZuMayAlias(ptr) = (const T *)&m_data;
    return *ptr;
  }
  template <typename T> ZuInline T &as() {
    T *ZuMayAlias(ptr) = (T *)&m_data;
    return *ptr;
  }

  ZuInline unsigned size() const { return m_size; }

private:
  unsigned	m_size;
  uintptr_t	m_data;
};

#include <ZuPolymorph.hpp>

typedef ZuAnyPOD_<ZuPolymorph> ZuAnyPOD;

template <typename T_, class Base,
  bool Small = sizeof(T_) <= sizeof(uintptr_t)>
class ZuPOD_ : public ZuAnyPOD_<Base> {
public:
  typedef T_ T;

  template <typename ...Args>
  ZuPOD_(Args &&... args) :
    ZuAnyPOD_<Base>(sizeof(T), ZuFwd<Args>(args)...) { }

  ZuInline const T *ptr() const { return &(this->template as<T>()); }
  ZuInline T *ptr() { return &(this->template as<T>()); }

  ZuInline const T &data() const { return this->template as<T>(); }
  ZuInline T &data() { return this->template as<T>(); }

private:
  char		m_data[sizeof(T) - sizeof(uintptr_t)];
};
template <typename T_, class Base>
class ZuPOD_<T_, Base, 1> : public ZuAnyPOD_<Base> {
public:
  typedef T_ T;

  template <typename ...Args>
  ZuPOD_(Args &&... args) :
    ZuAnyPOD_<Base>(sizeof(T), ZuFwd<Args>(args)...) { }

  ZuInline const T *ptr() const { return &(this->template as<T>()); }
  ZuInline T *ptr() { return &(this->template as<T>()); }

  ZuInline const T &data() const { return this->template as<T>(); }
  ZuInline T &data() { return this->template as<T>(); }
};

template <typename T> using ZuPOD = ZuPOD_<T, ZuPolymorph>;

#endif /* ZuPOD_HPP */
