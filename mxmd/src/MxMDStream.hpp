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
#include <ZiMultiplex.hpp>

#include <lz4.h>

#include <MxBase.hpp>
#include <MxQueue.hpp>
#include <MxMDFrame.hpp>
#include <MxMDTypes.hpp>

// App must conform to the following interface:
#if 0
struct App {
  // Rx

  // begin message shift - return ptr to message buffer
  const Frame *shift();
  // complete message shift
  void shift2();

  // Tx

  // begin message push - allocate message buffer
  void *push(unsigned size);
  // fill message header with MxMDFrame, return ptr to body
  void *out(void *ptr, unsigned length, unsigned type, ZmTime stamp);
  // complete message push
  void push2();
};
#endif

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
	Detach,			// ITC detach
	Login, EndOfSnapshot,	// TCP login, end of snapshot
	ResendReq);		// UDP resend request

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

	"Detach",
	"Login", "EndOfSnapshot",
	"ResendReq");

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
    template <typename ...Args>
    ZuInline Frame(Args &&... args) : MxMDFrame{ZuFwd<Args>(args)...} { }

    ZuInline void *ptr() { return (void *)&this[1]; }
    ZuInline const void *ptr() const { return (const void *)&this[1]; }

    template <typename T> ZuInline T &as() {
      T *ZuMayAlias(ptr) = (T *)&this[1];
      return *ptr;
    }
    template <typename T> ZuInline const T &as() const {
      const T *ZuMayAlias(ptr) = (const T *)&this[1];
      return *ptr;
    }

    template <typename T> ZuInline void pad() {
      if (ZuUnlikely(len < sizeof(T)))
	memset(((char *)&this[1]) + len, 0, sizeof(T) - len);
    }

    bool scan(unsigned length) {
      if (len + sizeof(Frame) > length) return true;
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
    MxNDP		pxNDP;
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
    MxNDP		pxNDP;
    MxValue		minPrice;
    MxValue		maxPrice;
    MxValue		tickSize;
  };

  struct AddSecurity {
    enum { Code = Type::AddSecurity };
    MxDateTime		transactTime;
    Key			key;
    MxUInt		shard;
    MxMDSecRefData	refData;
  };

  struct UpdateSecurity {
    enum { Code = Type::UpdateSecurity };
    MxDateTime		transactTime;
    Key			key;
    MxMDSecRefData	refData;
  };

  struct AddOrderBook {
    enum { Code = Type::AddOrderBook };
    MxDateTime		transactTime;
    Key			key;
    Key			security;
    MxIDString		tickSizeTbl;
    MxNDP		qtyNDP;
    MxMDLotSizes	lotSizes;
  };

  struct DelOrderBook {
    enum { Code = Type::DelOrderBook };
    MxDateTime		transactTime;
    Key			key;
  };

  struct AddCombination {
    enum { Code = Type::AddCombination };
    MxDateTime		transactTime;
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
    MxDateTime		transactTime;
    Key			key;
  };

  struct UpdateOrderBook {
    enum { Code = Type::UpdateOrderBook };
    MxDateTime		transactTime;
    Key			key;
    MxIDString		tickSizeTbl;
    MxMDLotSizes	lotSizes;
  };

  struct TradingSession {
    enum { Code = Type::TradingSession };
    MxDateTime		stamp;
    MxID		venue;
    MxID		segment;
    MxEnum		session;
  };

  struct L1 {
    enum { Code = Type::L1 };
    Key			key;
    MxMDL1Data		data;
  };

  struct PxLevel {
    enum { Code = Type::PxLevel };
    MxDateTime		transactTime;
    Key			key;
    MxEnum		side;
    uint8_t		delta;
    MxNDP		pxNDP;
    MxNDP		qtyNDP;
    MxValue		price;
    MxValue		qty;
    MxUInt		nOrders;
    MxFlags		flags;
  };

  struct L2 {
    enum { Code = Type::L2 };
    MxDateTime		stamp;
    Key			key;
    uint8_t		updateL1;
  };

  struct AddOrder {
    enum { Code = Type::AddOrder };
    MxDateTime		transactTime;
    Key			key;
    MxIDString		orderID;
    MxEnum		side;
    MxNDP		pxNDP;
    MxNDP		qtyNDP;
    MxInt		rank;
    MxValue		price;
    MxValue		qty;
    MxFlags		flags;
  };

  struct ModifyOrder {
    enum { Code = Type::ModifyOrder };
    MxDateTime		transactTime;
    Key			key;
    MxIDString		orderID;
    MxEnum		side;
    MxNDP		pxNDP;
    MxNDP		qtyNDP;
    MxInt		rank;
    MxValue		price;
    MxValue		qty;
    MxFlags		flags;
  };

  struct CancelOrder {
    enum { Code = Type::CancelOrder };
    MxDateTime		transactTime;
    Key			key;
    MxIDString		orderID;
    MxEnum		side;
  };

  struct ResetOB {
    enum { Code = Type::ResetOB };
    MxDateTime		transactTime;
    Key			key;
  };

  struct AddTrade {
    enum { Code = Type::AddTrade };
    MxDateTime		transactTime;
    Key			key;
    MxIDString		tradeID;
    MxNDP		pxNDP;
    MxNDP		qtyNDP;
    MxValue		price;
    MxValue		qty;
  };

  struct CorrectTrade {
    enum { Code = Type::CorrectTrade };
    MxDateTime		transactTime;
    Key			key;
    MxIDString		tradeID;
    MxNDP		pxNDP;
    MxNDP		qtyNDP;
    MxValue		price;
    MxValue		qty;
  };

  struct CancelTrade {
    enum { Code = Type::CancelTrade };
    MxDateTime		transactTime;
    Key			key;
    MxIDString		tradeID;
    MxNDP		pxNDP;
    MxNDP		qtyNDP;
    MxValue		price;
    MxValue		qty;
  };

  struct RefDataLoaded {
    enum { Code = Type::RefDataLoaded };
    MxID		venue;
  };

  // transport / session control messages follow

  struct Detach { // ITC detach
    enum { Code = Type::Detach };
    MxUInt		id;
  };

  struct Login { // TCP login
    enum { Code = Type::Login };
    MxIDString		username;
    MxIDString		password;
  };
  
  struct EndOfSnapshot { // TCP end of snapshot
    enum { Code = Type::EndOfSnapshot };
    MxSeqNo		seqNo;
  };

  struct ResendReq { // UDP resend request
    enum { Code = Type::ResendReq };
    MxSeqNo		seqNo;
    MxUInt		count;
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
      RefDataLoaded,
      Detach,
      Login,
      EndOfSnapshot,
      ResendReq>::T Largest;

  struct Buf {
    char	data[sizeof(Largest)];
  };

  struct MsgData {
  public:
    inline MsgData() { }

    ZuInline const Frame *frame() const { return &m_frame; }
    ZuInline Frame *frame() { return &m_frame; }

    ZuInline unsigned length() { return sizeof(Frame) + m_frame.len; }

  private:
    Frame	m_frame;
    Buf		m_buf;
  };

  struct Msg_HeapID {
    ZuInline static const char *id() { return "MxMDStream.Msg"; }
  };

  template <typename Heap>
  struct Msg_ : public Heap, public ZuPOD<MsgData> {
    ZuInline Msg_() { new (this->ptr()) MsgData(); }

    ZuInline const Frame *frame() const { return this->data().frame(); }
    ZuInline Frame *frame() { return this->data().frame(); }

    ZuInline unsigned length() { return this->data().length(); }
  };

  typedef Msg_<ZmHeap<Msg_HeapID, sizeof(Msg_<ZuNull>)> > Msg;
  typedef ZuRef<Msg> MsgRef;

  template <typename App>
  ZmRef<Msg> shift(App &app) {
    const Frame *frame = app.shift();
    if (ZuUnlikely(!frame)) return 0;
    if (frame->len > sizeof(Buf)) { app.shift2(); return 0; }
    ZmRef<Msg> msg = new Msg();
    memcpy(msg->frame(), frame, sizeof(Frame));
    memcpy(msg->frame()->ptr(), frame->ptr(), frame->len);
    app.shift2();
    return msg;
  }

  template <typename T, typename App>
  inline void *push(App &app) {
    void *ptr = app.push(sizeof(Frame) + sizeof(T));
    if (ZuUnlikely(!ptr)) return 0;
    return app.out(ptr, sizeof(T), T::Code, ZmTimeNow());
  }

#ifdef FnDeclare
#undef FnDeclare
#endif
#define FnDeclare(Fn, Type) \
  template <typename App, typename ...Args> \
  inline bool Fn(App &app, Args &&... args) { \
    void *ptr = push<Type>(app); \
    if (ZuUnlikely(!ptr)) return false; \
    new (ptr) Type{ZuFwd<Args>(args)...}; \
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

  // special processing for AddCombination's securities/sides/ratios fields
  template <typename App,
	   typename Key_, typename Security, typename TickSizeTbl>
  inline bool addCombination(App &app, MxDateTime transactTime,
      const Key_ &key, MxNDP pxNDP, MxNDP qtyNDP, unsigned legs,
      const Security *securities, const MxEnum *sides, const MxRatio *ratios,
      const TickSizeTbl &tickSizeTbl, const MxMDLotSizes &lotSizes) {
    void *ptr = push<AddCombination>(app);
    if (ZuUnlikely(!ptr)) return false;
    AddCombination *data = new (ptr) AddCombination{transactTime,
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

namespace MxMDStream {
  namespace UDP {
    void send(MxQMsg *msg, ZiConnection *cxn, const ZiSockAddr &addr) {
      msg->addr = addr;
      cxn->send(ZiIOFn([](MxQMsg *msg, ZiIOContext &io) {
	  io.init(ZiIOFn([](MxQMsg *msg, ZiIOContext &io) {
	      if (ZuUnlikely((io.offset += io.length) < io.size)) return;
	      msg->fn(msg, &io);
	    }, ZmMkRef(msg)),
	    msg->payload->ptr(), msg->length, 0, msg->addr);
	}, ZmMkRef(msg)));
    }

    void recv(MxQMsg *msg, ZiIOContext &io) {
      io.init(ZiIOFn([](MxQMsg *msg, ZiIOContext &io) {
	  msg->addr = io.addr;
	  msg->fn(msg, &io);
	}, ZmMkRef(msg)),
	msg->payload->ptr(), msg->payload->size(), 0);
    }
  }

  namespace TCP {
    void send(MxQMsg *msg, ZiConnection *cxn) {
      cxn->send(ZiIOFn([](MxQMsg *msg, ZiIOContext &io) {
	  io.init(ZiIOFn([](MxQMsg *msg, ZiIOContext &io) {
	      if (ZuUnlikely((io.offset += io.length) < io.size)) return;
	      msg->fn(msg, &io);
	    }, ZmMkRef(msg)),
	    msg->payload->ptr(), msg->length, 0);
	}, ZmMkRef(msg)));
    }

    void recv(MxQMsg *msg, ZiIOContext &io) {
      io.init(ZiIOFn([](MxQMsg *msg, ZiIOContext &io) {
	  unsigned len = io.offset += io.length;
	  while (len >= sizeof(Frame)) {
	    Frame &frame = msg->as<Frame>();
	    unsigned msgLen = sizeof(Frame) + frame.len;
	    if (ZuUnlikely(msgLen > msg->payload->size())) {
	      ZeLOG(Error, "received corrupt TCP message");
	      io.disconnect();
	      return;
	    }
	    if (ZuLikely(len < msgLen)) return;
	    msg->fn(msg, &io);
	    if (ZuUnlikely(io.completed())) return;
	    if (io.offset = len - msgLen)
	      memmove(io.ptr, io.ptr + msgLen, io.offset);
	    len = io.offset;
	  }
	}, ZmMkRef(msg)),
	msg->payload->ptr(), msg->payload->size(), 0);
    }
  }
}

#endif /* MxMDStream_HPP */
