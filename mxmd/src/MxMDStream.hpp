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

// MxMD library internal API

#ifndef MxMDStream_HPP
#define MxMDStream_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <ZuLargest.hpp>
#include <ZuPOD.hpp>

#include <ZmFn.hpp>

#include <ZiFile.hpp>
#include <ZiRing.hpp>

#include <lz4.h>

#include <MxBase.hpp>
#include <MxMDFrame.hpp>
#include <MxMDTypes.hpp>

#pragma pack(push, 1)

namespace MxMDStream {

  struct Type {
    MxEnumValues(
	AddTickSizeTbl, ResetTickSizeTbl, AddTickSize,
	AddSecurity, UpdateSecurity,
	AddOrderBook, DelOrderBook,
	AddCombination, DelCombination,
	UpdateOrderBook,
	TradingSession,
	L1, PxLevel, L2,
	AddOrder, ModifyOrder, CancelOrder,
	ResetOB,
	AddTrade, CorrectTrade, CancelTrade,
	RefDataLoaded,

	// control events follow
	Detach, EndOfSnapshot);

    MxEnumNames(
	"AddTickSizeTbl", "ResetTickSizeTbl", "AddTickSize",
	"AddSecurity", "UpdateSecurity",
	"AddOrderBook", "DelOrderBook",
	"AddCombination", "DelCombination",
	"UpdateOrderBook",
	"TradingSession",
	"L1", "PxLevel", "L2",
	"AddOrder", "ModifyOrder", "CancelOrder",
	"ResetOB",
	"AddTrade", "CorrectTrade", "CancelTrade",
	"RefDataLoaded",

	"Detach", "EndOfSnapshot");

    MxEnumMapAlias(Map, CSVMap);
  };

  struct FileHdr {
    enum {
      Magic = 0xc0def17e		// "code file"
    };

    inline FileHdr(ZuString id_, uint16_t vmajor_, uint16_t vminor_) :
	magic(Magic), vmajor(vmajor_), vminor(vminor_) {
      unsigned n = id_.length();
      if (n >= sizeof(id)) n = sizeof(id) - 1;
      memcpy(id, id_.data(), n);
      memset(&id[n], 0, sizeof(id) - n);
    }

    struct IOError { };
    struct InvalidFmt { };
    inline FileHdr(ZiFile &file, ZeError *e) {
      int n;
      if ((n = file.read(this, sizeof(FileHdr), e)) < (int)sizeof(FileHdr)) {
	if (n != Zi::EndOfFile) throw IOError();
	file.seek(0);
	new (this) FileHdr("RMD", 6, 0);
	return;
      }
      if (magic == Magic) return;
      throw InvalidFmt();
    }

    uint32_t	magic;	// must be Magic
    char	id[12];	// "RMD"
    uint16_t	vmajor;	// version - major number
    uint16_t	vminor;	// version - minor number
  };

  struct Frame : public MxMDFrame {
    inline void *ptr() { return (void *)&this[1]; }
    inline const void *ptr() const { return (const void *)&this[1]; }

    template <typename T> inline T &as() {
      T *ZuMayAlias(ptr) = (T *)&this[1];
      return *ptr;
    }
    template <typename T> inline const T &as() const {
      const T *ZuMayAlias(ptr) = (const T *)&this[1];
      return *ptr;
    }

    template <typename T> inline void pad() {
      if (ZuUnlikely(len < sizeof(T)))
	memset(((char *)&this[1]) + len, 0, sizeof(T) - len);
    }
  };

}
template <> struct ZiRingTraits<MxMDStream::Frame> {
  inline static unsigned size(const MxMDStream::Frame &hdr) {
    return sizeof(MxMDStream::Frame) + hdr.len;
  }
};
namespace MxMDStream {

  typedef ZiRing<Frame, ZiRingBase<ZmObject> > Ring;

  typedef MxSecKey Key;

  struct AddTickSizeTbl {
    enum { Code = Type::AddTickSizeTbl };
    MxID		venue;
    MxIDString		id;
  };

  struct ResetTickSizeTbl {
    enum { Code = Type::ResetTickSizeTbl };
    MxID		venue;
    MxIDString		id;
  };

  struct AddTickSize {
    enum { Code = Type::AddTickSize };
    MxID		venue;
    MxIDString		id;
    MxFixed		minPrice;
    MxFixed		maxPrice;
    MxFixed		tickSize;
  };

  struct AddSecurity {
    enum { Code = Type::AddSecurity };
    Key			key;
    MxUInt		shard;
    MxMDSecRefData	refData;
  };

  struct UpdateSecurity {
    enum { Code = Type::UpdateSecurity };
    Key			key;
    MxMDSecRefData	refData;
  };

  struct AddOrderBook {
    enum { Code = Type::AddOrderBook };
    Key			key;
    Key			security;
    MxIDString		tickSizeTbl;
    MxMDLotSizes	lotSizes;
  };

  struct DelOrderBook {
    enum { Code = Type::DelOrderBook };
    Key			key;
  };

  struct AddCombination {
    enum { Code = Type::AddCombination };
    Key			key;
    MxNDP		pxNDP;
    MxNDP		qtyNDP;
    MxUInt		legs;
    Key			securities[MxMDNLegs];
    MxEnum		sides[MxMDNLegs];
    MxRatio		ratios[MxMDNLegs];
    MxIDString		tickSizeTbl;
    MxMDLotSizes	lotSizes;
  };

  struct DelCombination {
    enum { Code = Type::DelCombination };
    Key			key;
  };

  struct UpdateOrderBook {
    enum { Code = Type::UpdateOrderBook };
    Key			key;
    MxIDString		tickSizeTbl;
    MxMDLotSizes	lotSizes;
  };

  struct TradingSession {
    enum { Code = Type::TradingSession };
    MxID		venue;
    MxID		segment;
    MxEnum		session;
    MxDateTime		stamp;
  };

  struct L1 {
    enum { Code = Type::L1 };
    Key			key;
    MxMDL1Data		data;
  };

  struct PxLevel {
    enum { Code = Type::PxLevel };
    Key			key;
    MxEnum		side;
    MxDateTime		transactTime;
    uint8_t		delta;
    MxFixed		price;
    MxFixed		qty;
    MxUInt		nOrders;
    MxFlags		flags;
  };

  struct L2 {
    enum { Code = Type::L2 };
    Key			key;
    MxDateTime		stamp;
    uint8_t		updateL1;
  };

  struct AddOrder {
    enum { Code = Type::AddOrder };
    Key			key;
    MxIDString		orderID;
    MxDateTime		transactTime;
    MxEnum		side;
    MxInt		rank;
    MxFixed		price;
    MxFixed		qty;
    MxFlags		flags;
  };

  struct ModifyOrder {
    enum { Code = Type::ModifyOrder };
    Key			key;
    MxIDString		orderID;
    MxDateTime		transactTime;
    MxEnum		side;
    MxInt		rank;
    MxFixed		price;
    MxFixed		qty;
    MxFlags		flags;
  };

  struct CancelOrder {
    enum { Code = Type::CancelOrder };
    Key			key;
    MxIDString		orderID;
    MxDateTime		transactTime;
    MxEnum		side;
  };

  struct ResetOB {
    enum { Code = Type::ResetOB };
    Key			key;
    MxDateTime		transactTime;
  };

  struct AddTrade {
    enum { Code = Type::AddTrade };
    Key			key;
    MxIDString		tradeID;
    MxDateTime		transactTime;
    MxFixed		price;
    MxFixed		qty;
  };

  struct CorrectTrade {
    enum { Code = Type::CorrectTrade };
    Key			key;
    MxIDString		tradeID;
    MxDateTime		transactTime;
    MxFixed		price;
    MxFixed		qty;
  };

  struct CancelTrade {
    enum { Code = Type::CancelTrade };
    Key			key;
    MxIDString		tradeID;
    MxDateTime		transactTime;
    MxFixed		price;
    MxFixed		qty;
  };

  struct RefDataLoaded {
    enum { Code = Type::RefDataLoaded };
    MxID		venue;
  };

  struct Detach {
    enum { Code = Type::Detach };
    MxUInt		id;
  };

  struct EndOfSnapshot {
    enum { Code = Type::EndOfSnapshot };
    uint8_t		pad_0 = 0;
  };

  typedef ZuLargest<
      AddTickSizeTbl,
      AddTickSize,
      ResetTickSizeTbl,
      AddSecurity,
      UpdateSecurity,
      AddOrderBook,
      DelOrderBook,
      AddCombination,
      DelCombination,
      UpdateOrderBook,
      TradingSession,
      L1,
      PxLevel,
      L2,
      AddOrder,
      ModifyOrder,
      CancelOrder,
      ResetOB,
      AddTrade,
      CorrectTrade,
      CancelTrade,
      RefDataLoaded>::T Largest;

  struct Buf {
    char	data[sizeof(Largest)];
  };

  struct MsgData {
  public:
    inline MsgData() { }
    inline MsgData(unsigned len, unsigned type) {
      init_(len, type);
    }
    inline MsgData(unsigned len, unsigned type, ZmTime stamp) {
      init_(len, type, stamp);
    }

  private:
    inline void init_(unsigned len, unsigned type) {
      m_frame.len = len;
      m_frame.type = type;
      m_frame.sec = 0;
      m_frame.nsec = 0;
    }
    inline void init_(unsigned len, unsigned type, ZmTime stamp) {
      m_frame.len = len;
      m_frame.type = type;
      m_frame.sec = stamp.sec();
      m_frame.nsec = stamp.nsec();
    }

  public:
    ZuInline const Frame *frame() const { return &m_frame; }
    ZuInline Frame *frame() { return &m_frame; }

  private:
    Frame	m_frame;
    Buf		m_buf;
  };

  struct Msg_HeapID {
    ZuInline static const char *id() { return "MxMDStream.Msg"; }
  };
  template <typename Heap>
  struct Msg_ : public Heap, public ZuPOD<MsgData> {
    template <typename ...Args> ZuInline Msg_(Args &&... args) {
      new (this->ptr()) MsgData(ZuFwd<Args>(args)...);
    }
    ZuInline const Frame *frame() const { return this->data().frame(); }
    ZuInline Frame *frame() { return this->data().frame(); }
  };
  typedef Msg_<ZmHeap<Msg_HeapID, sizeof(Msg_<ZuNull>)> > Msg;
  typedef ZuRef<Msg> MsgRef;

  template <typename App>
  MsgRef shift(App &app) {
    const Frame *frame = app.shift();
    if (ZuUnlikely(!frame)) return 0;
    if (frame->len > sizeof(Buf)) { app.shift2(); return 0; }
    MsgRef msg = new Msg();
    memcpy(msg->frame(), frame, sizeof(Frame));
    memcpy(msg->frame()->ptr(), frame->ptr(), frame->len);
    app.shift2();
    return msg;
  }

  template <typename App, typename T>
  inline Frame *push(App &app) {
    void *ptr = app.push(sizeof(Frame) + sizeof(T));
    if (ZuUnlikely(!ptr)) return 0;
    Frame *frame = new (ptr) Frame();
    frame->len = sizeof(T);
    frame->type = T::Code;
    {
      ZmTime now(ZmTime::Now);
      frame->sec = now.sec();
      frame->nsec = now.nsec();
    }
    return frame;
  }

#ifdef FnDeclare
#undef FnDeclare
#endif
#define FnDeclare(Fn, Type) \
  template <typename App, typename ...Args> \
  inline bool Fn(App &app, Args &&... args) { \
    Frame *frame = push<App, Type>(app); \
    if (ZuUnlikely(!frame)) return false; \
    new (frame->ptr()) Type{ZuFwd<Args>(args)...}; \
    app.push2(); \
    return true; \
  }

  FnDeclare(addTickSizeTbl, AddTickSizeTbl)
  FnDeclare(resetTickSizeTbl, ResetTickSizeTbl)
  FnDeclare(addTickSize, AddTickSize)
  FnDeclare(addSecurity, AddSecurity)
  FnDeclare(updateSecurity, UpdateSecurity)
  FnDeclare(addOrderBook, AddOrderBook)
  FnDeclare(delOrderBook, DelOrderBook)

  // AddCombination needs special handling due to securities/sides/ratios fields
  template <typename App,
	   typename Key_, typename Security, typename TickSizeTbl>
  inline bool addCombination(App &app,
      const Key_ &key, MxNDP pxNDP, MxNDP qtyNDP, unsigned legs,
      const Security *securities, const MxEnum *sides, const MxRatio *ratios,
      const TickSizeTbl &tickSizeTbl, const MxMDLotSizes &lotSizes) {
    Frame *frame = push<App, AddCombination>(app);
    if (ZuUnlikely(!frame)) return false;
    AddCombination *data = new (frame->ptr()) AddCombination{
	key, pxNDP, qtyNDP, legs, {}, {}, {}, tickSizeTbl, lotSizes};
    for (unsigned i = 0; i < legs; i++) {
      data->securities[i] = securities[i];
      data->sides[i] = sides[i];
      data->ratios[i] = ratios[i];
    }
    app.push2();
    return true;
  }

  FnDeclare(delCombination, DelCombination)
  FnDeclare(updateOrderBook, UpdateOrderBook)
  FnDeclare(tradingSession, TradingSession)
  FnDeclare(l1, L1)
  FnDeclare(pxLevel, PxLevel)
  FnDeclare(l2, L2)
  FnDeclare(addOrder, AddOrder)
  FnDeclare(modifyOrder, ModifyOrder)
  FnDeclare(cancelOrder, CancelOrder)
  FnDeclare(resetOB, ResetOB)
  FnDeclare(addTrade, AddTrade)
  FnDeclare(correctTrade, CorrectTrade)
  FnDeclare(cancelTrade, CancelTrade)
  FnDeclare(refDataLoaded, RefDataLoaded)
  FnDeclare(detach, Detach)
  FnDeclare(endOfSnapshot, EndOfSnapshot)

#undef FnDeclare
}

#pragma pack(pop)

#endif /* MxMDStream_HPP */
