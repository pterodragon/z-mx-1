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

// MxMD recording dump tool

#include <ZuLib.hpp>

#include <stdio.h>
#include <stddef.h>

#include <ZmTime.hpp>
#include <ZmTrap.hpp>

#include <ZeLog.hpp>

#include <ZiFile.hpp>

#include <MxBase.hpp>

#include <MxMDStream.hpp>
#include <MxMDCSV.hpp>
#include <MxMD.hpp>

#include <version.h>

class RealTimeCSV : public ZvCSV, public MxMDCSV<RealTimeCSV> {
public:
  struct L2Data {
    MxIDString		objectID;
    MxEnum		side;
    MxInt		rank;
    uint8_t		delta;
    MxValue		price;
    MxValue		qty;
    MxValue		nOrders;
    MxFlags		flags;
    MxFlags		orderFlags;
  };
  struct Data {
    MxID		venue;
    MxID		segment;
    MxSymString		id;
    MxEnum		event;
    MxEnum		session;
    MxMDL1Data		l1Data;
    L2Data		l2Data;
    MxBool		updateL1;
  };
  typedef ZuPOD<Data> POD;

  template <typename App>
  RealTimeCSV(App *app) {
    new ((m_pod = new POD())->ptr()) Data();
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new MxIDCol("venue", Offset(venue)));
    add(new MxIDCol("segment", Offset(segment)));
    add(new MxIDStrCol("id", Offset(id)));
    add(new MxEnumCol<MxMDStream::Type::CSVMap>("event", Offset(event)));
    add(new MxEnumCol<MxTradingSession::CSVMap>("session", Offset(session)));
#undef Offset
#define Offset(x) offsetof(Data, l1Data) + offsetof(MxMDL1Data, x)
    int pxNDP = Offset(pxNDP);
    int qtyNDP = Offset(qtyNDP);
    if (app && app->hhmmss())
      add(new MxHHMMSSCol("stamp", Offset(stamp),
	    app->yyyymmdd(), app->tzOffset()));
    else
      add(new MxTimeCol("stamp", Offset(stamp),
	    app ? app->tzOffset() : 0));
    add(new MxNDPCol("pxNDP", pxNDP));
    add(new MxNDPCol("qtyNDP", qtyNDP));
    add(new MxEnumCol<MxTradingStatus::CSVMap>("status", Offset(status)));
    this->addValCol(app, "base", Offset(base), pxNDP);
    for (unsigned i = 0; i < MxMDNSessions; i++) {
      ZuStringN<8> open = "open"; open << ZuBoxed(i);
      ZuStringN<8> close = "close"; close << ZuBoxed(i);
      this->addValCol(app, open, Offset(open) + (i * sizeof(MxValue)), pxNDP);
      this->addValCol(app, close, Offset(close) + (i * sizeof(MxValue)), pxNDP);
    }
    this->addValCol(app, "last", Offset(last), pxNDP);
    this->addValCol(app, "lastQty", Offset(lastQty), qtyNDP);
    this->addValCol(app, "bid", Offset(bid), pxNDP);
    this->addValCol(app, "bidQty", Offset(bidQty), qtyNDP);
    this->addValCol(app, "ask", Offset(ask), pxNDP);
    this->addValCol(app, "askQty", Offset(askQty), qtyNDP);
    add(new MxEnumCol<MxTickDir::CSVMap>("tickDir", Offset(tickDir)));
    this->addValCol(app, "high", Offset(high), pxNDP);
    this->addValCol(app, "low", Offset(low), pxNDP);
    this->addValCol(app, "accVol", Offset(accVol), pxNDP);
    this->addValCol(app, "accVolQty", Offset(accVolQty), qtyNDP);
    this->addValCol(app, "match", Offset(match), pxNDP);
    this->addValCol(app, "matchQty", Offset(matchQty), qtyNDP);
    this->addValCol(app, "surplusQty", Offset(surplusQty), qtyNDP);
    add(new MxMDVenueFlagsCol<MxMDL1Flags>("l1Flags",
	  Offset(flags), offsetof(Data, venue)));
#undef Offset
#define Offset(x) offsetof(Data, l2Data) + offsetof(L2Data, x)
    add(new MxIDStrCol("objectID", Offset(objectID)));
    add(new MxEnumCol<MxSide::CSVMap>("side", Offset(side)));
    add(new MxIntCol("rank", Offset(rank)));
    add(new MxBoolCol("delta", Offset(delta)));
    this->addValCol(app, "price", Offset(price), pxNDP);
    this->addValCol(app, "qty", Offset(qty), qtyNDP);
    this->addValCol(app, "nOrders", Offset(nOrders), qtyNDP);
    add(new MxMDVenueFlagsCol<MxMDL2Flags>("l2Flags",
	  Offset(flags), offsetof(Data, venue)));
    add(new MxMDVenueFlagsCol<MxMDOrderFlags>("orderFlags",
	  Offset(orderFlags), offsetof(Data, venue)));
#undef Offset
#define Offset(x) offsetof(Data, x)
    add(new MxBoolCol("updateL1", Offset(updateL1)));
#undef Offset
  }

  ZuAnyPOD *row(const MxMDStream::Msg *msg) {
    Data *data = ptr();

    {
      using namespace MxMDStream;
      const Hdr &hdr = msg->hdr();
      switch ((int)hdr.type) {
	case Type::TradingSession:
	  {
	    const TradingSession &obj = hdr.as<TradingSession>();
	    new (data) Data{
	      obj.venue, obj.segment, {}, hdr.type,
	      obj.session, { obj.stamp } };
	  }
	  break;
	case Type::L1:
	  {
	    const L1 &obj = hdr.as<L1>();
	    new (data) Data{
	      obj.key.venue(), obj.key.segment(), obj.key.id(), hdr.type,
	      {}, obj.data};
	  }
	  break;
	case Type::PxLevel:
	  {
	    const PxLevel &obj = hdr.as<PxLevel>();
	    new (data) Data{
	      obj.key.venue(), obj.key.segment(), obj.key.id(), hdr.type,
	      {}, { obj.transactTime, obj.pxNDP, obj.qtyNDP },
	      { {}, {}, obj.side, {}, obj.delta, obj.price, obj.qty,
		obj.nOrders, obj.flags } };
	  }
	  break;
	case Type::AddOrder:
	  {
	    const AddOrder &obj = hdr.as<AddOrder>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = hdr.type;
	    data->l1Data.stamp = obj.transactTime;
	    data->l1Data.pxNDP = obj.pxNDP;
	    data->l1Data.qtyNDP = obj.qtyNDP;
	    data->l2Data.objectID = obj.orderID;
	    data->l2Data.side = obj.side;
	    data->l2Data.rank = obj.rank;
	    data->l2Data.price = obj.price;
	    data->l2Data.qty = obj.qty;
	    data->l2Data.orderFlags = obj.flags;
	  }
	  break;
	case Type::ModifyOrder:
	  {
	    const ModifyOrder &obj = hdr.as<ModifyOrder>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = hdr.type;
	    data->l1Data.stamp = obj.transactTime;
	    data->l1Data.pxNDP = obj.pxNDP;
	    data->l1Data.qtyNDP = obj.qtyNDP;
	    data->l2Data.objectID = obj.orderID;
	    data->l2Data.side = obj.side;
	    data->l2Data.rank = obj.rank;
	    data->l2Data.price = obj.price;
	    data->l2Data.qty = obj.qty;
	    data->l2Data.orderFlags = obj.flags;
	  }
	  break;
	case Type::CancelOrder:
	  {
	    const CancelOrder &obj = hdr.as<CancelOrder>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = hdr.type;
	    data->l1Data.stamp = obj.transactTime;
	    data->l2Data.objectID = obj.orderID;
	    data->l2Data.side = obj.side;
	  }
	  break;
	case Type::L2:
	  {
	    const L2 &obj = hdr.as<L2>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = hdr.type;
	    data->l1Data.stamp = obj.stamp;
	    data->updateL1 = obj.updateL1;
	  }
	  break;
	case Type::ResetOB:
	  {
	    const ResetOB &obj = hdr.as<ResetOB>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = hdr.type;
	    data->l1Data.stamp = obj.transactTime;
	  }
	  break;
	case Type::AddTrade:
	  {
	    const AddTrade &obj = hdr.as<AddTrade>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = hdr.type;
	    data->l1Data.stamp = obj.transactTime;
	    data->l1Data.pxNDP = obj.pxNDP;
	    data->l1Data.qtyNDP = obj.qtyNDP;
	    data->l2Data.objectID = obj.tradeID;
	    data->l2Data.price = obj.price;
	    data->l2Data.qty = obj.qty;
	  }
	  break;
	case Type::CorrectTrade:
	  {
	    const CorrectTrade &obj = hdr.as<CorrectTrade>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = hdr.type;
	    data->l1Data.stamp = obj.transactTime;
	    data->l1Data.pxNDP = obj.pxNDP;
	    data->l1Data.qtyNDP = obj.qtyNDP;
	    data->l2Data.objectID = obj.tradeID;
	    data->l2Data.price = obj.price;
	    data->l2Data.qty = obj.qty;
	  }
	  break;
	case Type::CancelTrade:
	  {
	    const CancelTrade &obj = hdr.as<CancelTrade>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = hdr.type;
	    data->l1Data.stamp = obj.transactTime;
	    data->l1Data.pxNDP = obj.pxNDP;
	    data->l1Data.qtyNDP = obj.qtyNDP;
	    data->l2Data.objectID = obj.tradeID;
	    data->l2Data.price = obj.price;
	    data->l2Data.qty = obj.qty;
	  }
	  break;
	case Type::RefDataLoaded:
	  {
	    const RefDataLoaded &obj = hdr.as<RefDataLoaded>();
	    data->venue = obj.venue;
	    data->event = hdr.type;
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

template <class CSV>
class CSVWriter : public ZuObject, public CSV {
public:
  typedef MxMDStream::Msg Msg;

  template <typename App, typename Path>
  CSVWriter(App *app, const Path &path) : CSV(app), m_path(path) { }

  void start() { m_fn = this->writeFile(m_path); }

  void stop() { m_fn((ZuAnyPOD *)0); }

  inline void enqueue(Msg *msg) {
    ZuAnyPOD *pod = CSV::row(msg);
    if (ZuLikely(pod)) m_fn(pod);
  }

private:
  ZtString		m_path;
  ZvCSVWriteFn		m_fn;
};

class App {
  typedef ZmLHash<MxSecKey> SecIDHash;

public:
  App() : m_yyyymmdd(ZtDateNow().yyyymmdd()) { }

  // dump options

  inline void verbose(bool b) { m_verbose = b; }

  // CSV formatting

  inline bool hhmmss() const { return m_hhmmss; }
  inline unsigned yyyymmdd() const { return m_yyyymmdd; }
  inline int tzOffset() const { return m_tzOffset; }

  inline void hhmmss(bool b) { m_hhmmss = b; }
  inline void yyyymmdd(unsigned n) { m_yyyymmdd = n; }
  inline void tz(const char *tz) {
    ZtDate now(ZtDate::YYYYMMDD, m_yyyymmdd, ZtDate::HHMMSS, 120000, tz);
    m_tzOffset = now.offset(tz);
    m_isoFmt.offset(m_tzOffset);
  }

  inline bool raw() const { return m_raw; }
  inline void raw(bool b) { m_raw = b; }

  // filters

  inline void refData(bool b) { m_refData = b; }
  inline void l1(bool b) { m_l1 = b; }
  inline void l2(bool b) { m_l2 = b; }
  inline void trades(bool b) { m_trades = b; }

  inline void secID(const MxSecKey &key) { m_secIDs.add(key); }
  inline bool filterID(MxSecKey key) {
    if (!m_secIDs.count()) return false;
    if (m_secIDs.exists(key)) return false;
    if (*key.segment()) {
      key.segment() = MxID();
      if (m_secIDs.exists(key)) return false;
    }
    if (*key.venue()) {
      key.venue() = MxID();
      if (m_secIDs.exists(key)) return false;
    }
    return true;
  }

  // outputs

  inline const ZtString &path() const { return m_path; }
  inline const ZtString &outPath() const { return m_outPath; }

  template <typename Path>
  inline void path(const Path &path) { m_path = path; }
  template <typename Path>
  inline void outPath(const Path &path) { m_outPath = path; }

  template <typename Path>
  inline void tickSizeCSV(const Path &path) {
    m_tickSizeCSV = new CSVWriter<MxMDTickSizeCSV>(this, path);
  }
  template <typename Path>
  inline void securityCSV(const Path &path) {
    m_securityCSV = new CSVWriter<MxMDSecurityCSV>(this, path);
  }
  template <typename Path>
  inline void orderBookCSV(const Path &path) {
    m_orderBookCSV = new CSVWriter<MxMDOrderBookCSV>(this, path);
  }
  template <typename Path>
  inline void realTimeCSV(const Path &path) {
    m_realTimeCSV = new CSVWriter<RealTimeCSV>(this, path);
  }

  // application control

  void start() {
    ZeError e;
    if (m_file.open(m_path, ZiFile::ReadOnly, 0, &e) < 0) {
      ZeLOG(Error, ZtString() << '"' << m_path << "\": " << e);
      exit(1);
    }
    if (m_outPath) {
      ZeError e;
      if (m_outFile.open(m_outPath,
	    ZiFile::WriteOnly | ZiFile::Append | ZiFile::Create,
	    0666, &e) != Zi::OK) {
	ZeLOG(Error, ZtString() << '"' << m_outPath << "\": " << e);
	exit(1);
      }
      MxMDStream::FileHdr hdr("RMD",
	  MXMD_VMAJOR(MXMD_VERSION), 
	  MXMD_VMINOR(MXMD_VERSION));
      if (m_outFile.write(&hdr, sizeof(MxMDStream::FileHdr), &e) != Zi::OK) {
	m_outFile.close();
	ZeLOG(Error, ZtString() << '"' << m_outPath << "\": " << e);
	exit(1);
      }
    }
    if (m_tickSizeCSV) m_tickSizeCSV->start();
    if (m_securityCSV) m_securityCSV->start();
    if (m_orderBookCSV) m_orderBookCSV->start();
    if (m_realTimeCSV) m_realTimeCSV->start();
  }

  void stop() {
    m_file.close();
    if (m_outFile) m_outFile.close();
    if (m_tickSizeCSV) m_tickSizeCSV->stop();
    if (m_securityCSV) m_securityCSV->stop();
    if (m_orderBookCSV) m_orderBookCSV->stop();
    if (m_realTimeCSV) m_realTimeCSV->stop();
  }

  void read();

private:
  bool				m_refData = 0;
  bool				m_l1 = 0;
  bool				m_l2 = 0;
  bool				m_trades = 0;

  bool				m_hhmmss = 0;
  unsigned			m_yyyymmdd = 0;
  int				m_tzOffset = 0;

  bool				m_verbose = 0;
  bool				m_raw = 0;

  ZtDateFmt::ISO		m_isoFmt;

  SecIDHash			m_secIDs;

  ZtString			m_path;
  ZiFile			m_file;

  ZtString			m_outPath;
  ZiFile			m_outFile;

  ZmTime			m_lastTime;

  ZuRef<CSVWriter<MxMDTickSizeCSV> >	m_tickSizeCSV;
  ZuRef<CSVWriter<MxMDSecurityCSV> >	m_securityCSV;
  ZuRef<CSVWriter<MxMDOrderBookCSV> >	m_orderBookCSV;
  ZuRef<CSVWriter<RealTimeCSV> >	m_realTimeCSV;
};

void App::read()
{
  using namespace MxMDStream;

  if (!m_file) return;

  ZeError e;
  ZuPair<ZuBox0(uint16_t), ZuBox0(uint16_t)> v;
  int n;
  off_t o;

  try {
    FileHdr hdr(m_file, &e);
    v = ZuMkPair(hdr.vmajor, hdr.vminor);
    std::cout << "version: " <<
      ZuBoxed(v.p1()) << '.' << ZuBoxed(v.p2()) << '\n';
  } catch (const FileHdr::IOError &) {
    goto error;
  } catch (const FileHdr::InvalidFmt &) {
    ZeLOG(Error, ZtString() << '"' << m_path << "\": invalid format");
    return;
  }

  for (;;) {
    ZuRef<Msg> msg = new Msg();
    o = m_file.offset();
    n = m_file.read(msg->ptr(), sizeof(Hdr), &e);
    if (n == Zi::IOError) goto error;
    if (n == Zi::EndOfFile || (unsigned)n < sizeof(Hdr)) return;
    Hdr &hdr = msg->hdr();
    if (hdr.len > sizeof(Buf)) goto lenerror;
    n = m_file.read(hdr.body(), hdr.len, &e);
    if (n == Zi::IOError) goto error;
    if (n == Zi::EndOfFile || (unsigned)n < hdr.len) return;

    if (m_verbose) {
      if (hdr.nsec) {
	ZtDate stamp = m_lastTime + ZmTime((time_t)0, hdr.nsec);
	std::cout << "len: " << ZuBoxed(hdr.len).fmt(ZuFmt::Right<6>()) <<
	  "  type: " << ZuBoxed(hdr.type).fmt(ZuFmt::Right<6>()) <<
	  "  stamp: " << stamp.iso(m_isoFmt) << '\n';
      } else {
	std::cout << "len: " << ZuBoxed(hdr.len).fmt(ZuFmt::Right<6>()) <<
	  "  type: " << ZuBoxed(hdr.type).fmt(ZuFmt::Right<6>()) <<
	  "  stamp: (null)\n";
      }
    }
 
    {
      switch ((int)hdr.type) {
	case Type::HeartBeat:
	  m_lastTime = msg->as<HeartBeat>().stamp.zmTime();
	  break;
	case Type::AddTickSizeTbl:
	case Type::ResetTickSizeTbl:
	  if (n != (int)sizeof(AddTickSizeTbl)) goto dataerror;
	  if (m_tickSizeCSV) m_tickSizeCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::AddTickSize:
	  if (n != (int)sizeof(AddTickSize)) goto dataerror;
	  if (m_tickSizeCSV) m_tickSizeCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;

	case Type::AddSecurity:
	  if (n != (int)sizeof(AddSecurity)) goto dataerror;
	  if (m_securityCSV) m_securityCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::UpdateSecurity:
	  if (n != (int)sizeof(UpdateSecurity)) goto dataerror;
	  if (m_securityCSV) m_securityCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;

	case Type::AddOrderBook:
	  if (n != (int)sizeof(AddOrderBook)) goto dataerror;
	  if (m_orderBookCSV) m_orderBookCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::DelOrderBook:
	  if (n != (int)sizeof(DelOrderBook)) goto dataerror;
	  if (m_orderBookCSV) m_orderBookCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::AddCombination:
	  if (n != (int)sizeof(AddCombination)) goto dataerror;
	  if (m_orderBookCSV) m_orderBookCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::DelCombination:
	  if (n != (int)sizeof(DelCombination)) goto dataerror;
	  if (m_orderBookCSV) m_orderBookCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::UpdateOrderBook:
	  if (n != (int)sizeof(UpdateOrderBook)) goto dataerror;
	  if (m_orderBookCSV) m_orderBookCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;

	case Type::RefDataLoaded:
	  if (n != (int)sizeof(RefDataLoaded)) goto dataerror;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;

	case Type::TradingSession:
	  if (n != (int)sizeof(TradingSession)) goto dataerror;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::L1:
	  if (n != (int)sizeof(L1)) goto dataerror;
	  if (!m_l1) continue;
	  if (filterID(hdr.as<L1>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::PxLevel:
	  if (n != (int)sizeof(PxLevel)) goto dataerror;
	  if (!m_l2) continue;
	  if (filterID(hdr.as<PxLevel>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::AddOrder:
	case Type::ModifyOrder:
	  if (n != (int)sizeof(AddOrder)) goto dataerror;
	  if (!m_l2) continue;
	  if (filterID(hdr.as<AddOrder>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::CancelOrder:
	  if (n != (int)sizeof(CancelOrder)) goto dataerror;
	  if (!m_l2) continue;
	  if (filterID(hdr.as<CancelOrder>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::L2:
	  if (n != (int)sizeof(L2)) goto dataerror;
	  if (!m_l2) continue;
	  if (filterID(hdr.as<L2>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::ResetOB:
	  if (n != (int)sizeof(ResetOB)) goto dataerror;
	  if (!m_l2) continue;
	  if (filterID(hdr.as<ResetOB>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;

	case Type::AddTrade:
	  if (n != (int)sizeof(AddTrade)) goto dataerror;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  if (!m_trades) continue;
	  break;
	case Type::CorrectTrade:
	  if (n != (int)sizeof(CorrectTrade)) goto dataerror;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  if (!m_trades) continue;
	  break;
	case Type::CancelTrade:
	  if (n != (int)sizeof(CancelTrade)) goto dataerror;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  if (!m_trades) continue;
	  break;

	default:
	  continue;
      }
    }

    if (m_outFile) {
      ZeError e;
      if (m_outFile.write(
	    msg->ptr(), sizeof(Hdr) + hdr.len, &e) != Zi::OK) {
	ZeLOG(Error, ZtString() << '"' << m_outPath << "\": " << e);
	return;
      }
    }
  }

dataerror:
  ZeLOG(Error, ZtString() << '"' << m_path << "\": corrupt data error " <<
      ZuBoxed(n) << " at offset " << ZuBoxed(o));
  return;

lenerror:
  ZeLOG(Error, ZtString() << '"' << m_path << "\": message length >" <<
      ZuBoxed(sizeof(MxMDStream::Buf)) << " at offset " << ZuBoxed(o));
  return;

error:
  ZeLOG(Error, ZtString() << '"' << m_path << "\": " << e);
}

void usage()
{
  std::cerr <<
    "usage: recdump [OPTION]... RECFILE\n"
    "\tRECFILE\t- market data recording file\n\n"
    "Options:\n"
    "  -r\t\t- include reference data in output\n"
    "  -1\t\t- include Level 1 data in output\n"
    "  -2\t\t- include Level 2 data in output\n"
    "  -t\t\t- include trade data in output\n"
    "  -R CSV\t- dump real-time messages to CSV\n"
    "  -O CSV\t- dump order book messages to CSV\n"
    "  -S CSV\t- dump security messages to CSV\n"
    "  -T CSV\t- dump tick size messages to CSV\n"
    "  -n\t\t- CSV time stamps as HHMMSS instead of Excel format\n"
    "  -V\t\t- verbose - dump messages to standard output\n"
    "  -N\t\t- raw - output raw fixed-point values (without decimal point)\n"
    "  -d YYYYMMDD\t- CSV time stamps use date YYYYMMDD\n"
    "  -z ZONE\t- CSV time stamps in local time ZONE (defaults to GMT)\n"
    "  -v MIC\t- select venue MIC for following securities\n"
    "\t\t\t(may be specified multiple times)\n"
    "  -s SEGMENT\t- select SEGMENT for following securities\n"
    "\t\t\t(may be specified multiple times)\n"
    "  -i ID\t\t- filter for security ID\n"
    "\t\t\t(may be specified multiple times)\n"
    "  -o OUT\t- record filtered output in file OUT\n"
    << std::flush;
  exit(1);
}

int main(int argc, const char *argv[])
{
  App app;
  MxID venue;
  MxID segment;
  bool tickSizeCSV = false,
       securityCSV = false,
       orderBookCSV = false,
       realTimeCSV = false;

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      if (app.path()) usage();
      app.path(argv[i]);
      continue;
    }
    switch (argv[i][1]) {
      case 'r':
	app.refData(true);
	break;
      case '1':
	app.l1(true);
	break;
      case '2':
	app.l2(true);
	break;
      case 't':
	app.trades(true);
	break;
      case 'n':
	app.hhmmss(true);
	break;
      case 'd':
	if (++i >= argc) usage();
	app.yyyymmdd(atoi(argv[i]));
	break;
      case 'z':
	if (++i >= argc) usage();
	app.tz(argv[i]);
	break;
      case 'v':
	if (++i >= argc) usage();
	venue = argv[i];
	break;
      case 's':
	if (++i >= argc) usage();
	segment = argv[i];
	break;
      case 'i':
	if (++i >= argc) usage();
	app.secID(MxSecKey(venue, segment, argv[i]));
	break;
      case 'o':
	if (app.outPath()) usage();
	if (++i >= argc) usage();
	app.outPath(argv[i]);
	break;
      case 'T':
	if (tickSizeCSV || ++i >= argc) usage();
	tickSizeCSV = true;
	app.tickSizeCSV(argv[i]);
	break;
      case 'S':
	if (securityCSV || ++i >= argc) usage();
	securityCSV = true;
	app.securityCSV(argv[i]);
	break;
      case 'O':
	if (orderBookCSV || ++i >= argc) usage();
	orderBookCSV = true;
	app.orderBookCSV(argv[i]);
	break;
      case 'R':
	if (realTimeCSV || ++i >= argc) usage();
	realTimeCSV = true;
	app.realTimeCSV(argv[i]);
	break;
      case 'V':
	app.verbose(true);
	break;
      case 'N':
	app.raw(true);
	break;
      default:
	usage();
	break;
    }
  }
  if (!app.path()) usage();

  ZeLog::init("recdump");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink("&2"));
  ZeLog::start();

  app.start();
  app.read();
  app.stop();
  
  ZeLog::stop();
  return 0;
}
