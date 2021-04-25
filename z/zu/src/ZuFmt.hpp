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

// print/scan formatting

#ifndef ZuFmt_HPP
#define ZuFmt_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuIf.hpp>

// compile-time formatting
namespace ZuFmt {
  enum { MaxWidth = 54 };
  enum { MaxNDP = 19 };

  // NTP (named template parameters)
  struct Just { enum { None = 0, Left, Right, Frac }; };
  // NTP defaults
  struct Default {
    enum { Justification_ = Just::None };
    enum { Hex_ = 0 };			// Hexadecimal
    enum { Upper_ = 0 };		// Upper-case (Hex only)
    enum { Alt_ = 0 };			// Prefix with 0x (Hex only)
    enum { Comma_ = 0 };		// Comma character (Decimal only)
    enum { Width_ = 0 };		// Maximum length (Left/Right/Frac)
    enum { Pad_ = 0 };			// Padding character (Left/Right)
    enum { NDP_ = -MaxNDP };		// Decimal places (FP)
    enum { Trim_ = 0 };			// Trim character (FP/Frac)
    enum { Negative_ = 0 };		// Negative
  };
  // NTP - left-justify within a fixed-width field
  template <unsigned Width, char Pad = '\0', class NTP = Default>
  struct Left : public NTP {
    enum { Justification_ = Just::Left };
    enum { Width_ = Width > MaxWidth ? MaxWidth : Width };
    enum { Pad_ = Pad };
  };
  // NTP - right-justify within a fixed-width field
  template <unsigned Width, char Pad = '0', class NTP = Default>
  struct Right : public NTP {
    enum { Justification_ = Just::Right };
    enum { Width_ = (int)Width > MaxWidth ? MaxWidth : (int)Width };
    enum { Pad_ = Pad };
  };
  // NTP - justify a fixed-point value within a fixed-width field
  template <unsigned NDP, char Trim = '\0', class NTP = Default>
  struct Frac : public NTP {
    enum { Justification_ = Just::Frac };
    enum { NDP_ = (int)NDP > MaxNDP ? MaxNDP : (int)NDP };
    enum { Trim_ = Trim };
  };
  // NTP - specify hexadecimal
  template <bool Upper = 0, class NTP = Default>
  struct Hex : public NTP {
    enum { Hex_ = 1 };
    enum { Upper_ = Upper };
  };
  template <bool Enable, bool Upper = 0, class NTP = Default>
  struct HexEnable : public NTP {
    enum { Hex_ = Enable };
    enum { Upper_ = Upper };
  };
  // NTP - specify thousands comma character (decimal only, default is none)
  template <char Char = ',', class NTP = Default>
  struct Comma : public NTP {
    enum { Comma_ = Char };
  };
  // NTP - specify 'alternative' format
  template <class NTP = Default>
  struct Alt : public NTP {
    enum { Alt_ = 1 };
  };
  template <bool Enable = 1, class NTP = Default>
  struct AltEnable : public NTP {
    enum { Alt_ = Enable };
  };
  // NTP - floating point format (optionally specifying #DP and padding)
  template <int NDP = -MaxNDP, char Trim = '\0', class NTP = Default>
  struct FP : public NTP {
    enum { NDP_ = NDP < -MaxNDP ? -MaxNDP : NDP > MaxNDP ? MaxNDP : NDP };
    enum { Trim_ = Trim };
  };
};

// run-time variable formatting
struct ZuVFmt {
  ZuVFmt() :
    m_justification(ZuFmt::Just::None),
    m_hex(0), m_upper(0), m_alt(0),
    m_comma('\0'),
    m_width(0), m_pad(-1),
    m_ndp(-ZuFmt::MaxNDP), m_trim('\0') { }

  // initializers
  ZuVFmt &reset() {
    using namespace ZuFmt;
    m_justification = Just::None;
    m_hex = 0; m_upper = 0; m_alt = 0;
    m_comma = '\0';
    m_width = 0; m_pad = -1;
    m_ndp = -ZuFmt::MaxNDP; m_trim = '\0';
    return *this;
  }
  ZuVFmt &left(unsigned width, char pad = '\0') {
    using namespace ZuFmt;
    m_justification = Just::Left;
    m_width = ZuUnlikely(width > MaxWidth) ? MaxWidth : width;
    m_pad = pad;
    return *this;
  }
  ZuVFmt &right(unsigned width, char pad = '0') {
    using namespace ZuFmt;
    m_justification = Just::Right;
    m_width = ZuUnlikely(width > MaxWidth) ? MaxWidth : width;
    m_pad = pad;
    return *this;
  }
  ZuVFmt &frac(unsigned ndp, char trim = '\0') {
    using namespace ZuFmt;
    m_justification = Just::Frac;
    m_ndp = ZuUnlikely(ndp > MaxNDP) ? MaxNDP : ndp;
    m_trim = trim;
    return *this;
  }
  ZuVFmt &hex() {
    m_hex = 1;
    m_upper = 0;
    return *this;
  }
  ZuVFmt &hex(bool upper) {
    m_hex = 1;
    m_upper = upper;
    return *this;
  }
  ZuVFmt &hex(bool hex_, bool upper) {
    m_hex = hex_;
    m_upper = upper;
    return *this;
  }
  ZuVFmt &comma(char comma_ = ',') {
    m_comma = comma_;
    return *this;
  }
  ZuVFmt &alt() {
    m_alt = 1;
    return *this;
  }
  ZuVFmt &alt(bool alt_) {
    m_alt = alt_;
    return *this;
  }
  ZuVFmt &fp(int ndp = -ZuFmt::MaxNDP, char trim = '\0') {
    using namespace ZuFmt;
    m_ndp =
      ZuUnlikely(ndp < -MaxNDP) ? -MaxNDP :
      ZuUnlikely(ndp > MaxNDP) ? MaxNDP : ndp;
    m_trim = trim;
    return *this;
  }

  // accessors
  int justification() const { return m_justification; }
  bool hex() const { return m_hex; }
  bool upper() const { return m_upper; }
  bool alt() const { return m_alt; }
  char comma() const { return m_comma; }
  unsigned width() const { return m_width; }
  int pad() const { return m_pad; }
  int ndp() const { return m_ndp; }
  char trim() const { return m_trim; }

private:
#pragma pack(push, 1)
  int8_t	m_justification;
  uint8_t	m_hex:1,
  		m_upper:1,
  		m_alt:1;
  char		m_comma;
  uint8_t	m_width;
  int8_t	m_pad;
  int8_t	m_ndp;
  char		m_trim;
#pragma pack(pop)
};

template <typename Impl> struct ZuVFmtWrapper {
  ZuVFmt	fmt;

  Impl *impl() { return static_cast<Impl *>(this); }

  Impl &reset() { fmt.reset(); return *impl(); }
  Impl &left(unsigned width, char pad = '\0') {
    fmt.left(width, pad);
    return *impl();
  }
  Impl &right(unsigned width, char pad = '0') {
    fmt.right(width, pad);
    return *impl();
  }
  Impl &frac(unsigned ndp, char trim = '\0') {
    fmt.frac(ndp, trim);
    return *impl();
  }
  Impl &hex() { fmt.hex(); return *impl(); }
  Impl &hex(bool upper) { fmt.hex(upper); return *impl(); }
  Impl &hex(bool hex_, bool upper) {
    fmt.hex(hex_, upper);
    return *impl();
  }
  Impl &comma(char comma_ = ',') { fmt.comma(comma_); return *impl(); }
  Impl &alt() { fmt.alt(); return *impl(); }
  Impl &alt(bool alt_) { fmt.alt(alt_); return *impl(); }
  Impl &fp(int ndp = -ZuFmt::MaxNDP, char trim = '\0') {
    fmt.fp(ndp, trim);
    return *impl();
  }
};

#endif /* ZuFmt_HPP */
