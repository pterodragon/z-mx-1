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
#include <ZmShard.hpp>
#include <ZmScheduler.hpp>

#include <ZvCmd.hpp>

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

template <class T> struct MxMDFlags {
  struct Default {
    static void print(MxMDFlagsStr &out, MxFlags flags) {
      out << flags.hex(ZuFmt::Right<8>());
    }
    static void scan(const MxMDFlagsStr &in, MxFlags &flags) {
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

class MxMDCore;
class MxMDLib;
class MxMDShard;
class MxMDFeed;
class MxMDVenue;
class MxMDVenueShard;
class MxMDInstrument;
class MxMDOrderBook;
class MxMDOBSide;
class MxMDPxLevel_;
class MxMDOrder_;

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
    static void scan(const MxMDFlagsStr &in, MxFlags &flags) {
      flags = 0;
      for (unsigned i = 0; i < in.length(); i++) {
	if (in[i] == 'A') auction(flags, true);
	if (in[i] == 'P') printable(flags, true);
      }
    }
  };
};

// venue mapping

ZuTupleFields(MxMDVenueMapKey_, 1, venue, 2, segment);
typedef MxMDVenueMapKey_<MxID, MxID> MxMDVenueMapKey;
struct MxMDVenueMapping {
  MxID		venue;
  MxID		segment;
  unsigned	rank = 0;

  ZuInline bool operator !() const { return !venue; }
  ZuOpBool
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
  MxMDTickSizeTbl(MxMDVenue *venue, const ID &id, MxNDP pxNDP) :
    m_venue(venue), m_id(id), m_pxNDP(pxNDP) { }

public:
  ~MxMDTickSizeTbl() { }

  ZuInline MxMDVenue *venue() const { return m_venue; }
  ZuInline const MxIDString &id() const { return m_id; }
  ZuInline MxNDP pxNDP() const { return m_pxNDP; }

  void reset();
  void addTickSize(MxValue minPrice, MxValue maxPrice, MxValue tickSize);
  inline MxValue tickSize(MxValue price) {
    auto i = m_tickSizes.readIterator<ZmRBTreeGreaterEqual>(price);
    TickSizes::Node *node = i.iterate();
    if (!node) return MxValue();
    return node->key().tickSize();
  }
  template <typename L> // (const MxMDTickSize &) -> uintptr_t
  uintptr_t allTickSizes(L l) const {
    auto i = m_tickSizes.readIterator();
    while (const TickSizes::Node *node = i.iterate())
      if (uintptr_t v = l(node->key())) return v;
    return 0;
  }

private:
  void reset_();
  void addTickSize_(MxValue minPrice, MxValue maxPrice, MxValue tickSize);

  MxMDVenue	*m_venue;
  MxIDString	m_id;
  MxNDP		m_pxNDP;
  TickSizes	m_tickSizes;
};

// trades

struct MxMDTradeData {
  MxIDString	tradeID;
  MxDateTime	transactTime;
  MxValue	price;
  MxValue	qty;
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
  inline MxMDTrade_(MxMDInstrument *instrument, MxMDOrderBook *ob,
	const TradeID &tradeID, MxDateTime transactTime,
	MxValue price, MxValue qty) :
      m_security(instrument), m_orderBook(ob),
      m_data{tradeID, transactTime, price, qty} { }

public:
  ~MxMDTrade_() { }

  inline MxMDInstrument *instrument() const { return m_security; }
  inline MxMDOrderBook *orderBook() const { return m_orderBook; }

  inline const MxMDTradeData &data() const { return m_data; }

private:
  MxMDInstrument		*m_security;
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
  MxDateTime	transactTime;
  MxEnum	side;
  MxUInt	rank;
  MxValue	price;
  MxValue	qty;
  MxFlags	flags;		// MxMDOrderFlags
};

ZuTupleFields(MxMDOrderID2_, 1, obKey, 2, orderID);
typedef MxMDOrderID2_<MxInstrKey, MxIDString> MxMDOrderID2;
typedef MxMDOrderID2_<const MxInstrKey &, const MxIDString &> MxMDOrderID2Ref;
ZuTupleFields(MxMDOrderID3_, 1, obKey, 2, side, 3, orderID);
typedef MxMDOrderID3_<MxInstrKey, MxEnum, MxIDString> MxMDOrderID3;
typedef MxMDOrderID3_<const MxInstrKey &, const MxEnum &, const MxIDString &>
  MxMDOrderID3Ref;

class MxMDAPI MxMDOrder_ {
  MxMDOrder_(const MxMDOrder_ &) = delete;
  MxMDOrder_ &operator =(const MxMDOrder_ &) = delete;

friend class MxMDVenue;
friend class MxMDVenueShard;
friend class MxMDOrderBook;
friend class MxMDOBSide;
friend class MxMDPxLevel_;

private:
  static ZmRef<MxMDOBSide> bids_(const MxMDOrderBook *);
  static ZmRef<MxMDOBSide> asks_(const MxMDOrderBook *);

protected:
  template <typename ID>
  inline MxMDOrder_(MxMDOrderBook *ob,
    ID &&id, MxDateTime transactTime, MxEnum side,
    MxUInt rank, MxValue price, MxValue qty, MxFlags flags) :
      m_orderBook(ob),
      m_id(ZuFwd<ID>(id)),
      m_data{transactTime, side, rank, price, qty, flags} { }

public:
  inline ~MxMDOrder_() { }

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
    ZuInline static const MxIDString &value(const MxMDOrder_ *o) {
      return o->m_id;
    }
  };
  struct RankAccessor;
friend struct RankAccessor;
  struct RankAccessor : public ZuAccessor<MxMDOrder_ *, MxUInt> {
    ZuInline static MxUInt value(const MxMDOrder_ *o) {
      return o->data().rank;
    }
  };

public:
  ZuInline MxMDOrderBook *orderBook() const { return m_orderBook; }
  MxMDOBSide *obSide() const;
  ZuInline MxMDPxLevel_ *pxLevel() const { return m_pxLevel; }

  ZuInline const MxIDString &id() const { return m_id; }
  ZuInline const MxMDOrderData &data() const { return m_data; }

  template <typename T = uintptr_t>
  ZuInline T appData() const { return (T)m_appData; }
  template <typename T>
  ZuInline void appData(T v) { m_appData = (uintptr_t)v; }

private:
  inline void update_(MxUInt rank, MxValue price, MxValue qty, MxFlags flags) {
    if (*rank) m_data.rank = rank;
    if (*price) m_data.price = price;
    if (*qty) m_data.qty = qty;
    m_data.flags = flags;
  }
  inline void updateQty_(MxValue qty) {
    if (*qty) m_data.qty = qty;
  }
  inline void updateNDP(
      MxNDP oldPxNDP, MxNDP oldQtyNDP, MxNDP pxNDP, MxNDP qtyNDP) {
    if (pxNDP != oldPxNDP)
      m_data.price = MxValNDP{m_data.price, oldPxNDP}.adjust(pxNDP);
    if (qtyNDP != oldQtyNDP)
      m_data.qty = MxValNDP{m_data.qty, oldQtyNDP}.adjust(qtyNDP);
  }

  ZuInline void pxLevel(MxMDPxLevel_ *l) { m_pxLevel = l; }

  MxMDOrderBook		*m_orderBook;
  MxMDPxLevel_		*m_pxLevel = 0;

  MxIDString		m_id;
  MxMDOrderData		m_data;

  uintptr_t		m_appData = 0;
};

struct MxMDOrder_HeapID {
  inline static const char *id() { return "MxMDOrder"; }
};
template <class Heap>
class MxMDOrder__ : public Heap, public ZmObject, public MxMDOrder_ {
  MxMDOrder__(const MxMDOrder__ &) = delete;
  MxMDOrder__ &operator =(const MxMDOrder__ &) = delete;

template <typename, typename> friend struct ZuConversionFriend;

friend class MxMDOrderBook;

  template <typename ...Args>
  inline MxMDOrder__(Args &&... args) : MxMDOrder_(ZuFwd<Args>(args)...) { }

public:
  inline ~MxMDOrder__() = default;
};
typedef ZmHeap<MxMDOrder_HeapID, sizeof(MxMDOrder__<ZuNull>)> MxMDOrder_Heap;
typedef MxMDOrder__<MxMDOrder_Heap> MxMDOrder;

struct MxMDOrders_HeapID : public ZmHeapSharded {
  inline static const char *id() { return "MxMDLib.Orders"; }
};

// price levels

struct MxMDPxLvlData {
  inline MxMDPxLvlData &operator +=(const MxMDPxLvlData &data) {
    if (transactTime < data.transactTime) transactTime = data.transactTime;
    qty += data.qty;
    nOrders += data.nOrders;
    return *this;
  }

  MxDateTime		transactTime;
  MxValue		qty;
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

// - whatever is left over is added unless FOK/IOC
// - for external liquidity (consolidated book), match first against
// internal venue's OB, then against consolidated OB since internal
// matches are processed immediately and should be prioritized; external
// matches need to lock the taker while the child IOC orders are executed
// - return value from lambda indicates whether this match succeeded (
// if unsuccessful no further matching is attempted)
// - when matching a L2 feed venue, match entire px levels
// - when matching consolidated OB, ensure levels are broken up by venue
// and that venue is available in lambda

class MxMDPxLevel_ : public ZmObject {
  MxMDPxLevel_(const MxMDPxLevel_ &) = delete;
  MxMDPxLevel_ &operator =(const MxMDPxLevel_ &) = delete;

friend class MxMDOrderBook;
friend class MxMDOBSide;

  typedef ZmRBTree<ZmRef<MxMDOrder>,
	    ZmRBTreeIndex<typename MxMDOrder_::RankAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock,
		  ZmRBTreeHeapID<MxMDOrders_HeapID> > > > > Orders;

protected:
  inline MxMDPxLevel_(MxMDOBSide *obSide,
      MxDateTime transactTime, MxNDP pxNDP, MxNDP qtyNDP,
      MxValue price, MxValue qty, MxUInt nOrders, MxFlags flags) :
    m_obSide(obSide), m_pxNDP(pxNDP), m_qtyNDP(qtyNDP),
    m_price(price), m_data{transactTime, qty, nOrders, flags} { }

public:
  inline ~MxMDPxLevel_() { }

  ZuInline MxMDOBSide *obSide() const { return m_obSide; }
  MxEnum side() const;
  ZuInline MxNDP pxNDP() const { return m_pxNDP; }
  ZuInline MxNDP qtyNDP() const { return m_qtyNDP; }
  ZuInline MxValue price() const { return m_price; }
  ZuInline const MxMDPxLvlData &data() const { return m_data; }

  template <typename L> // (MxMDOrder *) -> uintptr_t
  uintptr_t allOrders(L l) const {
    auto i = m_orders.readIterator();
    while (const ZmRef<MxMDOrder> &order = i.iterateKey())
      if (uintptr_t v = l(order)) return v;
    return 0;
  }

private:
  void reset(MxDateTime transactTime);

  void updateAbs(
      MxDateTime transactTime, MxValue qty, MxUInt nOrders, MxFlags flags,
      MxValue &d_qty, MxUInt &d_nOrders);
  void updateAbs_(MxValue qty, MxUInt nOrders, MxFlags flags,
      MxValue &d_qty, MxUInt &d_nOrders);
  void updateDelta(
      MxDateTime transactTime, MxValue qty, MxUInt nOrders, MxFlags flags);
  void updateDelta_(MxValue qty, MxUInt nOrders, MxFlags flags);
  void update(MxDateTime transactTime, bool delta,
      MxValue qty, MxUInt nOrders, MxFlags flags,
      MxValue &d_qty, MxUInt &d_nOrders);

  void addOrder(MxMDOrder *order);
  void delOrder(MxUInt rank);

  template <typename Fill>
  uintptr_t match(MxDateTime transactTime,
      MxValue &qty, MxValue &cumQty, MxValue &cumValue, Fill fill) {
    auto i = this->m_orders.iterator();
    while (MxMDPxLevel_::Orders::Node *node = i.iterate()) {
      const ZmRef<MxMDOrder> &contra = node->key();
      MxValue cQty = contra->data().qty;
      if (cQty <= qty) {
	if (uintptr_t v = fill(qty, cumQty, cumValue, m_price, cQty, contra))
	  return v;
	cumQty += cQty;
	cumValue +=
	  (MxValNDP{m_price, m_pxNDP} * MxValNDP{cQty, m_qtyNDP}).value;
	deletedOrder_(contra, transactTime);
	i.del();
	--m_data.nOrders;
	m_data.qty -= cQty;
	qty -= cQty;
	if (!qty) return 0;
      } else {
	if (uintptr_t v = fill(qty, cumQty, cumValue, m_price, qty, contra))
	  return v;
	cumQty += qty;
	cumValue +=
	  (MxValNDP{m_price, m_pxNDP} * MxValNDP{qty, m_qtyNDP}).value;
	contra->updateQty_(cQty - qty);
	m_data.qty -= qty;
	qty = 0;
	return 0;
      }
    }
    return 0;
  }

  void deletedOrder_(MxMDOrder *order, MxDateTime transactTime);

  void updateNDP(MxNDP oldPxNDP, MxNDP oldQtyNDP, MxNDP pxNDP, MxNDP qtyNDP);

  MxMDOBSide		*m_obSide;
  MxNDP			m_pxNDP;
  MxNDP			m_qtyNDP;
  MxValue		m_price;
  MxMDPxLvlData	  	m_data;
  Orders		m_orders;
};

struct MxMDPxLevels_HeapID : public ZmHeapSharded {
  inline static const char *id() { return "MxMDPxLevel"; }
};

struct MxMDPxLevel_PxAccessor : public ZuAccessor<MxMDPxLevel_, MxValue> {
  ZuInline static MxValue value(const MxMDPxLevel_ &p) { return p.price(); }
};

typedef ZmRBTree<MxMDPxLevel_,
	  ZmRBTreeObject<ZmObject,
	    ZmRBTreeNodeIsKey<true,
	      ZmRBTreeIndex<MxMDPxLevel_PxAccessor,
		ZmRBTreeHeapID<MxMDPxLevels_HeapID,
		  ZmRBTreeLock<ZmNoLock> > > > > > MxMDPxLevels;

typedef MxMDPxLevels::Node MxMDPxLevel;

// event handlers (callbacks)

typedef ZmFn<MxMDLib *> MxMDLibFn;
typedef ZmFn<MxMDLib *, ZmRef<ZeEvent> > MxMDExceptionFn;
typedef ZmFn<MxMDFeed *> MxMDFeedFn;
typedef ZmFn<MxMDVenue *> MxMDVenueFn;

typedef ZmFn<MxMDTickSizeTbl *> MxMDTickSizeTblFn;
typedef ZmFn<MxMDTickSizeTbl *, const MxMDTickSize &> MxMDTickSizeFn;

// venue, segment data
typedef ZmFn<MxMDVenue *, MxMDSegment> MxMDTradingSessionFn;

typedef ZmFn<MxMDInstrument *, MxDateTime> MxMDInstrumentFn;
typedef ZmFn<MxMDOrderBook *, MxDateTime> MxMDOrderBookFn;

// order book, data
typedef ZmFn<MxMDOrderBook *, const MxMDL1Data &> MxMDLevel1Fn;
// price level, time stamp
typedef ZmFn<MxMDPxLevel *, MxDateTime> MxMDPxLevelFn;
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
  MxMDLib_Fn(MxMDVenueFn,		addVenue);
  MxMDLib_Fn(MxMDVenueFn,		refDataLoaded);
  MxMDLib_Fn(MxMDTickSizeTblFn,		addTickSizeTbl);
  MxMDLib_Fn(MxMDTickSizeTblFn,		resetTickSizeTbl);
  MxMDLib_Fn(MxMDTickSizeFn,		addTickSize);
  MxMDLib_Fn(MxMDInstrumentFn,		addInstrument);
  MxMDLib_Fn(MxMDInstrumentFn,		updatedInstrument);
  MxMDLib_Fn(MxMDOrderBookFn,		addOrderBook);
  MxMDLib_Fn(MxMDOrderBookFn,		updatedOrderBook);
  MxMDLib_Fn(MxMDOrderBookFn,		deletedOrderBook);
  MxMDLib_Fn(MxMDTradingSessionFn,	tradingSession);
  MxMDLib_Fn(MxMDTimerFn,		timer);
#undef MxMDLib_Fn
};

struct MxMDInstrHandler : public ZuObject {
#define MxMDInstrument_Fn(Type, member) \
  template <typename Arg> \
  inline MxMDInstrHandler & member##Fn(Arg &&arg) { \
    member = ZuFwd<Arg>(arg); \
    return *this; \
  } \
  Type	member
  MxMDInstrument_Fn(MxMDInstrumentFn,	updatedInstrument); // ref. data changed
  MxMDInstrument_Fn(MxMDOrderBookFn,	updatedOrderBook); // ''
  MxMDInstrument_Fn(MxMDLevel1Fn,	l1);
  MxMDInstrument_Fn(MxMDPxLevelFn,	addMktLevel);
  MxMDInstrument_Fn(MxMDPxLevelFn,	updatedMktLevel);
  MxMDInstrument_Fn(MxMDPxLevelFn,	deletedMktLevel);
  MxMDInstrument_Fn(MxMDPxLevelFn,	addPxLevel);
  MxMDInstrument_Fn(MxMDPxLevelFn,	updatedPxLevel);
  MxMDInstrument_Fn(MxMDPxLevelFn,	deletedPxLevel);
  MxMDInstrument_Fn(MxMDOrderBookFn,	l2);
  MxMDInstrument_Fn(MxMDOrderFn,	addOrder);
  MxMDInstrument_Fn(MxMDOrderFn,	modifiedOrder);
  MxMDInstrument_Fn(MxMDOrderFn,	deletedOrder);
  MxMDInstrument_Fn(MxMDTradeFn,	addTrade);
  MxMDInstrument_Fn(MxMDTradeFn,	correctedTrade);
  MxMDInstrument_Fn(MxMDTradeFn,	canceledTrade);
#undef MxMDInstrument_Fn
};

// order books

struct MxMDOBSideData {
  MxValue	nv;
  MxValue	qty;
};

class MxMDAPI MxMDOBSide : public ZmObject {
  MxMDOBSide(const MxMDOBSide &) = delete;
  MxMDOBSide &operator =(const MxMDOBSide &) = delete;

friend class MxMDOrderBook;
friend class MxMDPxLevel_;

private:
  inline MxMDOBSide(MxMDOrderBook *ob, MxEnum side) :
    m_orderBook(ob), m_side(side), m_data{0, 0} { }

public:
  ZuInline MxMDOrderBook *orderBook() const { return m_orderBook; }
  ZuInline MxEnum side() const { return m_side; }

  ZuInline const MxMDOBSideData &data() const { return m_data; }
  ZuInline MxValue vwap() const { return m_data.nv / m_data.qty; }

  ZuInline ZmRef<MxMDPxLevel> mktLevel() { return m_mktLevel; }

  // iterate over all price levels, best -> worst
  template <typename L> // (MxMDPxLevel *) -> uintptr_t
  uintptr_t allPxLevels(L l) const {
    if (m_side == MxSide::Buy) {
      auto i = m_pxLevels.readIterator<ZmRBTreeLessEqual>();
      while (const ZmRef<MxMDPxLevel> &pxLevel = i.iterate())
	if (uintptr_t v = l(pxLevel)) return v;
    } else {
      auto i = m_pxLevels.readIterator<ZmRBTreeGreaterEqual>();
      while (const ZmRef<MxMDPxLevel> &pxLevel = i.iterate())
	if (uintptr_t v = l(pxLevel)) return v;
    }
    return 0;
  }

  // iterate over price levels in range, best -> worst
  template <typename L> // (MxMDPxLevel *) -> uintptr_t
  uintptr_t pxLevels(MxValue minPrice, MxValue maxPrice, L l) {
    if (m_side == MxSide::Buy) {
      auto i = m_pxLevels.readIterator<ZmRBTreeLessEqual>(maxPrice);
      while (const ZmRef<MxMDPxLevel> &pxLevel = i.iterate()) {
	if (pxLevel->price() <= minPrice) break;
	if (uintptr_t v = l(pxLevel)) return v;
      }
    } else {
      auto i = m_pxLevels.readIterator<ZmRBTreeGreaterEqual>(minPrice);
      while (const ZmRef<MxMDPxLevel> &pxLevel = i.iterate()) {
	if (pxLevel->price() >= maxPrice) break;
	if (uintptr_t v = l(pxLevel)) return v;
      }
    }
    return 0;
  }

  // returns qty available for matching at limit price or better
  MxValue matchQty(MxValue px) {
    MxValue qty = 0;
    if (ZuUnlikely(!*px)) {
      if (m_side == MxSide::Buy) {
	auto i = m_pxLevels.readIterator<ZmRBTreeLessEqual>();
	while (const ZmRef<MxMDPxLevel> &pxLevel = i.iterate())
	  qty += pxLevel->data().qty;
      } else {
	auto i = m_pxLevels.readIterator<ZmRBTreeGreaterEqual>();
	while (const ZmRef<MxMDPxLevel> &pxLevel = i.iterate())
	  qty += pxLevel->data().qty;
      }
    } else {
      if (m_side == MxSide::Buy) {
	auto i = m_pxLevels.readIterator<ZmRBTreeLessEqual>();
	while (const ZmRef<MxMDPxLevel> &pxLevel = i.iterate()) {
	  MxValue cPx = pxLevel->price();
	  if (px > cPx) break;
	  qty += pxLevel->data().qty;
	}
      } else {
	auto i = m_pxLevels.readIterator<ZmRBTreeGreaterEqual>();
	while (const ZmRef<MxMDPxLevel> &pxLevel = i.iterate()) {
	  MxValue cPx = pxLevel->price();
	  if (px < cPx) break;
	  qty += pxLevel->data().qty;
	}
      }
    }
    return qty;
  }

private:
  template <int Direction, typename Fill, typename Limit>
  uintptr_t match(
      MxDateTime transactTime, MxValue px, MxValue qty,
      MxValue &cumQty, MxValue &cumValue,
      Fill &&fill, Limit &&limit) {
    auto i = m_pxLevels.readIterator<Direction>();
    while (const ZmRef<MxMDPxLevel> &pxLevel = i.iterate()) {
      if (!limit(px, pxLevel->price())) break;
      if (uintptr_t v = pxLevel->match(
	    transactTime, qty, cumQty, cumValue, fill))
	return v;
      if (!qty) break;
    }
    return 0;
  }

private:
  bool updateL1Bid(MxMDL1Data &l1Data, MxMDL1Data &delta);
  bool updateL1Ask(MxMDL1Data &l1Data, MxMDL1Data &delta);

  void pxLevel_(
      MxDateTime transactTime, bool delta,
      MxValue price, MxValue qty, MxUInt nOrders, MxFlags flags,
      const MxMDInstrHandler *handler,
      MxValue &d_qty, MxUInt &d_nOrders,
      const MxMDPxLevelFn *&, ZmRef<MxMDPxLevel> &);

  void addOrder_(
      MxMDOrder *order, MxDateTime transactTime,
      const MxMDInstrHandler *handler,
      const MxMDPxLevelFn *&fn, ZmRef<MxMDPxLevel> &pxLevel);
  void delOrder_(
      MxMDOrder *order, MxDateTime transactTime,
      const MxMDInstrHandler *handler,
      const MxMDPxLevelFn *&fn, ZmRef<MxMDPxLevel> &pxLevel);

  void reset(MxDateTime transactTime);

  void updateNDP(MxNDP oldPxNDP, MxNDP oldQtyNDP, MxNDP pxNDP, MxNDP qtyNDP);

  MxMDOrderBook		*m_orderBook;
  MxEnum		m_side;
  MxMDOBSideData	m_data;
  ZmRef<MxMDPxLevel>	m_mktLevel;
  MxMDPxLevels		m_pxLevels;
};

ZuInline MxEnum MxMDPxLevel_::side() const
{
  return m_obSide->side();
}

struct MxMDFeedOB : public ZmPolymorph {
  virtual ~MxMDFeedOB() { }
  virtual void subscribe(MxMDOrderBook *, MxMDInstrHandler *) { }
  virtual void unsubscribe(MxMDOrderBook *, MxMDInstrHandler *) { }
};

class MxMDAPI MxMDOrderBook : public ZmObject, public MxMDSharded {
  MxMDOrderBook(const MxMDOrderBook &) = delete;
  MxMDOrderBook &operator =(const MxMDOrderBook &) = delete;

friend class MxMDLib;
friend class MxMDShard;
friend class MxMDVenue;
friend class MxMDInstrument;
friend class MxMDOBSide;
friend class MxMDPxLevel_;

  struct KeyAccessor : public ZuAccessor<MxMDOrderBook *, MxInstrKey> {
    inline static const MxInstrKey &value(const MxMDOrderBook *ob) {
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
    MxID segment, ZuString id,
    MxMDInstrument *instrument,
    MxMDTickSizeTbl *tickSizeTbl,
    const MxMDLotSizes &lotSizes,
    MxMDInstrHandler *handler);

  MxMDOrderBook( // multi-leg
    MxMDShard *shard, 
    MxMDVenue *venue,
    MxID segment, ZuString id, MxNDP pxNDP, MxNDP qtyNDP,
    MxUInt legs, const ZmRef<MxMDInstrument> *instruments,
    const MxEnum *sides, const MxRatio *ratios,
    MxMDTickSizeTbl *tickSizeTbl,
    const MxMDLotSizes &lotSizes);

public:
  MxMDLib *md() const;

  ZuInline MxMDVenue *venue() const { return m_venue; };
  ZuInline MxMDVenueShard *venueShard() const { return m_venueShard; };

  ZuInline MxMDInstrument *instrument() const { return m_instruments[0]; }
  ZuInline MxMDInstrument *instrument(MxUInt leg) const {
    if (ZuUnlikely(!*leg)) leg = 0;
    return m_instruments[leg];
  }

  ZuInline MxMDOrderBook *out() const { return m_out; };

  ZuInline MxID venueID() const { return m_key.venue; }
  ZuInline MxID segment() const { return m_key.segment; }
  ZuInline const MxIDString &id() const { return m_key.id; };
  ZuInline const MxInstrKey &key() const { return m_key; }

  ZuInline MxUInt legs() const { return m_legs; };
  ZuInline MxEnum side(unsigned leg) const { return m_sides[leg]; }
  ZuInline MxRatio ratio(unsigned leg) const { return m_ratios[leg]; }

  ZuInline MxNDP pxNDP() const { return m_l1Data.pxNDP; }
  ZuInline MxNDP qtyNDP() const { return m_l1Data.qtyNDP; }

  ZuInline MxMDTickSizeTbl *tickSizeTbl() const { return m_tickSizeTbl; }

  ZuInline const MxMDLotSizes &lotSizes() const { return m_lotSizes; }

  ZuInline const MxMDL1Data &l1Data() const { return m_l1Data; }

  ZuInline ZmRef<MxMDOBSide> bids() const { return m_bids; };
  ZuInline ZmRef<MxMDOBSide> asks() const { return m_asks; };

  void l1(MxMDL1Data &data);
  void pxLevel(MxEnum side, MxDateTime transactTime,
      bool delta, MxValue price, MxValue qty, MxUInt nOrders,
      MxFlags flags = MxFlags());
  void l2(MxDateTime stamp, bool updateL1 = false);

  ZmRef<MxMDOrder> addOrder(
    ZuString orderID, MxDateTime transactTime,
    MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags);
  ZmRef<MxMDOrder> modifyOrder(
    ZuString orderID, MxDateTime transactTime,
    MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags);
  ZmRef<MxMDOrder> reduceOrder(
    ZuString orderID, MxDateTime transactTime,
    MxEnum side, MxValue reduceQty);
  ZmRef<MxMDOrder> cancelOrder(
    ZuString orderID, MxDateTime transactTime, MxEnum side);

  void reset(MxDateTime transactTime);

  void addTrade(ZuString tradeID,
      MxDateTime transactTime, MxValue price, MxValue qty);
  void correctTrade(ZuString tradeID,
      MxDateTime transactTime, MxValue price, MxValue qty);
  void cancelTrade(ZuString tradeID,
      MxDateTime transactTime, MxValue price, MxValue qty);

  void update(
      const MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes,
      MxDateTime transactTime);

  void subscribe(MxMDInstrHandler *);
  void unsubscribe();

  ZuInline const ZmRef<MxMDInstrHandler> &handler() const { return m_handler; }

  template <typename T = uintptr_t>
  ZuInline T libData() const { return (T)m_libData; }
  template <typename T>
  ZuInline void libData(T v) { m_libData = (uintptr_t)v; }

  template <typename T = uintptr_t>
  ZuInline T appData() const { return (T)m_appData; }
  template <typename T>
  ZuInline void appData(T v) { m_appData = (uintptr_t)v; }

  template <typename T = MxMDFeedOB>
  ZuInline typename ZuIs<MxMDFeedOB, T, ZmRef<T> &>::T feedOB() {
    ZmRef<T> *ZuMayAlias(ptr) = (ZmRef<T> *)&m_feedOB;
    return *ptr;
  }

private:
  template <int Direction, typename Fill, typename Leaves,
	   typename Limit, typename Side>
  uintptr_t match_(MxDateTime transactTime, MxValue px, MxValue qty,
      Fill &&fill, Leaves &&leaves, Limit &&limit, Side &&side) {
    MxValue cumQty = 0;
    MxValue cumValue = 0;
    auto l = [transactTime, px, qty, &cumQty, &cumValue, &fill, &limit, &side](
	     auto &recurse, MxMDOrderBook *ob) mutable -> uintptr_t {
      if (ZuLikely(!ob->m_in))
	return side()->template match<Direction>(
	    transactTime, px, qty, cumQty, cumValue, fill, limit);
      for (MxMDOrderBook *inOB = ob->m_in; inOB; inOB = inOB->m_next) {
	if (uintptr_t v = recurse(inOB)) return v;
	if (!qty) break;
      }
      return 0;
    };
    uintptr_t v = l(l, this); // recursive lambda
    leaves(qty, cumQty, cumValue);
    return v;
  }

public:
  // fill(leavesQty, cumQty, cumValue, px, qty, MxMDOrder *contra)
  //   /* fill(...) is called repeatedly for each contra order */
  // leaves(leavesQty, cumQty, cumValue) /* called on completion */

  // match order
  template <typename Fill, typename Leaves>
  uintptr_t match(
      MxDateTime transactTime, MxEnum side, MxValue px, MxValue qty,
      Fill &&fill, Leaves &&leaves) {
    if (ZuUnlikely(!*px)) {
      if (side == MxSide::Buy) {
	return match_<ZmRBTreeLessEqual>(
	    transactTime, side, px, qty, fill, leaves,
	    [](MxValue, MxValue) -> bool { return true; },
	    [](MxMDOrderBook *ob) -> MxMDOBSide * { return ob->m_asks; });
      } else {
	return match_<ZmRBTreeGreaterEqual>(
	    transactTime, side, px, qty, fill, leaves,
	    [](MxValue, MxValue) -> bool { return true; },
	    [](MxMDOrderBook *ob) -> MxMDOBSide * { return ob->m_bids; });
      }
    } else {
      if (side == MxSide::Buy) {
	return match_<ZmRBTreeLessEqual>(
	    transactTime, side, px, qty, fill, leaves,
	    [](MxValue px, MxValue cPx) -> bool { return px >= cPx; },
	    [](MxMDOrderBook *ob) -> MxMDOBSide * { return ob->m_asks; });
      } else {
	return match_<ZmRBTreeGreaterEqual>(
	    transactTime, side, px, qty, fill, leaves,
	    [](MxValue px, MxValue cPx) -> bool { return px <= cPx; },
	    [](MxMDOrderBook *ob) -> MxMDOBSide * { return ob->m_bids; });
      }
    }
  }

private:
  ZuInline static uint128_t venueSegment(MxID venue, MxID segment) {
    return (((uint128_t)venue)<<64U) | (uint128_t)segment;
  }

  void pxLevel_(
      MxEnum side, MxDateTime transactTime, bool delta,
      MxValue price, MxValue qty, MxUInt nOrders, MxFlags flags,
      MxValue *d_qty, MxUInt *d_nOrders);

  void reduceOrder_(MxMDOrder *order, MxDateTime transactTime,
      MxValue reduceQty);
  void modifyOrder_(MxMDOrder *order, MxDateTime transactTime,
      MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags);
  void cancelOrder_(MxMDOrder *order, MxDateTime transactTime);

  void addOrder_(
    MxMDOrder *order, MxDateTime transactTime,
    const MxMDInstrHandler *handler,
    const MxMDPxLevelFn *&fn, ZmRef<MxMDPxLevel> &pxLevel);
  void delOrder_(
    MxMDOrder *order, MxDateTime transactTime,
    const MxMDInstrHandler *handler,
    const MxMDPxLevelFn *&fn, ZmRef<MxMDPxLevel> &pxLevel);

  void reset_(MxMDPxLevel *pxLevel, MxDateTime transactTime);

  inline void deletedOrder_(MxMDOrder *order, MxDateTime transactTime) {
    if (m_handler) m_handler->deletedOrder(order, transactTime);
  }
  inline void deletedPxLevel_(MxMDPxLevel *pxLevel, MxDateTime transactTime) {
    if (m_handler) m_handler->deletedPxLevel(pxLevel, transactTime);
  }

  void updateNDP(MxNDP pxNDP, MxNDP qtyNDP);

  void map(unsigned inRank, MxMDOrderBook *outOB);

private:
  MxMDVenue			*m_venue;
  MxMDVenueShard		*m_venueShard;

  MxMDOrderBook			*m_in = 0;	// head of input list
  unsigned			m_rank = 0;	// rank in output list
  MxMDOrderBook			*m_next = 0;	// next in output list
  MxMDOrderBook			*m_out = 0;	// output order book

  MxInstrKey			m_key;
  MxUInt			m_legs;
  MxMDInstrument		*m_instruments[MxMDNLegs];
  MxEnum			m_sides[MxMDNLegs];
  MxRatio			m_ratios[MxMDNLegs];

  ZmRef<MxMDFeedOB>		m_feedOB;

  MxMDTickSizeTbl		*m_tickSizeTbl;
  MxMDLotSizes		 	m_lotSizes;

  MxMDL1Data			m_l1Data;

  ZmRef<MxMDOBSide>		m_bids;
  ZmRef<MxMDOBSide>		m_asks;

  ZmRef<MxMDInstrHandler>	m_handler;

  uintptr_t			m_libData = 0;
  uintptr_t			m_appData = 0;
};

ZuInline ZmRef<MxMDOBSide> MxMDOrder_::bids_(const MxMDOrderBook *ob)
{
  if (ZuLikely(ob)) return ob->bids();
  return 0;
}
ZuInline ZmRef<MxMDOBSide> MxMDOrder_::asks_(const MxMDOrderBook *ob)
{
  if (ZuLikely(ob)) return ob->asks();
  return 0;
}

ZuInline MxMDOBSide *MxMDOrder_::obSide() const
{
  return m_data.side == MxSide::Buy ?
    m_orderBook->bids() : m_orderBook->asks();
}

ZuInline MxMDOrderID2Ref MxMDOrder_::OrderID2Accessor::value(
  const MxMDOrder_ *o)
{
  return MxMDOrderID2Ref(o->orderBook()->key(), o->m_id);
}
ZuInline MxMDOrderID3Ref MxMDOrder_::OrderID3Accessor::value(
  const MxMDOrder_ *o)
{
  return MxMDOrderID3Ref(o->orderBook()->key(), o->m_data.side, o->m_id);
}

// instruments

class MxMDAPI MxMDDerivatives : public ZmObject {
  MxMDDerivatives(const MxMDDerivatives &) = delete;
  MxMDDerivatives &operator =(const MxMDDerivatives &) = delete;

friend class MxMDInstrument;

  struct Futures_HeapID {
    inline static const char *id() { return "MxMDLib.Futures"; }
  };
  typedef ZmRBTree<MxFutKey,			// mat
	    ZmRBTreeVal<MxMDInstrument *,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeHeapID<Futures_HeapID,
		  ZmRBTreeLock<ZmPLock> > > > > Futures;
  struct Options_HeapID {
    inline static const char *id() { return "MxMDLib.Options"; }
  };
  typedef ZmRBTree<MxOptKey,			// mat, putCall, strike
	    ZmRBTreeVal<MxMDInstrument *,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeHeapID<Options_HeapID,
		  ZmRBTreeLock<ZmPLock> > > > > Options;

  ZuInline MxMDDerivatives() { }

public:
  ZuInline MxMDInstrument *future(const MxFutKey &key) const
    { return m_futures.findVal(key); }
  uintptr_t allFutures(ZmFn<MxMDInstrument *>) const;

  ZuInline MxMDInstrument *option(const MxOptKey &key) const
    { return m_options.findVal(key); }
  uintptr_t allOptions(ZmFn<MxMDInstrument *>) const;

private:
  void add(MxMDInstrument *);
  void del(MxMDInstrument *);

  Futures		m_futures;
  Options		m_options;
};

class MxMDAPI MxMDInstrument : public ZmObject, public MxMDSharded {
  MxMDInstrument(const MxMDInstrument &) = delete;
  MxMDInstrument &operator =(const MxMDInstrument &) = delete;

friend class MxMDLib;
friend class MxMDShard;

  struct KeyAccessor : public ZuAccessor<MxMDInstrument *, MxInstrKey> {
    ZuInline static const MxInstrKey &value(const MxMDInstrument *instrument) {
      return instrument->key();
    }
  };

  struct OrderBooks_HeapID : public ZmHeapSharded {
    inline static const char *id() { return "MxMDInstrument.OrderBooks"; }
  };
  typedef ZmRBTree<ZmRef<MxMDOrderBook>,
	    ZmRBTreeIndex<MxMDOrderBook::VenueSegmentAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock,
		  ZmRBTreeHeapID<OrderBooks_HeapID> > > > > OrderBooks;

  MxMDInstrument(MxMDShard *shard, const MxInstrKey &key,
      const MxMDInstrRefData &refData);

public:
  MxMDLib *md() const;

  ZuInline MxID primaryVenue() const { return m_key.venue; }
  ZuInline MxID primarySegment() const { return m_key.segment; }
  ZuInline const MxIDString &id() const { return m_key.id; }
  ZuInline const MxInstrKey &key() const { return m_key; }

  ZuInline const MxMDInstrRefData &refData() const { return m_refData; }

  ZuInline MxMDInstrument *underlying() const { return m_underlying; }
  ZuInline MxMDDerivatives *derivatives() const { return m_derivatives; }

  void update(const MxMDInstrRefData &refData, MxDateTime transactTime);

  void subscribe(MxMDInstrHandler *);
  void unsubscribe();

  ZuInline const ZmRef<MxMDInstrHandler> &handler() const { return m_handler; }

  ZuInline ZmRef<MxMDOrderBook> orderBook(MxID venue) const {
    if (ZmRef<MxMDOrderBook> ob =
	m_orderBooks.readIterator<ZmRBTreeGreaterEqual>(
	  MxMDOrderBook::venueSegment(venue, MxID())).iterateKey())
      if (ob && ob->venueID() == venue) return ob;
    return (MxMDOrderBook *)nullptr;
  }
  ZuInline ZmRef<MxMDOrderBook> orderBook(MxID venue, MxID segment) const {
    if (ZmRef<MxMDOrderBook> ob = m_orderBooks.findKey(
	  MxMDOrderBook::venueSegment(venue, segment)))
      return ob;
    return (MxMDOrderBook *)nullptr;
  }
  template <typename L> // (MxMDOrderBook *) -> uintptr_t
  uintptr_t allOrderBooks(L l) const {
    auto i = m_orderBooks.readIterator();
    while (const ZmRef<MxMDOrderBook> &ob = i.iterateKey())
      if (uintptr_t v = l(ob)) return v;
    return 0;
  }

  ZmRef<MxMDOrderBook> addOrderBook(const MxInstrKey &key,
    MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes,
    MxDateTime transactTime);
  void delOrderBook(MxID venue, MxID segment, MxDateTime transactTime);

private:
  template <typename L>
  inline L keys_(L l) const {
    l(MxUniKey{.id = m_key.id, .venue = m_key.venue, .segment = m_key.segment});
    if (*m_refData.idSrc)
      l(MxUniKey{.id = m_refData.symbol, .src = m_refData.idSrc});
    if (*m_refData.altIDSrc)
      l(MxUniKey{.id = m_refData.altSymbol, .src = m_refData.altIDSrc});
    return l;
  }
public:
  template <typename L> // (MxUniKey)
  inline void keys(L l_) const {
    auto l = keys_(ZuMv(l_));
    if (m_underlying && *m_refData.mat) {
      if (*m_refData.strike)
	m_underlying->keys_([l = ZuMv(l),
	    refData = &m_refData](MxUniKey key) mutable {
	  key.mat = refData->mat;
	  key.putCall = refData->putCall;
	  key.strike = refData->strike;
	  l(key);
	});
      else
	m_underlying->keys_([l = ZuMv(l),
	    refData = &m_refData](MxUniKey key) mutable {
	  key.mat = refData->mat;
	  l(key);
	});
    }
  }

  template <typename T = uintptr_t>
  ZuInline T libData() const { return (T)m_libData; }
  template <typename T>
  ZuInline void libData(T v) { m_libData = (uintptr_t)v; }

  template <typename T = uintptr_t>
  ZuInline T appData() const { return (T)m_appData; }
  template <typename T>
  ZuInline void appData(T v) { m_appData = (uintptr_t)v; }

private:
  void addOrderBook_(MxMDOrderBook *);
  ZmRef<MxMDOrderBook> findOrderBook_(MxID venue, MxID segment);
  ZmRef<MxMDOrderBook> delOrderBook_(MxID venue, MxID segment);

  ZuInline void underlying(MxMDInstrument *u) { m_underlying = u; }
  ZuInline void addDerivative(MxMDInstrument *d) {
    if (!m_derivatives) m_derivatives = new MxMDDerivatives();
    m_derivatives->add(d);
  }
  ZuInline void delDerivative(MxMDInstrument *d) {
    if (!m_derivatives) return;
    m_derivatives->del(d);
  }

  void update_(const MxMDInstrRefData &, MxDateTime transactTime);

  MxInstrKey			m_key;

  MxMDInstrRefData		m_refData;

  MxMDInstrument		*m_underlying = 0;
  ZmRef<MxMDDerivatives>	m_derivatives;

  OrderBooks			m_orderBooks;

  ZmRef<MxMDInstrHandler>  	m_handler;

  uintptr_t			m_libData = 0;
  uintptr_t			m_appData = 0;
};

// feeds

class MxMDAPI MxMDFeed : public ZmPolymorph {
  MxMDFeed(const MxMDFeed &) = delete;
  MxMDFeed &operator =(const MxMDFeed &) = delete;

friend class MxMDCore;
friend class MxMDLib;

public:
  MxMDFeed(MxMDLib *md, MxID id, unsigned level);

  virtual ~MxMDFeed() { }

  struct IDAccessor : public ZuAccessor<MxMDFeed *, MxID> {
    inline static MxID value(const MxMDFeed *f) { return f->id(); }
  };

  ZuInline MxMDLib *md() const { return m_md; }
  ZuInline MxID id() const { return m_id; }
  ZuInline unsigned level() const { return m_level; }

  void connected();
  void disconnected();

protected:
  virtual void start();
  virtual void stop();
  virtual void final();

  virtual void addOrderBook(MxMDOrderBook *, MxDateTime transactTime);
  virtual void delOrderBook(MxMDOrderBook *, MxDateTime transactTime);

private:
  MxMDLib	*m_md;
  MxID		m_id;
  uint8_t	m_level;	// 1, 2 or 3
};

// venues

class MxMDAPI MxMDVenueShard : public ZuObject {
friend class MxMDVenue;
friend class MxMDOrderBook;
friend class MxMDOBSide;
friend class MxMDPxLevel_;

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
  ZuInline MxMDLib *md() const;
  ZuInline MxMDVenue *venue() const { return m_venue; }
  ZuInline MxMDShard *shard() const { return m_shard; }
  unsigned id() const;
  ZuInline MxEnum orderIDScope() const { return m_orderIDScope; }

  ZmRef<MxMDOrderBook> addCombination(
      MxID segment, ZuString orderBookID,
      MxNDP pxNDP, MxNDP qtyNDP,
      MxUInt legs, const ZmRef<MxMDInstrument> *instruments,
      const MxEnum *sides, const MxRatio *ratios,
      MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes,
      MxDateTime transactTime);
  void delCombination(MxID segment, ZuString orderBookID,
      MxDateTime transactTime);

private:
  void addOrder(MxMDOrder *order);
  template <typename OrderID>
  ZmRef<MxMDOrder> findOrder(
      const MxInstrKey &obKey, MxEnum side, OrderID &&orderID);
  template <typename OrderID>
  ZmRef<MxMDOrder> delOrder(
      const MxInstrKey &obKey, MxEnum side, OrderID &&orderID);

  inline void addOrder2(MxMDOrder *order) { m_orders2->add(order); }
  template <typename OrderID> ZuInline ZmRef<MxMDOrder> findOrder2(
      const MxInstrKey &obKey, OrderID &&orderID) {
    return m_orders2->findKey(ZuMkTuple(obKey, ZuFwd<OrderID>(orderID)));
  }
  template <typename OrderID> ZuInline ZmRef<MxMDOrder> delOrder2(
      const MxInstrKey &obKey, OrderID &&orderID) {
    return m_orders2->delKey(ZuMkTuple(obKey, ZuFwd<OrderID>(orderID)));
  }

  inline void addOrder3(MxMDOrder *order) { m_orders3->add(order); }
  template <typename OrderID> ZuInline ZmRef<MxMDOrder> findOrder3(
      const MxInstrKey &obKey, MxEnum side, OrderID &&orderID) {
    return m_orders3->findKey(ZuMkTuple(obKey, side, ZuFwd<OrderID>(orderID)));
  }
  template <typename OrderID> ZuInline ZmRef<MxMDOrder> delOrder3(
      const MxInstrKey &obKey, MxEnum side, OrderID &&orderID) {
    return m_orders3->delKey(ZuMkTuple(obKey, side, ZuFwd<OrderID>(orderID)));
  }

  MxMDVenue		*m_venue;
  MxMDShard		*m_shard;
  MxEnum		m_orderIDScope;
  ZmRef<Orders2>	m_orders2;
  ZmRef<Orders3>	m_orders3;
};

class MxMDAPI MxMDVenue : public ZmObject {
  MxMDVenue(const MxMDVenue &) = delete;
  MxMDVenue &operator =(const MxMDVenue &) = delete;

friend class MxMDLib;
friend class MxMDVenueShard;
friend class MxMDOrderBook;

  typedef ZtArray<ZuRef<MxMDVenueShard> > Shards;

  struct TickSizeTbls_HeapID {
    inline static const char *id() { return "MxMDVenue.TickSizeTbls"; }
  };
  typedef ZmRBTree<ZmRef<MxMDTickSizeTbl>,
	    ZmRBTreeIndex<MxMDTickSizeTbl::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmPRWLock,
		  ZmRBTreeHeapID<TickSizeTbls_HeapID> > > > > TickSizeTbls;
  struct Segment_IDAccessor : public ZuAccessor<MxMDSegment, MxID> {
    inline static MxID value(const MxMDSegment &s) { return s.id; }
  };
  struct Segments_ID {
    inline static const char *id() { return "MxMDVenue.Segments"; }
  };
  typedef ZmHash<MxMDSegment,
	    ZmHashIndex<Segment_IDAccessor,
	      ZmHashObject<ZuNull,
		ZmHashLock<ZmNoLock,
		  ZmHashID<Segments_ID> > > > > Segments;
  typedef ZmPLock SegmentsLock;
  typedef ZmGuard<SegmentsLock> SegmentsGuard;
  typedef ZmReadGuard<SegmentsLock> SegmentsReadGuard;

  typedef ZmHash<ZmRef<MxMDOrder>,
	    ZmHashIndex<MxMDOrder::OrderIDAccessor,
	      ZmHashObject<ZuNull,
		ZmHashLock<ZmPLock,
		  ZmHashHeapID<MxMDOrders_HeapID> > > > > Orders;

public:
  MxMDVenue(MxMDLib *md, MxMDFeed *feed, MxID id,
      MxEnum orderIDScope = MxMDOrderIDScope::Default,
      MxFlags flags = 0);
  virtual ~MxMDVenue() { }

  struct IDAccessor : public ZuAccessor<MxMDVenue *, MxID> {
    inline static MxID value(const MxMDVenue *v) { return v->id(); }
  };

  ZuInline MxMDLib *md() const { return m_md; }
  ZuInline MxMDFeed *feed() const { return m_feed; }
  ZuInline MxID id() const { return m_id; }
  ZuInline MxEnum orderIDScope() const { return m_orderIDScope; }
  ZuInline MxFlags flags() const { return m_flags; }

  MxMDVenueShard *shard(const MxMDShard *shard) const;

private:
  ZuInline void loaded_(bool b) { m_loaded = b; }
public:
  ZuInline bool loaded() const { return m_loaded; }

  ZmRef<MxMDTickSizeTbl> addTickSizeTbl(ZuString id, MxNDP pxNDP);
  template <typename ID>
  ZuInline ZmRef<MxMDTickSizeTbl> tickSizeTbl(const ID &id) const {
    return m_tickSizeTbls.findKey(id);
  }
  uintptr_t allTickSizeTbls(ZmFn<MxMDTickSizeTbl *>) const;

  uintptr_t allSegments(ZmFn<const MxMDSegment &>) const;

  inline MxMDSegment tradingSession(MxID segmentID = MxID()) const {
    SegmentsReadGuard guard(m_segmentsLock);
    Segments::Node *node = m_segments->find(segmentID);
    if (!node) return MxMDSegment();
    return node->key();
  }
  void tradingSession(MxMDSegment segment);

  void modifyOrder(ZuString orderID, MxDateTime transactTime,
      MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags,
      ZmFn<MxMDOrder *> fn = ZmFn<MxMDOrder *>());
  void reduceOrder(ZuString orderID,
      MxDateTime transactTime, MxValue reduceQty,
      ZmFn<MxMDOrder *> fn = ZmFn<MxMDOrder *>()); // qty -= reduceQty
  void cancelOrder(ZuString orderID, MxDateTime transactTime,
      ZmFn<MxMDOrder *> fn = ZmFn<MxMDOrder *>());

private:
  ZmRef<MxMDTickSizeTbl> findTickSizeTbl_(ZuString id);
  ZmRef<MxMDTickSizeTbl> addTickSizeTbl_(ZuString id, MxNDP pxNDP);

  inline void tradingSession_(const MxMDSegment &segment) {
    SegmentsGuard guard(m_segmentsLock);
    Segments::Node *node = m_segments->find(segment.id);
    if (!node) {
      m_segments->add(segment);
      return;
    }
    node->key() = segment;
  }

  ZuInline void addOrder(MxMDOrder *order) { m_orders->add(order); }
  template <typename OrderID>
  ZuInline ZmRef<MxMDOrder> findOrder(OrderID &&orderID) {
    return m_orders->findKey(ZuFwd<OrderID>(orderID));
  }
  template <typename OrderID>
  ZuInline ZmRef<MxMDOrder> delOrder(OrderID &&orderID) {
    return m_orders->delKey(ZuFwd<OrderID>(orderID));
  }

  ZuInline MxMDVenueShard *shard_(unsigned i) const { return m_shards[i]; }

  MxMDLib		*m_md;
  MxMDFeed		*m_feed;
  MxID			m_id;
  MxEnum		m_orderIDScope;
  MxFlags		m_flags;
  Shards		m_shards;

  TickSizeTbls	 	m_tickSizeTbls;
  SegmentsLock		m_segmentsLock;
    ZmRef<Segments>	  m_segments;
  ZmAtomic<unsigned>	m_loaded = 0;

  ZmRef<Orders>		m_orders;
};

inline void MxMDVenueShard::addOrder(MxMDOrder *order)
{
  switch ((int)m_orderIDScope) {
    case MxMDOrderIDScope::Venue: m_venue->addOrder(order); break;
    case MxMDOrderIDScope::OrderBook: addOrder2(order); break;
    case MxMDOrderIDScope::OBSide: addOrder3(order); break;
  }
}
template <typename OrderID>
inline ZmRef<MxMDOrder> MxMDVenueShard::findOrder(
    const MxInstrKey &obKey, MxEnum side, OrderID &&orderID)
{
  switch ((int)m_orderIDScope) {
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
    const MxInstrKey &obKey, MxEnum side, OrderID &&orderID)
{
  switch ((int)m_orderIDScope) {
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

class MxMDAPI MxMDShard : public ZuObject, public ZmShard {
friend class MxMDLib;

  struct Instruments_HeapID : public ZmHeapSharded {
    inline static const char *id() { return "MxMDShard.Instruments"; }
  };
  typedef ZmHash<MxMDInstrument *,
	    ZmHashIndex<MxMDInstrument::KeyAccessor,
	      ZmHashObject<ZuObject,
		ZmHashLock<ZmNoLock,
		  ZmHashHeapID<Instruments_HeapID> > > > > Instruments;

  struct OrderBooks_HeapID : public ZmHeapSharded {
    inline static const char *id() { return "MxMDShard.OrderBooks"; }
  };
  typedef ZmHash<MxMDOrderBook *,
	    ZmHashIndex<MxMDOrderBook::KeyAccessor,
	      ZmHashObject<ZuObject,
		ZmHashLock<ZmNoLock,
		  ZmHashHeapID<OrderBooks_HeapID> > > > > OrderBooks;

  MxMDShard(MxMDLib *md, ZmScheduler *sched, unsigned id, unsigned tid) :
      ZmShard(sched, tid), m_md(md), m_id(id) {
    m_instruments = new Instruments();
    m_orderBooks = new OrderBooks();
  }

public:
  ZuInline MxMDLib *md() const { return m_md; }
  ZuInline unsigned id() const { return m_id; }

  ZuInline ZmRef<MxMDInstrument> instrument(const MxInstrKey &key) const {
    return m_instruments->findKey(key);
  }
  uintptr_t allInstruments(ZmFn<MxMDInstrument *>) const;
  ZmRef<MxMDInstrument> addInstrument(ZmRef<MxMDInstrument> instr,
      const MxInstrKey &key, const MxMDInstrRefData &refData,
      MxDateTime transactTime);

  ZuInline ZmRef<MxMDOrderBook> orderBook(const MxInstrKey &key) const {
    return m_orderBooks->findKey(key);
  }
  uintptr_t allOrderBooks(ZmFn<MxMDOrderBook *>) const;

private:
  ZuInline void addInstrument(MxMDInstrument *instrument) {
    m_instruments->add(instrument);
  }
  ZuInline void delInstrument(MxMDInstrument *instrument) {
    m_instruments->del(instrument->key());
  }
  ZuInline void addOrderBook(MxMDOrderBook *ob) {
    m_orderBooks->add(ob);
  }
  ZuInline void delOrderBook(MxMDOrderBook *ob) {
    m_orderBooks->del(ob->key());
  }

  MxMDLib		*m_md = nullptr;
  unsigned		m_id;
  ZmRef<Instruments>	m_instruments;
  ZmRef<OrderBooks>	m_orderBooks;
};

ZuInline MxMDLib *MxMDOrderBook::md() const { return shard()->md(); }

ZuInline MxMDLib *MxMDInstrument::md() const { return shard()->md(); }

ZuInline unsigned MxMDVenueShard::id() const { return m_shard->id(); }

typedef ZmHandle<MxMDInstrument> MxMDInstrHandle;
typedef ZmHandle<MxMDOrderBook> MxMDOBHandle;

// library

class MxMDLib_JNI;
class MxMDAPI MxMDLib {
  MxMDLib(const MxMDLib &) = delete;
  MxMDLib &operator =(const MxMDLib &) = delete;

friend class MxMDCmd;
friend class MxMDTickSizeTbl;
friend class MxMDOrderBook;
friend class MxMDInstrument;
friend class MxMDFeed;
friend class MxMDVenue;
friend class MxMDVenueShard;
friend class MxMDShard;

friend class MxMDLib_JNI;

protected:
  MxMDLib(ZmScheduler *);

  void init_(void *);

  static MxMDLib *init(ZuString cf, ZmFn<ZmScheduler *> schedInitFn);

public:
  virtual ~MxMDLib() { }

  static MxMDLib *instance();

  inline static MxMDLib *init(ZuString cf) {
    return init(cf, ZmFn<ZmScheduler *>());
  }

  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void final() = 0;

  virtual bool record(ZuString path) = 0;
  virtual ZtString stopRecording() = 0;

  virtual bool replay(ZuString path,
      MxDateTime begin = MxDateTime(),
      bool filter = true) = 0;
  virtual ZtString stopReplaying() = 0;

  virtual void startTimer(MxDateTime begin = MxDateTime()) = 0;
  virtual void stopTimer() = 0;

  virtual void dumpTickSizes(ZuString path, MxID venue = MxID()) = 0;
  virtual void dumpInstruments(
      ZuString path, MxID venue = MxID(), MxID segment = MxID()) = 0;
  virtual void dumpOrderBooks(
      ZuString path, MxID venue = MxID(), MxID segment = MxID()) = 0;

  ZuInline unsigned nShards() const { return m_shards.length(); }
  template <typename L>
  ZuInline typename ZuNotMutable<L>::T shard(unsigned i, L l) const {
    MxMDShard *shard = m_shards[i];
    m_scheduler->invoke(shard->tid(), [l = ZuMv(l), shard]() { l(shard); });
  }
  template <typename L>
  ZuInline typename ZuIsMutable<L>::T shard(unsigned i, L l) const {
    MxMDShard *shard = m_shards[i];
    m_scheduler->invoke(shard->tid(),
	[l = ZuMv(l), shard]() mutable { l(shard); });
  }
  template <typename ...Args>
  ZuInline void shardRun(unsigned i, Args &&... args)
    { m_shards[i]->run(ZuFwd<Args>(args)...); }
  template <typename ...Args>
  ZuInline void shardInvoke(unsigned i, Args &&... args)
    { m_shards[i]->invoke(ZuFwd<Args>(args)...); }

  void sync(); // synchronize with all shards

  void raise(ZmRef<ZeEvent> e);

  void addFeed(MxMDFeed *feed);
  void addVenue(MxMDVenue *venue);

  void addVenueMapping(MxMDVenueMapKey key, MxMDVenueMapping map);
  MxMDVenueMapping venueMapping(MxMDVenueMapKey key);

  void loaded(MxMDVenue *venue);

  void subscribe(MxMDLibHandler *);
  void unsubscribe();

  ZuInline ZmRef<MxMDLibHandler> handler() {
    SubGuard guard(m_subLock);
    return m_handler;
  }

  // commands

  // add command
  virtual void addCmd(ZuString name, ZuString syntax,
      ZvCmdFn fn, ZtString brief, ZtString usage) = 0;

  // single instrument / order book lookup
  static ZtString lookupSyntax();
  static ZtString lookupOptions();

  MxUniKey parseInstrument(ZvCf *args, unsigned index) const;
  bool lookupInstrument(
      const MxUniKey &key, bool instrRequired, ZmFn<MxMDInstrument *> fn) const;
  MxUniKey parseOrderBook(ZvCf *args, unsigned index) const;
  bool lookupOrderBook(
      const MxUniKey &key, bool instrRequired, bool obRequired,
      ZmFn<MxMDInstrument *, MxMDOrderBook *> fn) const;

  // CLI time format (using local timezone)
  typedef ZuBoxFmt<ZuBox<unsigned>, ZuFmt::Right<6> > TimeFmt;
  inline TimeFmt timeFmt(MxDateTime t) {
    return ZuBox<unsigned>((t + m_tzOffset).hhmmss()).fmt(ZuFmt::Right<6>());
  }

private:
  typedef ZmPRWLock RWLock;
  typedef ZmGuard<RWLock> Guard;
  typedef ZmReadGuard<RWLock> ReadGuard;

  typedef ZmPLock SubLock;
  typedef ZmGuard<SubLock> SubGuard;

friend class ZmShard;
  template <typename ...Args>
  ZuInline void run(unsigned tid, Args &&... args)
    { m_scheduler->run(tid, ZuFwd<Args>(args)...); }
  template <typename ...Args>
  ZuInline void invoke(unsigned tid, Args &&... args)
    { m_scheduler->invoke(tid, ZuFwd<Args>(args)...); }

  typedef ZtArray<ZuRef<MxMDShard> > Shards;

  ZuInline MxMDShard *shard_(unsigned i) const { return m_shards[i]; }

  ZmRef<MxMDInstrument> addInstrument(
      MxMDShard *shard, ZmRef<MxMDInstrument> instr,
      const MxInstrKey &key, const MxMDInstrRefData &refData,
      MxDateTime transactTime);

  ZmRef<MxMDTickSizeTbl> addTickSizeTbl(
      MxMDVenue *venue, ZuString id, MxNDP pxNDP);
  void resetTickSizeTbl(MxMDTickSizeTbl *tbl);
  void addTickSize(MxMDTickSizeTbl *tbl,
      MxValue minPrice, MxValue maxPrice, MxValue tickSize);

  void updateInstrument(
      MxMDInstrument *instrument, const MxMDInstrRefData &refData,
      MxDateTime transactTime);
  void addInstrIndices(
      MxMDInstrument *, const MxMDInstrRefData &, MxDateTime transactTime);
  void delInstrIndices(
      MxMDInstrument *, const MxMDInstrRefData &);

  ZmRef<MxMDOrderBook> addOrderBook(
      MxMDInstrument *instrument, MxInstrKey key,
      MxMDTickSizeTbl *tickSizeTbl, MxMDLotSizes lotSizes,
      MxDateTime transactTime);
      
  ZmRef<MxMDOrderBook> addCombination(
      MxMDVenueShard *venueShard, MxID segment, ZuString id,
      MxNDP pxNDP, MxNDP qtyNDP,
      MxUInt legs, const ZmRef<MxMDInstrument> *instruments,
      const MxEnum *sides, const MxRatio *ratios,
      MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes,
      MxDateTime transactTime);

  void updateOrderBook(MxMDOrderBook *ob,
      const MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes,
      MxDateTime transactTime);

  void delOrderBook(MxMDInstrument *instrument, MxID venue, MxID segment,
      MxDateTime transactTime);
  void delCombination(MxMDVenueShard *venueShard, MxID segment, ZuString id,
      MxDateTime transactTime);

  void tradingSession(MxMDVenue *venue, MxMDSegment segment);

  void l1(const MxMDOrderBook *ob, const MxMDL1Data &l1Data);

  void pxLevel(const MxMDOrderBook *ob, MxEnum side, MxDateTime transactTime,
      bool delta, MxValue price, MxValue qty, MxUInt nOrders, MxFlags flags);

  void l2(const MxMDOrderBook *ob, MxDateTime stamp, bool updateL1);

  void addOrder(const MxMDOrderBook *ob,
      ZuString orderID, MxDateTime transactTime,
      MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags);
  void modifyOrder(const MxMDOrderBook *ob,
      ZuString orderID, MxDateTime transactTime,
      MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags);
  void cancelOrder(const MxMDOrderBook *ob,
      ZuString orderID, MxDateTime transactTime, MxEnum side);

  void resetOB(const MxMDOrderBook *ob, MxDateTime transactTime);

  void addTrade(const MxMDOrderBook *ob, ZuString tradeID,
      MxDateTime transactTime, MxValue price, MxValue qty);
  void correctTrade(const MxMDOrderBook *ob, ZuString tradeID,
      MxDateTime transactTime, MxValue price, MxValue qty);
  void cancelTrade(const MxMDOrderBook *ob, ZuString tradeID,
      MxDateTime transactTime, MxValue price, MxValue qty);

  // primary indices

  struct AllInstruments_HeapID {
    inline static const char *id() { return "MxMDLib.AllInstruments"; }
  };
  typedef ZmHash<ZmRef<MxMDInstrument>,
	    ZmHashIndex<MxMDInstrument::KeyAccessor,
	      ZmHashObject<ZuNull,
		ZmHashLock<ZmPLock,
		  ZmHashHeapID<AllInstruments_HeapID> > > > > AllInstruments;

  struct AllOrderBooks_HeapID {
    inline static const char *id() { return "MxMDLib.AllOrderBooks"; }
  };
  typedef ZmHash<ZmRef<MxMDOrderBook>,
	    ZmHashIndex<MxMDOrderBook::KeyAccessor,
	      ZmHashObject<ZuNull,
		ZmHashLock<ZmPLock,
		  ZmHashHeapID<AllOrderBooks_HeapID> > > > > AllOrderBooks;

  // secondary indices

  struct Instruments_HeapID {
    inline static const char *id() { return "MxMDLib.Instruments"; }
  };
  typedef ZmHash<MxSymKey,
	    ZmHashVal<MxMDInstrument *,
	      ZmHashObject<ZuNull,
		ZmHashLock<ZmPLock,
		  ZmHashHeapID<Instruments_HeapID> > > > > Instruments;

  // feeds, venues

  struct Feeds_HeapID {
    inline static const char *id() { return "MxMDLib.Feeds"; }
  };
  typedef ZmRBTree<ZmRef<MxMDFeed>,
	    ZmRBTreeIndex<MxMDFeed::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmPLock,
		  ZmRBTreeHeapID<Feeds_HeapID> > > > > Feeds;

  struct Venues_HeapID {
    inline static const char *id() { return "MxMDLib.Venues"; }
  };
  typedef ZmRBTree<ZmRef<MxMDVenue>,
	    ZmRBTreeIndex<MxMDVenue::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmPLock,
		  ZmRBTreeHeapID<Venues_HeapID> > > > > Venues;

  struct VenueMap_HeapID {
    inline static const char *id() { return "MxMDLib.VenueMap"; }
  };
  typedef ZmRBTree<MxMDVenueMapKey,
	    ZmRBTreeVal<MxMDVenueMapping,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmPLock,
		  ZmRBTreeHeapID<VenueMap_HeapID> > > > > VenueMap;

public:
  ZuInline MxMDInstrHandle instrument(const MxInstrKey &key) const {
    if (ZmRef<MxMDInstrument> instr = m_allInstruments->findKey(key))
      return MxMDInstrHandle{ZuMv(instr)};
    return MxMDInstrHandle{};
  }
private:
  ZuInline ZmRef<MxMDInstrument> instrument_(const MxInstrKey &key) const {
    return m_allInstruments->findKey(key);
  }
public:
  ZuInline MxMDInstrHandle instrument(const MxSymKey &key) const {
    if (ZmRef<MxMDInstrument> instr = m_instruments->findVal(key))
      return MxMDInstrHandle{ZuMv(instr)};
    return MxMDInstrHandle{};
  }
private:
  ZuInline ZmRef<MxMDInstrument> instrument_(const MxUniKey &key) const {
    ZmRef<MxMDInstrument> instr;
    if (*key.src)
      instr = m_instruments->findVal(MxSymKey{key.id, key.src});
    else
      instr = m_allInstruments->findKey(
	  MxInstrKey{key.id, key.venue, key.segment});
    if (instr && *key.mat && instr->derivatives()) {
      if (*key.strike)
	instr = instr->derivatives()->option(
	    MxOptKey{key.mat, key.putCall, key.strike});
      else
	instr = instr->derivatives()->future(MxFutKey{key.mat});
    }
    return ZuMv(instr);
  }
public:
  ZuInline MxMDInstrHandle instrument(const MxUniKey &key) const {
    ZmRef<MxMDInstrument> instr = instrument_(key);
    if (instr) return MxMDInstrHandle{ZuMv(instr)};
    return MxMDInstrHandle{};
  }
  ZuInline MxMDInstrHandle instrument(
      const MxInstrKey &key, unsigned shardID) const {
    if (ZmRef<MxMDInstrument> instr = m_allInstruments->findKey(key))
      return MxMDInstrHandle{ZuMv(instr)};
    return MxMDInstrHandle{m_shards[shardID % m_shards.length()]};
  }
  template <typename L> ZuInline void instrInvoke(
      const MxInstrKey &key, L l) const {
    if (ZmRef<MxMDInstrument> instr = m_allInstruments->findKey(key))
      instr->shard()->invoke(
	  [l = ZuMv(l), instr = ZuMv(instr)]() mutable { l(instr); });
    else
      l((MxMDInstrument *)nullptr);
  }
  template <typename L> ZuInline void instrInvoke(
      const MxSymKey &key, L l) const {
    if (ZmRef<MxMDInstrument> instr = m_instruments->findVal(key))
      instr->shard()->invoke(
	  [l = ZuMv(l), instr = ZuMv(instr)]() mutable { l(instr); });
    else
      l((MxMDInstrument *)nullptr);
  }
  template <typename L>
  ZuInline void instrInvoke(const MxUniKey &key, L l) const {
    if (*key.mat) {
      auto l_ = [key = key, l = ZuMv(l)](MxMDInstrument *instr) mutable {
	if (ZuLikely(instr && instr->derivatives())) {
	  if (*key.strike)
	    instr = instr->derivatives()->option(
		MxOptKey{key.mat, key.putCall, key.strike});
	  else
	    instr = instr->derivatives()->future(MxFutKey{key.mat});
	}
	l(instr);
      };
      if (*key.src)
	instrInvoke(MxSymKey{key.id, key.src}, ZuMv(l_));
      else
	instrInvoke(MxInstrKey{key.id, key.venue, key.segment}, ZuMv(l_));
    } else {
      if (*key.src)
	instrInvoke(MxSymKey{key.id, key.src}, ZuMv(l));
      else
	instrInvoke(MxInstrKey{key.id, key.venue, key.segment}, ZuMv(l));
    }
  }
  uintptr_t allInstruments(ZmFn<MxMDInstrument *>) const;

  ZuInline MxMDOBHandle orderBook(const MxInstrKey &key) const {
    if (ZmRef<MxMDOrderBook> ob = m_allOrderBooks->findKey(key))
      return MxMDOBHandle{ob};
    return MxMDOBHandle{};
  }
private:
  ZuInline ZmRef<MxMDOrderBook> orderBook_(const MxInstrKey &key) const {
    return m_allOrderBooks->findKey(key);
  }
public:
  ZuInline MxMDOBHandle orderBook(const MxUniKey &key) const {
    ZmRef<MxMDInstrument> instr = instrument_(key);
    if (!instr) return MxMDOBHandle{};
    if (ZmRef<MxMDOrderBook> ob = instr->orderBook(key.venue, key.segment))
      return MxMDOBHandle{ob};
    return MxMDOBHandle{};
  }
  ZuInline MxMDOBHandle orderBook(
      const MxInstrKey &key, unsigned shardID) const {
    if (ZmRef<MxMDOrderBook> ob = m_allOrderBooks->findKey(key))
      return MxMDOBHandle{ob};
    return MxMDOBHandle{m_shards[shardID % m_shards.length()]};
  }
  template <typename L>
  ZuInline void obInvoke(const MxInstrKey &key, L l) const {
    if (ZmRef<MxMDOrderBook> ob = m_allOrderBooks->findKey(key))
      ob->shard()->invoke([l = ZuMv(l), ob = ZuMv(ob)]() mutable { l(ob); });
    else
      l((MxMDOrderBook *)nullptr);
  }
  template <typename L>
  ZuInline void obInvoke(const MxUniKey &key, L l) const {
    instrInvoke(key, [key = key, l = ZuMv(l)](MxMDInstrument *instr) mutable {
	if (!instr)
	  l((MxMDOrderBook *)nullptr);
	else
	  l(instr->orderBook(key.venue, key.segment));
      });
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

  template <typename T = uintptr_t>
  ZuInline T libData() const { return (T)m_libData; }
  template <typename T>
  ZuInline void libData(T v) { m_libData = (uintptr_t)v; }

  template <typename T = uintptr_t>
  ZuInline T appData() const { return (T)m_appData; }
  template <typename T>
  ZuInline void appData(T v) { m_appData = (uintptr_t)v; }

private:
  ZmScheduler		*m_scheduler = 0;
  Shards		m_shards;

  ZmRef<AllInstruments>	m_allInstruments;
  ZmRef<AllOrderBooks>	m_allOrderBooks;
  ZmRef<Instruments>	m_instruments;
  Feeds			m_feeds;
  Venues		m_venues;
  VenueMap		m_venueMap;

  RWLock		m_refDataLock;	// serializes updates to containers

  int			m_tzOffset = 0;

  SubLock		m_subLock;
    ZmRef<MxMDLibHandler> m_handler;

  uintptr_t		m_libData = 0;
  uintptr_t		m_appData = 0;
};

inline void MxMDFeed::connected() {
  m_md->handler()->connected(this);
}
inline void MxMDFeed::disconnected() {
  m_md->handler()->disconnected(this);
}

ZuInline MxMDVenueShard *MxMDVenue::shard(const MxMDShard *shard) const {
  return m_shards[shard->id()];
}

ZuInline MxMDLib *MxMDVenueShard::md() const {
  return m_venue->md();
}

#endif /* MxMD_HPP */
