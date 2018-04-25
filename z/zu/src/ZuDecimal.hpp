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

// log10 lookup tables

#ifndef ZuLog10_HPP
#define ZuLog10_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

namespace ZuDecimal {
  ZuInline const unsigned pow10_32(unsigned i) {
    static constexpr unsigned pow10[] = {
      1U,
      10U,
      100U,
      1000U,
      10000U,
      100000U,
      1000000U,
      10000000U,
      100000000U,
      1000000000U
    };
    return pow10[i];
  }

  ZuInline const uint64_t pow10_64(unsigned i) {
    static constexpr uint64_t pow10[] = {
      1ULL,
      10ULL,
      100ULL,
      1000ULL,
      10000ULL,
      100000ULL,
      1000000ULL,
      10000000ULL,
      100000000ULL,
      1000000000ULL,
      10000000000ULL,
      100000000000ULL,
      1000000000000ULL,
      10000000000000ULL,
      100000000000000ULL,
      1000000000000000ULL,
      10000000000000000ULL,
      100000000000000000ULL,
      1000000000000000000ULL,
      10000000000000000000ULL
    };
    return pow10[i];
  }

  template <unsigned I> struct Pow10 { };
  template <> struct Pow10<0U> {
    ZuInline static constexpr unsigned pow10() { return 1U; }
  };
  template <> struct Pow10<1U> {
    ZuInline static constexpr unsigned pow10() { return 10U; }
  };
  template <> struct Pow10<2U> {
    ZuInline static constexpr unsigned pow10() { return 100U; }
  };
  template <> struct Pow10<3U> {
    ZuInline static constexpr unsigned pow10() { return 1000U; }
  };
  template <> struct Pow10<4U> {
    ZuInline static constexpr unsigned pow10() { return 10000U; }
  };
  template <> struct Pow10<5U> {
    ZuInline static constexpr unsigned pow10() { return 100000U; }
  };
  template <> struct Pow10<6U> {
    ZuInline static constexpr unsigned pow10() { return 1000000U; }
  };
  template <> struct Pow10<7U> {
    ZuInline static constexpr unsigned pow10() { return 10000000U; }
  };
  template <> struct Pow10<8U> {
    ZuInline static constexpr unsigned pow10() { return 100000000U; }
  };
  template <> struct Pow10<9U> {
    ZuInline static constexpr unsigned pow10() { return 1000000000U; }
  };
  template <> struct Pow10<10U> {
    ZuInline static constexpr uint64_t pow10() { return 10000000000ULL; }
  };
  template <> struct Pow10<11U> {
    ZuInline static constexpr uint64_t pow10() { return 100000000000ULL; }
  };
  template <> struct Pow10<12U> {
    ZuInline static constexpr uint64_t pow10() { return 1000000000000ULL; }
  };
  template <> struct Pow10<13U> {
    ZuInline static constexpr uint64_t pow10() { return 10000000000000ULL; }
  };
  template <> struct Pow10<14U> {
    ZuInline static constexpr uint64_t pow10() { return 100000000000000ULL; }
  };
  template <> struct Pow10<15U> {
    ZuInline static constexpr uint64_t pow10() { return 1000000000000000ULL; }
  };
  template <> struct Pow10<16U> {
    ZuInline static constexpr uint64_t pow10() { return 10000000000000000ULL; }
  };
  template <> struct Pow10<17U> {
    ZuInline static constexpr uint64_t pow10()
      { return 100000000000000000ULL; }
  };
  template <> struct Pow10<18U> {
    ZuInline static constexpr uint64_t pow10()
      { return 1000000000000000000ULL; }
  };
  template <> struct Pow10<19U> {
    ZuInline static constexpr uint64_t pow10()
      { return 10000000000000000000ULL; }
  };
}

#endif /* ZuLog10_HPP */
