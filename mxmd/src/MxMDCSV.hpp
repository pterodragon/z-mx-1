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

// MxMD CSV common definitions

#ifndef MxMDCSV_HPP
#define MxMDCSV_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <ZiIP.hpp>

#include <ZvCSV.hpp>

#include <MxBase.hpp>
#include <MxCSV.hpp>

#include <MxMDTypes.hpp>
#include <MxMDStream.hpp>

struct MxMDDefaultCSVApp {
  inline bool hhmmss() { return false; }
  inline unsigned yyyymmdd() { return 0; }
  inline int tzOffset() { return 0; }
};

template <typename Flags>
class MxMDVenueFlagsCol : public ZvCSVColumn<ZvCSVColType::Func, MxFlags> {
  typedef ZvCSVColumn<ZvCSVColType::Func, MxFlags> Base;
  typedef typename Base::ParseFn ParseFn;
  typedef typename Base::PlaceFn PlaceFn;
public:
  template <typename ID>
  inline MxMDVenueFlagsCol(const ID &id, int offset, int venueOffset) :
    Base(id, offset,
	ParseFn::Member<&MxMDVenueFlagsCol::parse>::fn(this),
	PlaceFn::Member<&MxMDVenueFlagsCol::place>::fn(this)),
    m_venueOffset(venueOffset - offset) { }
  virtual ~MxMDVenueFlagsCol() { }

  void parse(MxFlags *f, ZuString b) {
    unsigned i, n = b.length(); if (n > 9) n = 9;
    for (i = 0;; i++) {
      if (ZuUnlikely(i == n)) { *f = 0; return; }
      if (b[i] == ':') break;
    }
    MxID venue = ZuString(&b[0], i++);
    *(MxID *)((char *)(void *)f + m_venueOffset) = venue;
    MxMDFlagsStr in = ZuString(&b[i], b.length() - i);
    Flags::scan(in, venue, *f);
  }
  void place(ZtArray<char> &b, const MxFlags *f) {
    if (!*f) return;
    MxID venue =
      *(const MxID *)((const char *)(const void *)f + m_venueOffset);
    MxMDFlagsStr out;
    Flags::print(out, venue, *f);
    if (!out) return;
    b << venue << ':' << out;
  }

private:
  int	m_venueOffset;
};

class MxMDTickSizeCSV : public ZvCSV {
public:
  struct Data {
    MxEnum		event;
    MxID		venue;
    MxIDString		id;
    MxNDP		pxNDP;
    MxValue		minPrice;
    MxValue		maxPrice;
    MxValue		tickSize;
  };
  typedef ZuPOD<Data> POD;

  template <typename App = MxMDDefaultCSVApp>
  MxMDTickSizeCSV(App *app = 0) {
    new ((m_pod = new POD())->ptr()) Data{};
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new MxEnumCol<MxMDStream::Type::CSVMap>("event", Offset(event)));
    add(new MxIDCol("venue", Offset(venue)));
    add(new MxIDStrCol("id", Offset(id)));
    add(new MxNDPCol("pxNDP", Offset(pxNDP)));
    add(new MxValueCol("minPrice", Offset(minPrice), Offset(pxNDP)));
    add(new MxValueCol("maxPrice", Offset(maxPrice), Offset(pxNDP)));
    add(new MxValueCol("tickSize", Offset(tickSize), Offset(pxNDP)));
#undef Offset
  }

  void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

  template <typename File>
  void read(const File &file, ZvCSVReadFn fn) {
    ZvCSV::readFile(file,
	ZvCSVAllocFn::Member<&MxMDTickSizeCSV::alloc>::fn(this), fn);
  }

  ZuAnyPOD *row(const MxMDStream::Msg *msg) {
    Data *data = ptr();

    {
      using namespace MxMDStream;
      const Frame *frame = msg->frame();
      switch ((int)frame->type) {
	case Type::AddTickSizeTbl:
	case Type::ResetTickSizeTbl:
	  {
	    const AddTickSizeTbl &obj = frame->as<AddTickSizeTbl>();
	    new (data) Data{frame->type, obj.venue, obj.id, obj.pxNDP};
	  }
	  break;
	case Type::AddTickSize:
	  {
	    const AddTickSize &obj = frame->as<AddTickSize>();
	    new (data) Data{frame->type,
	      obj.venue, obj.id, obj.minPrice, obj.maxPrice, obj.tickSize};
	  }
	  break;
	default:
	  return 0;
      }
    }

    return pod();
  }

  ZuInline POD *pod() { return m_pod.ptr(); }
  ZuInline Data *ptr() { return m_pod->ptr(); }

private:
  ZuRef<POD>	m_pod;
};

class MxMDSecurityCSV : public ZvCSV {
public:
  struct Data {
    MxEnum		event;
    MxDateTime		transactTime;
    MxID		venue;
    MxID		segment;
    MxIDString		id;
    MxUInt		shard;
    MxMDSecRefData	refData;
  };
  typedef ZuPOD<Data> POD;

  template <typename App = MxMDDefaultCSVApp>
  MxMDSecurityCSV(App *app = 0) {
    new ((m_pod = new POD())->ptr()) Data{};
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new MxEnumCol<MxMDStream::Type::CSVMap>("event", Offset(event)));
    if (app && app->hhmmss())
      add(new MxHHMMSSCol("transactTime", Offset(transactTime),
	    app->yyyymmdd(), app->tzOffset()));
    else
      add(new MxTimeCol("transactTime", Offset(transactTime),
	    app ? app->tzOffset() : 0));
    add(new MxIDCol("venue", Offset(venue)));
    add(new MxIDCol("segment", Offset(segment)));
    add(new MxIDStrCol("id", Offset(id)));
    add(new MxUIntCol("shard", Offset(shard)));
#undef Offset
#define Offset(x) offsetof(Data, refData) + offsetof(MxMDSecRefData, x)
    add(new MxBoolCol("tradeable", Offset(tradeable)));
    add(new MxEnumCol<MxSecIDSrc::CSVMap>("idSrc", Offset(idSrc)));
    add(new MxSymStrCol("symbol", Offset(symbol)));
    add(new MxEnumCol<MxSecIDSrc::CSVMap>("altIDSrc", Offset(altIDSrc)));
    add(new MxSymStrCol("altSymbol", Offset(altSymbol)));
    add(new MxIDCol("underVenue", Offset(underVenue)));
    add(new MxIDCol("underSegment", Offset(underSegment)));
    add(new MxIDStrCol("underlying", Offset(underlying)));
    add(new MxUIntCol("mat", Offset(mat)));
    add(new MxEnumCol<MxPutCall::CSVMap>("putCall", Offset(putCall)));
    add(new MxNDPCol("pxNDP", Offset(pxNDP)));
    add(new MxNDPCol("qtyNDP", Offset(qtyNDP)));
    add(new MxValueCol("strike", Offset(strike), Offset(pxNDP)));
    add(new MxUIntCol("outstandingShares", Offset(outstandingShares)));
    add(new MxValueCol("adv", Offset(adv), Offset(pxNDP)));
#undef Offset
  }

  void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

  template <typename File>
  void read(const File &file, ZvCSVReadFn fn) {
    ZvCSV::readFile(file,
	ZvCSVAllocFn::Member<&MxMDSecurityCSV::alloc>::fn(this), fn);
  }

  ZuAnyPOD *row(const MxMDStream::Msg *msg) {
    Data *data = ptr();

    {
      using namespace MxMDStream;
      const Frame *frame = msg->frame();
      switch ((int)frame->type) {
	case Type::AddSecurity:
	  {
	    const AddSecurity &obj = frame->as<AddSecurity>();
	    new (data) Data{frame->type, obj.transactTime,
	      obj.key.venue(), obj.key.segment(), obj.key.id(),
	      obj.shard, obj.refData};
	  }
	  break;
	case Type::UpdateSecurity:
	  {
	    const UpdateSecurity &obj = frame->as<UpdateSecurity>();
	    new (data) Data{frame->type, obj.transactTime,
	      obj.key.venue(), obj.key.segment(), obj.key.id(),
	      {}, obj.refData};
	  }
	  break;
	default:
	  return 0;
      }
    }

    return pod();
  }

  ZuInline POD *pod() { return m_pod.ptr(); }
  ZuInline Data *ptr() { return m_pod->ptr(); }

private:
  ZuRef<POD>	m_pod;
};

class MxMDOrderBookCSV : public ZvCSV {
public:
  struct Data {
    MxEnum		event;
    MxDateTime		transactTime;
    MxID		venue;
    MxID		segment;
    MxIDString		id;
    MxNDP		pxNDP;
    MxNDP		qtyNDP;
    MxUInt		legs;
    MxIDString		tickSizeTbl;
    MxMDLotSizes	lotSizes;
    MxID		secVenues[MxMDNLegs];
    MxID		secSegments[MxMDNLegs];
    MxIDString		securities[MxMDNLegs];
    MxEnum		sides[MxMDNLegs];
    MxRatio		ratios[MxMDNLegs];
  };
  typedef ZuPOD<Data> POD;

  template <typename App = MxMDDefaultCSVApp>
  MxMDOrderBookCSV(App *app = 0) {
    new ((m_pod = new POD())->ptr()) Data{};
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new MxEnumCol<MxMDStream::Type::CSVMap>("event", Offset(event)));
    if (app && app->hhmmss())
      add(new MxHHMMSSCol("transactTime", Offset(transactTime),
	    app->yyyymmdd(), app->tzOffset()));
    else
      add(new MxTimeCol("transactTime", Offset(transactTime),
	    app ? app->tzOffset() : 0));
    add(new MxIDCol("venue", Offset(venue)));
    add(new MxIDCol("segment", Offset(segment)));
    add(new MxIDStrCol("id", Offset(id)));
    add(new MxNDPCol("pxNDP", Offset(pxNDP)));
    add(new MxNDPCol("qtyNDP", Offset(qtyNDP)));
    add(new MxUIntCol("legs", Offset(legs)));
    for (unsigned i = 0; i < MxMDNLegs; i++) {
      ZuStringN<16> secVenue = "secVenue"; secVenue << ZuBoxed(i);
      ZuStringN<16> secSegment = "secSegment"; secSegment << ZuBoxed(i);
      ZuStringN<16> security = "security"; security << ZuBoxed(i);
      ZuStringN<8> side = "side"; side << ZuBoxed(i);
      ZuStringN<8> ratio = "ratio"; ratio << ZuBoxed(i);
      add(new MxIDCol(secVenue, Offset(secVenues) + (i * sizeof(MxID))));
      add(new MxIDCol(secSegment, Offset(secSegments) + (i * sizeof(MxID))));
      add(new MxIDStrCol(security,
	    Offset(securities) + (i * sizeof(MxSymString))));
      add(new MxEnumCol<MxSide::CSVMap>(side,
	    Offset(sides) + (i * sizeof(MxEnum))));
      add(new MxRatioCol(ratio,
	    Offset(ratios) + (i * sizeof(MxFloat))));
    }
    add(new MxIDStrCol("tickSizeTbl", Offset(tickSizeTbl)));
    int offsetQtyNDP = Offset(qtyNDP);
#undef Offset
#define Offset(x) offsetof(Data, lotSizes) + offsetof(MxMDLotSizes, x)
    add(new MxValueCol("oddLotSize", Offset(oddLotSize), offsetQtyNDP));
    add(new MxValueCol("lotSize", Offset(lotSize), offsetQtyNDP));
    add(new MxValueCol("blockLotSize", Offset(blockLotSize), offsetQtyNDP));
#undef Offset
  }

  void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

  template <typename File>
  void read(const File &file, ZvCSVReadFn fn) {
    ZvCSV::readFile(file,
	ZvCSVAllocFn::Member<&MxMDOrderBookCSV::alloc>::fn(this), fn);
  }

  ZuAnyPOD *row(const MxMDStream::Msg *msg) {
    Data *data = ptr();

    {
      using namespace MxMDStream;
      const Frame *frame = msg->frame();
      switch ((int)frame->type) {
	case Type::AddOrderBook:
	  {
	    const AddOrderBook &obj = frame->as<AddOrderBook>();
	    new (data) Data{frame->type, obj.transactTime,
	      obj.key.venue(), obj.key.segment(), obj.key.id(),
	      {}, {}, 1, obj.tickSizeTbl, obj.lotSizes,
	      { obj.security.venue() },
	      { obj.security.segment() },
	      { obj.security.id() },
	      {}, {} };
	  }
	  break;
	case Type::DelOrderBook:
	  {
	    const DelOrderBook &obj = frame->as<DelOrderBook>();
	    new (data) Data{frame->type, obj.transactTime,
	      obj.key.venue(), obj.key.segment(), obj.key.id()};
	  }
	  break;
	case Type::AddCombination:
	  {
	    const AddCombination &obj = frame->as<AddCombination>();
	    new (data) Data{frame->type, obj.transactTime,
	      obj.key.venue(), obj.key.segment(), obj.key.id(),
	      obj.pxNDP, obj.qtyNDP, obj.legs,
	      obj.tickSizeTbl, obj.lotSizes,
	      {}, {}, {}, {}, {} };
	    for (unsigned i = 0, n = obj.legs; i < n; i++) {
	      data->secVenues[i] = obj.securities[i].venue();
	      data->secSegments[i] = obj.securities[i].segment();
	      data->securities[i] = obj.securities[i].id();
	      data->sides[i] = obj.sides[i];
	      data->ratios[i] = obj.ratios[i];
	    }
	  }
	  break;
	case Type::DelCombination:
	  {
	    const DelCombination &obj = frame->as<DelCombination>();
	    new (data) Data{frame->type, obj.transactTime,
	      obj.key.venue(), obj.key.segment(), obj.key.id() };
	  }
	  break;
	case Type::UpdateOrderBook:
	  {
	    const UpdateOrderBook &obj = frame->as<UpdateOrderBook>();
	    new (data) Data{frame->type, obj.transactTime,
	      obj.key.venue(), obj.key.segment(), obj.key.id(),
	      {}, {}, MxUInt(), obj.tickSizeTbl, obj.lotSizes,
	      {}, {}, {}, {}, {} };
	  }
	  break;
	default:
	  return 0;
      }
    }

    return pod();
  }

  ZuInline POD *pod() { return m_pod.ptr(); }
  ZuInline Data *ptr() { return m_pod->ptr(); }

private:
  ZuRef<POD>	m_pod;
};

#endif /* MxMDCSV_HPP */
