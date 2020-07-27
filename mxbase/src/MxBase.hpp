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
#include <zlib/ZuFixed.hpp>

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

using MxChar = ZuBox0(char);
using MxBool = ZuBox<int8_t>;
using MxUInt8 = ZuBox<uint8_t>;
using MxInt = ZuBox<int32_t>;
using MxUInt = ZuBox<uint32_t>;
using MxInt64 = ZuBox<int64_t>;
using MxUInt64 = ZuBox<uint64_t>;
using MxInt128 = ZuBox<int128_t>;
using MxUInt128 = ZuBox<uint128_t>;
#if MxBase_LongDouble
using MxFloat = ZuBox<long double>;
#else
using MxFloat = ZuBox<double>;
#endif
using MxDateTime = ZtDate;
#define MxNow ZtDateNow
using MxDeltaTime = ZmTime;
using MxEnum = ZtEnum;
using MxFlags = ZuBox0(uint32_t);
using MxFlags64 = ZuBox0(uint64_t);

#define MxString ZuStringN

using MxID = ZuID; // Note: different than MxIDString

using MxDecimal = ZuDecimal;

// FIXME - rename to be consistent with ZuFixed (NDP -> Exp, etc.)
using MxValue = ZuFixedVal;	// fixed point value (numerator)
#define MxValueMin ZuFixedMin
#define MxValueMax ZuFixedMax
#define MxValueReset ZuFixedReset
using MxNDP = ZuFixedExp;	// number of decimal places (log10(denominator))
using MxValNDP = ZuFixed;

using MxRatio = MxUInt8;	// ratio numerator (orders w/ multiple legs)
				// (denominator is sum of individual ratios)

// MxMatch*<> - SFINAE templates used in declaring overloaded templated
// functions that need to unambiguously specialize for the various Mx
// vocabulary types
template <typename T> struct MxIsChar { enum { OK =
  ZuConversion<char, T>::Same }; };
template <typename T, typename R = void, bool A = MxIsChar<T>::OK>
  struct MxMatchChar;
template <typename U, typename R>
  struct MxMatchChar<U, R, true> { using T = R; };

template <typename T> struct MxIsBool { enum { OK =
  ZuConversion<bool, T>::Same ||
  ZuConversion<MxBool, T>::Is }; };
template <typename T, typename R = void, bool A = MxIsBool<T>::OK>
  struct MxMatchBool;
template <typename U, typename R>
  struct MxMatchBool<U, R, true> { using T = R; };

template <typename T> struct MxIsEnum { enum { OK =
  ZuConversion<MxEnum, T>::Is }; };
template <typename T, typename R = void, bool A = MxIsEnum<T>::OK>
  struct MxMatchEnum;
template <typename U, typename R>
  struct MxMatchEnum<U, R, true> { using T = R; };

template <typename T> struct MxIsFlags { enum { OK =
  ZuConversion<MxFlags, T>::Is ||
  ZuConversion<MxFlags64, T>::Is }; };
template <typename T, typename R = void, bool A = MxIsFlags<T>::OK>
  struct MxMatchFlags;
template <typename U, typename R>
  struct MxMatchFlags<U, R, true> { using T = R; };

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
  struct MxMatchInt<U, R, true> { using T = R; };

template <typename T> struct MxIsFloat { enum { OK =
  ZuTraits<T>::IsFloatingPoint }; };
template <typename T, typename R = void, bool A = MxIsFloat<T>::OK>
  struct MxMatchFloat;
template <typename U, typename R>
  struct MxMatchFloat<U, R, true> { using T = R; };

template <typename T> struct MxIsString { enum { OK =
  ZuConversion<ZuStringN__, T>::Base &&
  !ZuTraits<T>::IsWString }; };
template <typename T, typename R = void, bool A = MxIsString<T>::OK>
  struct MxMatchString;
template <typename U, typename R>
  struct MxMatchString<U, R, true> { using T = R; };

template <typename T> struct MxIsTime { enum { OK =
  ZuConversion<MxDateTime, T>::Is }; };
template <typename T, typename R = void, bool A = MxIsTime<T>::OK>
  struct MxMatchTime;
template <typename U, typename R>
  struct MxMatchTime<U, R, true> { using T = R; };

// MxType<T>::T returns the MxType corresponding to the generic type T,
// except that string types are passed through as is
template <typename U> struct MxType;

template <typename U, bool OK> struct MxType_Time { };
template <typename U> struct MxType_Time<U, true> { using T = MxDateTime; };

template <typename U, bool OK> struct MxType_String :
  public MxType_Time<U, MxIsTime<U>::OK> { };
template <typename U> struct MxType_String<U, true> { using T = U; };

template <typename U, bool OK> struct MxType_Float :
  public MxType_String<U, MxIsString<U>::OK> { };
template <typename U> struct MxType_Float<U, true> { using T = MxFloat; };

template <bool, bool> struct MxType_Int_;
template <> struct MxType_Int_<0, 0> { using T = MxUInt; };
template <> struct MxType_Int_<0, 1> { using T = MxInt; };
template <> struct MxType_Int_<1, 0> { using T = MxUInt64; };
template <> struct MxType_Int_<1, 1> { using T = MxInt64; };
template <typename U, bool OK> struct MxType_Int :
  public MxType_Float<U, MxIsFloat<U>::OK> { };
template <typename U> struct MxType_Int<U, true> {
  using T = typename MxType_Int_<sizeof(U) <= 4, ZuTraits<U>::IsSigned>::T;
};

template <typename U, bool OK> struct MxType_Flags :
  public MxType_Int<U, MxIsInt<U>::OK> { };
template <typename U> struct MxType_Flags<U, true> { using T = U; };

template <typename U, bool OK> struct MxType_Enum :
  public MxType_Flags<U, MxIsFlags<U>::OK> { };
template <typename U> struct MxType_Enum<U, true> { using T = U; };

template <typename U, bool OK> struct MxType_Bool :
  public MxType_Enum<U, MxIsEnum<U>::OK> { };
template <typename U> struct MxType_Bool<U, true> { using T = MxBool; };

template <typename U, bool OK> struct MxType_Char :
  public MxType_Bool<U, MxIsBool<U>::OK> { };
template <typename U> struct MxType_Char<U, true> { using T = MxChar; };

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

using MxIDString = MxString<MxIDStrSize>;
using MxTxtString = MxString<MxTxtSize>;

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
// with an MxOptKey{20190300, MxPutCall::CALL, 10000} (assuming pxExp=2)
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

ZuDeclTuple(MxInstrKey, (MxIDString, id), (MxID, venue), (MxID, segment));
ZuDeclTuple(MxSymKey, (MxIDString, id), (MxEnum, src));

using MxFutKey = MxUInt;	// mat

ZuDeclTuple(MxOptKey, (MxValue, strike), (MxUInt, mat), (MxEnum, putCall));

ZuDeclTuple(MxUniKey,
    (MxIDString, id),
    (MxID, venue),
    (MxID, segment),
    (MxValue, strike),
    (MxUInt, mat),
    (MxEnum, src),
    (MxEnum, putCall));

#pragma pack(pop)

#endif /* MxBase_HPP */
