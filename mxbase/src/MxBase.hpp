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
#include <MxBaseLib.hpp>
#endif

#include <ZuInt.hpp>
#include <ZuBox.hpp>
#include <ZuPair.hpp>
#include <ZuTuple.hpp>
#include <ZuMixin.hpp>
#include <ZuStringN.hpp>
#include <ZuPrint.hpp>
#include <ZuID.hpp>

#include <ZmRef.hpp>
#include <ZmLHash.hpp>
#include <ZmStream.hpp>

#include <ZtDate.hpp>
#include <ZtString.hpp>
#include <ZtEnum.hpp>

#include <ZeLog.hpp>

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
typedef ZmTime MxDeltaTime;
typedef ZtEnum MxEnum;
typedef ZuBox0(uint32_t) MxFlags;
typedef ZuBox0(uint64_t) MxFlags64;

struct MxSeqNoCmp {
  ZuInline static int cmp(uint64_t v1, uint64_t v2) {
    int64_t d = v1 - v2; // handles wraparound ok
    return d | (d>>((sizeof(int64_t)-sizeof(int))<<3)); // safely narrows
  }
  ZuInline static bool equals(uint64_t v1, uint64_t v2) { return v1 == v2; }
  ZuInline static bool null(uint64_t v) { return !v; }
  ZuInline static uint64_t null() { return 0; }
};
typedef ZuBox<uint64_t, MxSeqNoCmp> MxSeqNo;

#define MxString ZuStringN

typedef ZuID MxID; // Note: different than MxIDString

typedef MxInt64 MxFixed;	// fixed point value (numerator)
typedef MxUInt64 MxNDP;		// number of decimal places (log10(denominator))
typedef MxUInt MxRatio;		// ratio numerator (orders w/ multiple legs)
				// (denominator is sum of individual ratios)

// combination of value and ndp, used as a temporary for conversions & IO
// constructors / scanning:
//   MxFixedNDP(MxFixed, ndp)
//   MxFixedNDP(MxFloat, ndp)
//   MxFixedNDP(<string>, ndp)	// scan
//   MxFixed x = MxFixedNDP("42.42", 2).value // x = 4242
// printing:
//   s << MxFixedNDP(...)		// print (default)
//   s << MxFixedNDP(...).fmt(ZuFmt...)	// print (compile-time formatted)
//   s << MxFixedNDP(...).vfmt(ZuVFmt...)// print (variable run-time formatted)
//   s << MxFixedNDP(x, 2) // s << "42.42"
//   x = 4200042; // 42000.42
//   s << MxFixedNDP(x, 2).fmt(ZuFmt::Comma<>()) // s << "42,000.42"
template <typename Fmt> struct MxFixedNDPFmt;
template <bool Ref = 0> struct MxFixedNDPVFmt;
struct MxFixedNDP {
  MxFixed	value;
  MxNDP		ndp;

  ZuInline explicit MxFixedNDP(MxFixed value_, MxNDP ndp_) :
    value(value_), ndp(ndp_) { }

  ZuInline explicit MxFixedNDP(MxFloat value_, MxNDP ndp_) :
    value(value_ * (MxFloat)ndp_), ndp(ndp_) { }

  template <typename S>
  inline MxFixedNDP(const S &s_, MxNDP ndp_,
      typename ZuIsString<S>::T *_ = 0) : ndp(ndp_) {
    ZuString s(s_);
    MxFixed iv = 0, fv = 0;
    s.offset(Zu_atoi(iv, s.data(), s.length()));
    unsigned n;
    if ((n = s.length()) > 1 && s[0] == '.') {
      if (--n > ndp) n = ndp;
      n = Zu_atou(fv, &s[1], n);
      if (n && ndp > n)
	fv *= ZuDecimal::pow10_64(ndp - n);
    }
    value = iv * ZuDecimal::pow10_64(ndp) + fv;
  }

  ZuInline MxFloat floating() const {
    return (MxFloat)value / (MxFloat)ZuDecimal::pow10_64(ndp);
  }

  template <typename S> void print(S &s) const;

  template <typename Fmt> MxFixedNDPFmt<Fmt> fmt(Fmt) const;
  MxFixedNDPVFmt<> vfmt() const;
  MxFixedNDPVFmt<1> vfmt(ZuVFmt &) const;
};
template <typename Fmt> struct MxFixedNDPFmt {
  const MxFixedNDP	&fixedNDP;

  template <typename S> inline void print(S &s) const {
    MxFixed iv = fixedNDP.value;
    if (ZuUnlikely(iv < 0)) { s << '-'; iv = -iv; }
    uint64_t factor = ZuDecimal::pow10_64(fixedNDP.ndp);
    MxFixed fv = iv % factor;
    iv /= factor;
    s << ZuBoxed(iv).fmt(Fmt());
    if (fv) s << '.' << ZuBoxed(fv).vfmt().frac(fixedNDP.ndp);
  }
};
template <class Fmt>
ZuInline MxFixedNDPFmt<Fmt> MxFixedNDP::fmt(Fmt) const
{
  return MxFixedNDPFmt<Fmt>{*this};
}
template <typename Fmt>
struct ZuPrint<MxFixedNDPFmt<Fmt> > : public ZuPrintFn { };
template <typename S> inline void MxFixedNDP::print(S &s) const
{
  s << MxFixedNDPFmt<ZuFmt::Default>{*this};
}
template <bool Ref>
struct MxFixedNDPVFmt : public ZuVFmtWrapper<MxFixedNDPVFmt<Ref>, Ref> {
  const MxFixedNDP	&fixedNDP;

  ZuInline MxFixedNDPVFmt(const MxFixedNDP &fixedNDP_) :
    fixedNDP{fixedNDP_} { }
  ZuInline MxFixedNDPVFmt(const MxFixedNDP &fixedNDP_, ZuVFmt &fmt_) :
    ZuVFmtWrapper<MxFixedNDPVFmt, Ref>{fmt_}, fixedNDP{fixedNDP_} { }

  template <typename S> inline void print(S &s) const {
    MxFixed iv = fixedNDP.value;
    if (ZuUnlikely(iv < 0)) { s << '-'; iv = -iv; }
    uint64_t factor = ZuDecimal::pow10_64(fixedNDP.ndp);
    MxFixed fv = iv % factor;
    iv /= factor;
    s << ZuBoxed(iv).vfmt(this->fmt);
    if (fv) s << '.' << ZuBoxed(fv).vfmt().frac(fixedNDP.ndp);
  }
};
ZuInline MxFixedNDPVFmt<> MxFixedNDP::vfmt() const
{
  return MxFixedNDPVFmt<>{*this};
}
ZuInline MxFixedNDPVFmt<1> MxFixedNDP::vfmt(ZuVFmt &fmt) const
{
  return MxFixedNDPVFmt<1>{*this, fmt};
}
template <bool Ref>
struct ZuPrint<MxFixedNDPVFmt<Ref> > : public ZuPrintFn { };

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

#ifndef MxSymSize
#define MxSymSize 32	// security symbol size (including null terminator)
#endif
#ifndef MxIDStrSize
#define MxIDStrSize 32	// ID size (order IDs, trade IDs, etc.)
#endif
#ifndef MxTxtSize
#define MxTxtSize 124	// text field size (alerts, error messages, etc.)
#endif

typedef MxString<MxSymSize> MxSymString;
typedef MxString<MxIDStrSize> MxIDString;
typedef MxString<MxTxtSize> MxTxtString;

#define MxEnumValues ZtEnumValues
#define MxEnumNames ZtEnumNames
#define MxEnumMap ZtEnumMap
#define MxEnumMapAlias(Map, Alias) typedef Map Alias
#define MxEnumFlags ZtEnumFlags

// application types

struct MxSecIDSrc {
  MxEnumValues(CUSIP, SEDOL, QUIK, ISIN, RIC, EXCH, CTA, BSYM, BBGID,
      FX, CRYPTO);
  MxEnumNames(
      "CUSIP", "SEDOL", "QUIK", "ISIN", "RIC", "EXCH", "CTA", "BSYM", "BBGID",
      "FX", "CRYPTO");
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "1", CUSIP, "2", SEDOL, "3", QUIK, "4", ISIN, "5", RIC, "8", EXCH,
      "9", CTA, "A", BSYM, "S", BBGID, "X", FX, "Y", CRYPTO);
};

struct MxPutCall {
  MxEnumValues(PUT, CALL);
  MxEnumNames("PUT", "CALL");
  MxEnumMap(CSVMap,
      "P", PUT, "PUT", PUT, "Put", PUT, "0", PUT,
      "C", CALL, "CALL", CALL, "Call", CALL, "1", CALL);
  MxEnumMap(FixMap, "0", PUT, "1", CALL);
};

struct MxTickDir {
  MxEnumValues(Up, LevelUp, Down, LevelDown, NoTick);
  MxEnumNames("Up", "LevelUp", "Down", "LevelDown", "NoTick");
  MxEnumMap(CSVMap,
      "U", Up, "0", Up,
      "UL", LevelUp, "1", LevelUp,
      "D", Down, "2", Down,
      "DL", LevelDown, "3", LevelDown);
  MxEnumMap(FixMap, "0", Up, "1", LevelUp, "2", Down, "3", LevelDown);
};

struct MxTradingStatus {
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
};

struct MxTradingSession {
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
};

struct MxSide {
  MxEnumValues(Buy, Sell, SellShort, SellShortExempt, Cross);
  MxEnumNames("Buy", "Sell", "SellShort", "SellShortExempt", "Cross");
  MxEnumMap(CSVMap,
      "B", Buy, "1", Buy,
      "S", Sell, "2", Sell, "C", Sell,
      "SS", SellShort, "5", SellShort,
      "SSE", SellShortExempt, "6", SellShortExempt,
      "X", Cross, "8", Cross);
  MxEnumMap(FixMap,
      "1", Buy, "B", Buy,
      "2", Sell, "C", Sell,
      "5", SellShort,
      "6", SellShortExempt,
      "8", Cross);
};

#define Mx_TupleField(Type, Fn, N) \
  ZuInline const Type &Fn() const { return T::ptr()->p ## N(); } \
  ZuInline Type &Fn() { return T::ptr()->p ## N(); }

typedef ZuTuple<MxID, MxID, MxIDString> MxSecKey_;
template <typename T> struct MxSecKey_Fn : public T {
  Mx_TupleField(MxID, venue, 1)
  Mx_TupleField(MxID, segment, 2)
  Mx_TupleField(MxIDString, id, 3)
  template <typename S> inline void print(S &s) const {
    s << venue() << '|' << segment() << '|' << id();
  }
};
typedef ZuMixin<MxSecKey_, MxSecKey_Fn> MxSecKey;
template <> struct ZuPrint<MxSecKey> : public ZuPrintFn { };

typedef ZuPair<MxEnum, MxSymString> MxSecSymKey_;
template <typename T> struct MxSecSymKey_Fn : public T {
  Mx_TupleField(MxEnum, src, 1)
  Mx_TupleField(MxSymString, id, 2)
  template <typename S> inline void print(S &s) const {
    s << MxSecIDSrc::name(src()) << '|' << id();
  }
};
typedef ZuMixin<MxSecSymKey_, MxSecSymKey_Fn> MxSecSymKey;
template <> struct ZuPrint<MxSecSymKey> : public ZuPrintFn { };

typedef MxUInt MxFutKey;	// mat

typedef ZuTuple<MxUInt, MxEnum, MxInt> MxOptKey_;
template <typename T> struct MxOptKey_Fn : public T {
  Mx_TupleField(MxUInt, mat, 1)
  Mx_TupleField(MxEnum, putCall, 2)
  Mx_TupleField(MxInt, strike, 3)
};
typedef ZuMixin<MxOptKey_, MxOptKey_Fn> MxOptKey;

#undef Mx_TupleField

#endif /* MxBase_HPP */
