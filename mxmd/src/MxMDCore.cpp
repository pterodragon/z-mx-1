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

// MxMD internal API

#include <MxMDCore.hpp>

#include <stddef.h>

#include <ZeLog.hpp>

#include <ZiModule.hpp>

#include <ZvCf.hpp>
#include <ZvHeapCf.hpp>

#include <MxMDCSV.hpp>

#include <version.h>

unsigned MxMDCore::vmajor() { return MXMD_VMAJOR(MXMD_VERSION); }
unsigned MxMDCore::vminor() { return MXMD_VMINOR(MXMD_VERSION); }

class MxMDVenueMapCSV : public ZvCSV {
public:
  struct Data {
    MxID		inVenue;
    MxID		inSegment;
    MxUInt		inRank;
    MxID		outVenue;
    MxID		outSegment;
  };
  typedef ZuPOD<Data> POD;

  MxMDVenueMapCSV() {
    new ((m_pod = new POD())->ptr()) Data{};
    add(new MxIDCol("inVenue", offsetof(Data, inVenue)));
    add(new MxIDCol("inSegment", offsetof(Data, inSegment)));
    add(new MxUIntCol("inRank", offsetof(Data, inRank)));
    add(new MxIDCol("outVenue", offsetof(Data, outVenue)));
    add(new MxIDCol("outSegment", offsetof(Data, outSegment)));
  }

  void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

  template <typename File>
  void read(const File &file, ZvCSVReadFn fn) {
    ZvCSV::readFile(file,
	ZvCSVAllocFn::Member<&MxMDVenueMapCSV::alloc>::fn(this), fn);
  }

  ZuInline POD *pod() { return m_pod.ptr(); }
  ZuInline Data *ptr() { return m_pod->ptr(); }

private:
  ZuRef<POD>	m_pod;
};

void MxMDCore::addVenueMapping_(ZuAnyPOD *pod)
{
  auto data = pod->as<MxMDVenueMapCSV::Data>();
  addVenueMapping(MxMDVenueMapKey(data.inVenue, data.inSegment),
      MxMDVenueMapping{data.outVenue, data.outSegment, data.inRank});
}

void MxMDCore::addTickSize_(ZuAnyPOD *pod)
{
  MxMDTickSizeCSV::Data *data = (MxMDTickSizeCSV::Data *)(pod->ptr());
  ZmRef<MxMDVenue> venue = this->venue(data->venue);
  ZmRef<MxMDTickSizeTbl> tbl = venue->addTickSizeTbl(data->id, data->pxNDP);
  tbl->addTickSize(data->minPrice, data->maxPrice, data->tickSize);
}

void MxMDCore::addInstrument_(ZuAnyPOD *pod)
{
  MxMDInstrumentCSV::Data *data = (MxMDInstrumentCSV::Data *)(pod->ptr());
  MxInstrKey key{data->venue, data->segment, data->id};
  MxMDInstrHandle instrHandle = instrument(key, data->shard);
  instrHandle.invokeMv([key, refData = data->refData,
      transactTime = data->transactTime](
	MxMDShard *shard, ZmRef<MxMDInstrument> instr) {
    shard->addInstrument(ZuMv(instr), key, refData, transactTime);
  });
}

void MxMDCore::addOrderBook_(ZuAnyPOD *pod)
{
  MxMDOrderBookCSV::Data *data = (MxMDOrderBookCSV::Data *)(pod->ptr());
  MxInstrKey instrKey{
    data->instrVenues[0], data->instrSegments[0], data->instruments[0]};
  MxMDInstrHandle instrHandle = instrument(instrKey);
  if (!instrHandle) throw ZtString() << "unknown instrument: " << instrKey;
  ZmRef<MxMDVenue> venue = this->venue(data->venue);
  ZmRef<MxMDTickSizeTbl> tbl =
    venue->addTickSizeTbl(data->tickSizeTbl, data->pxNDP);
  instrHandle.invokeMv(
      [data = *data, venue = ZuMv(venue), tbl = ZuMv(tbl)](
	MxMDShard *shard, ZmRef<MxMDInstrument> instr) {
    if (data.legs == 1) {
      MxID venueID = data.instrVenues[0];
      if (!*venueID) venueID = data.venue;
      MxID segment = data.instrSegments[0];
      if (!*segment) segment = data.segment;
      MxIDString id = data.instruments[0];
      if (!id) id = data.id;
      instr->addOrderBook(
	  MxInstrKey{data.id, data.venue, data.segment}, tbl, data.lotSizes,
	  data.transactTime);
    } else {
      ZmRef<MxMDInstrument> instruments[MxMDNLegs];
      MxEnum sides[MxMDNLegs];
      MxRatio ratios[MxMDNLegs];
      for (unsigned i = 0, n = data.legs; i < n; i++) {
	MxID venueID = data.instrVenues[i];
	if (!*venueID) venueID = data.venue;
	MxID segment = data.instrSegments[i];
	if (!*segment) segment = data.segment;
	MxIDString id = data.instruments[i];
	if (!id) return;
	if (!i)
	  instruments[i] = ZuMv(instr);
	else {
	  instruments[i] = shard->instrument(MxInstrKey{id, venueID, segment});
	  if (!instruments[i]) return;
	}
	sides[i] = data.sides[i];
	ratios[i] = data.ratios[i];
      }
      venue->shard(shard)->addCombination(
	  data.segment, data.id, data.pxNDP, data.qtyNDP,
	  data.legs, instruments, sides, ratios, tbl, data.lotSizes,
	  data.transactTime);
    }
  });
}

MxMDLib *MxMDLib::init(ZuString cf_, ZmFn<ZmScheduler *> schedInitFn)
{
  ZeLog::start(); // ensure error reporting works

  ZmRef<ZvCf> cf = new ZvCf();

  ZmRef<MxMDCore> md;

  if (cf_)
    try {
      cf->fromFile(cf_, false);
    } catch (const ZvError &e) {
      ZeLOG(Fatal, ZtString() << "MxMDLib - configuration error: \"" << cf_ << "\": " << e);
      return md;
    }
  else {
    cf->fromString(
      "mx {\n"
      "  core {\n"
      "    nThreads 4\n"	// thread IDs are 1-based
      "    rxThread 1\n"	// I/O Rx
      "    txThread 2\n"	// I/O Tx
      "    isolation 1-3\n"	// leave thread 4 for general purpose
      "  }\n"
      "}\n"
      "record {\n"
      "  rxThread 3\n"		// Record Rx - must be distinct from I/O Rx
      "  snapThread 4\n"	// Record snapshot - must be distinct from Rx
      "}\n"
      "replay {\n"
      "  rxThread 4\n"
      "}\n",
      false);
  }

  try {
    if (ZmRef<ZvCf> logCf = cf->subset("log", false)) {
      ZeLog::init("MxMD");
      ZeLog::level(logCf->getInt("level", 0, 5, false, 0));
      ZeLog::add(ZeLog::fileSink(
	  logCf->get("file"),
	  logCf->getInt("age", 0, INT_MAX, false, 8),
	  logCf->getInt("offset", INT_MIN, INT_MAX, false, 0)));
    }

    if (ZmRef<ZvCf> heapCf = cf->subset("heap", false)) {
      ZeLOG(Info, "MxMDLib - configuring heap...");
      ZvHeapMgrCf::init(heapCf);
    }

    {
      using MxTbl = MxMDCore::MxTbl;
      using Mx = MxMDCore::Mx;

      ZmRef<MxTbl> mxTbl = new MxTbl();

      if (ZmRef<ZvCf> mxCf = cf->subset("mx", false, true)) {
	ZvCf::Iterator i(mxCf);
	ZuString key;
	while (ZmRef<ZvCf> mxCf_ = i.subset(key))
	  mxTbl->add(new Mx(key, mxCf_));
      }

      Mx *coreMx;
      {
	MxTbl::Node *node;
	if (!(node = mxTbl->find(Mx::ID("core"))))
	  throw ZvCf::Required("mx:core");
	coreMx = node->key();
      }

      ZeLOG(Info, "starting multiplexers...");
      {
	bool failed = false;
	{
	  auto i = mxTbl->readIterator();
	  while (MxTbl::Node *node = i.iterate()) {
	    Mx *mx = node->key();
	    if (schedInitFn) schedInitFn(mx);
	    if (mx->start() != Zi::OK) {
	      failed = true;
	      ZeLOG(Fatal, ZtString() << node->key()->id() <<
		  " multiplexer start failed");
	      break;
	    }
	  }
	}
	if (failed) {
	  {
	    auto i = mxTbl->readIterator();
	    while (MxTbl::Node *node = i.iterate())
	      node->key()->stop(false);
	  }
	  return md = nullptr;
	}
      }

      md = new MxMDCore(ZuMv(mxTbl), coreMx);
    }

    md->init_(cf);

  } catch (const ZvError &e) {
    ZeLOG(Fatal, ZtString() << "MxMDLib - configuration error: " << e);
    return md = nullptr;
  } catch (const ZtString &e) {
    ZeLOG(Fatal, ZtString() << "MxMDLib - error: " << e);
    return md = nullptr;
  } catch (const ZeError &e) {
    ZeLOG(Fatal, ZtString() << "MxMDLib - error: " << e);
    return md = nullptr;
  } catch (...) {
    ZeLOG(Fatal, "MxMDLib - unknown exception during init");
    return md = nullptr;
  }

  return ZmSingleton<MxMDCore, 0>::instance(md);
}

MxMDLib *MxMDLib::instance()
{
  return ZmSingleton<MxMDCore, false>::instance();
}

MxMDCore::MxMDCore(ZmRef<MxTbl> mxTbl, Mx *mx) :
  MxMDLib(mx), m_mxTbl(ZuMv(mxTbl)), m_mx(mx)
{
}

void MxMDCore::init_(ZvCf *cf)
{
  m_cf = cf;

  MxMDLib::init_(cf);

  m_localFeed = new MxMDFeed(this, "_LOCAL", 3);
  addFeed(m_localFeed);

  if (ZmRef<ZvCf> feedsCf = cf->subset("feeds", false)) {
    ZeLOG(Info, "MxMDLib - configuring feeds...");
    ZvCf::Iterator i(feedsCf);
    ZuString key;
    while (ZmRef<ZvCf> feedCf = i.subset(key)) {
      if (key == "_LOCAL") {
	ZvCf::Iterator j(feedCf);
	ZuString id;
	while (ZmRef<ZvCf> venueCf = j.subset(id))
	  addVenue(new MxMDVenue(this, m_localFeed, id,
	      venueCf->getEnum<MxMDOrderIDScope::Map>("orderIDScope", false),
	      venueCf->getFlags<MxMDVenueFlags::Flags>("flags", false, 0)));
	continue;
      }
      ZtString e;
      ZiModule module;
      ZiModule::Path name = feedCf->get("module", true);
      int preload = feedCf->getInt("preload", 0, 1, false, 0);
      if (preload) preload = ZiModule::Pre;
      if (module.load(name, preload, &e) < 0)
	throw ZtString() << "failed to load \"" << name << "\": " << ZuMv(e);
      MxMDFeedPluginFn pluginFn =
	(MxMDFeedPluginFn)module.resolve("MxMDFeed_plugin", &e);
      if (!pluginFn) {
	module.unload();
	throw ZtString() <<
	  "failed to resolve \"MxMDFeed_plugin\" in \"" <<
	  name << "\": " << ZuMv(e);
      }
      (*pluginFn)(this, feedCf);
    }
  }

  if (ZtString venueMap = cf->get("venueMap")) {
    MxMDVenueMapCSV csv;
    csv.read(venueMap,
	ZvCSVReadFn::Member<&MxMDCore::addVenueMapping_>::fn(this));
  }

  if (const ZtArray<ZtString> *tickSizes =
	cf->getMultiple("tickSizes", 0, INT_MAX)) {
    ZeLOG(Info, "MxMDLib - reading tick size data...");
    MxMDTickSizeCSV csv;
    for (unsigned i = 0, n = tickSizes->length(); i < n; i++)
      csv.read((*tickSizes)[i],
	  ZvCSVReadFn::Member<&MxMDCore::addTickSize_>::fn(this));
  }
  if (const ZtArray<ZtString> *instruments =
	cf->getMultiple("instruments", 0, INT_MAX)) {
    ZeLOG(Info, "MxMDLib - reading instrument reference data...");
    MxMDInstrumentCSV csv;
    for (unsigned i = 0, n = instruments->length(); i < n; i++)
      csv.read((*instruments)[i],
	  ZvCSVReadFn::Member<&MxMDCore::addInstrument_>::fn(this));
  }
  if (const ZtArray<ZtString> *orderBooks =
	cf->getMultiple("orderBooks", 0, INT_MAX)) {
    ZeLOG(Info, "MxMDLib - reading order book reference data...");
    MxMDOrderBookCSV csv;
    for (unsigned i = 0, n = orderBooks->length(); i < n; i++)
      csv.read((*orderBooks)[i],
	  ZvCSVReadFn::Member<&MxMDCore::addOrderBook_>::fn(this));
  }

  m_broadcast.init(this);

  if (ZmRef<ZvCf> telCf = cf->subset("telemetry", false)) {
    m_telemetry = new MxMDTelemetry();
    m_telemetry->init(this, telCf);
  }

  if (ZmRef<ZvCf> cmdCf = cf->subset("cmd", false)) {
    m_cmd = new MxMDCmd();
    Mx *mx = this->mx(cmdCf->get("mx", false, "cmd"));
    if (!mx) throw ZvCf::Required("cmd:mx");
    m_cmd->init(mx, cmdCf);
    initCmds();
  }

  m_record = new MxMDRecord();
  m_record->init(this, cf->subset("record", false, true));
  m_replay = new MxMDReplay();
  m_replay->init(this, cf->subset("replay", false));

  if (ZmRef<ZvCf> publisherCf = cf->subset("publisher", false)) {
    m_publisher = new MxMDPublisher();
    m_publisher->init(this, publisherCf);
  }
  if (ZmRef<ZvCf> subscriberCf = cf->subset("subscriber", false)) {
    m_subscriber = new MxMDSubscriber();
    m_subscriber->init(this, subscriberCf);
  }

  ZeLOG(Info, "MxMDLib - initialized...");
}

void MxMDCore::addCmd(ZuString name, ZuString syntax,
  ZvCmdFn fn, ZtString brief, ZtString usage)
{
  if (!m_cmd) return;
  m_cmd->addCmd(name, syntax, ZuMv(fn), ZuMv(brief), ZuMv(usage));
}

void MxMDCore::initCmds()
{
  addCmd(
      "l1", ZtString("c csv csv { type flag }\n") + lookupSyntax(),
      ZvCmdFn::Member<&MxMDCore::l1>::fn(this),
      "dump L1 data",
      ZtString("usage: l1 OPTIONS SYMBOL [SYMBOL...]\n"
	"Display level 1 market data for SYMBOL(s)\n\nOptions:\n"
	"    -c, --csv\t\toutput CSV format\n") <<
	lookupOptions());
  addCmd(
      "l2", lookupSyntax(),
      ZvCmdFn::Member<&MxMDCore::l2>::fn(this),
      "dump L2 data",
      ZtString("usage: l2 OPTIONS SYMBOL\n"
	"Display level 2 market data for SYMBOL\n\nOptions:\n") <<
	lookupOptions());
  addCmd(
      "instrument", lookupSyntax(),
      ZvCmdFn::Member<&MxMDCore::instrument_>::fn(this),
      "dump instrument reference data",
      ZtString("usage: instrument OPTIONS SYMBOL\n"
	"Display instrument reference data (\"static data\") for SYMBOL\n"
	"\nOptions:\n") << lookupOptions());
  addCmd(
      "ticksizes", "",
      ZvCmdFn::Member<&MxMDCore::ticksizes>::fn(this),
      "dump ticksizes in CSV format",
      "usage: ticksizes [VENUE [SEGMENT]]\n"
      "dump tick sizes in CSV format");
  addCmd(
      "instruments", "",
      ZvCmdFn::Member<&MxMDCore::instruments>::fn(this),
      "dump instruments in CSV format",
      "usage: instruments [VENUE [SEGMENT]]\n"
      "dump instruments in CSV format");
  addCmd(
      "orderbooks", "",
      ZvCmdFn::Member<&MxMDCore::orderbooks>::fn(this),
      "dump order books in CSV format",
      "usage: orderbooks [VENUE [SEGMENT]]\n"
      "dump order books in CSV format");
#if 0
  addCmd(
      "subscribe",
      "s stop stop { type scalar }",
      ZvCmdFn::Member<&MxMDCore::subscribeCmd>::fn(this),
      "subscribe to market data",
      "usage: subscribe IPCRING\n"
      "       subscribe -s ID\n"
      "subscribe to market data, receiving snapshot via ring buffer IPCRING\n"
      "Options:\n"
      "-s, --stop=ID\tstop subscribing - detach subscriber ID\n");
#endif
}

void MxMDCore::start()
{
  Guard guard(m_stateLock);

  if (m_telemetry) {
    raise(ZeEVENT(Info, "starting telemetry..."));
    m_telemetry->start();
  }

  if (m_cmd) {
    raise(ZeEVENT(Info, "starting cmd server..."));
    m_cmd->start();
  }

  if (m_publisher) {
    raise(ZeEVENT(Info, "starting publisher..."));
    m_publisher->start();
  }
  if (m_subscriber) {
    raise(ZeEVENT(Info, "starting subscriber..."));
    m_subscriber->start();
  }

  raise(ZeEVENT(Info, "starting feeds..."));
  allFeeds([](MxMDFeed *feed) { try { feed->start(); } catch (...) { } });
}

void MxMDCore::stop()
{
  Guard guard(m_stateLock);

  raise(ZeEVENT(Info, "stopping feeds..."));
  allFeeds([](MxMDFeed *feed) { try { feed->stop(); } catch (...) { } });

  if (m_subscriber) {
    raise(ZeEVENT(Info, "stopping subscriber..."));
    m_subscriber->stop();
    // wait for stop() to complete
    thread_local ZmSemaphore sem;
    m_subscriber->rxInvoke([sem = &sem]() { sem->post(); });
    sem.wait();
  }
  if (m_publisher) {
    raise(ZeEVENT(Info, "stopping publisher..."));
    m_publisher->stop();
    // wait for stop() to complete
    thread_local ZmSemaphore sem;
    m_publisher->rxInvoke([sem = &sem]() { sem->post(); });
    sem.wait();
  }

  stopReplaying();
  stopRecording();

  if (m_cmd) {
    raise(ZeEVENT(Info, "stopping command server..."));
    m_cmd->stop();
  }

  if (m_telemetry) {
    raise(ZeEVENT(Info, "stopping telemetry..."));
    m_telemetry->stop();
  }

  raise(ZeEVENT(Info, "stopping multiplexers..."));
  m_mx->stop(false);
}

void MxMDCore::final()
{
  raise(ZeEVENT(Info, "finalizing cmd server..."));

  if (m_cmd) m_cmd->final();

  raise(ZeEVENT(Info, "finalizing feeds..."));
  allFeeds([](MxMDFeed *feed) { try { feed->final(); } catch (...) { } });
}

void MxMDCore::l1(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg)
{
  unsigned argc = ZuBox<unsigned>(args->get("#"));
  if (argc < 2) throw ZvCmdUsage();
  outMsg = new ZvCmdMsg();
  auto &out = outMsg->cmd();
  bool csv = !!args->get("csv");
  if (csv)
    out << "stamp,status,last,lastQty,bid,bidQty,ask,askQty,tickDir,"
      "high,low,accVol,accVolQty,match,matchQty,surplusQty,flags\n";
  for (unsigned i = 1; i < argc; i++) {
    MxUniKey key = parseOrderBook(args, i);
    lookupOrderBook(key, 1, 1,
	[this, &out, csv](MxMDInstrument *, MxMDOrderBook *ob) -> bool {
      const MxMDL1Data &l1Data = ob->l1Data();
      MxNDP pxNDP = l1Data.pxNDP;
      MxNDP qtyNDP = l1Data.qtyNDP;
      MxMDFlagsStr flags;
      MxMDL1Flags::print(flags, ob->venueID(), l1Data.flags);
      if (csv) out <<
	timeFmt(l1Data.stamp) << ',' <<
	MxTradingStatus::name(l1Data.status) << ',' <<
	MxValNDP{l1Data.last, pxNDP} << ',' <<
	MxValNDP{l1Data.lastQty, qtyNDP} << ',' <<
	MxValNDP{l1Data.bid, pxNDP} << ',' <<
	MxValNDP{l1Data.bidQty, qtyNDP} << ',' <<
	MxValNDP{l1Data.ask, pxNDP} << ',' <<
	MxValNDP{l1Data.askQty, qtyNDP} << ',' <<
	MxTickDir::name(l1Data.tickDir) << ',' <<
	MxValNDP{l1Data.high, pxNDP} << ',' <<
	MxValNDP{l1Data.low, pxNDP} << ',' <<
	MxValNDP{l1Data.accVol, pxNDP} << ',' <<
	MxValNDP{l1Data.accVolQty, qtyNDP} << ',' <<
	MxValNDP{l1Data.match, pxNDP} << ',' <<
	MxValNDP{l1Data.matchQty, qtyNDP} << ',' <<
	MxValNDP{l1Data.surplusQty, qtyNDP} << ',' <<
	flags << '\n';
      else out <<
	"stamp: " << timeFmt(l1Data.stamp) <<
	"\nstatus: " << MxTradingStatus::name(l1Data.status) <<
	"\nlast: " << MxValNDP{l1Data.last, pxNDP} <<
	"\nlastQty: " << MxValNDP{l1Data.lastQty, qtyNDP} <<
	"\nbid: " << MxValNDP{l1Data.bid, pxNDP} <<
	"\nbidQty: " << MxValNDP{l1Data.bidQty, qtyNDP} <<
	"\nask: " << MxValNDP{l1Data.ask, pxNDP} <<
	"\naskQty: " << MxValNDP{l1Data.askQty, qtyNDP} <<
	"\ntickDir: " << MxTickDir::name(l1Data.tickDir) <<
	"\nhigh: " << MxValNDP{l1Data.high, pxNDP} <<
	"\nlow: " << MxValNDP{l1Data.low, pxNDP} <<
	"\naccVol: " << MxValNDP{l1Data.accVol, pxNDP} <<
	"\naccVolQty: " << MxValNDP{l1Data.accVolQty, qtyNDP} <<
	"\nmatch: " << MxValNDP{l1Data.match, pxNDP} <<
	"\nmatchQty: " << MxValNDP{l1Data.matchQty, qtyNDP} <<
	"\nsurplusQty: " << MxValNDP{l1Data.surplusQty, qtyNDP} <<
	"\nflags: " << flags << '\n';
      return true;
    });
  }
}

void MxMDCore::l2(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg)
{
  ZuBox<int> argc = args->get("#");
  if (argc != 2) throw ZvCmdUsage();
  outMsg = new ZvCmdMsg();
  auto &out = outMsg->cmd();
  MxUniKey key = parseOrderBook(args, 1);
  lookupOrderBook(key, 1, 1,
      [this, &out](MxMDInstrument *, MxMDOrderBook *ob) -> bool {
    out << "bids:\n";
    l2_side(ob->bids(), out);
    out << "\nasks:\n";
    l2_side(ob->asks(), out);
    out << '\n';
    return true;
  });
}

void MxMDCore::l2_side(MxMDOBSide *side, ZtString &out)
{
  out << "  vwap: " << side->vwap();
  MxMDOrderBook *ob = side->orderBook();
  MxID venueID = ob->venueID();
  side->allPxLevels([this, venueID,
      pxNDP = ob->pxNDP(), qtyNDP = ob->qtyNDP(), &out](
	MxMDPxLevel *pxLevel) -> uintptr_t {
    const MxMDPxLvlData &pxLvlData = pxLevel->data();
    MxMDFlagsStr flags;
    MxMDL2Flags::print(flags, venueID, pxLvlData.flags);
    out << "\n    price: " << MxValNDP{pxLevel->price(), pxNDP} <<
      " qty: " << MxValNDP{pxLvlData.qty, qtyNDP} <<
      " nOrders: " << pxLvlData.nOrders;
    if (flags) out << " flags: " << flags;
    out << " transactTime: " << timeFmt(pxLvlData.transactTime);
    return 0;
  });
}

void MxMDCore::instrument_(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg)
{
  ZuBox<int> argc = args->get("#");
  if (argc != 2) throw ZvCmdUsage();
  outMsg = new ZvCmdMsg();
  auto &out = outMsg->cmd();
  MxUniKey key = parseOrderBook(args, 1);
  lookupOrderBook(key, 1, 0,
      [&out](MxMDInstrument *instr, MxMDOrderBook *ob) -> bool {
    const MxMDInstrRefData &refData = instr->refData();
    out <<
      "ID: " << instr->id() <<
      "\nIDSrc: " << refData.idSrc <<
      "\nsymbol: " << refData.symbol <<
      "\naltIDSrc: " << refData.altIDSrc <<
      "\naltSymbol: " << refData.altSymbol;
    if (refData.underVenue) out << "\nunderlying: " <<
      MxInstrKey{refData.underlying, refData.underVenue, refData.underSegment};
    if (*refData.mat) {
      out << "\nmat: " << refData.mat;
      if (*refData.putCall) out <<
	  "\nputCall: " << MxPutCall::name(refData.putCall) <<
	  "\nstrike: " << MxValNDP{refData.strike, refData.pxNDP};
    }
    if (*refData.outstandingShares)
      out << "\noutstandingShares: " <<
	MxValNDP{refData.outstandingShares, refData.qtyNDP};
    if (*refData.adv)
      out << "\nADV: " << MxValNDP{refData.adv, refData.pxNDP};
    if (ob) {
      const MxMDLotSizes &lotSizes = ob->lotSizes();
      out <<
	"\nmarket: " << ob->venueID() <<
	"\nsegment: " << ob->segment() <<
	"\nID: " << ob->id() <<
	"\nlot sizes: " <<
	  MxValNDP{lotSizes.oddLotSize, refData.qtyNDP} << ',' <<
	  MxValNDP{lotSizes.lotSize, refData.qtyNDP} << ',' <<
	  MxValNDP{lotSizes.blockLotSize, refData.qtyNDP} <<
	"\ntick sizes:";
      ob->tickSizeTbl()->allTickSizes(
	  [pxNDP = refData.pxNDP, &out](const MxMDTickSize &ts) -> uintptr_t {
	out << "\n  " <<
	  MxValNDP{ts.minPrice(), pxNDP} << '-' <<
	  MxValNDP{ts.maxPrice(), pxNDP} << ' ' <<
	  MxValNDP{ts.tickSize(), pxNDP};
	return 0;
      });
    }
    out << '\n';
    return true;
  });
}

static void writeTickSizes(
    const MxMDLib *md, MxMDTickSizeCSV &csv, ZvCSVWriteFn fn, MxID venueID)
{
  ZmFn<MxMDVenue *> venueFn{
      [&csv, &fn](MxMDVenue *venue) -> uintptr_t {
    return venue->allTickSizeTbls(
	[&csv, &fn, venue](MxMDTickSizeTbl *tbl) -> uintptr_t {
      return tbl->allTickSizes(
	  [&csv, &fn, venue, tbl](const MxMDTickSize &ts) -> uintptr_t {
	new (csv.ptr()) MxMDTickSizeCSV::Data{MxEnum(),
	  venue->id(), tbl->id(), tbl->pxNDP(),
	  ts.minPrice(), ts.maxPrice(), ts.tickSize() };
	fn(csv.pod());
	return 0;
      });
    });
  }};
  if (!*venueID)
    md->allVenues(venueFn);
  else
    if (ZmRef<MxMDVenue> venue = md->venue(venueID)) venueFn(venue);
  fn((ZuAnyPOD *)0);
}

void MxMDCore::dumpTickSizes(ZuString path, MxID venueID)
{
  MxMDTickSizeCSV csv;
  writeTickSizes(this, csv, csv.writeFile(path), venueID);
}

void MxMDCore::ticksizes(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg)
{
  ZuBox<int> argc = args->get("#");
  if (argc < 1 || argc > 3) throw ZvCmdUsage();
  outMsg = new ZvCmdMsg();
  auto &out = outMsg->data();
  MxID venueID;
  if (argc == 2) venueID = args->get("1");
  MxMDTickSizeCSV csv;
  writeTickSizes(this, csv, csv.writeData(out), venueID);
}

static void writeInstruments(
    const MxMDLib *md, MxMDInstrumentCSV &csv, ZvCSVWriteFn fn,
    MxID venueID, MxID segment)
{
  md->allInstruments(
      [&csv, &fn, venueID, segment](MxMDInstrument *instr) -> uintptr_t {
    if ((!*venueID || venueID == instr->primaryVenue()) &&
	(!*segment || segment == instr->primarySegment())) {
      new (csv.ptr()) MxMDInstrumentCSV::Data{
	instr->shard()->id(), MxMDStream::Type::AddInstrument, MxDateTime(),
	instr->primaryVenue(), instr->primarySegment(),
	instr->id(), instr->refData() };
      fn(csv.pod());
    }
    return 0;
  });
  fn((ZuAnyPOD *)0);
}

void MxMDCore::dumpInstruments(ZuString path, MxID venueID, MxID segment)
{
  MxMDInstrumentCSV csv;
  writeInstruments(this, csv, csv.writeFile(path), venueID, segment);
}

void MxMDCore::instruments(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg)
{
  ZuBox<int> argc = args->get("#");
  if (argc < 1 || argc > 3) throw ZvCmdUsage();
  outMsg = new ZvCmdMsg();
  auto &out = outMsg->data();
  MxID venueID, segment;
  if (argc == 2) venueID = args->get("1");
  if (argc == 3) segment = args->get("2");
  MxMDInstrumentCSV csv;
  writeInstruments(this, csv, csv.writeData(out), venueID, segment);
}

static void writeOrderBooks(
    const MxMDLib *md, MxMDOrderBookCSV &csv, ZvCSVWriteFn fn,
    MxID venueID, MxID segment)
{
  md->allOrderBooks(
      [&csv, &fn, venueID, segment](MxMDOrderBook *ob) -> uintptr_t {
    if ((!*venueID || venueID == ob->venueID()) &&
	(!*segment || segment == ob->segment())) {
      MxMDOrderBookCSV::Data *data =
	new (csv.ptr()) MxMDOrderBookCSV::Data{
	  ob->shard()->id(), MxMDStream::Type::AddOrderBook, MxDateTime(),
	  ob->venueID(), ob->segment(), ob->id(),
	  ob->pxNDP(), ob->qtyNDP(),
	  ob->legs(), ob->tickSizeTbl()->id(), ob->lotSizes() };
      for (unsigned i = 0, n = ob->legs(); i < n; i++) {
	MxMDInstrument *instr;
	if (!(instr = ob->instrument(i))) break;
	data->instrVenues[i] = instr->primaryVenue();
	data->instrSegments[i] = instr->primarySegment();
	data->instruments[i] = instr->id();
	data->sides[i] = ob->side(i);
	data->ratios[i] = ob->ratio(i);
      }
      fn(csv.pod());
    }
    return 0;
  });
  fn((ZuAnyPOD *)0);
}

void MxMDCore::dumpOrderBooks(ZuString path, MxID venueID, MxID segment)
{
  MxMDOrderBookCSV csv;
  writeOrderBooks(this, csv, csv.writeFile(path), venueID, segment);
}

void MxMDCore::orderbooks(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg)
{
  ZuBox<int> argc = args->get("#");
  if (argc < 1 || argc > 3) throw ZvCmdUsage();
  outMsg = new ZvCmdMsg();
  auto &out = outMsg->data();
  MxID venueID, segment;
  if (argc == 2) venueID = args->get("1");
  if (argc == 3) segment = args->get("2");
  MxMDOrderBookCSV csv;
  writeOrderBooks(this, csv, csv.writeData(out), venueID, segment);
}

bool MxMDCore::record(ZuString path)
{
  return m_record->record(path);
}
ZtString MxMDCore::stopRecording()
{
  return m_record->stopRecording();
}

bool MxMDCore::replay(ZuString path, MxDateTime begin, bool filter)
{
  m_mx->del(&m_timer);
  return m_replay->replay(path, begin, filter);
}
ZtString MxMDCore::stopReplaying()
{
  return m_replay->stopReplaying();
}

#if 0
void MxMDCore::subscribeCmd(ZvCf *args, ZtArray<char> &out)
{
  ZuBox<int> argc = args->get("#");
  if (ZtString id_ = args->get("stop")) {
    ZuBox<int> id = ZvCf::toInt("spin", id_, 0, 63);
    MxMDStream::detach(m_broadcast, m_broadcast.id());
    m_broadcast.close();
    out << "detached " << id << "\n";
    return;
  }
  if (argc != 2) throw ZvCmdUsage();
  ZiRingParams ringParams(args->get("1"), m_broadcast.params());
  if (ZtString spin = args->get("spin"))
    ringParams.spin(ZvCf::toInt("spin", spin, 0, INT_MAX));
  if (ZtString timeout = args->get("timeout"))
    ringParams.timeout(ZvCf::toInt("timeout", timeout, 0, 3600));
  if (!m_broadcast.open())
    throw ZtString() << '"' << m_broadcast.params().name() <<
	"\": failed to open IPC shared memory ring buffer";
  m_snapper.snap(ringParams);
}
#endif

void MxMDCore::pad(Hdr &hdr)
{
  using namespace MxMDStream;

  switch ((int)hdr.type) {
    case Type::AddTickSizeTbl:	hdr.pad<AddTickSizeTbl>(); break;
    case Type::ResetTickSizeTbl: hdr.pad<ResetTickSizeTbl>(); break;
    case Type::AddTickSize:	hdr.pad<AddTickSize>(); break;
    case Type::AddInstrument:	hdr.pad<AddInstrument>(); break;
    case Type::UpdateInstrument:	hdr.pad<UpdateInstrument>(); break;
    case Type::AddOrderBook:	hdr.pad<AddOrderBook>(); break;
    case Type::DelOrderBook:	hdr.pad<DelOrderBook>(); break;
    case Type::AddCombination:	hdr.pad<AddCombination>(); break;
    case Type::DelCombination:	hdr.pad<DelCombination>(); break;
    case Type::UpdateOrderBook:	hdr.pad<UpdateOrderBook>(); break;
    case Type::TradingSession:	hdr.pad<TradingSession>(); break;
    case Type::L1:		hdr.pad<L1>(); break;
    case Type::PxLevel:		hdr.pad<PxLevel>(); break;
    case Type::L2:		hdr.pad<L2>(); break;
    case Type::AddOrder:	hdr.pad<AddOrder>(); break;
    case Type::ModifyOrder:	hdr.pad<ModifyOrder>(); break;
    case Type::CancelOrder:	hdr.pad<CancelOrder>(); break;
    case Type::ResetOB:		hdr.pad<ResetOB>(); break;
    case Type::AddTrade:	hdr.pad<AddTrade>(); break;
    case Type::CorrectTrade:	hdr.pad<CorrectTrade>(); break;
    case Type::CancelTrade:	hdr.pad<CancelTrade>(); break;
    case Type::RefDataLoaded:	hdr.pad<RefDataLoaded>(); break;
    case Type::AddVenue:	hdr.pad<AddVenue>(); break;
    default: break;
  }
}

void MxMDCore::apply(const Hdr &hdr, bool filter)
{
  using namespace MxMDStream;

  switch ((int)hdr.type) {
    case Type::AddTickSizeTbl:
      {
	const AddTickSizeTbl &obj = hdr.as<AddTickSizeTbl>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  venue->addTickSizeTbl(obj.id, obj.pxNDP);
      }
      break;
    case Type::ResetTickSizeTbl:
      {
	const ResetTickSizeTbl &obj = hdr.as<ResetTickSizeTbl>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  if (ZmRef<MxMDTickSizeTbl> tbl = venue->tickSizeTbl(obj.id))
	    tbl->reset();
      }
      break;
    case Type::AddTickSize:
      {
	const AddTickSize &obj = hdr.as<AddTickSize>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  if (ZmRef<MxMDTickSizeTbl> tbl = venue->tickSizeTbl(obj.id)) {
	    if (ZuUnlikely(tbl->pxNDP() != obj.pxNDP)) {
	      MxNDP oldNDP = obj.pxNDP, newNDP = tbl->pxNDP();
#ifdef adjustNDP
#undef adjustNDP
#endif
#define adjustNDP(v) (MxValNDP{v, oldNDP}.adjust(newNDP))
	      tbl->addTickSize(
		adjustNDP(obj.minPrice),
		adjustNDP(obj.maxPrice),
		adjustNDP(obj.tickSize));
#undef adjustNDP
	    } else
	      tbl->addTickSize(obj.minPrice, obj.maxPrice, obj.tickSize);
	  }
      }
      break;
    case Type::TradingSession:
      {
	const TradingSession &obj = hdr.as<TradingSession>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  venue->tradingSession(
	      MxMDSegment{obj.segment, obj.session, obj.stamp});
      }
      break;
    case Type::AddInstrument:
      if (hdr.shard < nShards()) {
	const AddInstrument &obj = hdr.as<AddInstrument>();
	shard(hdr.shard, [obj = obj](MxMDShard *shard) {
	  shard->addInstrument(shard->instrument(obj.key),
	      obj.key, obj.refData, obj.transactTime);
	});
      }
      break;
    case Type::UpdateInstrument:
      if (hdr.shard < nShards()) {
	const UpdateInstrument &obj = hdr.as<UpdateInstrument>();
	shard(hdr.shard, [obj = obj](MxMDShard *shard) {
	  if (ZmRef<MxMDInstrument> instr = shard->instrument(obj.key))
	    instr->update(obj.refData, obj.transactTime);
	});
      }
      break;
    case Type::AddOrderBook:
      if (hdr.shard < nShards()) {
	const AddOrderBook &obj = hdr.as<AddOrderBook>();
	shard(hdr.shard, [obj = obj](MxMDShard *shard) mutable {
	  if (ZmRef<MxMDVenue> venue = shard->md()->venue(obj.key.venue))
	    if (ZmRef<MxMDTickSizeTbl> tbl =
		    venue->tickSizeTbl(obj.tickSizeTbl))
	      if (ZmRef<MxMDInstrument> instr = shard->instrument(obj.instrument)) {
		if (ZuUnlikely(instr->refData().qtyNDP != obj.qtyNDP)) {
		  MxNDP newNDP = instr->refData().qtyNDP;
#ifdef adjustNDP
#undef adjustNDP
#endif
#define adjustNDP(v) v = MxValNDP{v, obj.qtyNDP}.adjust(newNDP)
		  adjustNDP(obj.lotSizes.oddLotSize);
		  adjustNDP(obj.lotSizes.lotSize);
		  adjustNDP(obj.lotSizes.blockLotSize);
#undef adjustNDP
		  obj.qtyNDP = newNDP;
		}
		instr->addOrderBook(obj.key, tbl, obj.lotSizes, obj.transactTime);
	      }
	});
      }
      break;
    case Type::DelOrderBook:
      if (hdr.shard < nShards()) {
	const DelOrderBook &obj = hdr.as<DelOrderBook>();
	shard(hdr.shard, [obj = obj](MxMDShard *shard) {
	  if (ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key))
	    ob->instrument()->delOrderBook(
	      obj.key.venue, obj.key.segment, obj.transactTime);
	});
      }
      break;
    case Type::AddCombination:
      if (hdr.shard < nShards()) {
	const AddCombination &obj = hdr.as<AddCombination>();
	shard(hdr.shard, [obj = obj](MxMDShard *shard) {
	  if (ZmRef<MxMDVenue> venue = shard->md()->venue(obj.key.venue))
	    if (ZmRef<MxMDTickSizeTbl> tbl =
		venue->tickSizeTbl(obj.tickSizeTbl)) {
	    ZmRef<MxMDInstrument> instruments[MxMDNLegs];
	    for (unsigned i = 0; i < obj.legs; i++)
	      if (!(instruments[i] = shard->instrument(obj.instruments[i])))
		return;
	    venue->shard(shard)->addCombination(
		obj.key.segment, obj.key.id,
		obj.pxNDP, obj.qtyNDP,
		obj.legs, instruments, obj.sides, obj.ratios,
		tbl, obj.lotSizes, obj.transactTime);
	  }
	});
      }
      break;
    case Type::DelCombination:
      if (hdr.shard < nShards()) {
	const DelCombination &obj = hdr.as<DelCombination>();
	shard(hdr.shard, [obj = obj](MxMDShard *shard) {
	  if (ZmRef<MxMDVenue> venue = shard->md()->venue(obj.key.venue))
	    venue->shard(shard)->delCombination(
		obj.key.segment, obj.key.id, obj.transactTime);
	});
      }
      break;
    case Type::UpdateOrderBook:
      if (hdr.shard < nShards()) {
	const UpdateOrderBook &obj = hdr.as<UpdateOrderBook>();
	shard(hdr.shard, [obj = obj](MxMDShard *shard) {
	  if (ZmRef<MxMDVenue> venue = shard->md()->venue(obj.key.venue))
	    if (ZmRef<MxMDTickSizeTbl> tbl =
		venue->tickSizeTbl(obj.tickSizeTbl))
	      if (ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key))
		ob->update(tbl, obj.lotSizes, obj.transactTime);
	});
      }
      break;
    case Type::L1:
      if (hdr.shard < nShards()) {
	const L1 &obj = hdr.as<L1>();
	shard(hdr.shard, [obj = obj, filter](MxMDShard *shard) mutable {
	  // inconsistent NDP handled within MxMDOrderBook::l1()
	  ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key);
	  if (ob && (!filter || ob->handler())) ob->l1(obj.data);
	});
      }
      break;
    case Type::PxLevel:
      if (hdr.shard < nShards()) {
	const PxLevel &obj = hdr.as<PxLevel>();
	shard(hdr.shard, [obj = obj, filter](MxMDShard *shard) mutable {
	  ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key);
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != obj.pxNDP))
	      obj.price = MxValNDP{obj.price, obj.pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != obj.qtyNDP))
	      obj.qty = MxValNDP{obj.qty, obj.qtyNDP}.adjust(ob->qtyNDP());
	    ob->pxLevel(obj.side, obj.transactTime, obj.delta,
		obj.price, obj.qty, obj.nOrders, obj.flags);
	  }
	});
      }
      break;
    case Type::L2:
      if (hdr.shard < nShards()) {
	const L2 &obj = hdr.as<L2>();
	shard(hdr.shard, [obj = obj, filter](MxMDShard *shard) mutable {
	  ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key);
	  if (ob && (!filter || ob->handler())) ob->l2(obj.stamp, obj.updateL1);
	});
      }
      break;
    case Type::AddOrder:
      if (hdr.shard < nShards()) {
	const AddOrder &obj = hdr.as<AddOrder>();
	shard(hdr.shard, [obj = obj, filter](MxMDShard *shard) mutable {
	  ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key);
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != obj.pxNDP))
	      obj.price = MxValNDP{obj.price, obj.pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != obj.qtyNDP))
	      obj.qty = MxValNDP{obj.qty, obj.qtyNDP}.adjust(ob->qtyNDP());
	    ob->addOrder(obj.orderID, obj.transactTime,
		obj.side, obj.rank, obj.price, obj.qty, obj.flags);
	  }
	});
      }
      break;
    case Type::ModifyOrder:
      if (hdr.shard < nShards()) {
	const ModifyOrder &obj = hdr.as<ModifyOrder>();
	shard(hdr.shard, [obj = obj, filter](MxMDShard *shard) mutable {
	  ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key);
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != obj.pxNDP))
	      obj.price = MxValNDP{obj.price, obj.pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != obj.qtyNDP))
	      obj.qty = MxValNDP{obj.qty, obj.qtyNDP}.adjust(ob->qtyNDP());
	    ob->modifyOrder(obj.orderID, obj.transactTime,
		obj.side, obj.rank, obj.price, obj.qty, obj.flags);
	  }
	});
      }
      break;
    case Type::CancelOrder:
      if (hdr.shard < nShards()) {
	const CancelOrder &obj = hdr.as<CancelOrder>();
	shard(hdr.shard, [obj = obj, filter](MxMDShard *shard) mutable {
	  ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key);
	  if (ob && (!filter || ob->handler()))
	    ob->cancelOrder(obj.orderID, obj.transactTime, obj.side);
	});
      }
      break;
    case Type::ResetOB:
      if (hdr.shard < nShards()) {
	const ResetOB &obj = hdr.as<ResetOB>();
	shard(hdr.shard, [obj = obj, filter](MxMDShard *shard) mutable {
	  ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key);
	  if (ob && (!filter || ob->handler()))
	    ob->reset(obj.transactTime);
	});
      }
      break;
    case Type::AddTrade:
      if (hdr.shard < nShards()) {
	const AddTrade &obj = hdr.as<AddTrade>();
	shard(hdr.shard, [obj = obj, filter](MxMDShard *shard) mutable {
	  ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key);
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != obj.pxNDP))
	      obj.price = MxValNDP{obj.price, obj.pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != obj.qtyNDP))
	      obj.qty = MxValNDP{obj.qty, obj.qtyNDP}.adjust(ob->qtyNDP());
	    ob->addTrade(obj.tradeID, obj.transactTime, obj.price, obj.qty);
	  }
	});
      }
      break;
    case Type::CorrectTrade:
      if (hdr.shard < nShards()) {
	const CorrectTrade &obj = hdr.as<CorrectTrade>();
	shard(hdr.shard, [obj = obj, filter](MxMDShard *shard) mutable {
	  ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key);
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != obj.pxNDP))
	      obj.price = MxValNDP{obj.price, obj.pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != obj.qtyNDP))
	      obj.qty = MxValNDP{obj.qty, obj.qtyNDP}.adjust(ob->qtyNDP());
	    ob->correctTrade(obj.tradeID, obj.transactTime, obj.price, obj.qty);
	  }
	});
      }
      break;
    case Type::CancelTrade:
      if (hdr.shard < nShards()) {
	const CancelTrade &obj = hdr.as<CancelTrade>();
	shard(hdr.shard, [obj = obj, filter](MxMDShard *shard) mutable {
	  ZmRef<MxMDOrderBook> ob = shard->orderBook(obj.key);
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != obj.pxNDP))
	      obj.price = MxValNDP{obj.price, obj.pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != obj.qtyNDP))
	      obj.qty = MxValNDP{obj.qty, obj.qtyNDP}.adjust(ob->qtyNDP());
	    ob->cancelTrade(obj.tradeID, obj.transactTime, obj.price, obj.qty);
	  }
	});
      }
      break;
    case Type::AddVenue:
      {
	const AddVenue &obj = hdr.as<AddVenue>();
	this->venue(obj.venue, obj.orderIDScope, obj.flags);
      }
      break;
    case Type::RefDataLoaded:
      {
	const RefDataLoaded &obj = hdr.as<RefDataLoaded>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  this->loaded(venue);
      }
    case Type::HeartBeat:
    case Type::Wake:
    case Type::EndOfSnapshot:
    case Type::Login:
    case Type::ResendReq:
      break;
    default:
      raise(ZeEVENT(Error, "MxMDLib - unknown message type"));
      break;
  }
}

void MxMDCore::startTimer(MxDateTime begin)
{
  ZmTime next = !begin ? ZmTimeNow() : begin.zmTime();
  {
    Guard guard(m_timerLock);
    m_timerNext = next;
  }
  m_mx->add(ZmFn<>::Member<&MxMDCore::timer>::fn(this), next, &m_timer);
}

void MxMDCore::stopTimer()
{
  m_mx->del(&m_timer);
  Guard guard(m_timerLock);
  m_timerNext = ZmTime();
}

void MxMDCore::timer()
{
  MxDateTime now, next;
  {
    Guard guard(m_timerLock);
    now = m_timerNext;
  }
  this->handler()->timer(now, next);
  {
    Guard guard(m_timerLock);
    m_timerNext = !next ? ZmTime() : next.zmTime();
  }
  if (!next)
    m_mx->del(&m_timer);
  else
    m_mx->add(ZmFn<>::Member<&MxMDCore::timer>::fn(this),
	next.zmTime(), &m_timer);
}

void MxMDTelemetry::init(MxMDCore *core, ZvCf *cf)
{
  using namespace MxTelemetry;

  MxMultiplex *mx = core->mx(cf->get("mx", false, "telemetry"));
  if (!mx) throw ZvCf::Required("telemetry:mx");
  m_core = core;
  Server::init(mx, cf);
}

void MxMDTelemetry::run(MxTelemetry::Server::Cxn *cxn)
{
  using namespace MxTelemetry;

  // heaps
  ZmHeapMgr::stats(ZmHeapMgr::StatsFn{
      [](Cxn *cxn, const ZmHeapInfo &info, const ZmHeapStats &stats) {
	cxn->transmit(heap(
	      info.id, info.config.cacheSize, info.config.cpuset.uint64(),
	      stats.cacheAllocs, stats.heapAllocs, stats.frees,
	      stats.allocated, stats.maxAllocated,
	      info.size, (uint16_t)info.partition,
	      (uint8_t)info.config.alignment));
      }, cxn});

  // threads
  ZmSpecific<ZmThreadContext>::all(ZmFn<ZmThreadContext *>{
      [](Cxn *cxn, ZmThreadContext *tc) {
	ZmThreadName name;
	tc->name(name);
	cxn->transmit(thread(
	      name, (uint64_t)tc->tid(), tc->stackSize(),
	      tc->cpuset().uint64(), tc->id(),
	      tc->priority(), (uint16_t)tc->partition(),
	      tc->main(), tc->detached()));
      }, cxn});

  // thread queues (ZmScheduler)
  m_core->allMx([cxn](MxMultiplex *mx) {
	unsigned n = mx->nThreads();
	ZmThreadName name;
	uint64_t inCount, inBytes, outCount, outBytes;
	for (unsigned tid = 1; tid <= n; tid++) {
	  mx->threadName(tid, name);
	  const ZmScheduler::Ring &ring = mx->ring(tid);
	  ring.stats(inCount, inBytes, outCount, outBytes);
	  cxn->transmit(queue(
		name, (uint64_t)0, (uint64_t)ring.count(),
		inCount, inBytes, outCount, outBytes,
		(uint32_t)ring.params().size(), (uint8_t)QueueType::Thread));
	}
      });

  // IPC queues (MD broadcast)
  if (ZmRef<MxMDBroadcast::Ring> ring = m_core->broadcast().ring()) {
    uint64_t inCount, inBytes, outCount, outBytes;
    ring->stats(inCount, inBytes, outCount, outBytes);
    int count = ring->readStatus();
    if (count < 0) count = 0;
    cxn->transmit(queue(
	  ring->params().name(), (uint64_t)0, (uint64_t)count,
	  inCount, inBytes, outCount, outBytes,
	  (uint32_t)ring->params().size(), (uint8_t)QueueType::IPC));
  }

  {
    ReadGuard guard(m_lock);

    // I/O Engines
    {
      while (ZmRef<MxEngine> engine_ = m_engines.shift()) {
	unsigned down, disabled, transient, up, reconn, failed;
        int state = engine_->state(
	    down, disabled, transient, up, reconn, failed);
	cxn->transmit(MxTelemetry::engine(
	      engine_->id(), engine_->mx()->id(),
	      down, disabled, transient, up, reconn, failed,
	      (uint16_t)engine_->nLinks(),
	      (uint8_t)engine_->rxThread(), (uint8_t)engine_->txThread(),
	      (uint8_t)state));
      }
    }
    // I/O Links
    {
      while (ZmRef<MxAnyLink> link_ = m_links.shift()) {
	unsigned reconnects;
	int state = link_->state(&reconnects);
	cxn->transmit(MxTelemetry::link(
	      link_->id(), link_->rxSeqNo(), link_->txSeqNo(),
	      reconnects, (uint8_t)state));
      }
    }
    // I/O Queues
    {
      auto i = m_queues.readIterator();
      while (Queues::Node *node = i.iterate()) {
	MxQueue *queue_ = node->val();
	const auto &key = node->key();
	uint64_t inCount, inBytes, outCount, outBytes;
	queue_->stats(inCount, inBytes, outCount, outBytes);
	cxn->transmit(queue(
	      key.p1(), queue_->head(), queue_->count(),
	      inCount, inBytes, outCount, outBytes, (uint32_t)0,
	      (uint8_t)(key.p2() ? QueueType::Tx : QueueType::Rx)));
      }
    }
  }
}

void MxMDTelemetry::engine(MxEngine *engine)
{
  Guard guard(m_lock);
  m_engines.push(engine);
}

void MxMDTelemetry::link(MxAnyLink *link)
{
  Guard guard(m_lock);
  m_links.push(link);
}

void MxMDTelemetry::addQueue(MxID id, bool tx, MxQueue *queue)
{
  auto key = ZuMkPair(id, tx);
  Guard guard(m_lock);
  if (!m_queues.find(key)) m_queues.add(key, queue);
}

void MxMDTelemetry::delQueue(MxID id, bool tx)
{
  auto key = ZuMkPair(id, tx);
  Guard guard(m_lock);
  m_queues.del(key);
}
