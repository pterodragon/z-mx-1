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

typedef ZuBox0(char) MxChar;
typedef ZuBox<int8_t> MxBool;
typedef ZuBox<uint8_t> MxUInt8;
typedef ZuBox<int32_t> MxInt;
typedef ZuBox<uint32_t> MxUInt;
typedef ZuBox<int64_t> MxInt64;
typedef ZuBox<uint64_t> MxUInt64;
typedef ZuBox<double> MxFloat;
typedef ZtDate MxDateTime;
typedef ZmTime MxDeltaTime;
typedef ZtEnum MxEnum;
typedef ZuBox0(uint32_t) MxFlags;
typedef ZuBox0(uint64_t) MxFlags64;
struct MxSeqNoCmp {
  inline static int cmp(uint64_t v1, uint64_t v2) {
    int64_t d = v1 - v2; // handles wraparound ok
    return d>>((sizeof(int64_t)-sizeof(int))<<3); // safely narrows
  }
  inline static bool equals(uint64_t v1, uint64_t v2) { return v1 == v2; }
  inline static bool null(uint64_t v) { return !v; }
  inline static uint64_t null() { return 0; }
};
typedef ZuBox<uint64_t, MxSeqNoCmp> MxSeqNo;
#define MxString ZuStringN
typedef ZuID MxID; // Note: different than MxIDString

// MxMatch*<> - SFINAE templates used in declaring overloaded templated
// functions that need to unambiguously specialize for the various Mx
// vocabulary types
template <typename T> struct MxMatchChar_ { enum { OK =
  ZuConversion<char, T>::Same }; };
template <typename T, typename R = void, bool A = MxMatchChar_<T>::OK>
  struct MxMatchChar;
template <typename U, typename R>
  struct MxMatchChar<U, R, true> { typedef R T; };

template <typename T> struct MxMatchBool_ { enum { OK =
  ZuConversion<bool, T>::Same ||
  ZuConversion<MxBool, T>::Is }; };
template <typename T, typename R = void, bool A = MxMatchBool_<T>::OK>
  struct MxMatchBool;
template <typename U, typename R>
  struct MxMatchBool<U, R, true> { typedef R T; };

template <typename T> struct MxMatchEnum_ { enum { OK =
  ZuConversion<MxEnum, T>::Is }; };
template <typename T, typename R = void, bool A = MxMatchEnum_<T>::OK>
  struct MxMatchEnum;
template <typename U, typename R>
  struct MxMatchEnum<U, R, true> { typedef R T; };

template <typename T> struct MxMatchFlags_ { enum { OK =
  ZuConversion<MxFlags, T>::Is ||
  ZuConversion<MxFlags64, T>::Is }; };
template <typename T, typename R = void, bool A = MxMatchFlags_<T>::OK>
  struct MxMatchFlags;
template <typename U, typename R>
  struct MxMatchFlags<U, R, true> { typedef R T; };

template <typename T> struct MxMatchInt_ { enum { OK =
  ZuTraits<T>::IsReal &&
  ZuTraits<T>::IsIntegral &&
  !MxMatchChar_<T>::OK &&
  !MxMatchBool_<T>::OK &&
  !MxMatchEnum_<T>::OK &&
  !MxMatchFlags_<T>::OK }; };
template <typename T, typename R = void, bool A = MxMatchInt_<T>::OK>
  struct MxMatchInt;
template <typename U, typename R>
  struct MxMatchInt<U, R, true> { typedef R T; };

template <typename T> struct MxMatchFloat_ { enum { OK =
  ZuTraits<T>::IsFloatingPoint }; };
template <typename T, typename R = void, bool A = MxMatchFloat_<T>::OK>
  struct MxMatchFloat;
template <typename U, typename R>
  struct MxMatchFloat<U, R, true> { typedef R T; };

template <typename T> struct MxMatchString_ { enum { OK =
  ZuConversion<ZuStringN__, T>::Base &&
  !ZuTraits<T>::IsWString }; };
template <typename T, typename R = void, bool A = MxMatchString_<T>::OK>
  struct MxMatchString;
template <typename U, typename R>
  struct MxMatchString<U, R, true> { typedef R T; };

template <typename T> struct MxMatchTime_ { enum { OK =
  ZuConversion<MxDateTime, T>::Is }; };
template <typename T, typename R = void, bool A = MxMatchTime_<T>::OK>
  struct MxMatchTime;
template <typename U, typename R>
  struct MxMatchTime<U, R, true> { typedef R T; };

// MxType<T>::T returns the MxType corresponding to the generic type T,
// except that string types are passed through as is
template <typename U> struct MxType;

template <typename U, bool OK> struct MxType_Time { };
template <typename U> struct MxType_Time<U, true> { typedef MxDateTime T; };

template <typename U, bool OK> struct MxType_String :
  public MxType_Time<U, MxMatchTime_<U>::OK> { };
template <typename U> struct MxType_String<U, true> { typedef U T; };

template <typename U, bool OK> struct MxType_Float :
  public MxType_String<U, MxMatchString_<U>::OK> { };
template <typename U> struct MxType_Float<U, true> { typedef MxFloat T; };

template <bool, bool> struct MxType_Int_;
template <> struct MxType_Int_<0, 0> { typedef MxUInt T; };
template <> struct MxType_Int_<0, 1> { typedef MxInt T; };
template <> struct MxType_Int_<1, 0> { typedef MxUInt64 T; };
template <> struct MxType_Int_<1, 1> { typedef MxInt64 T; };
template <typename U, bool OK> struct MxType_Int :
  public MxType_Float<U, MxMatchFloat_<U>::OK> { };
template <typename U> struct MxType_Int<U, true> {
  typedef typename MxType_Int_<sizeof(U) <= 4, ZuTraits<U>::IsSigned>::T T;
};

template <typename U, bool OK> struct MxType_Flags :
  public MxType_Int<U, MxMatchInt_<U>::OK> { };
template <typename U> struct MxType_Flags<U, true> { typedef U T; };

template <typename U, bool OK> struct MxType_Enum :
  public MxType_Flags<U, MxMatchFlags_<U>::OK> { };
template <typename U> struct MxType_Enum<U, true> { typedef U T; };

template <typename U, bool OK> struct MxType_Bool :
  public MxType_Enum<U, MxMatchEnum_<U>::OK> { };
template <typename U> struct MxType_Bool<U, true> { typedef MxBool T; };

template <typename U, bool OK> struct MxType_Char :
  public MxType_Bool<U, MxMatchBool_<U>::OK> { };
template <typename U> struct MxType_Char<U, true> { typedef MxChar T; };

template <typename U> struct MxType :
    public MxType_Char<U, MxMatchChar_<U>::OK> { };

#define MxDateTimeNow ZtDateNow

#ifndef MxSymSize
#define MxSymSize 32	// security symbol size (including null terminator)
#endif
#ifndef MxCcySize
#define MxCcySize 4	// currency symbol size
#endif
#ifndef MxIDStrSize
#define MxIDStrSize 32	// ID size (order IDs, trade IDs, etc.)
#endif
#ifndef MxTxtSize
#define MxTxtSize 124	// text field size (alerts, error messages, etc.)
#endif

typedef MxString<MxSymSize> MxSymString;
typedef MxString<MxCcySize> MxCcyString;
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
      FXP, CFXP); // FIXME ?
  MxEnumNames(
      "CUSIP", "SEDOL", "QUIK", "ISIN", "RIC", "EXCH", "CTA", "BSYM", "BBGID",
      "FXP", "CFXP"); // FIXME ?
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "1", CUSIP, "2", SEDOL, "3", QUIK, "4", ISIN, "5", RIC, "8", EXCH,
      "9", CTA, "A", BSYM, "S", BBGID /* FIXME */);
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

typedef ZuTuple<MxID, MxID, MxSymString> MxSecKey_;
template <typename T> struct MxSecKey_Fn : public T {
  ZuInline const MxID &venue() const { return T::ptr()->p1(); }
  ZuInline MxID &venue() { return T::ptr()->p1(); }
  ZuInline const MxID &segment() const { return T::ptr()->p2(); }
  ZuInline MxID &segment() { return T::ptr()->p2(); }
  ZuInline const MxSymString &id() const { return T::ptr()->p3(); }
  ZuInline MxSymString &id() { return T::ptr()->p3(); }
};
typedef ZuMixin<MxSecKey_, MxSecKey_Fn> MxSecKey;

typedef ZuPair<MxEnum, MxSymString> MxSecSymKey_;
template <typename T> struct MxSecSymKey_Fn : public T {
  ZuInline const MxEnum &src() const { return T::ptr()->p1(); }
  ZuInline MxEnum &src() { return T::ptr()->p1(); }
  ZuInline const MxSymString &id() const { return T::ptr()->p2(); }
  ZuInline MxSymString &id() { return T::ptr()->p2(); }
};
typedef ZuMixin<MxSecSymKey_, MxSecSymKey_Fn> MxSecSymKey;

typedef MxUInt MxFutKey;	// mat

typedef ZuTuple<MxUInt, MxEnum, MxInt> MxOptKey_;
template <typename T> struct MxOptKey_Fn : public T {
  ZuInline const MxUInt &mat() const { return T::ptr()->p1(); }
  ZuInline MxUInt &mat() { return T::ptr()->p1(); }
  ZuInline const MxEnum &putCall() const { return T::ptr()->p2(); }
  ZuInline MxEnum &putCall() { return T::ptr()->p2(); }
  ZuInline const MxInt &strike() const { return T::ptr()->p3(); }
  ZuInline MxInt &strike() { return T::ptr()->p3(); }
};
typedef ZuMixin<MxOptKey_, MxOptKey_Fn> MxOptKey;

#endif /* MxBase_HPP */
