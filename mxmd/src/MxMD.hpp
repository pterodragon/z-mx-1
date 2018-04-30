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

// MxMD library

#ifndef MxMD_HPP
#define MxMD_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <ZuIndex.hpp>
#include <ZuObject.hpp>
#include <ZuRef.hpp>

#include <ZmFn.hpp>
#include <ZmObject.hpp>
#include <ZmRef.hpp>
#include <ZmHash.hpp>
#include <ZmLHash.hpp>
#include <ZmRBTree.hpp>
#include <ZmRWLock.hpp>
#include <ZmHeap.hpp>
#include <ZmScheduler.hpp>
#include <ZmShard.hpp>

#include <MxBase.hpp>

#include <MxMDTypes.hpp>

// exceptions

inline auto MxMDOrderNotFound(const char *op, MxIDString id) {
  return [op, id](const ZeEvent &, ZmStream &s) {
      s << "MxMD " << op << ": order " << id << " not found"; };
}
inline auto MxMDDuplicateOrderID(const char *op, MxIDString id) {
  return [op, id](const ZeEvent &, ZmStream &s) {
      s << "MxMD " << op << ": order " << id << " already exists"; };
}
inline auto MxMDNoPxLevel(const char *op) {
  return [op](const ZeEvent &, ZmStream &s) {
      s << "MxMD " << op << ": internal error - missing price level"; };
}
inline auto MxMDNoOrderBook(const char *op) {
  return [op](const ZeEvent &, ZmStream &s) {
      s << "MxMD " << op << ": internal error - missing order book"; };
}
inline auto MxMDMissedUpdates(MxID venue, unsigned n) {
  return [venue, n](const ZeEvent &, ZmStream &s) {
      s << "MxMD: missed " << ZuBoxed(n) << " updates from " << venue; };
}
inline auto MxMDMissedOBUpdates(MxID venue, unsigned n, MxIDString id) {
  return [venue, n, id](const ZeEvent &, ZmStream &s) {
      s << "MxMD: missed " << ZuBoxed(n) << " updates from " << venue <<
	" for " << id; };
}

// generic venue-specific flags handling

typedef MxString<12> MxMDFlagsStr;

template <class T> struct MxMDFlags {
  struct Default {
    static void print(MxMDFlagsStr &out, MxFlags flags) {
      out << flags.hex(ZuFmt::Right<8>());
    }
    static void scan(const MxMDFlagsStr &in, MxFlags &flags) {
      if (in.length() < 8) return;
      flags.scan(ZuFmt::Hex<0, ZuFmt::Right<8> >(), in);
    }
  };
  typedef Default XTKS;
  typedef Default XTKD;
  typedef Default XOSE;
  typedef Default XASX;

  typedef void (*PrintFn)(MxMDFlagsStr &, MxFlags);
  typedef void (*ScanFn)(const MxMDFlagsStr &, MxFlags &);

  template <class, typename> struct FnPtr;
  template <class Venue> struct FnPtr<Venue, PrintFn> {
    inline static PrintFn fn() { return &Venue::print; }
  };
  template <class Venue> struct FnPtr<Venue, ScanFn> {
    inline static ScanFn fn() { return &Venue::scan; }
  };

  template <typename Fn>
  class FnMap {
    typedef ZmRBTree<MxID,
	      ZmRBTreeVal<Fn,
		ZmRBTreeObject<ZuNull,
		  ZmRBTreeLock<ZmNoLock> > > > Map;

  public:
    FnMap() {
      m_map.add("XTKS", FnPtr<typename T::XTKS, Fn>::fn());
      m_map.add("XTKD", FnPtr<typename T::XTKD, Fn>::fn());
      m_map.add("XOSE", FnPtr<typename T::XOSE, Fn>::fn());
      m_map.add("XASX", FnPtr<typename T::XASX, Fn>::fn());
    }
    inline Fn fn(MxID venue) { return m_map.findVal(venue); }

  private:
    Map	m_map;
  };

  inline static void print(MxMDFlagsStr &out, MxID venue, MxFlags flags) {
    static FnMap<PrintFn> map;
    PrintFn fn = map.fn(venue);
    if (fn) (*fn)(out, flags);
  }
  inline static MxMDFlagsStr print(MxID venue, MxFlags flags) {
    MxMDFlagsStr out;
    print(out, venue, flags);
    return out;
  }
  inline static void scan(
      const MxMDFlagsStr &in, MxID venue, MxFlags &flags) {
    static FnMap<ScanFn> map;
    ScanFn fn = map.fn(venue);
    if (fn) (*fn)(in, flags);
  }
};

// pre-declarations

class MxMDLib;
class MxMDCore;
class MxMDShard;
class MxMDVenue;
class MxMDVenueShard;
class MxMDSecurity;
class MxMDOrderBook;
class MxMDOBSide;

typedef ZmSharded<MxMDShard> MxMDSharded;

// L1 flags

struct MxMDL1Flags : public MxMDFlags<MxMDL1Flags> {
  struct Liffe : public Default {
    static unsigned tradeType(const MxFlags &flags) {
      return (unsigned)flags;
    }
    static void tradeType(MxFlags &flags, unsigned tradeType) {
      flags = tradeType;
    }
  };
  typedef Liffe XTKD;

  struct XASX : public Default {
    static bool printable(const MxFlags &flags) {
      return flags & 1;
    }
    static void printable(MxFlags &flags, bool printable) {
      flags = ((unsigned)flags & ~1) | (unsigned)printable;
    }
    static bool auction(const MxFlags &flags) {
      return flags & 2;
    }
    static void auction(MxFlags &flags, bool auction) {
      flags = ((unsigned)flags & ~2) | (((unsigned)auction)<<1);
    }
    static void print(MxMDFlagsStr &out, MxFlags flags) {
      if (auction(flags)) out << 'A';
      if (printable(flags)) out << 'P';
    }
    static void scan(const MxMDFlagsStr &out, MxFlags &flags) {
      flags = 0;
      for (unsigned i = 0; i < out.length(); i++) {
	if (out[i] == 'A') auction(flags, true);
	if (out[i] == 'P') printable(flags, true);
      }
    }
  };
};

// tick size table

class MxMDAPI MxMDTickSizeTbl : public ZmObject {
  MxMDTickSizeTbl(const MxMDTickSizeTbl &) = delete;
  MxMDTickSizeTbl &operator =(const MxMDTickSizeTbl &) = delete;

friend class MxMDLib;
friend class MxMDVenue;

  struct TickSizes_HeapID {
    inline static const char *id() { return "MxMDTickSizeTbl.TickSizes"; }
  };
  typedef ZmRBTree<MxMDTickSize,
	    ZmRBTreeIndex<MxMDTickSize_MinPxAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmRWLock,
		  ZmRBTreeHeapID<TickSizes_HeapID> > > > > TickSizes;

  struct IDAccessor : public ZuAccessor<MxMDTickSizeTbl *, MxIDString> {
    ZuInline static const MxIDString &value(const MxMDTickSizeTbl *t) {
      return t->id();
    }
  };

  template <typename ID>
  MxMDTickSizeTbl(MxMDVenue *venue, const ID &id) :
    m_venue(venue), m_id(id) { }

public:
  ~MxMDTickSizeTbl() { }

  ZuInline MxMDVenue *venue() const { return m_venue; }
  ZuInline const MxIDString &id() const { return m_id; }

  void reset();
  void addTickSize(MxFloat minPrice, MxFloat maxPrice, MxFloat tickSize);
  inline MxFloat tickSize(MxFloat price) {
    TickSizes::Node *node = m_tickSizes.find(price, TickSizes::GreaterEqual);
    if (!node) return MxFloat();
    return node->key().tickSize();
  }
  template <typename L> // (const MxMDTickSize &) -> uintptr_t
  uintptr_t allTickSizes(L l) const {
    TickSizes::ReadIterator i(m_tickSizes);
    while (const TickSizes::Node *node = i.iterate())
      if (uintptr_t v = l(node->key())) return v;
    return 0;
  }

private:
  void reset_();
  void addTickSize_(MxFloat minPrice, MxFloat maxPrice, MxFloat tickSize);

  MxMDVenue	*m_venue;
  MxIDString	m_id;
  TickSizes	m_tickSizes;
};

// trades

struct MxMDTradeData {
  MxIDString	tradeID;
  MxDateTime	transactTime;
  MxFloat	price;
  MxFloat	qty;
};

struct MxMDTrade_HeapID {
  inline static const char *id() { return "MxMDTrade"; }
};
template <class Heap> class MxMDTrade_ : public Heap, public ZmObject {
  MxMDTrade_(const MxMDTrade_ &) = delete;
  MxMDTrade_ &operator =(const MxMDTrade_ &) = delete;

friend class MxMDOrderBook;
friend class MxMDLib;

  template <typename TradeID>
  inline MxMDTrade_(MxMDSecurity *security, MxMDOrderBook *ob,
	const TradeID &tradeID, MxDateTime transactTime,
	MxFloat price, MxFloat qty) :
      m_security(security), m_orderBook(ob),
      m_data{tradeID, transactTime, price, qty} { }

public:
  ~MxMDTrade_() { }

  inline MxMDSecurity *security() const { return m_security; }
  inline MxMDOrderBook *orderBook() const { return m_orderBook; }
  inline const MxMDTradeData &data() const { return m_data; }

private:
  MxMDSecurity		*m_security;
  MxMDOrderBook		*m_orderBook;
  MxMDTradeData		m_data;
};
typedef ZmHeap<MxMDTrade_HeapID, sizeof(MxMDTrade_<ZuNull>)> MxMDTrade_Heap;
typedef MxMDTrade_<MxMDTrade_Heap> MxMDTrade;

// orders

struct MxMDOrderFlags : public MxMDFlags<MxMDOrderFlags> {
  struct OMX : public Default {
    static bool implied(const MxFlags &flags) {
      return flags & 1;
    }
    static void implied(MxFlags &flags, bool implied) {
      flags = ((unsigned)flags & ~1) | (unsigned)implied;
    }
    static void print(MxMDFlagsStr &out, MxFlags flags) {
      if (implied(flags)) out << 'I';
    }
    static void scan(const MxMDFlagsStr &out, MxFlags &flags) {
      flags = 0;
      for (unsigned i = 0; i < out.length(); i++) {
	if (out[i] == 'I') implied(flags, true);
      }
    }
  };
  typedef OMX XOSE;
};

struct MxMDOrderData {
  MxDateTime		transactTime;
  MxEnum		side;
  MxUInt		rank;
  MxFloat		price;
  MxFloat		qty;
  MxFlags		flags;		// MxMDOrderFlags
};

struct MxMDOrderIDScope {
  MxEnumValues(Venue, OrderBook, OBSide);
  MxEnumNames("Venue", "OrderBook", "OBSide");
  MxEnumMapAlias(Map, CSVMap);
};

typedef ZuTuple<MxSecKey, MxIDString> MxMDOrderID2_;
template <typename T> struct MxMDOrderID2_Fn : public T {
  inline const MxSecKey &obKey() const { return T::ptr()->p1(); }
  inline const MxIDString &orderID() const { return T::ptr()->p2(); }
};
typedef ZuMixin<MxMDOrderID2_, MxMDOrderID2_Fn> MxMDOrderID2;
typedef ZuTuple<MxSecKey, MxEnum, MxIDString> MxMDOrderID3_;
template <typename T> struct MxMDOrderID3_Fn : public T {
  inline const MxSecKey &obKey() const { return T::ptr()->p1(); }
  inline const MxEnum &side() const { return T::ptr()->p2(); }
  inline const MxIDString &orderID() const { return T::ptr()->p3(); }
};
typedef ZuMixin<MxMDOrderID3_, MxMDOrderID3_Fn> MxMDOrderID3;
typedef ZuTuple<
  const MxSecKey &, const MxEnum &, const MxIDString &> MxMDOrderID3Ref;
typedef ZuTuple<MxSecKey, MxIDString> MxMDOrderID2_;
typedef ZuTuple<const MxSecKey &, const MxIDString &> MxMDOrderID2Ref;

struct MxMDOrder_HeapID {
  inline static const char *id() { return "MxMDOrder"; }
};
template <class Heap, class PxLevel>
class MxMDOrder_ : public Heap, public ZmObject {
  MxMDOrder_(const MxMDOrder_ &) = delete;
  MxMDOrder_ &operator =(const MxMDOrder_ &) = delete;

friend class MxMDFeed;
friend class MxMDVenue;
friend class MxMDVenueShard;
friend class MxMDOrderBook;
friend class MxMDOBSide;
template <class> friend class MxMDPxLevel_;

private:
  static MxMDOBSide *bids_(MxMDOrderBook *);
  static MxMDOBSide *asks_(MxMDOrderBook *);

  template <typename OrderID>
  inline MxMDOrder_(MxMDOrderBook *ob,
    const OrderID &id, MxEnum idScope,
    MxDateTime transactTime, MxEnum side,
    MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags) :
      m_orderBook(ob),
      m_obSide(side == MxSide::Buy ? bids_(ob) : asks_(ob)),
      m_pxLevel(0),
      m_id(id),
      m_idScope(idScope),
      m_data{transactTime, side, rank, price, qty, flags} { }

public:
  ~MxMDOrder_() { }

private:
  struct OrderID3Accessor;
friend struct OrderID3Accessor;
  struct OrderID3Accessor : public ZuAccessor<MxMDOrder_ *, MxMDOrderID3> {
    static MxMDOrderID3Ref value(const MxMDOrder_ *o);
  };
  struct OrderID2Accessor;
friend struct OrderID2Accessor;
  struct OrderID2Accessor : public ZuAccessor<MxMDOrder_ *, MxMDOrderID2> {
    static MxMDOrderID2Ref value(const MxMDOrder_ *o);
  };
  struct OrderIDAccessor;
friend struct OrderIDAccessor;
  struct OrderIDAccessor : public ZuAccessor<MxMDOrder_ *, MxIDString> {
    inline static const MxIDString &value(const MxMDOrder_ *o) {
      return o->m_id;
    }
  };
  struct RankAccessor;
friend struct RankAccessor;
  struct RankAccessor : public ZuAccessor<MxMDOrder_ *, MxUInt> {
    inline static const MxUInt &value(const MxMDOrder_ *o) {
      return o->m_data.rank;
    }
  };

public:
  ZuInline MxMDOrderBook *orderBook() const { return m_orderBook; }
  ZuInline MxMDOBSide *obSide() const { return m_obSide; }
  ZuInline PxLevel *pxLevel() const { return m_pxLevel; }

  ZuInline const MxIDString &id() const { return m_id; }
  ZuInline MxEnum idScope() const { return m_idScope; }
  ZuInline const MxMDOrderData &data() const { return m_data; }

private:
  inline void update_(MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags) {
    if (*rank) m_data.rank = rank;
    if (*price) m_data.price = price;
    if (*qty) m_data.qty = qty;
    m_data.flags = flags;
  }
  void updateQty_(MxFloat qty) {
    if (*qty) m_data.qty = qty;
  }

  ZuInline void pxLevel(PxLevel *l) { m_pxLevel = l; }

  MxMDOrderBook		*m_orderBook;
  MxMDOBSide		*m_obSide;
  PxLevel		*m_pxLevel;

  MxIDString		m_id;
  MxEnum		m_idScope;
  MxMDOrderData		m_data;
};
typedef ZmHeap<MxMDOrder_HeapID,
	       sizeof(MxMDOrder_<ZuNull, ZuObject>)> MxMDOrder_Heap;

struct MxMDOrders_HeapID : public ZmHeapSharded {
  inline static const char *id() { return "MxMDLib.Orders"; }
};

// price levels

struct MxMDPxLvlData {
  MxDateTime		transactTime;
  MxFloat		qty;
  MxUInt		nOrders;
  MxFlags		flags;		// MxMDL2Flags
};

struct MxMDL2Flags : public MxMDFlags<MxMDL2Flags> {
  // FIXME - XASX
  struct XTKS : public Default {
    struct QuoteType {
      enum _ { Empty = 0, General = 1, SQ = 3, CQ = 4, Unknown = 7 };
    };
    static unsigned quoteType(const MxFlags &flags) {
      return (unsigned)flags & 7;
    }
    static void quoteType(MxFlags &flags, unsigned quoteType) {
      if (quoteType > 7) quoteType = QuoteType::Unknown;
      flags = ((unsigned)flags & ~7) | quoteType;
    }
    struct MatchType {
      enum _ { Normal = 0, InsideSQ = 1, OutsideSQ = 2, Unknown = 3 };
    };
    static unsigned matchType(const MxFlags &flags) {
      return ((unsigned)flags>>3) & 3;
    }
    static void matchType(MxFlags &flags, unsigned matchType) {
      if (matchType > 3) matchType = MatchType::Unknown;
      flags = ((unsigned)flags & ~(3<<3)) | (matchType<<3);
    }
    static bool closing(const MxFlags &flags) {
      return ((unsigned)flags>>5) & 1;
    }
    static void closing(MxFlags &flags, bool closing) {
      flags = ((unsigned)flags & ~(1<<5)) | (((unsigned)closing)<<5);
    }

    static void print(MxMDFlagsStr &out, MxFlags flags) {
      if (closing(flags)) out << 'C';
      switch (quoteType(flags)) {
	case QuoteType::Empty: break;
	case QuoteType::General: out << 'G'; break;
	case QuoteType::SQ: out << 'S'; break;
	case QuoteType::CQ: out << 'K'; break;		// per TSE terminal
      }
      switch (matchType(flags)) {
	case MatchType::Normal: break;
	case MatchType::InsideSQ: out << '#'; break;	// per TSE terminal
	case MatchType::OutsideSQ: out << '*'; break;	// per TSE terminal
      }
    }
    static void scan(const MxMDFlagsStr &out, MxFlags &flags) {
      flags = 0;
      for (unsigned i = 0; i < out.length(); i++) {
	switch ((int)out[i]) {
	  case 'C': closing(flags, true); break;
	  case 'G': quoteType(flags, QuoteType::General); break;
	  case 'S': quoteType(flags, QuoteType::SQ); break;
	  case 'K': quoteType(flags, QuoteType::CQ); break;
	  case '#': matchType(flags, MatchType::InsideSQ); break;
	  case '*': matchType(flags, MatchType::OutsideSQ); break;
	}
      }
    }
  };

  struct Liffe : public Default {
    static bool rfqx(const MxFlags &flags) {
      return flags & 1;
    }
    static void rfqx(MxFlags &flags, bool rfqx) {
      flags = ((unsigned)flags & ~1) | (unsigned)rfqx;
    }
    static bool implied(const MxFlags &flags) {
      return flags & 2;
    }
    static void implied(MxFlags &flags, bool implied) {
      flags = ((unsigned)flags & ~2) | (((unsigned)implied)<<1);
    }

    static void print(MxMDFlagsStr &out, MxFlags flags) {
      if (rfqx(flags)) out << 'R';
      if (implied(flags)) out << 'I';
    }
    static void scan(const MxMDFlagsStr &out, MxFlags &flags) {
      flags = 0;
      for (unsigned i = 0; i < out.length(); i++) {
	switch ((int)out[i]) {
	  case 'R': rfqx(flags, true); break;
	  case 'I': implied(flags, true); break;
	}
      }
    }
  };
  typedef Liffe XTKD;
};

struct MxMDPxLevel_HeapID : public ZmHeapSharded {
  inline static const char *id() { return "MxMDPxLevel"; }
};
template <class Heap> class MxMDPxLevel_ : public Heap, public ZuObject {
  MxMDPxLevel_(const MxMDPxLevel_ &);
  MxMDPxLevel_ &operator =(const MxMDPxLevel_ &);

friend class MxMDOrderBook;
friend class MxMDOrderBook;
friend class MxMDOBSide;

  typedef MxMDOrder_<MxMDOrder_Heap, MxMDPxLevel_> Order;

  typedef ZmRBTree<ZuRef<Order>,
	    ZmRBTreeIndex<typename Order::RankAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock,
		  ZmRBTreeHeapID<MxMDOrders_HeapID> > > > > Orders;

  inline MxMDPxLevel_(MxMDOBSide *obSide,
    MxEnum side, MxDateTime transactTime, MxFloat price, MxFloat qty,
    MxUInt nOrders, MxFlags flags) : m_obSide(obSide),
      m_side(side), m_price(price),
      m_data{transactTime, qty, nOrders, flags} { }

public:
  ~MxMDPxLevel_() { }

  struct PxAccessor : public ZuAccessor<MxMDPxLevel_ *, MxFloat> {
    typedef MxFloat::FCmp Cmp;
    ZuInline static const MxFloat &value(const MxMDPxLevel_ *p) {
      return p->price();
    }
  };

  ZuInline MxMDOBSide *obSide() const { return m_obSide; }
  ZuInline const MxEnum &side() const { return m_side; }
  ZuInline const MxFloat &price() const { return m_price; }
  ZuInline const MxMDPxLvlData &data() const { return m_data; }

  template <typename L> // (MxMDOrder *) -> uintptr_t
  uintptr_t allOrders(L l) const {
    typename Orders::ReadIterator i(m_orders);
    while (const ZuRef<Order> &order = i.iterateKey())
      if (uintptr_t v = l(order)) return v;
    return 0;
  }

private:
  void reset(MxDateTime transactTime,
      const ZmFn<Order *, MxDateTime> *canceledOrderFn) {
    if (canceledOrderFn) {
      typename Orders::ReadIterator i(m_orders);
      while (const ZuRef<Order> &order = i.iterateKey())
	(*canceledOrderFn)(order, transactTime);
    }
    m_orders.clean();
    m_data.transactTime = transactTime;
    m_data.qty = 0;
    m_data.nOrders = 0;
  }

  void updateAbs(
      MxDateTime transactTime, MxFloat qty, MxFloat nOrders, MxFlags flags,
      MxFloat &d_qty, MxFloat &d_nOrders) {
    m_data.transactTime = transactTime;
    updateAbs_(qty, nOrders, flags, d_qty, d_nOrders);
    if (m_data.qty.feq(0.0))
      m_data.qty = m_data.nOrders = 0;
  }
  void updateAbs_(MxFloat qty, MxFloat nOrders, MxFlags flags,
      MxFloat &d_qty, MxFloat &d_nOrders) {
    d_qty = qty - m_data.qty;
    m_data.qty = qty;
    d_nOrders = nOrders - m_data.nOrders;
    m_data.nOrders = nOrders;
    m_data.flags = flags;
  }
  void updateDelta(
      MxDateTime transactTime, MxFloat qty, MxFloat nOrders, MxFlags flags) {
    m_data.transactTime = transactTime;
    updateDelta_(qty, nOrders, flags);
    if (m_data.qty.feq(0.0))
      m_data.qty = m_data.nOrders = 0;
  }
  void updateDelta_(MxFloat qty, MxFloat nOrders, MxFlags flags) {
    m_data.qty += qty;
    m_data.nOrders += nOrders;
    if (qty.fgt(0.0))
      m_data.flags |= flags;
    else
      m_data.flags &= ~flags;
  }
  void update(MxDateTime transactTime, bool delta,
      MxFloat qty, MxFloat nOrders, MxFlags flags,
      MxFloat &d_qty, MxFloat &d_nOrders) {
    m_data.transactTime = transactTime;
    if (!delta)
      updateAbs_(qty, nOrders, flags, d_qty, d_nOrders);
    else {
      d_qty = qty, d_nOrders = nOrders;
      updateDelta_(qty, nOrders, flags);
    }
    if (m_data.qty.feq(0.0))
      m_data.qty = m_data.nOrders = 0;
  }

  ZuInline void addOrder(Order *order) { m_orders.add(order); }
  ZuInline void delOrder(MxUInt rank) { m_orders.delVal(rank); }

  MxMDOBSide		*m_obSide;
  MxEnum		m_side;
  MxFloat		m_price;
  MxMDPxLvlData	  	m_data;
  Orders		m_orders;
};
typedef ZmHeap<MxMDPxLevel_HeapID,
	       sizeof(MxMDPxLevel_<ZuNull>)> MxMDPxLevel_Heap;
typedef MxMDPxLevel_<MxMDPxLevel_Heap> MxMDPxLevel;

typedef MxMDOrder_<MxMDOrder_Heap, MxMDPxLevel> MxMDOrder;

// event handlers (callbacks)

typedef ZmFn<MxMDLib *> MxMDLibFn;
typedef ZmFn<MxMDLib *, ZeEvent *> MxMDExceptionFn;
typedef ZmFn<MxMDFeed *> MxMDFeedFn;
typedef ZmFn<MxMDVenue *> MxMDVenueFn;

typedef ZmFn<MxMDTickSizeTbl *> MxMDTickSizeTblFn;
typedef ZmFn<MxMDTickSizeTbl *, const MxMDTickSize &> MxMDTickSizeFn;

// venue, segment, trading session, time stamp
typedef ZmFn<MxMDVenue *, MxID, MxEnum, MxDateTime>
  MxMDTradingSessionFn;

typedef ZmFn<MxMDSecurity *> MxMDSecurityFn;
typedef ZmFn<MxMDOrderBook *> MxMDOrderBookFn;

// order book, data
typedef ZmFn<MxMDOrderBook *, const MxMDL1Data &> MxMDLevel1Fn;
// price level, time stamp
typedef ZmFn<MxMDPxLevel *, MxDateTime> MxMDPxLevelFn;
// order book, time stamp
typedef ZmFn<MxMDOrderBook *, MxDateTime> MxMDLevel2Fn;
// order, time stamp
typedef ZmFn<MxMDOrder *, MxDateTime> MxMDOrderFn;
// trade, time stamp
typedef ZmFn<MxMDTrade *, MxDateTime> MxMDTradeFn;
// time stamp, next time stamp
typedef ZmFn<MxDateTime, MxDateTime &> MxMDTimerFn;

struct MxMDLibHandler : public ZmObject {
#define MxMDLib_Fn(Type, member) \
  template <typename Arg> \
  inline MxMDLibHandler & member##Fn(Arg &&arg) { \
    member = ZuFwd<Arg>(arg); \
    return *this; \
  } \
  Type	member
  MxMDLib_Fn(MxMDExceptionFn,		exception);
  MxMDLib_Fn(MxMDFeedFn,		connected);
  MxMDLib_Fn(MxMDFeedFn,		disconnected);
  MxMDLib_Fn(MxMDLibFn,			eof);
  MxMDLib_Fn(MxMDVenueFn,		refDataLoaded);
  MxMDLib_Fn(MxMDTickSizeTblFn,		addTickSizeTbl);
  MxMDLib_Fn(MxMDTickSizeTblFn,		resetTickSizeTbl);
  MxMDLib_Fn(MxMDTickSizeFn,		addTickSize);
  MxMDLib_Fn(MxMDSecurityFn,		addSecurity);
  MxMDLib_Fn(MxMDSecurityFn,		updatedSecurity);
  MxMDLib_Fn(MxMDOrderBookFn,		addOrderBook);
  MxMDLib_Fn(MxMDOrderBookFn,		updatedOrderBook);
  MxMDLib_Fn(MxMDOrderBookFn,		deletedOrderBook);
  MxMDLib_Fn(MxMDTradingSessionFn,	tradingSession);
  MxMDLib_Fn(MxMDTimerFn,		timer);
#undef MxMDLib_Fn
};

struct MxMDSecHandler : public ZuObject {
#define MxMDSecurity_Fn(Type, member) \
  template <typename Arg> \
  inline MxMDSecHandler & member##Fn(Arg &&arg) { \
    member = ZuFwd<Arg>(arg); \
    return *this; \
  } \
  Type	member
  MxMDSecurity_Fn(MxMDSecurityFn,	updatedSecurity); // ref. data changed
  MxMDSecurity_Fn(MxMDOrderBookFn,	updatedOrderBook); // ''
  MxMDSecurity_Fn(MxMDLevel1Fn,		l1);
  MxMDSecurity_Fn(MxMDPxLevelFn,	addMktLevel);
  MxMDSecurity_Fn(MxMDPxLevelFn,	updatedMktLevel);
  MxMDSecurity_Fn(MxMDPxLevelFn,	deletedMktLevel);
  MxMDSecurity_Fn(MxMDPxLevelFn,	addPxLevel);
  MxMDSecurity_Fn(MxMDPxLevelFn,	updatedPxLevel);
  MxMDSecurity_Fn(MxMDPxLevelFn,	deletedPxLevel);
  MxMDSecurity_Fn(MxMDLevel2Fn,		l2);
  MxMDSecurity_Fn(MxMDOrderFn,		addOrder);
  MxMDSecurity_Fn(MxMDOrderFn,		modifiedOrder);
  MxMDSecurity_Fn(MxMDOrderFn,		canceledOrder);
  MxMDSecurity_Fn(MxMDTradeFn,		addTrade);
  MxMDSecurity_Fn(MxMDTradeFn,		correctedTrade);
  MxMDSecurity_Fn(MxMDTradeFn,		canceledTrade);
#undef MxMDSecurity_Fn
};

// order books

class MxMDAPI MxMDOBSide : public ZuObject {
  MxMDOBSide(const MxMDOBSide &) = delete;
  MxMDOBSide &operator =(const MxMDOBSide &) = delete;

friend class MxMDOrderBook;
template <class Heap> friend class MxMDPxLevel_;

  struct PxLevels_HeapID : public ZmHeapSharded {
    inline static const char *id() { return "MxMDOBSide.PxLevels"; }
  };
  typedef ZmRBTree<ZuRef<MxMDPxLevel>,
	    ZmRBTreeIndex<MxMDPxLevel::PxAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmPRWLock,
		  ZmRBTreeHeapID<PxLevels_HeapID> > > > > PxLevels;

public:
  struct Data {
    MxFloat		nv;
    MxFloat		qty;
  };

private:
  inline MxMDOBSide(MxMDOrderBook *ob, MxEnum side) :
    m_orderBook(ob), m_side(side), m_data{0, 0} { }

public:
  ZuInline MxMDOrderBook *orderBook() const { return m_orderBook; }

  ZuInline MxEnum side() const { return m_side; }

  ZuInline const Data &data() const { return m_data; }
  ZuInline MxFloat vwap() const { return m_data.nv / m_data.qty; }

  ZuInline ZuRef<MxMDPxLevel> mktLevel() { return m_mktLevel; }
  inline ZuRef<MxMDPxLevel> pxLevel(MxFloat price, MxFloat tickSize) {
    tickSize /= 2.0;
    PxLevels::ReadIterator i(m_pxLevels, price - tickSize, PxLevels::Greater);
    price += tickSize;
    if (ZuRef<MxMDPxLevel> pxLevel = i.iterateKey()) {
      if (pxLevel->price() < price) return pxLevel;
    }
    return (MxMDPxLevel *)0;
  }
  template <typename L> // (MxMDPxLevel *) -> uintptr_t
  inline uintptr_t allPxLevels(L l) const {
    PxLevels::ReadIterator i(m_pxLevels,
	m_side == MxSide::Sell ? PxLevels::GreaterEqual : PxLevels::LessEqual);
    while (const ZuRef<MxMDPxLevel> &pxLevel = i.iterateKey())
      if (uintptr_t v = l(pxLevel)) return v;
    return 0;
  }

private:
  bool updateL1Bid(MxMDL1Data &l1Data, MxMDL1Data &delta);
  bool updateL1Ask(MxMDL1Data &l1Data, MxMDL1Data &delta);

  void pxLevel_(MxDateTime transactTime, bool delta,
    MxFloat price, MxFloat qty, MxFloat nOrders, MxFlags flags,
    const MxMDSecHandler *handler,
    MxFloat &d_qty, MxFloat &d_nOrders,
    const MxMDPxLevelFn *&, ZuRef<MxMDPxLevel> &);

  void addOrder_(
    MxMDOrder *order, MxDateTime transactTime,
    const MxMDSecHandler *handler,
    const MxMDPxLevelFn *&fn, ZuRef<MxMDPxLevel> &pxLevel);
  void delOrder_(
    MxMDOrder *order, MxDateTime transactTime,
    const MxMDSecHandler *handler,
    const MxMDPxLevelFn *&fn, ZuRef<MxMDPxLevel> &pxLevel);

  void reset(MxDateTime transactTime, const MxMDSecHandler *handler);

  MxMDOrderBook		*m_orderBook;
  MxEnum		m_side;
  Data			m_data;
  ZuRef<MxMDPxLevel>	m_mktLevel;
  PxLevels		m_pxLevels;
};

struct MxMDFeedOB : public ZmPolymorph {
  virtual ~MxMDFeedOB() { }
  virtual void subscribe(MxMDOrderBook *, MxMDSecHandler *) { }
  virtual void unsubscribe(MxMDOrderBook *, MxMDSecHandler *) { }
};

class MxMDAPI MxMDOrderBook : public ZmObject, public MxMDSharded {
  MxMDOrderBook(const MxMDOrderBook &) = delete;
  MxMDOrderBook &operator =(const MxMDOrderBook &) = delete;

friend class MxMDLib;
friend class MxMDShard;
friend class MxMDVenue;
friend class MxMDSecurity;
friend class MxMDOBSide;

  struct KeyAccessor : public ZuAccessor<MxMDOrderBook *, MxSecKey> {
    inline static const MxSecKey &value(const MxMDOrderBook *ob) {
      return ob->key();
    }
  };

  struct VenueSegmentAccessor : public ZuAccessor<MxMDOrderBook *, uint128_t> {
    inline static uint128_t value(const MxMDOrderBook *ob) {
      return venueSegment(ob->venueID(), ob->segment());
    }
  };

  MxMDOrderBook( // single leg
    MxMDShard *shard, 
    MxMDVenue *venue,
    MxMDOrderBook *consolidated,
    MxID segment, ZuString id,
    MxMDSecurity *security,
    MxMDTickSizeTbl *tickSizeTbl,
    const MxMDLotSizes &lotSizes,
    MxMDSecHandler *handler);

  MxMDOrderBook( // multi-leg
    MxMDShard *shard, 
    MxMDVenue *venue,
    MxID segment, ZuString id,
    MxUInt legs, const ZmRef<MxMDSecurity> *securities,
    const MxEnum *sides, const MxFloat *ratios,
    MxMDTickSizeTbl *tickSizeTbl,
    const MxMDLotSizes &lotSizes);

public:
  MxMDLib *md() const;

  ZuInline MxMDVenue *venue() const { return m_venue; };

  ZuInline MxMDSecurity *security() const { return m_securities[0]; }
  ZuInline MxMDSecurity *security(MxUInt leg) const {
    if (ZuUnlikely(!*leg)) leg = 0;
    return m_securities[leg];
  }

  ZuInline MxMDOrderBook *consolidated() const { return m_consolidated; };

  ZuInline const MxID &venueID() const { return m_key.venue(); }
  ZuInline const MxID &segment() const { return m_key.segment(); }
  ZuInline const MxSymString &id() const { return m_key.id(); };
  ZuInline const MxSecKey &key() const { return m_key; }

  ZuInline const MxUInt &legs() const { return m_legs; };
  ZuInline const MxEnum &side(MxUInt leg) const { return m_sides[leg]; }
  ZuInline const MxFloat &ratio(MxUInt leg) const { return m_ratios[leg]; }

  ZuInline MxMDTickSizeTbl *tickSizeTbl() const { return m_tickSizeTbl; }

  ZuInline const MxMDLotSizes &lotSizes() const { return m_lotSizes; }

  ZuInline const MxMDL1Data &l1Data() const { return m_l1Data; }

  ZuInline MxMDOBSide *bids() const { return m_bids; };
  ZuInline MxMDOBSide *asks() const { return m_asks; };

  void l1(MxMDL1Data &data);
  void pxLevel(MxEnum side, MxDateTime transactTime,
      bool delta, MxFloat price, MxFloat qty, MxFloat nOrders,
      MxFlags flags = MxFlags());
  void l2(MxDateTime stamp, bool updateL1 = false);

  ZmRef<MxMDOrder> addOrder(
    ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
    MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags);
  ZmRef<MxMDOrder> modifyOrder(
    ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
    MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags);
  ZmRef<MxMDOrder> reduceOrder(
    ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
    MxEnum side, MxFloat reduceQty);
  ZmRef<MxMDOrder> cancelOrder(
    ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
    MxEnum side);

  void reset(MxDateTime transactTime);

  void addTrade(ZuString tradeID,
      MxDateTime transactTime, MxFloat price, MxFloat qty);
  void correctTrade(ZuString tradeID,
      MxDateTime transactTime, MxFloat price, MxFloat qty);
  void cancelTrade(ZuString tradeID,
      MxDateTime transactTime, MxFloat price, MxFloat qty);

  void update(
      const MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes);

  void subscribe(MxMDSecHandler *);
  void unsubscribe();

  ZuInline const ZmRef<MxMDSecHandler> &handler() const { return m_handler; }

  template <typename T = MxMDFeedOB>
  ZuInline typename ZuIs<MxMDFeedOB, T, ZmRef<T> &>::T feedOB() {
    ZmRef<T> *ZuMayAlias(ptr) = (ZmRef<T> *)&m_feedOB;
    return *ptr;
  }

private:
  ZuInline static uint64_t venueSegment(MxID venue, MxID segment) {
    return (((uint128_t)venue)<<64) | (uint128_t)segment;
  }

  ZuInline void consolidated(MxMDOrderBook *ob) { m_consolidated = ob; }

  void pxLevel_(
      MxEnum side, MxDateTime transactTime, bool delta,
      MxFloat price, MxFloat qty, MxFloat nOrders, MxFlags flags,
      bool consolidated, MxFloat *d_qty, MxFloat *d_nOrders);

  void reduceOrder_(MxMDOrder *order, MxDateTime transactTime,
      MxFloat reduceQty);
  void modifyOrder_(MxMDOrder *order, MxDateTime transactTime,
      MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags);
  void cancelOrder_(MxMDOrder *order, MxDateTime transactTime);

  void addOrder_(
    MxMDOrder *order, MxDateTime transactTime,
    const MxMDSecHandler *handler,
    const MxMDPxLevelFn *&fn, ZuRef<MxMDPxLevel> &pxLevel);
  void delOrder_(
    MxMDOrder *order, MxDateTime transactTime,
    const MxMDSecHandler *handler,
    const MxMDPxLevelFn *&fn, ZuRef<MxMDPxLevel> &pxLevel);

  void reset_(MxMDPxLevel *pxLevel, MxDateTime transactTime);

  MxMDVenue			*m_venue;
  MxMDVenueShard		*m_venueShard;

  MxMDOrderBook			*m_consolidated;

  MxSecKey			m_key;
  MxUInt			m_legs;
  MxMDSecurity			*m_securities[MxMDNLegs];
  MxEnum			m_sides[MxMDNLegs];
  MxFloat			m_ratios[MxMDNLegs];

  ZmRef<MxMDFeedOB>		m_feedOB;

  MxMDTickSizeTbl		*m_tickSizeTbl;
  MxMDLotSizes		 	m_lotSizes;

  MxMDL1Data			m_l1Data;

  ZuRef<MxMDOBSide>		m_bids;
  ZuRef<MxMDOBSide>		m_asks;

  ZmRef<MxMDSecHandler>		m_handler;
};

template <class Heap, class PxLevel>
ZuInline MxMDOBSide *MxMDOrder_<Heap, PxLevel>::bids_(MxMDOrderBook *ob)
{
  return ob ? ob->bids() : (MxMDOBSide *)0;
}
template <class Heap, class PxLevel>
ZuInline MxMDOBSide *MxMDOrder_<Heap, PxLevel>::asks_(MxMDOrderBook *ob)
{
  return ob ? ob->asks() : (MxMDOBSide *)0;
}

template <class Heap, class PxLevel>
ZuInline MxMDOrderID2Ref MxMDOrder_<Heap, PxLevel>::OrderID2Accessor::value(
  const MxMDOrder_<Heap, PxLevel> *o)
{
  return MxMDOrderID2Ref(o->m_orderBook->key(), o->m_id);
}
template <class Heap, class PxLevel>
ZuInline MxMDOrderID3Ref MxMDOrder_<Heap, PxLevel>::OrderID3Accessor::value(
  const MxMDOrder_<Heap, PxLevel> *o)
{
  return MxMDOrderID3Ref(o->m_orderBook->key(), o->m_obSide->side(), o->m_id);
}

// securities

class MxMDDerivatives : public ZmObject {
  MxMDDerivatives(const MxMDDerivatives &) = delete;
  MxMDDerivatives &operator =(const MxMDDerivatives &) = delete;

friend class MxMDSecurity;

  struct Futures_HeapID : public ZmHeapSharded {
    inline static const char *id() { return "MxMDLib.Futures"; }
  };
  typedef ZmRBTree<MxFutKey,			// mat
	    ZmRBTreeVal<MxMDSecurity *,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeHeapID<Futures_HeapID,
		  ZmRBTreeLock<ZmNoLock> > > > > Futures;
  struct Options_HeapID : public ZmHeapSharded {
    inline static const char *id() { return "MxMDLib.Options"; }
  };
  typedef ZmRBTree<MxOptKey,			// mat, putCall, strike
	    ZmRBTreeVal<MxMDSecurity *,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeHeapID<Options_HeapID,
		  ZmRBTreeLock<ZmNoLock> > > > > Options;

  ZuInline MxMDDerivatives() { }

public:
  uintptr_t allFutures(ZmFn<MxMDSecurity *>) const;
  ZuInline MxMDSecurity *future(const MxFutKey &key) const
    { return m_futures.findVal(key); }

  uintptr_t allOptions(ZmFn<MxMDSecurity *>) const;
  ZuInline MxMDSecurity *option(const MxOptKey &key) const
    { return m_options.findVal(key); }

private:
  void add(MxMDSecurity *);
  void del(MxMDSecurity *);

  Futures		m_futures;
  Options		m_options;
};

class MxMDAPI MxMDSecurity : public ZmObject, public MxMDSharded {
  MxMDSecurity(const MxMDSecurity &) = delete;
  MxMDSecurity &operator =(const MxMDSecurity &) = delete;

friend class MxMDLib;
friend class MxMDShard;

  struct KeyAccessor : public ZuAccessor<MxMDSecurity *, MxSecKey> {
    ZuInline static const MxSecKey &value(const MxMDSecurity *security) {
      return security->key();
    }
  };

  struct OrderBooks_HeapID : public ZmHeapSharded {
    inline static const char *id() { return "MxMDSecurity.OrderBooks"; }
  };
  typedef ZmRBTree<ZmRef<MxMDOrderBook>,
	    ZmRBTreeIndex<MxMDOrderBook::VenueSegmentAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock,
		  ZmRBTreeHeapID<OrderBooks_HeapID> > > > > OrderBooks;

  MxMDSecurity(MxMDShard *shard,
      MxID primaryVenue, MxID primarySegment, ZuString id,
      const MxMDSecRefData &refData);

public:
  MxMDLib *md() const;

  ZuInline const MxID &primaryVenue() const { return m_key.venue(); }
  ZuInline const MxID &primarySegment() const { return m_key.segment(); }
  ZuInline const MxSymString &id() const { return m_key.id(); }
  ZuInline const MxSecKey &key() const { return m_key; }

  ZuInline const MxMDSecRefData &refData() const { return m_refData; }

  ZuInline MxMDSecurity *underlying() const { return m_underlying; }
  ZuInline MxMDDerivatives *derivatives() const { return m_derivatives; }

  void update(const MxMDSecRefData &refData);

  void subscribe(MxMDSecHandler *);
  void unsubscribe();

  ZuInline const ZmRef<MxMDSecHandler> &handler() const { return m_handler; }

  ZuInline ZmRef<MxMDOrderBook> consolidated() const {
    return m_orderBooks.findKey(MxMDOrderBook::venueSegment(MxID(), MxID()));
  }
  ZuInline ZmRef<MxMDOrderBook> orderBook() const {
    // find consolidated book if it exists
    if (ZmRef<MxMDOrderBook> ob = m_orderBooks.findKey(
	  MxMDOrderBook::venueSegment(MxID(), MxID())))
      return ob;
    // no consolidated book implies only one order book available
    if (ZmRef<MxMDOrderBook> ob = m_orderBooks.minimumKey())
      return ob;
    return (MxMDOrderBook *)0;
  }
  ZuInline ZmRef<MxMDOrderBook> orderBook(MxID venue) const {
    if (ZmRef<MxMDOrderBook> ob = OrderBooks::ReadIterator(m_orderBooks,
	  MxMDOrderBook::venueSegment(venue, MxID())).iterateKey())
      if (ob && ob->venueID() == venue) return ob;
    return (MxMDOrderBook *)0;
  }
  ZuInline ZmRef<MxMDOrderBook> orderBook(MxID venue, MxID segment) const {
    if (ZmRef<MxMDOrderBook> ob = m_orderBooks.findKey(
	  MxMDOrderBook::venueSegment(MxID(), MxID())))
      return ob;
    return (MxMDOrderBook *)0;
  }
  uintptr_t allOrderBooks(ZmFn<MxMDOrderBook *>) const;

  ZmRef<MxMDOrderBook> addOrderBook(
    MxID venue, MxID segment, ZuString id,
    MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes);
  void delOrderBook(MxID venue, MxID segment);

private:
  void addOrderBook_(MxMDOrderBook *);
  ZmRef<MxMDOrderBook> findOrderBook_(MxID venue, MxID segment);
  ZmRef<MxMDOrderBook> delOrderBook_(MxID venue, MxID segment);

  ZuInline void underlying(MxMDSecurity *u) { m_underlying = u; }
  ZuInline void addDerivative(MxMDSecurity *d) {
    if (!m_derivatives) m_derivatives = new MxMDDerivatives();
    m_derivatives->add(d);
  }
  ZuInline void delDerivative(MxMDSecurity *d) {
    if (!m_derivatives) return;
    m_derivatives->del(d);
  }

  void update_(const MxMDSecRefData &);

  MxSecKey			m_key;

  MxMDSecRefData		m_refData;

  MxMDSecurity			*m_underlying = 0;
  ZmRef<MxMDDerivatives>	m_derivatives;

  OrderBooks			m_orderBooks;

  ZmRef<MxMDSecHandler>	  	m_handler;
};

// feeds

class MxMDAPI MxMDFeed : public ZmPolymorph {
  MxMDFeed(const MxMDFeed &) = delete;
  MxMDFeed &operator =(const MxMDFeed &) = delete;

friend class MxMDLib;
friend class MxMDCore;
friend class MxMDVenue;
friend class MxMDSecurity;
friend class MxMDOrderBook;

public:
  MxMDFeed(MxMDLib *md, MxID id);

  virtual ~MxMDFeed() { }

  struct IDAccessor : public ZuAccessor<MxMDFeed *, MxID> {
    inline static const MxID &value(const MxMDFeed *f) { return f->id(); }
  };

  inline MxMDLib *md() const { return m_md; }
  inline const MxID &id() const { return m_id; }

  virtual bool isConnected() const { return true; }

  void connected();
  void disconnected();

protected:
  virtual void start();
  virtual void stop();
  virtual void final();

  virtual void addOrderBook(MxMDOrderBook *);
  virtual void delOrderBook(MxMDOrderBook *);

  MxMDLib	*m_md;
  MxID		m_id;
};

// venues

class MxMDAPI MxMDVenueShard : public ZuObject {
friend class MxMDVenue;
friend class MxMDOrderBook;
friend class MxMDOBSide;

  typedef ZmHash<ZmRef<MxMDOrder>,
	    ZmHashIndex<MxMDOrder::OrderID2Accessor,
	      ZmHashObject<ZuNull,
		ZmHashLock<ZmNoLock,
		  ZmHashHeapID<MxMDOrders_HeapID> > > > > Orders2;
  typedef ZmHash<ZmRef<MxMDOrder>,
	    ZmHashIndex<MxMDOrder::OrderID3Accessor,
	      ZmHashObject<ZuNull,
		ZmHashLock<ZmNoLock,
		  ZmHashHeapID<MxMDOrders_HeapID> > > > > Orders3;

  MxMDVenueShard(MxMDVenue *venue, MxMDShard *shard);

public:
  unsigned id() const;

private:
  void addOrder(MxMDOrder *order);
  template <typename OrderID>
  ZmRef<MxMDOrder> findOrder(
      const MxSecKey &obKey, MxEnum side, OrderID &&orderID,
      MxEnum orderIDScope);
  template <typename OrderID>
  ZmRef<MxMDOrder> delOrder(
      const MxSecKey &obKey, MxEnum side, OrderID &&orderID,
      MxEnum orderIDScope);

  inline void addOrder2(MxMDOrder *order) { m_orders2.add(order); }
  template <typename OrderID> ZuInline ZmRef<MxMDOrder> findOrder2(
      const MxSecKey &obKey, OrderID &&orderID) {
    return m_orders2.findKey(ZuMkTuple(obKey, ZuFwd<OrderID>(orderID)));
  }
  template <typename OrderID> ZuInline ZmRef<MxMDOrder> delOrder2(
      const MxSecKey &obKey, OrderID &&orderID) {
    return m_orders2.delKey(ZuMkTuple(obKey, ZuFwd<OrderID>(orderID)));
  }

  inline void addOrder3(MxMDOrder *order) { m_orders3.add(order); }
  template <typename OrderID> ZuInline ZmRef<MxMDOrder> findOrder3(
      const MxSecKey &obKey, MxEnum side, OrderID &&orderID) {
    return m_orders3.findKey(ZuMkTuple(obKey, side, ZuFwd<OrderID>(orderID)));
  }
  template <typename OrderID> ZuInline ZmRef<MxMDOrder> delOrder3(
      const MxSecKey &obKey, MxEnum side, OrderID &&orderID) {
    return m_orders3.delKey(ZuMkTuple(obKey, side, ZuFwd<OrderID>(orderID)));
  }

  MxMDVenue		*m_venue;
  MxMDShard		*m_shard;
  Orders2		m_orders2;
  Orders3		m_orders3;
};

class MxMDAPI MxMDVenue : public ZmObject {
  MxMDVenue(const MxMDVenue &) = delete;
  MxMDVenue &operator =(const MxMDVenue &) = delete;

friend class MxMDLib;
friend class MxMDVenueShard;
friend class MxMDOrderBook;

  typedef ZuPair<MxEnum, MxDateTime> Session; // MxTradingSession, stamp

  typedef ZtArray<ZuRef<MxMDVenueShard> > Shards;

  struct TickSizeTbls_HeapID {
    inline static const char *id() { return "MxMDVenue.TickSizeTbls"; }
  };
  struct Segments_ID {
    inline static const char *id() { return "MxMDVenue.Segments"; }
  };
  typedef ZmRBTree<ZmRef<MxMDTickSizeTbl>,
	    ZmRBTreeIndex<MxMDTickSizeTbl::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmPRWLock,
		  ZmRBTreeHeapID<TickSizeTbls_HeapID> > > > > TickSizeTbls;
  typedef ZmLHash<MxID,
	    ZmLHashVal<Session,
	      ZmLHashLock<ZmPRWLock,
		ZmLHashID<Segments_ID> > > > Segments;
  typedef ZmHash<ZmRef<MxMDOrder>,
	    ZmHashIndex<MxMDOrder::OrderIDAccessor,
	      ZmHashObject<ZuNull,
		ZmHashLock<ZmPLock,
		  ZmHashHeapID<MxMDOrders_HeapID> > > > > Orders;

public:
  MxMDVenue(MxMDLib *md, MxMDFeed *feed, MxID id);

  virtual ~MxMDVenue() { }

  struct IDAccessor : public ZuAccessor<MxMDVenue *, MxID> {
    inline static MxID value(const MxMDVenue *v) { return v->id(); }
  };

  ZuInline MxMDLib *md() const { return m_md; }
  ZuInline MxMDFeed *feed() const { return m_feed; }
  ZuInline MxID id() const { return m_id; }

private:
  ZuInline void loaded_(bool b) { m_loaded = b; }
public:
  ZuInline bool loaded() const { return m_loaded; }

  ZmRef<MxMDTickSizeTbl> addTickSizeTbl(ZuString id);
  template <typename ID>
  ZuInline ZmRef<MxMDTickSizeTbl> tickSizeTbl(const ID &id) const {
    return m_tickSizeTbls.findKey(id);
  }
  uintptr_t allTickSizeTbls(ZmFn<MxMDTickSizeTbl *>) const;

  uintptr_t allSegments(ZmFn<MxID, MxEnum, MxDateTime>) const;

  Session tradingSession(MxID segment = MxID()) const;
  void tradingSession(MxID segment, MxEnum session, MxDateTime stamp);

  ZmRef<MxMDOrderBook> addCombination(
      MxID segment, ZuString orderBookID,
      MxUInt legs, const ZmRef<MxMDSecurity> *securities,
      const MxEnum *sides, const MxFloat *ratios,
      MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes);
  void delCombination(MxID segment, ZuString orderBookID);

  void modifyOrder(ZuString orderID, MxDateTime transactTime,
      MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags,
      ZmFn<MxMDOrder *> fn = ZmFn<MxMDOrder *>());
  void reduceOrder(ZuString orderID,
      MxDateTime transactTime, MxFloat reduceQty,
      ZmFn<MxMDOrder *> fn = ZmFn<MxMDOrder *>()); // qty -= reduceQty
  void cancelOrder(ZuString orderID, MxDateTime transactTime,
      ZmFn<MxMDOrder *> fn = ZmFn<MxMDOrder *>());

private:
  ZmRef<MxMDTickSizeTbl> findTickSizeTbl_(ZuString id);
  ZmRef<MxMDTickSizeTbl> addTickSizeTbl_(ZuString id);

  void tradingSession_(MxID segment, MxEnum session, MxDateTime stamp);

  ZuInline void addOrder(MxMDOrder *order) { m_orders.add(order); }
  template <typename OrderID>
  ZuInline ZmRef<MxMDOrder> findOrder(OrderID &&orderID) {
    return m_orders.findKey(ZuFwd<OrderID>(orderID));
  }
  template <typename OrderID>
  ZuInline ZmRef<MxMDOrder> delOrder(OrderID &&orderID) {
    return m_orders.delKey(ZuFwd<OrderID>(orderID));
  }

  ZuInline unsigned nShards() const { return m_shards.length(); }
  ZuInline MxMDVenueShard *shard(unsigned i) const { return m_shards[i]; }

  MxMDLib		*m_md;
  MxMDFeed		*m_feed;
  MxID			m_id;
  Shards		m_shards;

  TickSizeTbls	 	m_tickSizeTbls;
  Segments		m_segments;
  ZmAtomic<unsigned>	m_loaded = 0;

  Orders		m_orders;
};

inline void MxMDVenueShard::addOrder(MxMDOrder *order)
{
  switch ((int)order->idScope()) {
    case MxMDOrderIDScope::Venue: m_venue->addOrder(order); break;
    case MxMDOrderIDScope::OrderBook: addOrder2(order); break;
    case MxMDOrderIDScope::OBSide: addOrder3(order); break;
  }
}
template <typename OrderID>
inline ZmRef<MxMDOrder> MxMDVenueShard::findOrder(
    const MxSecKey &obKey, MxEnum side, OrderID &&orderID,
    MxEnum orderIDScope)
{
  switch ((int)orderIDScope) {
    case MxMDOrderIDScope::Venue:
      return m_venue->findOrder(ZuFwd<OrderID>(orderID));
    case MxMDOrderIDScope::OrderBook:
      return findOrder2(obKey, ZuFwd<OrderID>(orderID));
    case MxMDOrderIDScope::OBSide:
      return findOrder3(obKey, side, ZuFwd<OrderID>(orderID));
  }
  return 0;
}
template <typename OrderID>
inline ZmRef<MxMDOrder> MxMDVenueShard::delOrder(
    const MxSecKey &obKey, MxEnum side, OrderID &&orderID,
    MxEnum orderIDScope)
{
  switch ((int)orderIDScope) {
    case MxMDOrderIDScope::Venue:
      return m_venue->delOrder(ZuFwd<OrderID>(orderID));
    case MxMDOrderIDScope::OrderBook:
      return delOrder2(obKey, ZuFwd<OrderID>(orderID));
    case MxMDOrderIDScope::OBSide:
      return delOrder3(obKey, side, ZuFwd<OrderID>(orderID));
  }
  return 0;
}

// shard

class MxMDAPI MxMDShard : public ZuObject, public ZmShard<MxMDLib> {
friend class MxMDLib;
friend class MxMDCore;

  ZuInline MxMDShard(MxMDLib *md, unsigned id, unsigned tid) :
    ZmShard<MxMDLib>(md, tid), m_id(id) { }

public:
  ZuInline MxMDLib *md() const { return mgr(); }
  ZuInline unsigned id() const { return m_id; }

  ZuInline ZmRef<MxMDSecurity> security(const MxSecKey &key) const {
    return m_securities.findKey(key);
  }
  uintptr_t allSecurities(ZmFn<MxMDSecurity *>) const;
  ZmRef<MxMDSecurity> addSecurity(
      MxID primaryVenue, MxID primarySegment, ZuString id,
      const MxMDSecRefData &refData);

  ZuInline ZmRef<MxMDOrderBook> orderBook(const MxSecKey &key) const {
    return m_orderBooks.findKey(key);
  }
  uintptr_t allOrderBooks(ZmFn<MxMDOrderBook *>) const;

private:
  struct Securities_HeapID : public ZmHeapSharded {
    ZuInline static const char *id() { return "MxMDShard.Securities"; }
  };
  typedef ZmRBTree<MxMDSecurity *,
	    ZmRBTreeIndex<MxMDSecurity::KeyAccessor,
	      ZmRBTreeObject<ZuObject,
		ZmRBTreeLock<ZmNoLock,
		  ZmRBTreeHeapID<Securities_HeapID> > > > > Securities;
  ZuInline void addSecurity(MxMDSecurity *security) {
    m_securities.add(security);
  }
  ZuInline void delSecurity(MxMDSecurity *security) {
    m_securities.del(security->key());
  }

  struct OrderBooks_HeapID : public ZmHeapSharded {
    ZuInline static const char *id() { return "MxMDShard.OrderBooks"; }
  };
  typedef ZmRBTree<MxMDOrderBook *,
	    ZmRBTreeIndex<MxMDOrderBook::KeyAccessor,
	      ZmRBTreeObject<ZuObject,
		ZmRBTreeLock<ZmNoLock,
		    ZmRBTreeHeapID<OrderBooks_HeapID> > > > > OrderBooks;
  ZuInline void addOrderBook(MxMDOrderBook *ob) {
    m_orderBooks.add(ob);
  }
  ZuInline void delOrderBook(MxMDOrderBook *ob) {
    m_orderBooks.del(ob->key());
  }

  unsigned	m_id;
  Securities	m_securities;
  OrderBooks	m_orderBooks;
};

ZuInline MxMDLib *MxMDOrderBook::md() const { return shard()->mgr(); }

ZuInline MxMDLib *MxMDSecurity::md() const { return shard()->mgr(); }

ZuInline unsigned MxMDVenueShard::id() const { return m_shard->id(); }

// library

class MxMDAPI MxMDLib {
  MxMDLib(const MxMDLib &) = delete;
  MxMDLib &operator =(const MxMDLib &) = delete;

friend class MxMDTickSizeTbl;
friend class MxMDOrderBook;
friend class MxMDOrderBook;
friend class MxMDSecurity;
friend class MxMDFeed;
friend class MxMDVenue;
friend class MxMDShard;

protected:
  MxMDLib(ZmScheduler *);

public:
  virtual ~MxMDLib() { }

  static MxMDLib *instance();

  static MxMDLib *init(const char *cf);

  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void final() = 0;

  virtual void record(ZuString path) = 0;
  virtual void stopRecording() = 0;
  virtual void stopStreaming() = 0;

  virtual void replay(
    ZuString path,
    MxDateTime begin = MxDateTime(),
    bool filter = true) = 0;
  virtual void stopReplaying() = 0;

  virtual void startTimer(MxDateTime begin = MxDateTime()) = 0;
  virtual void stopTimer() = 0;

  virtual void dumpTickSizes(ZuString path, MxID venue = MxID()) = 0;
  virtual void dumpSecurities(
      ZuString path, MxID venue = MxID(), MxID segment = MxID()) = 0;
  virtual void dumpOrderBooks(
      ZuString path, MxID venue = MxID(), MxID segment = MxID()) = 0;

  template <typename> friend class ZmShard;
private:
  template <typename ...Args>
  ZuInline void run(unsigned tid, Args &&... args)
    { m_scheduler->run(tid, ZuFwd<Args>(args)...); }
  template <typename ...Args>
  ZuInline void invoke(unsigned tid, Args &&... args)
    { m_scheduler->invoke(tid, ZuFwd<Args>(args)...); }

  typedef ZtArray<ZuRef<MxMDShard> > Shards;

public:
  ZuInline unsigned nShards() const { return m_shards.length(); }
  template <typename L>
  ZuInline void shard(unsigned i, L l) const {
    MxMDShard *shard = m_shards[i];
    shard->invoke([l = ZuMv(l), shard]() { l(shard); });
  }
  template <typename ...Args>
  ZuInline void shardRun(unsigned i, Args &&... args)
    { m_shards[i]->run(ZuFwd<Args>(args)...); }
  template <typename ...Args>
  ZuInline void shardInvoke(unsigned i, Args &&... args)
    { m_shards[i]->invoke(ZuFwd<Args>(args)...); }

  void raise(ZeEvent *e);

  void addFeed(MxMDFeed *feed);
  void addVenue(MxMDVenue *venue);

public:
  void loaded(MxMDVenue *venue);

  void subscribe(MxMDLibHandler *);
  void unsubscribe();

  ZuInline ZmRef<MxMDLibHandler> handler() {
    SubGuard guard(m_subLock);
    return m_handler;
  }

protected:
  typedef ZmPRWLock RWLock;
  typedef ZmGuard<RWLock> Guard;
  typedef ZmReadGuard<RWLock> ReadGuard;

  typedef ZmPLock SubLock;
  typedef ZmGuard<SubLock> SubGuard;

private:
  ZuInline MxMDShard *shard_(unsigned i) const { return m_shards[i]; }

protected:
  ZmRef<MxMDSecurity> addSecurity(
      MxID primaryVenue, MxID primarySegment, ZuString id,
      unsigned shardID, const MxMDSecRefData &refData);

private:
  ZmRef<MxMDTickSizeTbl> addTickSizeTbl(MxMDVenue *venue, ZuString id);
  void resetTickSizeTbl(MxMDTickSizeTbl *tbl);
  void addTickSize(MxMDTickSizeTbl *tickSizeTbl,
      MxFloat minPrice, MxFloat maxPrice, MxFloat tickSize);

  void updateSecurity(MxMDSecurity *security, const MxMDSecRefData &refData);
  void addSecIndices(MxMDSecurity *, const MxMDSecRefData &);
  void delSecIndices(MxMDSecurity *, const MxMDSecRefData &);

  ZmRef<MxMDOrderBook> addOrderBook(
      MxMDSecurity *security,
      MxID venue, MxID segment, ZuString id,
      MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes);
      
  ZmRef<MxMDOrderBook> addCombination(
      MxMDVenue *venue, MxID segment, ZuString id,
      MxUInt legs, const ZmRef<MxMDSecurity> *securities,
      const MxEnum *sides, const MxFloat *ratios,
      MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes);
      
  void updateOrderBook(MxMDOrderBook *ob,
      const MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes);
  void delOrderBook(MxMDSecurity *security, MxID venue, MxID segment);
  void delCombination(MxMDVenue *venue, MxID segment, ZuString id);

  void tradingSession(
      MxMDVenue *venue, MxID segment, MxEnum session, MxDateTime stamp);

  void l1(const MxMDOrderBook *ob, const MxMDL1Data &l1Data);

  void pxLevel(const MxMDOrderBook *ob, MxEnum side, MxDateTime transactTime,
      bool delta, MxFloat price, MxFloat qty, MxFloat nOrders, MxFlags flags);

  void l2(const MxMDOrderBook *ob, MxDateTime stamp, bool updateL1);

  void addOrder(const MxMDOrderBook *ob,
      ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
      MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags);
  void modifyOrder(const MxMDOrderBook *ob,
      ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
      MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags);
  void cancelOrder(const MxMDOrderBook *ob, ZuString orderID,
      MxEnum orderIDScope, MxDateTime transactTime, MxEnum side);

  void resetOB(const MxMDOrderBook *ob, MxDateTime transactTime);

  void addTrade(const MxMDOrderBook *ob, ZuString tradeID,
      MxDateTime transactTime, MxFloat price, MxFloat qty);
  void correctTrade(const MxMDOrderBook *ob, ZuString tradeID,
      MxDateTime transactTime, MxFloat price, MxFloat qty);
  void cancelTrade(const MxMDOrderBook *ob, ZuString tradeID,
      MxDateTime transactTime, MxFloat price, MxFloat qty);

  // primary indices

  struct AllSecurities_HeapID {
    ZuInline static const char *id() { return "MxMDLib.AllSecurities"; }
  };
  typedef ZmHash<ZmRef<MxMDSecurity>,
	    ZmHashIndex<MxMDSecurity::KeyAccessor,
	      ZmHashObject<ZuNull,
		ZmHashLock<RWLock,
		  ZmHashHeapID<AllSecurities_HeapID> > > > > AllSecurities;

  struct AllOrderBooks_HeapID {
    ZuInline static const char *id() { return "MxMDLib.AllOrderBooks"; }
  };
  typedef ZmHash<ZmRef<MxMDOrderBook>,
	    ZmHashIndex<MxMDOrderBook::KeyAccessor,
	      ZmHashObject<ZuNull,
		ZmHashLock<RWLock,
		  ZmHashHeapID<AllOrderBooks_HeapID> > > > > AllOrderBooks;

  // secondary indices

  struct Securities_HeapID {
    ZuInline static const char *id() { return "MxMDLib.Securities"; }
  };
  typedef ZmHash<MxSecSymKey,
	    ZmHashVal<MxMDSecurity *,
	      ZmHashObject<ZuNull,
		ZmHashLock<RWLock,
		  ZmHashHeapID<Securities_HeapID> > > > > Securities;

  // feeds, venues

  struct Feeds_HeapID {
    ZuInline static const char *id() { return "MxMDLib.Feeds"; }
  };
  typedef ZmHash<ZmRef<MxMDFeed>,
	    ZmHashIndex<MxMDFeed::IDAccessor,
	      ZmHashObject<ZuNull,
		ZmHashLock<RWLock,
		  ZmHashHeapID<Feeds_HeapID> > > > > Feeds;

  struct Venues_HeapID {
    ZuInline static const char *id() { return "MxMDLib.Venues"; }
  };
  typedef ZmHash<ZmRef<MxMDVenue>,
	    ZmHashIndex<MxMDVenue::IDAccessor,
	      ZmHashObject<ZuNull,
		ZmHashLock<RWLock,
		  ZmHashHeapID<Venues_HeapID> > > > > Venues;

public:
  template <typename L>
  ZuInline void security(const MxSecKey &key, L l) const {
    if (ZmRef<MxMDSecurity> sec = m_allSecurities.findKey(key))
      sec->invoke([l = ZuMv(l), sec = ZuMv(sec)]() { l(sec); });
    else
      l((MxMDSecurity *)0);
  }
  template <typename L>
  ZuInline void security(const MxSecSymKey &key, L l) const {
    if (ZmRef<MxMDSecurity> sec = m_securities.findVal(key))
      sec->invoke([l = ZuMv(l), sec = ZuMv(sec)]() { l(sec); });
    else
      l((MxMDSecurity *)0);
  }
  uintptr_t allSecurities(ZmFn<MxMDSecurity *>) const;

  template <typename L>
  ZuInline void orderBook(const MxSecKey &key, L l) const {
    if (ZmRef<MxMDOrderBook> ob = m_allOrderBooks.findKey(key))
      ob->invoke([l = ZuMv(l), ob = ZuMv(ob)]() { l(ob); });
    else
      l((MxMDOrderBook *)0);
  }
  uintptr_t allOrderBooks(ZmFn<MxMDOrderBook *>) const;

  ZuInline ZmRef<MxMDFeed> feed(MxID id) const {
    return m_feeds.findKey(id);
  }
  uintptr_t allFeeds(ZmFn<MxMDFeed *>) const;

  ZuInline ZmRef<MxMDVenue> venue(MxID id) const {
    return m_venues.findKey(id);
  }
  uintptr_t allVenues(ZmFn<MxMDVenue *>) const;

protected:
  ZmScheduler		*m_scheduler = 0;
  Shards		m_shards;

  AllSecurities		m_allSecurities;
  AllOrderBooks		m_allOrderBooks;
  Securities		m_securities;
  Feeds			m_feeds;
  Venues		m_venues;

  RWLock		m_refDataLock;	// serializes updates to containers

  SubLock		m_subLock;
    ZmRef<MxMDLibHandler> m_handler;
};

inline void MxMDFeed::connected() {
  m_md->handler()->connected(this);
}
inline void MxMDFeed::disconnected() {
  m_md->handler()->disconnected(this);
}

inline MxMDVenue::Session MxMDVenue::tradingSession(MxID segment) const {
  return m_segments.findVal(segment);
}

#endif /* MxMD_HPP */
