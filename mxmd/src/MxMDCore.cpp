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

#include <ZuUnion.hpp>

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

void MxMDCore::addSecurity_(ZuAnyPOD *pod)
{
  MxMDSecurityCSV::Data *data = (MxMDSecurityCSV::Data *)(pod->ptr());
  MxSecKey key{data->venue, data->segment, data->id};
  MxMDSecHandle secHandle = security(key, data->shard);
  secHandle.invokeMv([key, refData = data->refData,
      transactTime = data->transactTime](
	MxMDShard *shard, ZmRef<MxMDSecurity> sec) {
    shard->addSecurity(ZuMv(sec), key, refData, transactTime);
  });
}

void MxMDCore::addOrderBook_(ZuAnyPOD *pod)
{
  MxMDOrderBookCSV::Data *data = (MxMDOrderBookCSV::Data *)(pod->ptr());
  MxSecKey secKey{
    data->secVenues[0], data->secSegments[0], data->securities[0]};
  MxMDSecHandle secHandle = security(secKey);
  if (!secHandle) throw ZtString() << "unknown security: " << secKey;
  ZmRef<MxMDVenue> venue = this->venue(data->venue);
  ZmRef<MxMDTickSizeTbl> tbl =
    venue->addTickSizeTbl(data->tickSizeTbl, data->pxNDP);
  secHandle.invokeMv(
      [data = *data, venue = ZuMv(venue), tbl = ZuMv(tbl)](
	MxMDShard *shard, ZmRef<MxMDSecurity> sec) {
    if (data.legs == 1) {
      MxID venueID = data.secVenues[0];
      if (!*venueID) venueID = data.venue;
      MxID segment = data.secSegments[0];
      if (!*segment) segment = data.segment;
      MxIDString id = data.securities[0];
      if (!id) id = data.id;
      sec->addOrderBook(
	  MxSecKey{data.venue, data.segment, data.id}, tbl, data.lotSizes,
	  data.transactTime);
    } else {
      ZmRef<MxMDSecurity> securities[MxMDNLegs];
      MxEnum sides[MxMDNLegs];
      MxRatio ratios[MxMDNLegs];
      for (unsigned i = 0, n = data.legs; i < n; i++) {
	MxID venueID = data.secVenues[i];
	if (!*venueID) venueID = data.venue;
	MxID segment = data.secSegments[i];
	if (!*segment) segment = data.segment;
	MxIDString id = data.securities[i];
	if (!id) return;
	if (!i)
	  securities[i] = ZuMv(sec);
	else {
	  securities[i] = shard->security(MxSecKey{venueID, segment, id});
	  if (!securities[i]) return;
	}
	sides[i] = data.sides[i];
	ratios[i] = data.ratios[i];
      }
      venue->shard(shard)->addCombination(
	  data.segment, data.id, data.pxNDP, data.qtyNDP,
	  data.legs, securities, sides, ratios, tbl, data.lotSizes,
	  data.transactTime);
    }
  });
}

MxMDLib *MxMDLib::init(ZuString cf_, ZmFn<ZmScheduler *> schedInitFn)
{
  ZeLog::start(); // ensure error reporting works

  ZmRef<ZvCf> cf = new ZvCf();

  if (cf_)
    cf->fromFile(cf_, false);
  else {
    cf->fromString(
      "mx {\n"
	"nThreads 4\n"		// thread IDs are 1-based
	"rxThread 1\n"		// I/O Rx
	"txThread 2\n"		// I/O Tx
	"isolation 1-3\n"	// leave thread 4 for general purpose
      "}\n"
      "record {\n"
      "  rxThread 3\n"		// Record Rx - must be distinct from I/O Rx
      "  snapThread 4\n"	// Record snapshot - must be distinct from Rx
      "}\n",
      false);
  }

  ZmRef<MxMDCore> md;

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
      ZmRef<MxMDCore::Mx> mx;
      if (ZmRef<ZvCf> mxCf = cf->subset("mx", false))
	mx = new MxMDCore::Mx("mx", mxCf);
      else
	mx = new MxMDCore::Mx("mx");

      if (schedInitFn) schedInitFn(mx);

      ZeLOG(Info, "starting multiplexer...");
      if (mx->start() != Zi::OK) {
	ZeLOG(Fatal, "multiplexer start failed");
	return md = 0;
      }

      md = new MxMDCore(ZuMv(mx));
    }

    md->init_(cf);

  } catch (const ZvError &e) {
    ZeLOG(Fatal, ZtString() << "MxMDLib - configuration error: " << e);
    return md = 0;
  } catch (const ZtString &e) {
    ZeLOG(Fatal, ZtString() << "MxMDLib - error: " << e);
    return md = 0;
  } catch (const ZeError &e) {
    ZeLOG(Fatal, ZtString() << "MxMDLib - error: " << e);
    return md = 0;
  } catch (...) {
    ZeLOG(Fatal, "MxMDLib - unknown exception during init");
    return md = 0;
  }

  return ZmSingleton<MxMDCore, 0>::instance(md);
}

MxMDLib *MxMDLib::instance()
{
  return ZmSingleton<MxMDCore, false>::instance();
}

MxMDCore::MxMDCore(ZmRef<Mx> mx) : MxMDLib(mx), m_mx(ZuMv(mx))
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
  if (const ZtArray<ZtString> *securities =
	cf->getMultiple("securities", 0, INT_MAX)) {
    ZeLOG(Info, "MxMDLib - reading security reference data...");
    MxMDSecurityCSV csv;
    for (unsigned i = 0, n = securities->length(); i < n; i++)
      csv.read((*securities)[i],
	  ZvCSVReadFn::Member<&MxMDCore::addSecurity_>::fn(this));
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

  if (ZmRef<ZvCf> cmdCf = cf->subset("cmd", false)) {
    m_cmd = new MxMDCmd();
    m_cmd->init(this, cmdCf);
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
  CmdFn fn, ZtString brief, ZtString usage)
{
  if (!m_cmd) return;
  m_cmd->addCmd(name, syntax, ZuMv(fn), ZuMv(brief), ZuMv(usage));
}

ZtString MxMDCore::lookupSyntax()
{
  return 
    "S src src { type scalar } "
    "m market market { type scalar } "
    "s segment segment { type scalar }";
}

ZtString MxMDCore::lookupOptions()
{
  return
    "    -S --src=SRC\t- symbol ID source is SRC\n"
    "\t(CUSIP|SEDOL|QUIK|ISIN|RIC|EXCH|CTA|BSYM|BBGID|FX|CRYPTO)\n"
    "    -m --market=MIC\t - market MIC, e.g. XTKS\n"
    "    -s --segment=SEGMENT\t- market segment SEGMENT\n";
}

void MxMDCore::lookupSecurity(
    const CmdArgs &args, unsigned index,
    bool secRequired, ZmFn<MxMDSecurity *> fn)
{
  ZuString symbol = args.get(ZuStringN<16>(ZuBoxed(index)));
  MxID venue = args.get("market");
  MxID segment = args.get("segment");
  bool notFound = 0;
  thread_local ZmSemaphore sem;
  if (ZuString src_ = args.get("src")) {
    MxEnum src = MxSecIDSrc::lookup(src_);
    MxSecSymKey key{src, symbol};
    secInvoke(key,
	[secRequired, sem = &sem, &notFound, fn = ZuMv(fn)](MxMDSecurity *sec) {
      if (secRequired && ZuUnlikely(!sec))
	notFound = 1;
      else
	fn(sec);
      sem->post();
    });
    sem.wait();
    if (ZuUnlikely(notFound))
      throw ZtString() << "security " << key << " not found";
  } else {
    if (!*venue) throw CmdUsage();
    MxSecKey key{venue, segment, symbol};
    secInvoke(key,
	[secRequired, sem = &sem, &notFound, fn = ZuMv(fn)](MxMDSecurity *sec) {
      if (secRequired && ZuUnlikely(!sec))
	notFound = 1;
      else
	fn(sec);
      sem->post();
    });
    sem.wait();
    if (ZuUnlikely(notFound))
      throw ZtString() << "security " << key << " not found";
  }
}

void MxMDCore::lookupOrderBook(
    const CmdArgs &args, unsigned index,
    bool secRequired, bool obRequired,
    ZmFn<MxMDSecurity *, MxMDOrderBook *> fn)
{
  MxID venue = args.get("market");
  MxID segment = args.get("segment");
  bool notFound = 0;
  thread_local ZmSemaphore sem;
  lookupSecurity(args, index, secRequired || obRequired,
      [obRequired, &notFound, sem = &sem, venue, segment, fn = ZuMv(fn)](
	MxMDSecurity *sec) {
    ZmRef<MxMDOrderBook> ob = sec->orderBook(venue, segment);
    if (obRequired && ZuUnlikely(!ob))
      notFound = 1;
    else
      fn(sec, ob);
    sem->post();
  });
  sem.wait();
  if (ZuUnlikely(notFound))
    throw ZtString() << "order book " <<
	MxSecKey{venue, segment, args.get(ZuStringN<16>(ZuBoxed(index)))} <<
	" not found";
}

void MxMDCore::initCmds()
{
  addCmd(
      "l1", ZtString("c csv csv { type flag }\n") + lookupSyntax(),
      CmdFn::Member<&MxMDCore::l1>::fn(this),
      "dump L1 data",
      ZtString("usage: l1 OPTIONS SYMBOL [SYMBOL...]\n"
	"Display level 1 market data for SYMBOL(s)\n\nOptions:\n"
	"-c, --csv\toutput CSV format\n") +
	lookupOptions());
  addCmd(
      "l2", lookupSyntax(),
      CmdFn::Member<&MxMDCore::l2>::fn(this),
      "dump L2 data",
      ZtString("usage: l2 OPTIONS SYMBOL\n"
	"Display level 2 market data for SYMBOL\n\nOptions:\n") +
	lookupOptions());
  addCmd(
      "security", lookupSyntax(),
      CmdFn::Member<&MxMDCore::security_>::fn(this),
      "dump security reference data",
      ZtString("usage: security OPTIONS SYMBOL\n"
	"Display security reference data (\"static data\") for SYMBOL\n"
	"\nOptions:\n") + lookupOptions());
  addCmd(
      "ticksizes", "",
      CmdFn::Member<&MxMDCore::ticksizes>::fn(this),
      "dump in CSV format",
      "usage: ticksizes [VENUE [SEGMENT]]\n"
      "dump tick sizes in CSV format");
  addCmd(
      "securities", "",
      CmdFn::Member<&MxMDCore::securities>::fn(this),
      "dump securities in CSV format",
      "usage: securities [VENUE [SEGMENT]]\n"
      "dump securities in CSV format");
  addCmd(
      "orderbooks", "",
      CmdFn::Member<&MxMDCore::orderbooks>::fn(this),
      "dump order books in CSV format",
      "usage: orderbooks [VENUE [SEGMENT]]\n"
      "dump order books in CSV format");
#if 0
  addCmd(
      "subscribe",
      "s stop stop { type scalar }",
      CmdFn::Member<&MxMDCore::subscribeCmd>::fn(this),
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
  }
  if (m_publisher) {
    raise(ZeEVENT(Info, "stopping publisher..."));
    m_publisher->stop();
  }

  stopReplaying();
  stopRecording();

  m_broadcast.eof();
  m_broadcast.close();

  if (m_cmd) {
    raise(ZeEVENT(Info, "stopping command server..."));
    m_cmd->stop();
  }

  raise(ZeEVENT(Info, "stopping multiplexer..."));
  m_mx->stop(false);
}

void MxMDCore::final()
{
  raise(ZeEVENT(Info, "finalizing cmd server..."));

  if (m_cmd) m_cmd->final();

  raise(ZeEVENT(Info, "finalizing feeds..."));
  allFeeds([](MxMDFeed *feed) { try { feed->final(); } catch (...) { } });
}

void MxMDCore::l1(const CmdArgs &args, ZtArray<char> &out)
{
  unsigned argc = ZuBox<unsigned>(args.get("#"));
  if (argc < 2) throw CmdUsage();
  bool csv = !!args.get("csv");
  if (csv)
    out << "stamp,status,last,lastQty,bid,bidQty,ask,askQty,tickDir,"
      "high,low,accVol,accVolQty,match,matchQty,surplusQty,flags\n";
  for (unsigned i = 1; i < argc; i++)
    lookupOrderBook(args, i, 1, 1,
	[this, &out, csv](MxMDSecurity *, MxMDOrderBook *ob) {
      const MxMDL1Data &l1Data = ob->l1Data();
      MxNDP pxNDP = l1Data.pxNDP;
      MxNDP qtyNDP = l1Data.qtyNDP;
      MxMDFlagsStr flags;
      MxMDL1Flags::print(flags, ob->venueID(), l1Data.flags);
      if (csv) out <<
	m_cmd->timeFmt(l1Data.stamp) << ',' <<
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
	"stamp: " << m_cmd->timeFmt(l1Data.stamp) <<
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
    });
}

void MxMDCore::l2(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (argc != 2) throw CmdUsage();
  lookupOrderBook(args, 1, 1, 1,
      [this, &out](MxMDSecurity *, MxMDOrderBook *ob) {
    out << "bids:\n";
    l2_side(ob->bids(), out);
    out << "\nasks:\n";
    l2_side(ob->asks(), out);
    out << '\n';
  });
}

void MxMDCore::l2_side(MxMDOBSide *side, ZtArray<char> &out)
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
    out << " transactTime: " << m_cmd->timeFmt(pxLvlData.transactTime);
    return 0;
  });
}

void MxMDCore::security_(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (argc != 2) throw CmdUsage();
  lookupOrderBook(args, 1, 1, 0,
      [&out](MxMDSecurity *sec, MxMDOrderBook *ob) {
    const MxMDSecRefData &refData = sec->refData();
    out <<
      "ID: " << sec->id() <<
      "\nIDSrc: " << refData.idSrc <<
      "\nsymbol: " << refData.symbol <<
      "\naltIDSrc: " << refData.altIDSrc <<
      "\naltSymbol: " << refData.altSymbol;
    if (refData.underVenue) out << "\nunderlying: " <<
      MxSecKey{refData.underVenue, refData.underSegment, refData.underlying};
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

void MxMDCore::ticksizes(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (argc < 1 || argc > 3) throw CmdUsage();
  MxID venueID;
  if (argc == 2) venueID = args.get("1");
  MxMDTickSizeCSV csv;
  writeTickSizes(this, csv, csv.writeData(out), venueID);
}

static void writeSecurities(
    const MxMDLib *md, MxMDSecurityCSV &csv, ZvCSVWriteFn fn,
    MxID venueID, MxID segment)
{
  md->allSecurities(
      [&csv, &fn, venueID, segment](MxMDSecurity *sec) -> uintptr_t {
    if ((!*venueID || venueID == sec->primaryVenue()) &&
	(!*segment || segment == sec->primarySegment())) {
      new (csv.ptr()) MxMDSecurityCSV::Data{
	MxMDStream::Type::AddSecurity, MxDateTime(),
	sec->primaryVenue(), sec->primarySegment(),
	sec->id(), sec->shard()->id(), sec->refData() };
      fn(csv.pod());
    }
    return 0;
  });
  fn((ZuAnyPOD *)0);
}

void MxMDCore::dumpSecurities(ZuString path, MxID venueID, MxID segment)
{
  MxMDSecurityCSV csv;
  writeSecurities(this, csv, csv.writeFile(path), venueID, segment);
}

void MxMDCore::securities(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (argc < 1 || argc > 3) throw CmdUsage();
  MxID venueID, segment;
  if (argc == 2) venueID = args.get("1");
  if (argc == 3) segment = args.get("2");
  MxMDSecurityCSV csv;
  writeSecurities(this, csv, csv.writeData(out), venueID, segment);
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
	  MxMDStream::Type::AddOrderBook, MxDateTime(),
	  ob->venueID(), ob->segment(), ob->id(),
	  ob->pxNDP(), ob->qtyNDP(),
	  ob->legs(), ob->tickSizeTbl()->id(), ob->lotSizes() };
      for (unsigned i = 0, n = ob->legs(); i < n; i++) {
	MxMDSecurity *sec;
	if (!(sec = ob->security(i))) break;
	data->secVenues[i] = sec->primaryVenue();
	data->secSegments[i] = sec->primarySegment();
	data->securities[i] = sec->id();
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

void MxMDCore::orderbooks(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (argc < 1 || argc > 3) throw CmdUsage();
  MxID venueID, segment;
  if (argc == 2) venueID = args.get("1");
  if (argc == 3) segment = args.get("2");
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
void MxMDCore::subscribeCmd(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (ZuString id_ = args.get("stop")) {
    ZuBox<int> id = ZvCf::toInt("spin", id_, 0, 63);
    MxMDStream::detach(m_broadcast, m_broadcast.id());
    m_broadcast.close();
    out << "detached " << id << "\n";
    return;
  }
  if (argc != 2) throw CmdUsage();
  ZiRingParams ringParams(args.get("1"), m_broadcast.params());
  if (ZuString spin = args.get("spin"))
    ringParams.spin(ZvCf::toInt("spin", spin, 0, INT_MAX));
  if (ZuString timeout = args.get("timeout"))
    ringParams.timeout(ZvCf::toInt("timeout", timeout, 0, 3600));
  if (!m_broadcast.open())
    throw ZtString() << '"' << m_broadcast.params().name() <<
	"\": failed to open IPC shared memory ring buffer";
  m_snapper.snap(ringParams);
}
#endif

void MxMDCore::pad(Frame &frame)
{
  using namespace MxMDStream;

  switch ((int)frame.type) {
    case Type::AddTickSizeTbl:	frame.pad<AddTickSizeTbl>(); break;
    case Type::ResetTickSizeTbl: frame.pad<ResetTickSizeTbl>(); break;
    case Type::AddTickSize:	frame.pad<AddTickSize>(); break;
    case Type::AddSecurity:	frame.pad<AddSecurity>(); break;
    case Type::UpdateSecurity:	frame.pad<UpdateSecurity>(); break;
    case Type::AddOrderBook:	frame.pad<AddOrderBook>(); break;
    case Type::DelOrderBook:	frame.pad<DelOrderBook>(); break;
    case Type::AddCombination:	frame.pad<AddCombination>(); break;
    case Type::DelCombination:	frame.pad<DelCombination>(); break;
    case Type::UpdateOrderBook:	frame.pad<UpdateOrderBook>(); break;
    case Type::TradingSession:	frame.pad<TradingSession>(); break;
    case Type::L1:		frame.pad<L1>(); break;
    case Type::PxLevel:		frame.pad<PxLevel>(); break;
    case Type::L2:		frame.pad<L2>(); break;
    case Type::AddOrder:	frame.pad<AddOrder>(); break;
    case Type::ModifyOrder:	frame.pad<ModifyOrder>(); break;
    case Type::CancelOrder:	frame.pad<CancelOrder>(); break;
    case Type::ResetOB:		frame.pad<ResetOB>(); break;
    case Type::AddTrade:	frame.pad<AddTrade>(); break;
    case Type::CorrectTrade:	frame.pad<CorrectTrade>(); break;
    case Type::CancelTrade:	frame.pad<CancelTrade>(); break;
    case Type::RefDataLoaded:	frame.pad<RefDataLoaded>(); break;
    default: break;
  }
}

void MxMDCore::apply(const Frame &frame, bool filter)
{
  using namespace MxMDStream;

  switch ((int)frame.type) {
    case Type::AddTickSizeTbl:
      {
	const AddTickSizeTbl &obj = frame.as<AddTickSizeTbl>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  venue->addTickSizeTbl(obj.id, obj.pxNDP);
      }
      break;
    case Type::ResetTickSizeTbl:
      {
	const ResetTickSizeTbl &obj = frame.as<ResetTickSizeTbl>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  if (ZmRef<MxMDTickSizeTbl> tbl = venue->tickSizeTbl(obj.id))
	    tbl->reset();
      }
      break;
    case Type::AddTickSize:
      {
	const AddTickSize &obj = frame.as<AddTickSize>();
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
    case Type::AddSecurity:
      {
	const AddSecurity &obj = frame.as<AddSecurity>();
	MxMDSecHandle secHandle = security(obj.key, obj.shard);
	secHandle.invokeMv([key = obj.key, refData = obj.refData,
	    transactTime = obj.transactTime](
	      MxMDShard *shard, ZmRef<MxMDSecurity> sec) {
	  shard->addSecurity(ZuMv(sec), key, refData, transactTime);
	});
      }
      break;
    case Type::UpdateSecurity:
      {
	const UpdateSecurity &obj = frame.as<UpdateSecurity>();
	MxMDLib::secInvoke(obj.key, [refData = obj.refData,
	    transactTime = obj.transactTime](MxMDSecurity *sec) {
	  if (sec) sec->update(refData, transactTime);
	});
      }
      break;
    case Type::AddOrderBook:
      {
	const AddOrderBook &obj = frame.as<AddOrderBook>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.key.venue()))
	  if (ZmRef<MxMDTickSizeTbl> tbl =
		  venue->tickSizeTbl(obj.tickSizeTbl))
	    MxMDLib::secInvoke(obj.security, [key = obj.key,
		tbl = ZuMv(tbl), qtyNDP = obj.qtyNDP, lotSizes = obj.lotSizes,
		transactTime = obj.transactTime](
		  MxMDSecurity *sec) mutable {
	      if (sec) {
		if (ZuUnlikely(sec->refData().qtyNDP != qtyNDP)) {
		  MxNDP newNDP = sec->refData().qtyNDP;
#ifdef adjustNDP
#undef adjustNDP
#endif
#define adjustNDP(v) v = MxValNDP{v, qtyNDP}.adjust(newNDP)
		  adjustNDP(lotSizes.oddLotSize);
		  adjustNDP(lotSizes.lotSize);
		  adjustNDP(lotSizes.blockLotSize);
#undef adjustNDP
		  qtyNDP = newNDP;
		}
		sec->addOrderBook(key, tbl, lotSizes, transactTime);
	      }
	    });
      }
      break;
    case Type::DelOrderBook:
      {
	const DelOrderBook &obj = frame.as<DelOrderBook>();
	obInvoke(obj.key, [transactTime = obj.transactTime](
	      MxMDOrderBook *ob) {
	  if (ob) ob->security()->delOrderBook(
	      ob->venueID(), ob->segment(), transactTime);
	});
      }
      break;
    case Type::AddCombination:
      {
	const AddCombination &obj = frame.as<AddCombination>();
	MxMDSecHandle secHandle = security(obj.securities[0]);
	if (secHandle)
	  if (ZmRef<MxMDVenue> venue = this->venue(obj.key.venue()))
	    if (ZmRef<MxMDTickSizeTbl> tbl =
		venue->tickSizeTbl(obj.tickSizeTbl))
	      secHandle.invokeMv(
		  [obj = obj, venue = ZuMv(venue), tbl = ZuMv(tbl)](
		    MxMDShard *shard, ZmRef<MxMDSecurity> sec) {
		ZmRef<MxMDSecurity> securities[MxMDNLegs];
		for (unsigned i = 0; i < obj.legs; i++) {
		  if (!i)
		    securities[i] = ZuMv(sec);
		  else {
		    if (!(securities[i] = shard->security(obj.securities[i])))
		    return;
		  }
		}
		venue->shard(shard)->addCombination(
		    obj.key.segment(), obj.key.id(),
		    obj.pxNDP, obj.qtyNDP,
		    obj.legs, securities, obj.sides, obj.ratios,
		    tbl, obj.lotSizes, obj.transactTime);
	      });
      }
      break;
    case Type::DelCombination:
      {
	const DelCombination &obj = frame.as<DelCombination>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.key.venue()))
	obInvoke(obj.key, [venue = ZuMv(venue),
	    transactTime = obj.transactTime](MxMDOrderBook *ob) {
	  if (ob)
	    venue->shard(ob->shard())->delCombination(
		ob->segment(), ob->id(), transactTime);
	});
      }
      break;
    case Type::UpdateOrderBook:
      {
	const UpdateOrderBook &obj = frame.as<UpdateOrderBook>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.key.venue()))
	  if (ZmRef<MxMDTickSizeTbl> tbl = venue->tickSizeTbl(obj.tickSizeTbl))
	    obInvoke(obj.key, [
		lotSizes = obj.lotSizes, transactTime = obj.transactTime,
		tbl = ZuMv(tbl)](MxMDOrderBook *ob) {
	      ob->update(tbl, lotSizes, transactTime);
	    });
      }
      break;
    case Type::TradingSession:
      {
	const TradingSession &obj = frame.as<TradingSession>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  venue->tradingSession(
	      MxMDSegment{obj.segment, obj.session, obj.stamp});
      }
      break;
    case Type::L1:
      {
	const L1 &obj = frame.as<L1>();
	obInvoke(obj.key, [data = obj.data, filter](MxMDOrderBook *ob) mutable {
	  // inconsistent NDP handled within MxMDOrderBook::l1()
	  if (ob && (!filter || ob->handler())) ob->l1(data);
	});
      }
      break;
    case Type::PxLevel:
      {
	const PxLevel &obj = frame.as<PxLevel>();
	obInvoke(obj.key, [
	    side = obj.side, transactTime = obj.transactTime,
	    delta = obj.delta, pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty,
	    nOrders = obj.nOrders, flags = obj.flags, filter](
	      MxMDOrderBook *ob) mutable {
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != pxNDP))
	      price = MxValNDP{price, pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != qtyNDP))
	      qty = MxValNDP{qty, qtyNDP}.adjust(ob->qtyNDP());
	    ob->pxLevel(side, transactTime, delta, price, qty, nOrders, flags);
	  }
	});
      }
      break;
    case Type::L2:
      {
	const L2 &obj = frame.as<L2>();
	obInvoke(obj.key, [
	    stamp = obj.stamp, updateL1 = obj.updateL1, filter](
	      MxMDOrderBook *ob) {
	  if (ob && (!filter || ob->handler())) ob->l2(stamp, updateL1);
	});
      }
      break;
    case Type::AddOrder:
      {
	const AddOrder &obj = frame.as<AddOrder>();
	obInvoke(obj.key, [
	    orderID = obj.orderID, transactTime = obj.transactTime,
	    side = obj.side, rank = obj.rank,
	    pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty, flags = obj.flags, filter](
	      MxMDOrderBook *ob) mutable {
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != pxNDP))
	      price = MxValNDP{price, pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != qtyNDP))
	      qty = MxValNDP{qty, qtyNDP}.adjust(ob->qtyNDP());
	    ob->addOrder(
		orderID, transactTime,
		side, rank, price, qty, flags);
	  }
	});
      }
      break;
    case Type::ModifyOrder:
      {
	const ModifyOrder &obj = frame.as<ModifyOrder>();
	obInvoke(obj.key, [
	    orderID = obj.orderID, transactTime = obj.transactTime,
	    side = obj.side, rank = obj.rank,
	    pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty, flags = obj.flags, filter](
	      MxMDOrderBook *ob) mutable {
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != pxNDP))
	      price = MxValNDP{price, pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != qtyNDP))
	      qty = MxValNDP{qty, qtyNDP}.adjust(ob->qtyNDP());
	    ob->modifyOrder(
		orderID, transactTime,
		side, rank, price, qty, flags);
	  }
	});
      }
      break;
    case Type::CancelOrder:
      {
	const CancelOrder &obj = frame.as<CancelOrder>();
	obInvoke(obj.key, [
	    orderID = obj.orderID,
	    transactTime = obj.transactTime,
	    side = obj.side, filter](MxMDOrderBook *ob) {
	  if (ob && (!filter || ob->handler()))
	    ob->cancelOrder(orderID, transactTime, side);
	});
      }
      break;
    case Type::ResetOB:
      {
	const ResetOB &obj = frame.as<ResetOB>();
	obInvoke(obj.key, [transactTime = obj.transactTime, filter](
	      MxMDOrderBook *ob) {
	  if (ob && (!filter || ob->handler()))
	    ob->reset(transactTime);
	});
      }
      break;
    case Type::AddTrade:
      {
	const AddTrade &obj = frame.as<AddTrade>();
	obInvoke(obj.key, [
	    tradeID = obj.tradeID, transactTime = obj.transactTime,
	    pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty, filter](
	      MxMDOrderBook *ob) mutable {
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != pxNDP))
	      price = MxValNDP{price, pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != qtyNDP))
	      qty = MxValNDP{qty, qtyNDP}.adjust(ob->qtyNDP());
	    ob->addTrade(tradeID, transactTime, price, qty);
	  }
	});
      }
      break;
    case Type::CorrectTrade:
      {
	const CorrectTrade &obj = frame.as<CorrectTrade>();
	obInvoke(obj.key, [
	    tradeID = obj.tradeID, transactTime = obj.transactTime,
	    pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty, filter](
	      MxMDOrderBook *ob) mutable {
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != pxNDP))
	      price = MxValNDP{price, pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != qtyNDP))
	      qty = MxValNDP{qty, qtyNDP}.adjust(ob->qtyNDP());
	    ob->correctTrade(tradeID, transactTime, price, qty);
	  }
	});
      }
      break;
    case Type::CancelTrade:
      {
	const CancelTrade &obj = frame.as<CancelTrade>();
	obInvoke(obj.key, [
	    tradeID = obj.tradeID, transactTime = obj.transactTime,
	    pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty, filter](
	      MxMDOrderBook *ob) mutable {
	  if (ob && (!filter || ob->handler())) {
	    if (ZuUnlikely(ob->pxNDP() != pxNDP))
	      price = MxValNDP{price, pxNDP}.adjust(ob->pxNDP());
	    if (ZuUnlikely(ob->qtyNDP() != qtyNDP))
	      qty = MxValNDP{qty, qtyNDP}.adjust(ob->qtyNDP());
	    ob->cancelTrade(tradeID, transactTime, price, qty);
	  }
	});
      }
      break;
    case Type::RefDataLoaded:
      {
	const RefDataLoaded &obj = frame.as<RefDataLoaded>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  this->loaded(venue);
      }
    default:
      raise(ZeEVENT(Error, "MxMDLib - unknown message type"));
      break;
  }
}

ZmRef<MxMDVenue> MxMDCore::venue(MxID id)
{
  ZmRef<MxMDVenue> venue;
  if (ZuLikely(venue = MxMDLib::venue(id))) return venue;
  venue = new MxMDVenue(this, m_localFeed, id);
  addVenue(venue);
  return venue;
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
