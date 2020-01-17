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

// optimized integer and floating point scanning

#ifndef Zu_aton_HPP
#define Zu_aton_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuInt.hpp>
#include <zlib/ZuFP.hpp>
#include <zlib/ZuFmt.hpp>
#include <zlib/ZuDecimalFn.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wenum-compare"
#endif

namespace Zu_aton {
  using namespace ZuFmt;

  template <typename T_, unsigned Size = sizeof(T_)> struct Unsigned {
    typedef uint64_t T;
  };
  template <typename T_> struct Unsigned<T_, 16> { typedef uint128_t T; };

  template <class Fmt,
    char Skip = Fmt::Comma_,
    int Justification = Fmt::Justification_>
  struct Base10 {
    template <typename T>
    static unsigned scan(T &v_, int c, const char *buf, unsigned n) {
      T v;
      if (ZuLikely(c >= '0' && c <= '9'))
	v = c - '0';
      else if (ZuLikely(c == Skip))
	v = 0;
      else
	return 0;
      unsigned o = 1;
      if (n > Fmt::Width_) n = Fmt::Width_;
      while (ZuLikely(o < n)) {
	if (ZuLikely((c = buf[o]) >= '0' && c <= '9'))
	  v = v * 10U + (c - '0');
	else if (ZuUnlikely(c != Skip))
	  break;
	++o;
      }
      v_ = v;
      return o;
    }
  };
  template <class Fmt, int Justification>
  struct Base10<Fmt, '\0', Justification> {
    template <typename T>
    static unsigned scan(T &v_, int c, const char *buf, unsigned n) {
      if (ZuUnlikely(c < '0' || c > '9')) return 0;
      T v = c - '0';
      unsigned o = 1;
      if (n > Fmt::Width_) n = Fmt::Width_;
      while (ZuLikely(o < n && (c = buf[o]) >= '0' && c <= '9')) {
	v = v * 10U + (c - '0');
	++o;
      }
      v_ = v;
      return o;
    }
  };
  template <class Fmt, char Skip>
  struct Base10<Fmt, Skip, Just::None> {
    template <typename T>
    static unsigned scan(T &v_, int c, const char *buf, unsigned n) {
      T v;
      if (ZuLikely(c >= '0' && c <= '9'))
	v = c - '0';
      else if (ZuLikely(c == Skip))
	v = 0;
      else
	return 0;
      unsigned o = 1;
      while (ZuLikely(o < n)) {
	if (ZuLikely((c = buf[o]) >= '0' && c <= '9'))
	  v = v * 10U + (c - '0');
	else if (ZuUnlikely(c != Skip))
	  break;
	++o;
      }
      v_ = v;
      return o;
    }
  };
  template <class Fmt>
  struct Base10<Fmt, '\0', Just::None> {
    template <typename T>
    static unsigned scan(T &v_, int c, const char *buf, unsigned n) {
      if (ZuUnlikely(c < '0' || c > '9')) return 0;
      T v = c - '0';
      unsigned o = 1;
      while (ZuLikely(o < n && (c = buf[o]) >= '0' && c <= '9')) {
	v = v * 10U + (c - '0');
	++o;
      }
      v_ = v;
      return o;
    }
  };
  template <class Fmt>
  struct Base10<Fmt, '\0', Just::Frac> {
    template <typename T>
    static unsigned scan(T &v_, int c, const char *buf, unsigned n) {
      if (ZuUnlikely(c < '0' || c > '9')) return 0;
      T v = c - '0';
      unsigned o = 1;
      if (n > Fmt::NDP_) n = Fmt::NDP_;
      while (ZuLikely(o < n && (c = buf[o]) >= '0' && c <= '9')) {
	v = v * 10U + (c - '0');
	++o;
      }
      unsigned r = o;
      while (ZuLikely(o < Fmt::NDP_)) {
	v = v * 10U;
	++o;
      }
      v_ = v;
      return r;
    }
  };

  template <class Fmt> struct Base16 {
    ZuInline static int hexDigit(char c) {
      if (ZuLikely(c >= '0' && c <= '9')) return c - '0';
      if (ZuLikely(c >= 'a' && c <= 'f')) return c - 'a' + 10;
      if (ZuLikely(c >= 'A' && c <= 'F')) return c - 'A' + 10;
      return -1;
    }

    template <typename T>
    static unsigned scan(T &v_, int i, const char *buf, unsigned n) {
      i = hexDigit(i);
      if (ZuUnlikely(i < 0)) return 0;
      T v = i;
      if (ZuUnlikely(n == 1)) { v_ = v; return 1; }
      unsigned o = 1;
      i = buf[1];
      if (!v && i == 'x') {
	if (ZuUnlikely(n == 2)) { v_ = 0; return 2; }
	i = buf[++o];
      }
      while (ZuLikely(o < n && (i = hexDigit(i)) >= 0)) {
	v = (v<<4U) + i;
	i = buf[++o];
      }
      v_ = v;
      return o;
    }
  };

  template <class Fmt, bool Hex = Fmt::Hex_>
  struct BaseN : public Base10<Fmt> { };
  template <class Fmt> struct BaseN<Fmt, 1> : public Base16<Fmt> { };
}

template <class Fmt> struct Zu_nscan_ {
  template <bool Constrain, unsigned Width> struct Len_ {
    ZuInline static constexpr unsigned len(unsigned n) {
      return n > Width ? Width : n;
    }
  };
  template <unsigned Width> struct Len_<0, Width> {
    ZuInline static constexpr unsigned len(unsigned n) { return n; }
  };
  typedef Len_<
    Fmt::Justification_ != ZuFmt::Just::None,
    Fmt::Justification_ != ZuFmt::Just::Frac ? Fmt::Width_ : Fmt::NDP_> Len;

  template <typename T>
  static unsigned atou_(T &v, const char *buf, unsigned n) {
    using namespace Zu_aton;
    return BaseN<Fmt>::scan(v, *buf, buf, n);
  }
  template <typename T>
  static unsigned atou(T &v, const char *buf, unsigned n) {
    n = Len::len(n);
    if (ZuUnlikely(!buf || !n)) return 0;
    return atou_(v, buf, n);
  }

  template <typename T>
  static unsigned atoi_(T &v_, const char *buf, unsigned n) {
    using namespace Zu_aton;
    typename Unsigned<T>::T v = 0;
    unsigned o;
    int c = *buf;
    if (c == '-') {
      if (ZuUnlikely(n == 1)) { v_ = 0; return 1; }
      o = BaseN<Fmt>::scan(v, buf[1], buf + 1, n - 1);
      if (ZuUnlikely(!o)) return 0;
      v_ = -((T)v);
      return o + 1;
    }
    o = BaseN<Fmt>::scan(v, c, buf, n);
    v_ = v;
    return o;
  }
  template <typename T>
  static unsigned atoi(T &v_, const char *buf, unsigned n) {
    n = Len::len(n);
    if (ZuUnlikely(!buf || !n)) return 0;
    return atoi_(v_, buf, n);
  }

  // floating point - integer portion format
  template <class Fmt_> struct FPIntFmt_ : public Fmt_ {
    enum { Justification_ = ZuFmt::Just::None };
    enum { Hex_ = 0 };
  };
  // floating point - fractional portion format
  template <class Fmt_> struct FPFracFmt_ : public Fmt_ {
    enum { Justification_ = ZuFmt::Just::None }; // Note: not Frac
    enum { Hex_ = 0 };
    enum { Comma_ = 0 };
    enum { Pad_ = 0 };
  };
  template <typename T>
  static unsigned atof_(T &v_, const char *buf, unsigned n) {
    using namespace Zu_aton;
    typedef ZuFP<T> FP;
    uint64_t iv;
    unsigned o = 0;
    int c = *buf;
    bool negative = c == '-';
    if (negative) {
      if (ZuUnlikely(n == 1)) return 0;
      c = buf[++o];
    }
    if (ZuUnlikely(c == '.')) { iv = 0; goto frac; }
    if (ZuLikely(n - o >= 3)) {
      if (ZuUnlikely(c == 'n' && buf[o + 1] == 'a' && buf[o + 2] == 'n')) {
	v_ = FP::nan();
	return 3 + negative;
      }
      if (ZuUnlikely(c == 'i' && buf[o + 1] == 'n' && buf[o + 2] == 'f')) {
	v_ = negative ? -FP::inf() : FP::inf();
	return 3 + negative;
      }
    }
    {
      unsigned i = Base10<FPIntFmt_<Fmt> >::scan(iv, c, buf + o, n - o);
      if (ZuUnlikely(!i)) return 0;
      o += i;
    }
    if (o >= n - 1 || (c = buf[o]) != '.') {
      v_ = (T)iv;
      if (negative) v_ = -v_;
      return o;
    }
  frac:
    {
      ++o;
      unsigned l = n - o;
      if (l >= FP::MaxDigits) l = FP::MaxDigits - 1;
      uint64_t fv;
      unsigned i = Base10<FPFracFmt_<Fmt> >::scan(fv, buf[o], buf + o, l);
      if (ZuUnlikely(!i)) {
	v_ = (T)iv;
	if (negative) v_ = -v_;
	return o;
      }
      o += i;
      T v = (T)(fv * ZuDecimalFn::pow10_64(FP::MaxDigits - 1 - i));
      if (o < n && (c = buf[o]) >= '0' && c <= '9') {
	++o;
	v += (T)0.1 * (T)(c - '0');
      }
      v_ = (T)iv + v / (T)ZuDecimalFn::Pow10<FP::MaxDigits - 1>::pow10();
      if (negative) v_ = -v_;
      return o;
    }
  }
  template <typename T>
  ZuInline static unsigned atof(T &v_, const char *buf, unsigned n) {
    n = Len::len(n);
    if (ZuUnlikely(!buf || !n)) return 0;
    return atof_(v_, buf, n);
  }
};
template <class Fmt = ZuFmt::Default, int Skip =
  (Fmt::Justification_ == ZuFmt::Just::Right && Fmt::Pad_ != '0') ?
    Fmt::Pad_ : -1>
struct Zu_nscan : public Zu_nscan_<Fmt> {
  typedef typename Zu_nscan_<Fmt>::Len Len;
  static unsigned skip(const char *buf, unsigned n) {
    unsigned o = 0;
    while (ZuLikely(o < n && buf[o] == Skip)) ++o;
    return o;
  }
  template <typename T>
  static unsigned atou(T &v, const char *buf, unsigned n) {
    n = Len::len(n);
    if (ZuUnlikely(!buf || !n)) return 0;
    unsigned o = skip(buf, n);
    if (ZuUnlikely(o >= n)) return 0;
    return Zu_nscan_<Fmt>::atou_(v, buf + o, n - o) + o;
  }
  template <typename T>
  static unsigned atoi(T &v, const char *buf, unsigned n) {
    n = Len::len(n);
    if (ZuUnlikely(!buf || !n)) return 0;
    unsigned o = skip(buf, n);
    if (ZuUnlikely(o >= n)) return 0;
    return Zu_nscan_<Fmt>::atoi_(v, buf + o, n - o) + o;
  }
  template <typename T>
  static unsigned atof(T &v, const char *buf, unsigned n) {
    n = Len::len(n);
    if (ZuUnlikely(!buf || !n)) return 0;
    unsigned o = skip(buf, n);
    if (ZuUnlikely(o >= n)) return 0;
    return Zu_nscan_<Fmt>::atof_(v, buf + o, n - o) + o;
  }
};
template <class Fmt> struct Zu_nscan<Fmt, -1> : public Zu_nscan_<Fmt> { };

template <typename T>
ZuInline unsigned Zu_atou(T &v, const char *buf, unsigned n) {
  return Zu_nscan<>::atou(v, buf, n);
}
template <typename T>
ZuInline unsigned Zu_atoi(T &v, const char *buf, unsigned n) {
  return Zu_nscan<>::atoi(v, buf, n);
}
template <typename T>
ZuInline unsigned Zu_atof(T &v, const char *buf, unsigned n) {
  return Zu_nscan<>::atof(v, buf, n);
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif /* Zu_aton_HPP */
