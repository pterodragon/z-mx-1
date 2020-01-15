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

// MxBase library

#ifndef MxBase_HPP
#define MxBase_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxBaseLib_HPP
#include <mxbase/MxBaseLib.hpp>
#endif

#include <zlib/ZuInt.hpp>
#include <zlib/ZuDecimal.hpp>
#include <zlib/ZuDecimalFn.hpp>
#include <zlib/ZuBox.hpp>
#include <zlib/ZuPair.hpp>
#include <zlib/ZuStringN.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuID.hpp>

#include <zlib/ZmRef.hpp>
#include <zlib/ZmLHash.hpp>
#include <zlib/ZmStream.hpp>

#include <zlib/ZtDate.hpp>
#include <zlib/ZtString.hpp>
#include <zlib/ZtEnum.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZvCSV.hpp>

// vocabulary types

#ifndef MxBase_LongDouble
#define MxBase_LongDouble 1	// MxFloat is long double instead of double
#endif

typedef ZuBox0(char) MxChar;
typedef ZuBox<int8_t> MxBool;
typedef ZuBox<uint8_t> MxUInt8;
typedef ZuBox<int32_t> MxInt;
typedef ZuBox<uint32_t> MxUInt;
typedef ZuBox<int64_t> MxInt64;
typedef ZuBox<uint64_t> MxUInt64;
typedef ZuBox<int128_t> MxInt128;
typedef ZuBox<uint128_t> MxUInt128;
#if MxBase_LongDouble
typedef ZuBox<long double> MxFloat;
#else
typedef ZuBox<double> MxFloat;
#endif
typedef ZtDate MxDateTime;
#define MxNow ZtDateNow
typedef ZmTime MxDeltaTime;
typedef ZtEnum MxEnum;
typedef ZuBox0(uint32_t) MxFlags;
typedef ZuBox0(uint64_t) MxFlags64;

#define MxString ZuStringN

typedef ZuID MxID; // Note: different than MxIDString

typedef ZuDecimal MxDecimal;

// MxValue value range is +/- 10^18
typedef MxInt64 MxValue;	// fixed point value (numerator)
typedef MxUInt8 MxNDP;		// number of decimal places (log10(denominator))
typedef MxUInt8 MxRatio;	// ratio numerator (orders w/ multiple legs)
				// (denominator is sum of individual ratios)
#define MxValueMin (MxValue{(int64_t)-999999999999999999LL})
#define MxValueMax (MxValue{(int64_t)999999999999999999LL})
// MxValueReset is the distinct sentinel value used to reset values to null
#define MxValueReset (MxValue{(int64_t)-1000000000000000000LL})

// combination of value and ndp, used as a temporary for conversions & IO
// constructors / scanning:
//   MxValNDP(<integer>, ndp)			// {1042, 2} -> 10.42
//   MxValNDP(<floating point>, ndp)		// {10.42, 2} -> 10.42
//   MxValNDP(<string>, ndp)			// {"10.42", 2} -> 10.42
//   MxValue x = MxValNDP{"42.42", 2}.value	// x == 4242
//   MxValNDP xn{x, 2}; xn *= MxValNDP{2000, 3}; x = xn.value // x == 8484
// printing:
//   s << MxValNDP{...}			// print (default)
//   s << MxValNDP{...}.fmt(ZuFmt...)	// print (compile-time formatted)
//   s << MxValNDP{...}.vfmt(ZuVFmt...)	// print (variable run-time formatted)
//   s << MxValNDP(x, 2)			// s << "42.42"
//   x = 4200042;				// 42000.42
//   s << MxValNDP(x, 2).fmt(ZuFmt::Comma<>())	// s << "42,000.42"

template <typename Fmt> struct MxValNDPFmt;	// internal
template <bool Ref = 0> struct MxValNDPVFmt;	// internal

struct MxValNDP {
  MxValue	value;
  unsigned	ndp = 0;

  ZuInline MxValNDP() { }

  template <typename V>
  ZuInline MxValNDP(V value_, unsigned ndp_,
      typename ZuIsIntegral<V>::T *_ = 0) :
    value(value_), ndp(ndp_) { }

  template <typename V>
  ZuInline MxValNDP(V value_, unsigned ndp_,
      typename ZuIsFloatingPoint<V>::T *_ = 0) :
    value((MxFloat)value_ * ZuDecimalFn::pow10_64(ndp_)), ndp(ndp_) { }

  // multiply: NDP of result is taken from the LHS
  // a 128bit integer intermediary is used to avoid overflow
  inline MxValNDP operator *(const MxValNDP &v) const {
    int128_t i = (typename MxValue::T)value;
    i *= (typename MxValue::T)v.value;
    i /= ZuDecimalFn::pow10_64(v.ndp);
    if (ZuUnlikely(i >= 1000000000000000000ULL))
      return MxValNDP{MxValue(), ndp};
    return MxValNDP{(int64_t)i, ndp};
  }

  // divide: NDP of result is taken from the LHS
  // a 128bit integer intermediary is used to avoid overflow
  inline MxValNDP operator /(const MxValNDP &v) const {
    int128_t i = (typename MxValue::T)value;
    i *= ZuDecimalFn::pow10_64(v.ndp);
    i /= (typename MxValue::T)v.value;
    if (ZuUnlikely(i >= 1000000000000000000ULL))
      return MxValNDP{MxValue(), ndp};
    return MxValNDP{(int64_t)i, ndp};
  }

  // scan from string
  template <typename S>
  inline MxValNDP(const S &s_, int ndp_ = -1,
      typename ZuIsString<S>::T *_ = 0) {
    ZuString s(s_);
    if (ZuUnlikely(!s || ndp_ > 18)) goto null;
    {
      bool negative = s[0] == '-';
      if (ZuUnlikely(negative)) {
	s.offset(1);
	if (ZuUnlikely(!s)) goto null;
      }
      while (s[0] == '0') s.offset(1);
      if (!s) { value = 0; return; }
      uint64_t iv = 0, fv = 0;
      unsigned n = s.length();
      if (ZuUnlikely(s[0] == '.')) {
	if (ZuUnlikely(n == 1)) { value = 0; return; }
	if (ndp_ < 0) ndp_ = n - 1;
	goto frac;
      }
      n = Zu_atou(iv, s.data(), n);
      if (ZuUnlikely(!n)) goto null;
      if (ZuUnlikely(n > (18 - (ndp_ < 0 ? 0 : ndp_)))) goto null; // overflow
      s.offset(n);
      if (ndp_ < 0) ndp_ = 18 - n;
      if ((n = s.length()) > 1 && s[0] == '.') {
  frac:
	if (--n > ndp_) n = ndp_;
	n = Zu_atou(fv, &s[1], n);
	if (fv && ndp_ > n)
	  fv *= ZuDecimalFn::pow10_64(ndp_ - n);
      }
      value = iv * ZuDecimalFn::pow10_64(ndp_) + fv;
      if (ZuUnlikely(negative)) value = -value;
    }
    ndp = ndp_;
    return;
  null:
    value = MxValue();
    return;
  }

  // convert to floating point (MxFloat)
  ZuInline MxFloat fp() const {
    if (ZuUnlikely(!*value)) return MxFloat();
    return (MxFloat)value / (MxFloat)ZuDecimalFn::pow10_64(ndp);
  }

  // adjust to another NDP
  ZuInline MxValue adjust(unsigned ndp) const {
    if (ZuLikely(ndp == this->ndp)) return value;
    if (!*value) return MxValue();
    return (MxValNDP{1.0, ndp} * (*this)).value;
  }

  // ! is zero, unary * is !null
  ZuInline bool operator !() const { return !value; }
  ZuOpBool

  ZuInline bool operator *() const { return *value; }

  template <typename S> void print(S &s) const;

  template <typename Fmt> MxValNDPFmt<Fmt> fmt(Fmt) const;
  MxValNDPVFmt<> vfmt() const;
  MxValNDPVFmt<1> vfmt(ZuVFmt &) const;
};
template <typename Fmt> struct MxValNDPFmt {
  const MxValNDP	&fixedNDP;

  template <typename S> inline void print(S &s) const {
    MxValue iv = fixedNDP.value;
    if (ZuUnlikely(!*iv)) return;
    if (ZuUnlikely(iv < 0)) { s << '-'; iv = -iv; }
    uint64_t factor = ZuDecimalFn::pow10_64(fixedNDP.ndp);
    MxValue fv = iv % factor;
    iv /= factor;
    s << ZuBoxed(iv).fmt(Fmt());
    if (fv) s << '.' << ZuBoxed(fv).vfmt().frac(fixedNDP.ndp);
  }
};
template <typename Fmt>
struct ZuPrint<MxValNDPFmt<Fmt> > : public ZuPrintFn { };
template <class Fmt>
ZuInline MxValNDPFmt<Fmt> MxValNDP::fmt(Fmt) const
{
  return MxValNDPFmt<Fmt>{*this};
}
template <typename S> inline void MxValNDP::print(S &s) const
{
  s << MxValNDPFmt<ZuFmt::Default>{*this};
}
template <> struct ZuPrint<MxValNDP> : public ZuPrintFn { };
template <bool Ref>
struct MxValNDPVFmt : public ZuVFmtWrapper<MxValNDPVFmt<Ref>, Ref> {
  const MxValNDP	&fixedNDP;

  ZuInline MxValNDPVFmt(const MxValNDP &fixedNDP_) :
    fixedNDP{fixedNDP_} { }
  ZuInline MxValNDPVFmt(const MxValNDP &fixedNDP_, ZuVFmt &fmt_) :
    ZuVFmtWrapper<MxValNDPVFmt, Ref>{fmt_}, fixedNDP{fixedNDP_} { }

  template <typename S> inline void print(S &s) const {
    MxValue iv = fixedNDP.value;
    if (ZuUnlikely(!*iv)) return;
    if (ZuUnlikely(iv < 0)) { s << '-'; iv = -iv; }
    uint64_t factor = ZuDecimalFn::pow10_64(fixedNDP.ndp);
    MxValue fv = iv % factor;
    iv /= factor;
    s << ZuBoxed(iv).vfmt(this->fmt);
    if (fv) s << '.' << ZuBoxed(fv).vfmt().frac(fixedNDP.ndp);
  }
};
ZuInline MxValNDPVFmt<> MxValNDP::vfmt() const
{
  return MxValNDPVFmt<>{*this};
}
ZuInline MxValNDPVFmt<1> MxValNDP::vfmt(ZuVFmt &fmt) const
{
  return MxValNDPVFmt<1>{*this, fmt};
}
template <bool Ref>
struct ZuPrint<MxValNDPVFmt<Ref> > : public ZuPrintFn { };

// MxMatch*<> - SFINAE templates used in declaring overloaded templated
// functions that need to unambiguously specialize for the various Mx
// vocabulary types
template <typename T> struct MxIsChar { enum { OK =
  ZuConversion<char, T>::Same }; };
template <typename T, typename R = void, bool A = MxIsChar<T>::OK>
  struct MxMatchChar;
template <typename U, typename R>
  struct MxMatchChar<U, R, true> { typedef R T; };

template <typename T> struct MxIsBool { enum { OK =
  ZuConversion<bool, T>::Same ||
  ZuConversion<MxBool, T>::Is }; };
template <typename T, typename R = void, bool A = MxIsBool<T>::OK>
  struct MxMatchBool;
template <typename U, typename R>
  struct MxMatchBool<U, R, true> { typedef R T; };

template <typename T> struct MxIsEnum { enum { OK =
  ZuConversion<MxEnum, T>::Is }; };
template <typename T, typename R = void, bool A = MxIsEnum<T>::OK>
  struct MxMatchEnum;
template <typename U, typename R>
  struct MxMatchEnum<U, R, true> { typedef R T; };

template <typename T> struct MxIsFlags { enum { OK =
  ZuConversion<MxFlags, T>::Is ||
  ZuConversion<MxFlags64, T>::Is }; };
template <typename T, typename R = void, bool A = MxIsFlags<T>::OK>
  struct MxMatchFlags;
template <typename U, typename R>
  struct MxMatchFlags<U, R, true> { typedef R T; };

template <typename T> struct MxIsInt { enum { OK =
  ZuTraits<T>::IsReal &&
  ZuTraits<T>::IsIntegral &&
  !MxIsChar<T>::OK &&
  !MxIsBool<T>::OK &&
  !MxIsEnum<T>::OK &&
  !MxIsFlags<T>::OK }; };
template <typename T, typename R = void, bool A = MxIsInt<T>::OK>
  struct MxMatchInt;
template <typename U, typename R>
  struct MxMatchInt<U, R, true> { typedef R T; };

template <typename T> struct MxIsFloat { enum { OK =
  ZuTraits<T>::IsFloatingPoint }; };
template <typename T, typename R = void, bool A = MxIsFloat<T>::OK>
  struct MxMatchFloat;
template <typename U, typename R>
  struct MxMatchFloat<U, R, true> { typedef R T; };

template <typename T> struct MxIsString { enum { OK =
  ZuConversion<ZuStringN__, T>::Base &&
  !ZuTraits<T>::IsWString }; };
template <typename T, typename R = void, bool A = MxIsString<T>::OK>
  struct MxMatchString;
template <typename U, typename R>
  struct MxMatchString<U, R, true> { typedef R T; };

template <typename T> struct MxIsTime { enum { OK =
  ZuConversion<MxDateTime, T>::Is }; };
template <typename T, typename R = void, bool A = MxIsTime<T>::OK>
  struct MxMatchTime;
template <typename U, typename R>
  struct MxMatchTime<U, R, true> { typedef R T; };

// MxType<T>::T returns the MxType corresponding to the generic type T,
// except that string types are passed through as is
template <typename U> struct MxType;

template <typename U, bool OK> struct MxType_Time { };
template <typename U> struct MxType_Time<U, true> { typedef MxDateTime T; };

template <typename U, bool OK> struct MxType_String :
  public MxType_Time<U, MxIsTime<U>::OK> { };
template <typename U> struct MxType_String<U, true> { typedef U T; };

template <typename U, bool OK> struct MxType_Float :
  public MxType_String<U, MxIsString<U>::OK> { };
template <typename U> struct MxType_Float<U, true> { typedef MxFloat T; };

template <bool, bool> struct MxType_Int_;
template <> struct MxType_Int_<0, 0> { typedef MxUInt T; };
template <> struct MxType_Int_<0, 1> { typedef MxInt T; };
template <> struct MxType_Int_<1, 0> { typedef MxUInt64 T; };
template <> struct MxType_Int_<1, 1> { typedef MxInt64 T; };
template <typename U, bool OK> struct MxType_Int :
  public MxType_Float<U, MxIsFloat<U>::OK> { };
template <typename U> struct MxType_Int<U, true> {
  typedef typename MxType_Int_<sizeof(U) <= 4, ZuTraits<U>::IsSigned>::T T;
};

template <typename U, bool OK> struct MxType_Flags :
  public MxType_Int<U, MxIsInt<U>::OK> { };
template <typename U> struct MxType_Flags<U, true> { typedef U T; };

template <typename U, bool OK> struct MxType_Enum :
  public MxType_Flags<U, MxIsFlags<U>::OK> { };
template <typename U> struct MxType_Enum<U, true> { typedef U T; };

template <typename U, bool OK> struct MxType_Bool :
  public MxType_Enum<U, MxIsEnum<U>::OK> { };
template <typename U> struct MxType_Bool<U, true> { typedef MxBool T; };

template <typename U, bool OK> struct MxType_Char :
  public MxType_Bool<U, MxIsBool<U>::OK> { };
template <typename U> struct MxType_Char<U, true> { typedef MxChar T; };

template <typename U> struct MxType :
    public MxType_Char<U, MxIsChar<U>::OK> { };

#define MxDateTimeNow ZtDateNow

// Note: MxString/ZuStringN overhead is 4 bytes

#ifndef MxIDStrSize
#define MxIDStrSize 28	// ID size (symbols, order IDs, trade IDs, etc.)
#endif
#ifndef MxTxtSize
#define MxTxtSize 124	// text field size (alerts, error messages, etc.)
#endif

typedef MxString<MxIDStrSize> MxIDString;
typedef MxString<MxTxtSize> MxTxtString;

#define MxEnumValues ZtEnumValues
#define MxEnumNames ZtEnumNames
#define MxEnumMap ZtEnumMap
#define MxEnumMapAlias(Map, Alias) typedef Map Alias
#define MxEnumFlags ZtEnumFlags

// application types

namespace MxInstrIDSrc {
  MxEnumValues(CUSIP, SEDOL, QUIK, ISIN, RIC, EXCH, CTA, BSYM, BBGID,
      FX, CRYPTO);
  MxEnumNames(
      "CUSIP", "SEDOL", "QUIK", "ISIN", "RIC", "EXCH", "CTA", "BSYM", "BBGID",
      "FX", "CRYPTO");
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "1", CUSIP, "2", SEDOL, "3", QUIK, "4", ISIN, "5", RIC, "8", EXCH,
      "9", CTA, "A", BSYM, "S", BBGID, "X", FX, "C", CRYPTO);
}

namespace MxPutCall {
  MxEnumValues(PUT, CALL);
  MxEnumNames("PUT", "CALL");
  MxEnumMap(CSVMap,
      "P", PUT, "PUT", PUT, "Put", PUT, "0", PUT,
      "C", CALL, "CALL", CALL, "Call", CALL, "1", CALL);
  MxEnumMap(FixMap, "0", PUT, "1", CALL);
}

namespace MxTickDir {
  MxEnumValues(Up, LevelUp, Down, LevelDown, NoTick);
  MxEnumNames("Up", "LevelUp", "Down", "LevelDown", "NoTick");
  MxEnumMap(CSVMap,
      "U", Up, "0", Up,
      "UL", LevelUp, "1", LevelUp,
      "D", Down, "2", Down,
      "DL", LevelDown, "3", LevelDown);
  MxEnumMap(FixMap, "0", Up, "1", LevelUp, "2", Down, "3", LevelDown);
}

namespace MxTradingStatus {
  MxEnumValues(
      Open, Closed, PreOpen, Auction,
      Halted, Resumed, NotTraded, Unwinding, Unknown);
  MxEnumNames(
      "Open", "Closed", "PreOpen", "Auction",
      "Halted", "Resumed", "NotTraded", "Unwinding", "Unknown");
  MxEnumMap(CSVMap,
      "Open", Open, "17", Open,
      "Closed", Closed, "18", Closed,
      "PreOpen", PreOpen, "21", PreOpen,
      "Auction", Auction, "5", Auction,
      "Halted", Halted, "2", Halted,
      "Resumed", Resumed, "3", Resumed,
      "NotTraded", NotTraded, "19", NotTraded,
      "Unwinding", Unwinding, "100", Unwinding,
      "Unknown", Unknown, "20", Unknown);
  MxEnumMap(FixMap,
      "17", Open, "18", Closed, "21", PreOpen, "5", Auction,
      "2", Halted, "3", Resumed, "19", NotTraded, "20", Unknown);
}

namespace MxTradingSession {
  MxEnumValues(
      PreTrading, Opening, Continuous, Closing, PostTrading,
      IntradayAuction, Quiescent);
  MxEnumNames(
      "PreTrading", "Opening", "Continuous", "Closing", "PostTrading",
      "IntradayAuction", "Quiescent");
  MxEnumMap(CSVMap,
      "PreTrading", PreTrading, "1", PreTrading,
      "Opening", Opening, "2", Opening,
      "Continuous", Continuous, "3", Continuous,
      "Closing", Closing, "4", Closing,
      "PostTrading", PostTrading, "5", PostTrading,
      "IntradayAuction", IntradayAuction, "6", IntradayAuction,
      "Quiescent", Quiescent, "7", Quiescent);
  MxEnumMap(FixMap,
      "1", PreTrading,
      "2", Opening,
      "3", Continuous,
      "4", Closing,
      "5", PostTrading,
      "6", IntradayAuction,
      "7", Quiescent);
}

namespace MxSide {
  MxEnumValues(Buy, Sell, SellShort, SellShortExempt, Cross);
  MxEnumNames("Buy", "Sell", "SellShort", "SellShortExempt", "Cross");
  MxEnumMap(CSVMap,
      "Buy", Buy, "1", Buy,
      "Sell", Sell, "2", Sell,
      "SellShort", SellShort, "5", SellShort,
      "SellShortExempt", SellShortExempt, "6", SellShortExempt,
      "Cross", Cross, "8", Cross);
  MxEnumMap(FixMap,
      "1", Buy,
      "2", Sell,
      "5", SellShort,
      "6", SellShortExempt,
      "8", Cross);
}

// instruments are fundamentally identified either by
// venue/segment and the venue's native identifier - MxInstrKey, or
// instrument ID source (aka symbology) and a unique symbol - MxSymKey;
// if not directly identified by MxInstrKey or MxSymKey, individual
// futures/options can be specified by underlying + parameters, e.g.
// "MSFT Mar 2019 Call Option @100", which is expressed as
// the MxInstrKey or MxSymKey for the underlying (MSFT) together
// with an MxOptKey{20190300, MxPutCall::CALL, 10000} (assuming pxNDP=2)
//
// an individual instrument might therefore be identified by:
//
// MxInstrKey - market-native ID
// MxSymKey - industry standard symbology
// MxInstrKey or MxSymKey, with MxFutKey - future by maturity
// MxInstrKey or MxSymKey, with MxOptKey - option by mat/putCall/strike
//
// for cases (FIX message parsing, etc.) where the type of key cannot be
// pre-determined at compile time, MxUniKey ("universal key") can be used -
// this is capable of encapsulating all the above possibilities but unused
// fields potentially waste memory

#pragma pack(push, 1)

struct MxInstrKey {
  MxIDString	id;
  MxID		venue;
  MxID		segment;

  inline bool operator ==(const MxInstrKey &v) const {
    return id == v.id && venue == v.venue && segment == v.segment;
  }
  inline bool operator !=(const MxInstrKey &v) const { return !operator ==(v); }

  inline int cmp(const MxInstrKey &v) const {
    int i;
    if (i = id.cmp(v.id)) return i;
    if (i = venue.cmp(v.venue)) return i;
    return segment.cmp(v.segment);
  }
  inline bool operator >(const MxInstrKey &v) { return cmp(v) > 0; }
  inline bool operator >=(const MxInstrKey &v) { return cmp(v) >= 0; }
  inline bool operator <(const MxInstrKey &v) { return cmp(v) < 0; }
  inline bool operator <=(const MxInstrKey &v) { return cmp(v) <= 0; }

  inline uint32_t hash() const {
    return id.hash() ^ venue.hash() ^ segment.hash();
  }

  template <typename S> inline void print(S &s) const {
    s << venue << '|' << segment << '|' << id;
  }
};
template <> struct ZuTraits<MxInstrKey> : public ZuGenericTraits<MxInstrKey> {
  enum { IsPOD = 1, IsComparable = 1, IsHashable = 1 };
};
template <> struct ZuPrint<MxInstrKey> : public ZuPrintFn { };

struct MxSymKey {
  MxIDString	id;
  MxEnum	src;

  inline bool operator ==(const MxSymKey &v) const {
    return id == v.id && src == v.src;
  }
  inline bool operator !=(const MxSymKey &v) const { return !operator ==(v); }

  inline int cmp(const MxSymKey &v) const {
    int i;
    if (i = id.cmp(v.id)) return i;
    return src.cmp(v.src);
  }
  inline bool operator >(const MxSymKey &v) { return cmp(v) > 0; }
  inline bool operator >=(const MxSymKey &v) { return cmp(v) >= 0; }
  inline bool operator <(const MxSymKey &v) { return cmp(v) < 0; }
  inline bool operator <=(const MxSymKey &v) { return cmp(v) <= 0; }

  inline uint32_t hash() const {
    return id.hash() ^ src.hash();
  }

  template <typename S> inline void print(S &s) const {
    s << src << '|' << id;
  }
};
template <> struct ZuTraits<MxSymKey> : public ZuGenericTraits<MxSymKey> {
  enum { IsPOD = 1, IsComparable = 1, IsHashable = 1 };
};
template <> struct ZuPrint<MxSymKey> : public ZuPrintFn { };

typedef MxUInt MxFutKey;	// mat

struct MxOptKey {
  MxValue	strike;
  MxUInt	mat;
  MxEnum	putCall;

  inline bool operator ==(const MxOptKey &v) const {
    return strike == v.strike && mat == v.mat && putCall == v.putCall;
  }
  inline bool operator !=(const MxOptKey &v) const { return !operator ==(v); }

  inline int cmp(const MxOptKey &v) const {
    int i;
    if (i = strike.cmp(v.strike)) return i;
    if (i = mat.cmp(v.mat)) return i;
    return putCall.cmp(v.putCall);
  }
  inline bool operator >(const MxOptKey &v) { return cmp(v) > 0; }
  inline bool operator >=(const MxOptKey &v) { return cmp(v) >= 0; }
  inline bool operator <(const MxOptKey &v) { return cmp(v) < 0; }
  inline bool operator <=(const MxOptKey &v) { return cmp(v) <= 0; }

  inline uint32_t hash() const {
    return strike.hash() ^ mat.hash() ^ putCall.hash();
  }

  template <typename S> inline void print(S &s) const {
    s << mat << '|' << MxPutCall::name(putCall) << '|' << strike;
  }
};
template <> struct ZuTraits<MxOptKey> : public ZuGenericTraits<MxOptKey> {
  enum { IsPOD = 1, IsComparable = 1, IsHashable = 1 };
};
template <> struct ZuPrint<MxOptKey> : public ZuPrintFn { };

struct MxUniKey {
  MxIDString	id;
  MxID		venue;
  MxID		segment;
  MxValue	strike;
  MxUInt	mat;
  MxEnum	src;
  MxEnum	putCall;

  ZuInline bool operator !() const { return !id; }
  ZuOpBool

  inline bool operator ==(const MxUniKey &v) const {
    if (id != v.id || src != v.src) return false;
    if (!*src && (venue != v.venue || segment != v.venue)) return false;
    if (mat != v.mat) return false;
    if (*mat) {
      if (strike != v.strike) return false;
      if (*strike && (putCall != v.putCall)) return false;
    }
    return true;
  }
  inline bool operator !=(const MxUniKey &v) const { return !operator ==(v); }

  inline int cmp(const MxUniKey &v) const {
    int i;
    if (i = id.cmp(v.id)) return i;
    if (i = src.cmp(v.src)) return i;
    if (!*src) {
      if (i = venue.cmp(v.venue)) return i;
      if (i = segment.cmp(v.segment)) return i;
    }
    if (i = mat.cmp(v.mat)) return i;
    if (!*mat) return 0;
    if (i = strike.cmp(v.strike)) return i;
    if (!*strike) return 0;
    return putCall.cmp(v.putCall);
  }
  inline bool operator >(const MxUniKey &v) { return cmp(v) > 0; }
  inline bool operator >=(const MxUniKey &v) { return cmp(v) >= 0; }
  inline bool operator <(const MxUniKey &v) { return cmp(v) < 0; }
  inline bool operator <=(const MxUniKey &v) { return cmp(v) <= 0; }

  inline uint32_t hash() const {
    uint32_t code = id.hash();
    if (*src)
      code ^= src.hash();
    else
      code ^= venue.hash() ^ segment.hash();
    if (!*mat) return code;
    code ^= mat.hash();
    if (!*strike) return code;
    return code ^ strike.hash() ^ (uint32_t)putCall;
  }

  template <typename S> inline void print(S &s) const {
    if (*src)
      s << MxSymKey{id, src};
    else
      s << MxInstrKey{id, venue, segment};
    if (*mat) {
      if (*strike)
	s << '|' << MxOptKey{mat, putCall, strike};
      else
	s << '|' << MxFutKey{mat};
    }
  }
};
template <> struct ZuTraits<MxUniKey> : public ZuGenericTraits<MxUniKey> {
  enum { IsPOD = 1, IsComparable = 1, IsHashable = 1 };
};
template <> struct ZuPrint<MxUniKey> : public ZuPrintFn { };

#pragma pack(pop)

#endif /* MxBase_HPP */
