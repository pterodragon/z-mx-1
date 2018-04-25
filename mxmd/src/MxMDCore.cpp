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

#include <ZvHeapCf.hpp>

#include <MxCSV.hpp>

#include <version.h>

unsigned MxMDCore::vmajor() { return MXMD_VMAJOR(MXMD_VERSION); }
unsigned MxMDCore::vminor() { return MXMD_VMINOR(MXMD_VERSION); }

class MxIDCol : public ZvCSVColumn<ZvCSVColType::Func, MxID>  {
  typedef ZvCSVColumn<ZvCSVColType::Func, MxID> Base;
  typedef Base::ParseFn ParseFn;
  typedef Base::PlaceFn PlaceFn;
public:
  template <typename ID>
  inline MxIDCol(const ID &id, int offset) :
      Base(id, offset,
	   ParseFn::Ptr<&MxIDCol::parse>::fn(),
	   PlaceFn::Ptr<&MxIDCol::place>::fn()) { }
  static void parse(MxID *i, ZuString b) { *i = b; }
  static void place(ZtArray<char> &b, const MxID *i) { b << *i; }
};

class MxMDTickSizeCSV : public ZvCSV {
public:
  struct Data {
    MxID		venue;
    MxIDString		id;
    MxFloat		minPrice;
    MxFloat		maxPrice;
    MxFloat		tickSize;
  };
  typedef ZuPOD<Data> POD;

  MxMDTickSizeCSV() {
    new ((m_pod = new POD())->ptr()) Data{};
    add(new MxIDCol("venue", offsetof(Data, venue)));
    add(new MxIDStringCol("id", offsetof(Data, id)));
    add(new MxFloatCol("minPrice", offsetof(Data, minPrice)));
    add(new MxFloatCol("maxPrice", offsetof(Data, maxPrice)));
    add(new MxFloatCol("tickSize", offsetof(Data, tickSize)));
  }

  void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

  template <typename File>
  void read(const File &file, ZvCSVReadFn fn) {
    ZvCSV::readFile(file,
	ZvCSVAllocFn::Member<&MxMDTickSizeCSV::alloc>::fn(this), fn);
  }

  ZuInline POD *pod() { return m_pod.ptr(); }
  ZuInline Data *ptr() { return m_pod->ptr(); }

private:
  ZuRef<POD>	m_pod;
};

class MxMDSecurityCSV : public ZvCSV {
public:
  struct Data {
    MxID		venue;
    MxID		segment;
    MxSymString		id;
    MxUInt		shard;
    MxMDSecRefData	refData;
  };
  typedef ZuPOD<Data> POD;

  MxMDSecurityCSV() {
    new ((m_pod = new POD())->ptr()) Data();
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new MxIDCol("venue", Offset(venue)));
    add(new MxIDCol("segment", Offset(segment)));
    add(new MxSymStringCol("id", Offset(id)));
    add(new MxUIntCol("shard", Offset(shard)));
#undef Offset
#define Offset(x) offsetof(Data, refData) + offsetof(MxMDSecRefData, x)
    add(new MxBoolCol("tradeable", Offset(tradeable)));
    add(new MxEnumCol<MxSecIDSrc::CSVMap>("idSrc", Offset(idSrc)));
    add(new MxSymStringCol("symbol", Offset(symbol)));
    add(new MxEnumCol<MxSecIDSrc::CSVMap>("altIDSrc", Offset(altIDSrc)));
    add(new MxSymStringCol("altSymbol", Offset(altSymbol)));
    add(new MxIDCol("underVenue", Offset(underVenue)));
    add(new MxIDCol("underSegment", Offset(underSegment)));
    add(new MxSymStringCol("underlying", Offset(underlying)));
    add(new MxUIntCol("mat", Offset(mat)));
    add(new MxEnumCol<MxPutCall::CSVMap>("putCall", Offset(putCall)));
    add(new MxIntCol("strike", Offset(strike)));
    add(new MxFloatCol("strikeMultiplier", Offset(strikeMultiplier)));
    add(new MxFloatCol("outstandingShares", Offset(outstandingShares)));
    add(new MxFloatCol("adv", Offset(adv)));
#undef Offset
  }

  void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

  template <typename File>
  void read(const File &file, ZvCSVReadFn fn) {
    ZvCSV::readFile(file,
	ZvCSVAllocFn::Member<&MxMDSecurityCSV::alloc>::fn(this), fn);
  }

  ZuInline POD *pod() { return m_pod.ptr(); }
  ZuInline Data *ptr() { return m_pod->ptr(); }

private:
  ZuRef<POD>	m_pod;
};

class MxMDOrderBookCSV : public ZvCSV {
public:
  struct Data {
    MxID		venue;
    MxID		segment;
    MxSymString		id;
    MxUInt		legs;
    MxIDString		tickSizeTbl;
    MxMDLotSizes	lotSizes;
    MxID		secVenues[MxMDNLegs];
    MxID		secSegments[MxMDNLegs];
    MxSymString		securities[MxMDNLegs];
    MxEnum		sides[MxMDNLegs];
    MxFloat		ratios[MxMDNLegs];
  };
  typedef ZuPOD<Data> POD;

  MxMDOrderBookCSV() {
    new ((m_pod = new POD())->ptr()) Data();
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new MxIDCol("venue", Offset(venue)));
    add(new MxIDCol("segment", Offset(segment)));
    add(new MxSymStringCol("id", Offset(id)));
    add(new MxUIntCol("legs", Offset(legs)));
    for (unsigned i = 0; i < MxMDNLegs; i++) {
      ZuStringN<16> secVenue = "secVenue"; secVenue << ZuBoxed(i);
      ZuStringN<16> secSegment = "secSegment"; secSegment << ZuBoxed(i);
      ZuStringN<16> security = "security"; security << ZuBoxed(i);
      ZuStringN<8> side = "side"; side << ZuBoxed(i);
      ZuStringN<8> ratio = "ratio"; ratio << ZuBoxed(i);
      add(new MxIDCol(secVenue, Offset(secVenues) + (i * sizeof(MxID))));
      add(new MxIDCol(secSegment, Offset(secSegments) + (i * sizeof(MxID))));
      add(new MxSymStringCol(security,
	    Offset(securities) + (i * sizeof(MxSymString))));
      add(new MxEnumCol<MxSide::CSVMap>(side,
	    Offset(sides) + (i * sizeof(MxEnum))));
      add(new MxFloatCol(ratio,
	    Offset(ratios) + (i * sizeof(MxFloat))));
    }
    add(new MxIDStringCol("tickSizeTbl", Offset(tickSizeTbl)));
#undef Offset
#define Offset(x) offsetof(Data, lotSizes) + offsetof(MxMDLotSizes, x)
    add(new MxFloatCol("oddLotSize", Offset(oddLotSize)));
    add(new MxFloatCol("lotSize", Offset(lotSize)));
    add(new MxFloatCol("blockLotSize", Offset(blockLotSize)));
#undef Offset
  }

  void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

  template <typename File>
  void read(const File &file, ZvCSVReadFn fn) {
    ZvCSV::readFile(file,
	ZvCSVAllocFn::Member<&MxMDOrderBookCSV::alloc>::fn(this), fn);
  }

  ZuInline POD *pod() { return m_pod.ptr(); }
  ZuInline Data *ptr() { return m_pod->ptr(); }

private:
  ZuRef<POD>	m_pod;
};

void MxMDCore::addTickSize_(ZuAnyPOD *pod)
{
  MxMDTickSizeCSV::Data *data = (MxMDTickSizeCSV::Data *)(pod->ptr());
  ZmRef<MxMDVenue> venue = this->venue(data->venue);
  ZmRef<MxMDTickSizeTbl> tbl = venue->addTickSizeTbl(data->id);
  tbl->addTickSize(data->minPrice, data->maxPrice, data->tickSize);
}

void MxMDCore::addSecurity_(ZuAnyPOD *pod)
{
  MxMDSecurityCSV::Data *data = (MxMDSecurityCSV::Data *)(pod->ptr());
  MxMDLib::addSecurity(
    data->venue, data->segment, data->id, data->shard, data->refData);
}

void MxMDCore::addOrderBook_(ZuAnyPOD *pod)
{
  MxMDOrderBookCSV::Data *data = (MxMDOrderBookCSV::Data *)(pod->ptr());
  ZmRef<MxMDVenue> venue = this->venue(data->venue);
  ZmRef<MxMDTickSizeTbl> tbl = venue->tickSizeTbl(data->tickSizeTbl);
  if (!tbl)
    throw ZtZString() <<
      data->venue << '/' << data->segment << '/' << data->id <<
      ": unknown tick size table \"" << data->tickSizeTbl << '"';
  if (data->legs == 1) {
    MxID venueID = data->secVenues[0];
    if (!*venueID) venueID = data->venue;
    MxID segment = data->secSegments[0];
    if (!*segment) segment = data->segment;
    MxSymString id = data->securities[0];
    if (!id) id = data->id;
    MxMDLib::security(MxSecKey{venueID, segment, id},
	[data, &tbl](MxMDSecurity *sec) {
      if (sec) sec->addOrderBook(
	data->venue, data->segment, data->id, tbl, data->lotSizes);
    });
  } else {
    ZmRef<MxMDSecurity> securities[MxMDNLegs];
    MxEnum sides[MxMDNLegs];
    MxFloat ratios[MxMDNLegs];
    for (unsigned i = 0, n = data->legs; i < n; i++) {
      MxID venueID = data->secVenues[i];
      if (!*venueID) venueID = data->venue;
      MxID segment = data->secSegments[i];
      if (!*segment) segment = data->segment;
      MxSymString id = data->securities[i];
      if (!id) return;
      MxMDLib::security(MxSecKey{venueID, segment, id},
	  [&securities, i](MxMDSecurity *sec) { securities[i] = sec; });
      if (!securities[i]) return;
      sides[i] = data->sides[i];
      ratios[i] = data->ratios[i];
    }
    venue->addCombination(
	data->segment, data->id, data->legs,
	securities, sides, ratios, tbl, data->lotSizes);
  }
}

MxMDLib *MxMDLib::init(const char *cf_)
{
  ZeLog::start(); // ensure error reporting works

  ZmRef<ZvCf> cf = new ZvCf();
  ZmRef<MxMDCore> md;

  try {
    if (cf_) cf->fromFile(cf_, false);

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

      md = new MxMDCore(ZuMv(mx), cf);
    }

    md->m_localFeed = new MxMDFeed("_LCL", md);
    md->addFeed(md->m_localFeed);

    if (ZmRef<ZvCf> feedsCf = cf->subset("feeds", false)) {
      ZeLOG(Info, "MxMDLib - configuring feeds...");
      ZvCf::Iterator i(feedsCf);
      ZtZString key;
      while (ZmRef<ZvCf> feedCf = i.subset(key)) {
	if (key == "_LCL") {
	  ZvCf::Iterator j(feedCf);
	  ZtZString id;
	  while (ZmRef<ZvCf> venueCf = j.subset(id))
	    md->addVenue(new MxMDVenue(id, md->m_localFeed, md));
	  continue;
	}
	ZtZString e;
	ZiModule module;
	ZiModule::Path name = feedCf->get("module", true);
	int preload = feedCf->getInt("preload", 0, 1, false, 0);
	if (preload) preload = ZiModule::Pre;
	if (module.load(name, preload, &e) < 0)
	  throw ZtZString() << "failed to load \"" << name << "\": " << e;
	MxMDFeedPluginFn pluginFn =
	  (MxMDFeedPluginFn)module.resolve("MxMDFeed_plugin", &e);
	if (!pluginFn) {
	  module.unload();
	  throw ZtZString() <<
	    "failed to resolve \"MxMDFeed_plugin\" in \"" <<
	    name << "\": " << e;
	}
	(*pluginFn)(md, feedCf);
      }
    }

    if (const ZtArray<ZtString> *tickSizes =
	  cf->getMultiple("tickSizes", 0, INT_MAX)) {
      ZeLOG(Info, "MxMDLib - reading tick size data...");
      MxMDTickSizeCSV csv;
      for (unsigned i = 0, n = tickSizes->length(); i < n; i++)
	csv.read((*tickSizes)[i],
	    ZvCSVReadFn::Member<&MxMDCore::addTickSize_>::fn(md));
    }
    if (const ZtArray<ZtString> *securities =
	  cf->getMultiple("securities", 0, INT_MAX)) {
      ZeLOG(Info, "MxMDLib - reading security reference data...");
      MxMDSecurityCSV csv;
      for (unsigned i = 0, n = securities->length(); i < n; i++)
	csv.read((*securities)[i],
	    ZvCSVReadFn::Member<&MxMDCore::addSecurity_>::fn(md));
    }
    if (const ZtArray<ZtString> *orderBooks =
	  cf->getMultiple("orderBooks", 0, INT_MAX)) {
      ZeLOG(Info, "MxMDLib - reading order book reference data...");
      MxMDOrderBookCSV csv;
      for (unsigned i = 0, n = orderBooks->length(); i < n; i++)
	csv.read((*orderBooks)[i],
	    ZvCSVReadFn::Member<&MxMDCore::addOrderBook_>::fn(md));
    }

    md->m_recordCfPath = cf->get("record");

    if (ZmRef<ZvCf> ringCf = cf->subset("ring", false))
      md->m_broadcast.init(ringCf);

    if (ZmRef<ZvCf> threadsCf = cf->subset("threads", false)) {
      if (ZmRef<ZvCf> threadCf = threadsCf->subset("recReceiver", false))
	md->m_recReceiverParams.init(threadCf);
      if (ZmRef<ZvCf> threadCf = threadsCf->subset("recSnapper", false))
	md->m_recSnapperParams.init(threadCf);
      if (ZmRef<ZvCf> threadCf = threadsCf->subset("snapper", false))
	md->m_snapperParams.init(threadCf);
    }

    md->m_replayCfPath = cf->get("replay");

    ZeLOG(Info, "MxMDLib - initialized...");

  } catch (const ZvError &e) {
    ZeLOG(Fatal, ZtZString() << "MxMDLib - configuration error: " << e);
    return md = 0;
  } catch (const ZtString &e) {
    ZeLOG(Fatal, ZtZString() << "MxMDLib - error: " << e);
    return md = 0;
  } catch (const ZeError &e) {
    ZeLOG(Fatal, ZtZString() << "MxMDLib - error: " << e);
    return md = 0;
  } catch (...) {
    ZeLOG(Fatal, "MxMDLib - unknown exception during init");
    return md = 0;
  }

  return ZmSingleton<MxMDCore, 0>::instance(md);
}

MxMDLib *MxMDLib::instance()
{
  return ZmSingleton<MxMDCore, 0>::instance();
}

MxMDCore::MxMDCore(ZmRef<Mx> mx, ZvCf *cf) :
  MxMDLib(mx),
  m_mx(ZuMv(mx)),
  m_cf(cf),
  m_broadcast(this),
  m_snapper(this),
  m_recorder(this)
{
  if (ZmRef<ZvCf> shardsCf = cf->subset("shards", false)) {
    ZeLOG(Info, "MxMDLib - configuring shards...");
    m_shards.length(shardsCf->count());
    ZvCf::Iterator i(shardsCf);
    ZtZString key;
    while (ZmRef<ZvCf> shardCf = i.subset(key)) {
      ZuBox<unsigned> id = key;
      if (id >= m_shards.length())
	throw ZvCf::RangeInt(0, m_shards.size(), id, "shards");
      unsigned tid = shardCf->getInt("tid", 1, m_mx->nThreads(), true);
      m_shards[id] = new MxMDShard(this, id, tid);
    }
  } else {
    m_shards.length(1);
    for (unsigned tid = 1, n = m_mx->nThreads(); tid <= n; tid++)
      if (tid != m_mx->rxThread() && tid != m_mx->txThread()) {
	m_shards[0] = new MxMDShard(this, 0, tid);
	break;
      }
    if (!m_shards[0]) throw ZtString("mx misconfigured - no worker threads");
  }

  if (cf->subset("mx", false))
    if (ZmRef<ZvCf> cmdCf = cf->subset("cmd", false)) {
      m_cmd = new MxMDCore::CmdServer(this);
      m_cmd->init(cmdCf);
      initCmds();
    }
}

void MxMDCore::initCmds()
{
  addCmd(
      "l1", ZtZString("c csv csv { type flag }\n") + lookupSyntax(),
      CmdFn::Member<&MxMDCore::l1>::fn(this),
      "dump L1 data",
      ZtZString("usage: l1 OPTIONS SYMBOL [SYMBOL...]\n"
	"Display level 1 market data for SYMBOL(s)\n\nOptions:\n"
	"-c, --csv\toutput CSV format\n") +
	lookupOptions());
  addCmd(
      "l2", lookupSyntax(),
      CmdFn::Member<&MxMDCore::l2>::fn(this),
      "dump L2 data",
      ZtZString("usage: l2 OPTIONS SYMBOL\n"
	"Display level 2 market data for SYMBOL\n\nOptions:\n") +
	lookupOptions());
  addCmd(
      "security", lookupSyntax(),
      CmdFn::Member<&MxMDCore::security>::fn(this),
      "dump security reference data",
      ZtZString("usage: security OPTIONS SYMBOL\n"
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
  addCmd(
      "record",
      "s stop stop { type flag }",
      CmdFn::Member<&MxMDCore::recordCmd>::fn(this),
      "record market data to file", 
      "usage: record FILE\n"
      "       record -s\n"
      "record market data to FILE\n\n"
      "Options:\n"
      "-s, --stop\tstop recording\n");
  addCmd(
      "replay",
      "s stop stop { type flag }",
      CmdFn::Member<&MxMDCore::replayCmd>::fn(this),
      "replay market data from file",
      "usage: replay FILE\n"
      "       replay -s\n"
      "replay market data from FILE\n\n"
      "Options:\n"
      "-s, --stop\tstop replaying\n");
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
}

void MxMDCore::start()
{
  Guard guard(m_stateLock);

  if (m_mx) {
    raise(ZeEVENT(Info, "starting multiplexer..."));
    if (m_mx->start() != Zi::OK) {
      raise(ZeEVENT(Fatal, "multiplexer start failed"));
      return;
    }
    if (m_cmd) {
      raise(ZeEVENT(Info, "starting cmd server..."));
      m_cmd->start();
    }
  }

  if (m_mx) {
    { if (!!m_recordCfPath) record(m_recordCfPath); }
    { if (!!m_replayCfPath) replay(m_replayCfPath); }
  }

  raise(ZeEVENT(Info, "starting feeds..."));
  allFeeds([](MxMDFeed *feed) { try { feed->start(); } catch (...) { } });
}

void MxMDCore::stop()
{
  Guard guard(m_stateLock);

  raise(ZeEVENT(Info, "stopping feeds..."));
  allFeeds([](MxMDFeed *feed) { try { feed->stop(); } catch (...) { } });

  if (m_mx) {
    stopRecording();
    stopStreaming();
    stopReplaying();
  }

  if (m_cmd) {
    raise(ZeEVENT(Info, "stopping command server..."));
    m_cmd->stop();
  }

  raise(ZeEVENT(Info, "stopping primary multiplexer..."));
  m_mx->stop(false);
}

void MxMDCore::final()
{
  raise(ZeEVENT(Info, "finalizing cmd server..."));

  if (m_cmd) m_cmd->final();

  raise(ZeEVENT(Info, "finalizing feeds..."));
  allFeeds([](MxMDFeed *feed) { try { feed->final(); } catch (...) { } });
}

MxMDCore::CmdServer::CmdServer(MxMDCore *md) : m_md(md), m_syntax(new ZvCf())
{
}

void MxMDCore::CmdServer::init(ZvCf *cf)
{
  ZvCmdServer::init(cf, m_md->m_mx, ZvCmdDiscFn(),
      ZvCmdRcvdFn::Member<&MxMDCore::CmdServer::rcvd>::fn(this));
  // Assumption: DST transitions do not occur while market is open
  {
    ZtDate now(ZtDate::Now);
    ZtZString timezone = cf->get("timezone"); // default to system tz
    now.sec() = 0, now.nsec() = 0; // midnight GMT (start of today)
    now += ZmTime((time_t)(now.offset(timezone) + 43200)); // midday local time
    m_tzOffset = now.offset(timezone);
  }
  m_md->addCmd(
      "help", "", CmdFn::Member<&MxMDCore::CmdServer::help>::fn(this),
      "list commands", "usage: help [COMMAND]");
}

void MxMDCore::CmdServer::final()
{
  m_syntax = 0;
  m_cmds.clean();
  ZvCmdServer::final();
}

MxMDCmd *MxMDCmd::instance(MxMDLib *md)
{
  return static_cast<MxMDCmd *>(static_cast<MxMDCore *>(md));
}

void MxMDCmd::addCmd(ZuString name, ZuString syntax, CmdFn fn,
  ZuString brief, ZuString usage)
{
  MxMDCore::CmdServer *server = static_cast<MxMDCore *>(this)->m_cmd;
  if (!server) return;
  {
    ZmRef<ZvCf> cf = server->m_syntax->subset(name, true);
    cf->fromString(syntax, false);
    cf->set("help:type", "flag");
  }
  if (MxMDCore::CmdServer::Cmds::NodeRef cmd = server->m_cmds.find(name))
    cmd->val() = ZuMkTuple(fn, brief, usage);
  else
    server->m_cmds.add(name, ZuMkTuple(fn, brief, usage));
}

void MxMDCore::CmdServer::rcvd(
  ZvCmdLine *line, const ZvInvocation &inv, ZvAnswer &ans)
{
  ZmRef<ZvCf> cf = new ZvCf();
  ZtZString name;
  Cmds::NodeRef cmd;
  try {
    cf->fromCLI(m_syntax, inv.cmd());
    name = cf->get("0");
    cmd = m_cmds.find(name);
    if (!cmd) throw ZtZString("unknown command");
    if (cf->getInt("help", 0, 1, false, 0))
      ans.make(ZvCmd::Success, Ze::Info,
	       ZvAnswerArgs, cmd->val().p3());
    else {
      CmdArgs args;
      ZvCf::Iterator i(cf);
      {
	ZtZString arg;
	const ZtArray<ZtString> *values;
	while (values = i.getMultiple(arg, 0, INT_MAX)) args.add(arg, values);
      }
      ZtZArray<char> out;
      (cmd->val().p1())(args, out);
      ans.make(ZvCmd::Success, out);
    }
  } catch (const CmdUsage &) {
    ans.make(ZvCmd::Fail, Ze::Warning, ZvAnswerArgs, cmd->val().p3().data());
  } catch (const ZvError &e) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtZString() << '"' << name << "\": " << e);
  } catch (const ZeError &e) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtZString() << '"' << name << "\": " << e);
  } catch (const ZtString &s) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtZString() << '"' << name << "\": " << s);
  } catch (const ZtArray<char> &s) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtZString() << '"' << name << "\": " << s);
  } catch (...) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtZString() << '"' << name << "\": unknown exception");
  }
}

void MxMDCore::CmdServer::help(const CmdArgs &args, ZtArray<char> &help)
{
  int argc = ZuBox<int>(args.get("#"));
  if (argc > 2) throw CmdUsage();
  if (ZuUnlikely(argc == 2)) {
    Cmds::NodeRef cmd = m_cmds.find(args.get("1"));
    if (!cmd) throw CmdUsage();
    help = cmd->val().p3();
    return;
  }
  help.size(m_cmds.count() * 80 + 40);
  help += "List of commands:\n\n";
  {
    Cmds::ReadIterator i(m_cmds);
    while (Cmds::NodeRef cmd = i.iterate()) {
      help += cmd->key();
      help += " -- ";
      help += cmd->val().p2();
      help += "\n";
    }
  }
}

ZtZString MxMDCmd::lookupSyntax()
{
  return 
    "S src src { type scalar } " // FIXME - ID src
    "m market market { type scalar } "
    "s segment segment { type scalar }";
}

ZtZString MxMDCmd::lookupOptions()
{
  return
    "    -r --ric\t- symbol is RIC\n"
    "    -m --market=MIC\t - market MIC, e.g. XTKS\n"
    "    -s --segment=SEGMENT\t- market segment SEGMENT\n";
}

void MxMDCmd::lookupSecurity(
    MxMDLib *md, const MxMDCmd::CmdArgs &args, unsigned index,
    bool secRequired, ZmFn<MxMDSecurity *> fn)
{
  ZtZString symbol = args.get(ZuStringN<16>(ZuBoxed(index)));
  MxID venue = args.get("market");
  MxID segment = args.get("segment");
  thread_local ZmSemaphore sem;
  bool notFound = 0;
  if (ZtZString src_ = args.get("src")) {
    MxEnum src = MxSecIDSrc::lookup(src_);
    md->security(MxSecSymKey{src, symbol},
	[secRequired, sem = &sem, &notFound, fn = ZuMv(fn)](MxMDSecurity *sec) {
      if (secRequired && ZuUnlikely(!sec))
	notFound = 1;
      else
	fn(sec);
      sem->post();
    });
    sem.wait();
    if (ZuUnlikely(notFound))
      throw ZtZString() << "security " << src << ':' << symbol << " not found";
  } else {
    if (!*venue) throw MxMDCmd::CmdUsage();
    md->security(MxSecKey{venue, segment, symbol},
	[secRequired, sem = &sem, &notFound, fn = ZuMv(fn)](MxMDSecurity *sec) {
      if (secRequired && ZuUnlikely(!sec))
	notFound = 1;
      else
	fn(sec);
      sem->post();
    });
    sem.wait();
    if (ZuUnlikely(notFound))
      throw ZtZString() << "security " <<
	  venue << '/' << segment << '/' << symbol << " not found";
  }
}

void MxMDCmd::lookupOrderBook(
    MxMDLib *md, const MxMDCmd::CmdArgs &args, unsigned index,
    bool secRequired, bool obRequired,
    ZmFn<MxMDSecurity *, MxMDOrderBook *> fn)
{
  MxID venue = args.get("market");
  MxID segment = args.get("segment");
  thread_local ZmSemaphore sem;
  bool notFound = 0;
  lookupSecurity(md, args, index, secRequired || obRequired,
      [obRequired, sem = &sem, &notFound, venue, segment, fn = ZuMv(fn)](
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
    throw ZtZString() << "order book " <<
	venue << '/' << segment << '/' <<
	args.get(ZuStringN<16>(ZuBoxed(index))) << " not found";
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
    lookupOrderBook(this, args, i, 1, 1,
	[this, &out, csv](MxMDSecurity *, MxMDOrderBook *ob) {
      const MxMDL1Data &l1Data = ob->l1Data();
      MxMDFlagsStr flags;
      MxMDL1Flags::print(flags, ob->venueID(), l1Data.flags);
      if (csv) out <<
	m_cmd->timeFmt(l1Data.stamp) << ',' <<
	MxTradingStatus::name(l1Data.status) << ',' <<
	l1Data.last << ',' <<
	l1Data.lastQty << ',' <<
	l1Data.bid << ',' <<
	l1Data.bidQty << ',' <<
	l1Data.ask << ',' <<
	l1Data.askQty << ',' <<
	MxTickDir::name(l1Data.tickDir) << ',' <<
	l1Data.high << ',' <<
	l1Data.low << ',' <<
	l1Data.accVol << ',' <<
	l1Data.accVolQty << ',' <<
	l1Data.match << ',' <<
	l1Data.matchQty << ',' <<
	l1Data.surplusQty << ',' <<
	flags << '\n';
      else out <<
	"stamp: " << m_cmd->timeFmt(l1Data.stamp) <<
	"\nstatus: " << MxTradingStatus::name(l1Data.status) <<
	"\nlast: " << l1Data.last <<
	"\nlastQty: " << l1Data.lastQty <<
	"\nbid: " << l1Data.bid <<
	"\nbidQty: " << l1Data.bidQty <<
	"\nask: " << l1Data.ask <<
	"\naskQty: " << l1Data.askQty <<
	"\ntickDir: " << MxTickDir::name(l1Data.tickDir) <<
	"\nhigh: " << l1Data.high <<
	"\nlow: " << l1Data.low <<
	"\naccVol: " << l1Data.accVol <<
	"\naccVolQty: " << l1Data.accVolQty <<
	"\nmatch: " << l1Data.match <<
	"\nmatchQty: " << l1Data.matchQty <<
	"\nsurplusQty: " << l1Data.surplusQty <<
	"\nflags: " << flags << '\n';
    });
}

void MxMDCore::l2(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (argc != 2) throw CmdUsage();
  lookupOrderBook(this, args, 1, 1, 1,
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
  MxID venueID = side->orderBook()->venueID();
  side->allPxLevels([this, venueID, &out](MxMDPxLevel *pxLevel) -> uintptr_t {
    const MxMDPxLvlData &pxLvlData = pxLevel->data();
    MxMDFlagsStr flags;
    MxMDL2Flags::print(flags, venueID, pxLvlData.flags);
    out << "\n    price: " << pxLevel->price() <<
      " qty: " << pxLvlData.qty <<
      " nOrders: " << pxLvlData.nOrders;
    if (flags) out << " flags: " << flags;
    out << " transactTime: " << m_cmd->timeFmt(pxLvlData.transactTime);
    return 0;
  });
}

void MxMDCore::security(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (argc != 2) throw CmdUsage();
  lookupOrderBook(this, args, 1, 1, 0,
      [&out](MxMDSecurity *sec, MxMDOrderBook *ob) {
    const MxMDSecRefData &refData = sec->refData();
    out <<
      "ID: " << sec->id() <<
      "\nIDSrc: " << refData.idSrc <<
      "\nsymbol: " << refData.symbol <<
      "\naltIDSrc: " << refData.altIDSrc <<
      "\naltSymbol: " << refData.altSymbol;
    if (refData.underVenue) out << "\nunderlying: " <<
      refData.underVenue << '/' <<
      refData.underSegment << '/' <<
      refData.underlying;
    if (*refData.mat) {
      out << "\nmat: " << refData.mat;
      if (*refData.putCall) out <<
	  "\nputCall: " << MxPutCall::name(refData.putCall) <<
	  "\nstrike: " << refData.strike;
    }
    if (*refData.outstandingShares)
      out << "\noutstandingShares: " << refData.outstandingShares;
    if (*refData.adv)
      out << "\nADV: " << refData.adv;
    if (ob) {
      const MxMDLotSizes &lotSizes = ob->lotSizes();
      out <<
	"\nmarket: " << ob->venueID() <<
	"\nsegment: " << ob->segment() <<
	"\nID: " << ob->id() <<
	"\nlot sizes: " <<
	  lotSizes.oddLotSize << ',' <<
	  lotSizes.lotSize << ',' <<
	  lotSizes.blockLotSize <<
	"\ntick sizes:";
      ob->tickSizeTbl()->allTickSizes(
	  [&out](const MxMDTickSize &ts) -> uintptr_t {
	out << "\n  " << ts.minPrice() << '-' << ts.maxPrice() << ' ' <<
	  ts.tickSize();
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
	new (csv.ptr()) MxMDTickSizeCSV::Data{
	    venue->id(), tbl->id(),
	    ts.minPrice(), ts.maxPrice(), ts.tickSize()};
	fn(csv.pod());
	return 0;
      });
    });
  }};
  if (!*venueID)
    md->allVenues(venueFn);
  else
    if (ZmRef<MxMDVenue> venue = md->venue(venueID)) venueFn(venue);
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
	  sec->primaryVenue(), sec->primarySegment(),
	  sec->id(), sec->shard()->id(), sec->refData()};
      fn(csv.pod());
    }
    return 0;
  });
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
      MxMDOrderBookCSV::Data *data = new (csv.ptr()) MxMDOrderBookCSV::Data{
	ob->venueID(), ob->segment(),
	ob->id(), ob->legs(),
	ob->tickSizeTbl()->id(), ob->lotSizes()};
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

void MxMDCore::recordCmd(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (argc < 1 || argc > 2) throw CmdUsage();
  if (!!args.get("stop")) {
    if (argc == 2) throw CmdUsage();
    ZiFile::Path path = m_recorder.path();
    m_recorder.stop();
    if (m_recorder.running())
      out << "failed to stop";
    else
      out << "stopped";
    out << " recording to \"" << path << "\"\n";
    return;
  }
  if (argc != 2) throw CmdUsage();
  ZiFile::Path path = args.get("1");
  if (!m_recorder.start(path))
    out << "failed to start";
  else
    out << "started";
  out << " recording to \"" << path << "\"\n";
}

#define fileERROR(path__, code) \
  raise(ZeEVENT(Error, \
    ([=, path = MxTxtString(path__)](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))
#define fileINFO(path__, code) \
  raise(ZeEVENT(Info, \
    ([=, path = MxTxtString(path__)](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))

void MxMDCore::record(ZuString path)
{
  if (!m_recorder.start(path))
    fileERROR(path, "failed to start recording");
  else
    fileINFO(path, "started recording");
}

void MxMDCore::stopRecording()
{
  m_recorder.stop();
  raise(ZeEVENT(Info, "stopped recording"));
}

void MxMDCore::stopStreaming()
{
  m_snapper.stop();
  m_recorder.stop();
  m_broadcast.eof();
}

void MxMDCore::threadName(ZmThreadName &name, unsigned id)
{
  switch ((int)id) {
    default: name = "MxMDCore"; break;
    case ThreadID::RecReceiver: name = "MxMDCore.RecReceiver"; break;
    case ThreadID::RecSnapper: name = "MxMDCore.RecSnapper"; break;
    case ThreadID::Snapper: name = "MxMDCore.Snapper"; break;
  }
}

void MxMDCore::subscribeCmd(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (ZtZString id_ = args.get("stop")) {
    ZuBox<int> id = ZvCf::toInt("spin", id_, 0, 63);
    m_broadcast.reqDetach(id);
    m_broadcast.close();
    out << "detached " << id << "\n";
    return;
  }
  if (argc != 2) throw CmdUsage();
  ZiRingParams ringParams(args.get("1"), m_broadcast.config());
  if (ZtZString spin = args.get("spin"))
    ringParams.spin(ZvCf::toInt("spin", spin, 0, INT_MAX));
  if (ZtZString timeout = args.get("timeout"))
    ringParams.timeout(ZvCf::toInt("timeout", timeout, 0, 3600));
  if (!m_broadcast.open())
    throw ZtZString() << '"' << m_broadcast.config().name() <<
	"\": failed to open IPC shared memory ring buffer";
  m_snapper.snap(ringParams);
}

void MxMDCore::replayCmd(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (argc < 1 || argc > 2) throw CmdUsage();
  if (!!args.get("stop")) {
    ReplayGuard guard(m_replayLock);
    if (!!m_replayFile) {
      m_replayFile.close();
      out << "stopped replaying to \"" << m_replayPath << "\"\n";
    }
    return;
  }
  if (argc != 2) throw CmdUsage();
  ZtZString path = args.get("1");
  ReplayGuard guard(m_replayLock);
  if (m_replayPath != path) {
    if (!!m_replayFile) {
      m_replayFile.close();
      out << "stopped replaying to \"" << m_replayPath << "\"";
    }
  }
  if (!replayStart(guard, path)) {
    if (out) out << "; ";
    out << "failed to start replaying from \"" << path << "\"";
    throw out;
  }
  if (out) out << "\n";
  out << "started replaying from \"" << path << "\"\n";
}

void MxMDCore::replay(ZuString path, MxDateTime begin, bool filter)
{
  m_mx->del(&m_timer);
  ZtString message;
  {
    ReplayGuard guard(m_replayLock);
    m_timerNext = !begin ? ZmTime() : begin.zmTime();
    m_replayFilter = filter;
    if (!replayStart(guard, path))
      fileERROR(path, "failed to start replaying");
    else
      fileINFO(path, "started replaying");
  }
}

void MxMDCore::stopReplaying()
{
  ReplayGuard guard(m_replayLock);
  if (!!m_replayFile) {
    m_replayFile.close();
    m_replayMsg = 0;
    m_timerNext = ZmTime();
  }
}

bool MxMDCore::replayStart(Guard &guard, ZuString path)
{
  using namespace MxMDStream;

  if (!!m_replayFile) m_replayFile.close();
  ZeError e;
  if (m_replayFile.open(path, ZiFile::ReadOnly, 0, &e) != Zi::OK) {
    guard.unlock();
    fileERROR(path, e);
    return false;
  }
  m_replayPath = path;
  try {
    FileHdr hdr(m_replayFile, &e);
    m_replayVersion = ZuMkPair(hdr.vmajor, hdr.vminor);
  } catch (const FileHdr::IOError &) {
    guard.unlock();
    fileERROR(path, e);
    return false;
  } catch (const FileHdr::InvalidFmt &) {
    guard.unlock();
    fileERROR(m_replayPath, "invalid format");
    return false;
  }
  if (!m_replayMsg) m_replayMsg = new Msg();
  m_mx->add(ZmFn<>::Member<&MxMDCore::replay2>::fn(this));
  return true;
}

void MxMDCore::replay2()
{
  using namespace MxMDStream;

  ZeError e;

  {
    ZmRef<Msg> msg;

    {
      ReplayGuard guard(m_replayLock);
      if (!m_replayFile) return;
      msg = m_replayMsg;
      Frame *frame = msg->frame();
  // retry:
      int n = m_replayFile.read(frame, sizeof(Frame), &e);
      if (n == Zi::IOError) goto error;
      if (n == Zi::EndOfFile || (unsigned)n < sizeof(Frame)) goto eof;
      if (frame->len > sizeof(Buf)) goto lenerror;
      n = m_replayFile.read(frame->ptr(), frame->len, &e);
      if (n == Zi::IOError) goto error;
      if (n == Zi::EndOfFile || (unsigned)n < frame->len) goto eof;

      if (frame->sec) {
	ZmTime next((time_t)frame->sec, (int32_t)frame->nsec);

	while (!!m_timerNext && next > m_timerNext) {
	  MxDateTime now = m_timerNext, timerNext;
	  guard.unlock();
	  this->handler()->timer(now, timerNext);
	  guard = Guard(m_replayLock);
	  m_timerNext = !timerNext ? ZmTime() : timerNext.zmTime();
	}
      }
    }

    pad(msg->frame());
    apply(msg->frame());
    m_mx->add(ZmFn<>::Member<&MxMDCore::replay2>::fn(this));
  }

  return;

eof:
  fileINFO(m_replayPath, "EOF");
  this->handler()->eof(this);
  return;

error:
  fileERROR(m_replayPath, e);
  return;

lenerror:
  {
    uint64_t offset = m_replayFile.offset();
    offset -= sizeof(Frame);
    fileERROR(m_replayPath,
	"message length >" << ZuBoxed(sizeof(Buf)) <<
	" at offset " << ZuBoxed(offset));
  }
}

void MxMDCore::pad(Frame *frame)
{
  using namespace MxMDStream;

  switch ((int)frame->type) {
    case Type::AddTickSizeTbl:	frame->pad<AddTickSizeTbl>(); break;
    case Type::ResetTickSizeTbl: frame->pad<ResetTickSizeTbl>(); break;
    case Type::AddTickSize:	frame->pad<AddTickSize>(); break;
    case Type::AddSecurity:	frame->pad<AddSecurity>(); break;
    case Type::UpdateSecurity:	frame->pad<UpdateSecurity>(); break;
    case Type::AddOrderBook:	frame->pad<AddOrderBook>(); break;
    case Type::DelOrderBook:	frame->pad<DelOrderBook>(); break;
    case Type::AddCombination:	frame->pad<AddCombination>(); break;
    case Type::DelCombination:	frame->pad<DelCombination>(); break;
    case Type::UpdateOrderBook:	frame->pad<UpdateOrderBook>(); break;
    case Type::TradingSession:	frame->pad<TradingSession>(); break;
    case Type::L1:		frame->pad<L1>(); break;
    case Type::PxLevel:		frame->pad<PxLevel>(); break;
    case Type::L2:		frame->pad<L2>(); break;
    case Type::AddOrder:	frame->pad<AddOrder>(); break;
    case Type::ModifyOrder:	frame->pad<ModifyOrder>(); break;
    case Type::CancelOrder:	frame->pad<CancelOrder>(); break;
    case Type::ResetOB:		frame->pad<ResetOB>(); break;
    case Type::AddTrade:	frame->pad<AddTrade>(); break;
    case Type::CorrectTrade:	frame->pad<CorrectTrade>(); break;
    case Type::CancelTrade:	frame->pad<CancelTrade>(); break;
    case Type::RefDataLoaded:	frame->pad<RefDataLoaded>(); break;
    default: break;
  }
}

void MxMDCore::apply(Frame *frame)
{
  using namespace MxMDStream;

  switch ((int)frame->type) {
    case Type::AddTickSizeTbl:
      {
	const AddTickSizeTbl &obj = frame->as<AddTickSizeTbl>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  venue->addTickSizeTbl(obj.id);
      }
      break;
    case Type::ResetTickSizeTbl:
      {
	const ResetTickSizeTbl &obj = frame->as<ResetTickSizeTbl>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  if (ZmRef<MxMDTickSizeTbl> tickSizeTbl = venue->tickSizeTbl(obj.id))
	    tickSizeTbl->reset();
      }
      break;
    case Type::AddTickSize:
      {
	const AddTickSize &obj = frame->as<AddTickSize>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  if (ZmRef<MxMDTickSizeTbl> tbl = venue->addTickSizeTbl(obj.id))
	    tbl->addTickSize(
		obj.minPrice, obj.maxPrice, obj.tickSize);
      }
      break;
    case Type::AddSecurity:
      {
	const AddSecurity &obj = frame->as<AddSecurity>();
	addSecurity(
	    obj.key.venue(), obj.key.segment(), obj.key.id(),
	    obj.shard, obj.refData);
      }
      break;
    case Type::UpdateSecurity:
      {
	const UpdateSecurity &obj = frame->as<UpdateSecurity>();
	if (ZmRef<MxMDSecurity> security = m_allSecurities.findKey(obj.key))
	  security->update(obj.refData);
      }
      break;
    case Type::AddOrderBook:
      {
	const AddOrderBook &obj = frame->as<AddOrderBook>();
	if (ZmRef<MxMDSecurity> security =
	      m_allSecurities.findKey(obj.security))
	  if (ZmRef<MxMDVenue> venue = this->venue(obj.key.venue()))
	    if (ZmRef<MxMDTickSizeTbl> tickSizeTbl =
		  venue->tickSizeTbl(obj.tickSizeTbl))
	      security->addOrderBook(
		  obj.key.venue(), obj.key.segment(), obj.key.id(),
		  tickSizeTbl, obj.lotSizes);
      }
      break;
    case Type::DelOrderBook:
      {
	const DelOrderBook &obj = frame->as<DelOrderBook>();
	if (ZmRef<MxMDOrderBook> orderBook = m_allOrderBooks.findKey(obj.key))
	  orderBook->security()->delOrderBook(
	      obj.key.venue(), obj.key.segment());
      }
      break;
    case Type::AddCombination:
      {
	const AddCombination &obj = frame->as<AddCombination>();
	ZmRef<MxMDSecurity> securities[MxMDNLegs];
	MxUInt i, n = obj.legs;
	for (i = 0; i < n; i++)
	  if (!(securities[i] = m_allSecurities.findKey(
		  obj.securities[i]))) break;
	if (i < n) break;
	if (ZmRef<MxMDVenue> venue = this->venue(obj.key.venue()))
	  if (ZmRef<MxMDTickSizeTbl> tickSizeTbl = venue->tickSizeTbl(
		obj.tickSizeTbl))
	    venue->addCombination(
		obj.key.segment(), obj.key.id(),
		n, securities, obj.sides, obj.ratios,
		tickSizeTbl, obj.lotSizes);
      }
      break;
    case Type::DelCombination:
      {
	const DelCombination &obj = frame->as<DelCombination>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.key.venue()))
	  venue->delCombination(obj.key.segment(), obj.key.id());
      }
      break;
    case Type::UpdateOrderBook:
      {
	const UpdateOrderBook &obj = frame->as<UpdateOrderBook>();
	if (ZmRef<MxMDOrderBook> orderBook = m_allOrderBooks.findKey(obj.key))
	  if (ZmRef<MxMDVenue> venue = this->venue(obj.key.venue()))
	    if (ZmRef<MxMDTickSizeTbl> tickSizeTbl = venue->tickSizeTbl(
		  obj.tickSizeTbl))
	      orderBook->update(tickSizeTbl, obj.lotSizes);
      }
      break;
    case Type::TradingSession:
      {
	const TradingSession &obj = frame->as<TradingSession>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  venue->tradingSession(
	      obj.segment, obj.session, obj.stamp);
      }
      break;
    case Type::L1:
      {
	const L1 &obj = frame->as<L1>();
	if (ZmRef<MxMDOrderBook> orderBook =
	      m_allOrderBooks.findKey(obj.key))
	  if (!m_replayFilter || orderBook->handler())
	    orderBook->l1(const_cast<MxMDL1Data &>(obj.data));
      }
      break;
    case Type::PxLevel:
      {
	const PxLevel &obj = frame->as<PxLevel>();
	if (ZmRef<MxMDOrderBook> orderBook = m_allOrderBooks.findKey(obj.key))
	  if (!m_replayFilter || orderBook->handler())
	    orderBook->pxLevel(
		obj.side, obj.transactTime, obj.delta, obj.price, obj.qty,
		obj.nOrders, obj.flags);
      }
      break;
    case Type::L2:
      {
	const L2 &obj = frame->as<L2>();
	if (ZmRef<MxMDOrderBook> orderBook = m_allOrderBooks.findKey(obj.key))
	  if (!m_replayFilter || orderBook->handler())
	    orderBook->l2(obj.stamp, obj.updateL1);
      }
      break;
    case Type::AddOrder:
      {
	const AddOrder &obj = frame->as<AddOrder>();
	if (ZmRef<MxMDOrderBook> orderBook = m_allOrderBooks.findKey(obj.key))
	  if (!m_replayFilter || orderBook->handler())
	    orderBook->addOrder(
		obj.orderID, obj.orderIDScope, obj.transactTime,
		obj.side, obj.rank, obj.price, obj.qty, obj.flags);
      }
      break;
    case Type::ModifyOrder:
      {
	const ModifyOrder &obj = frame->as<ModifyOrder>();
	if (ZmRef<MxMDOrderBook> orderBook = m_allOrderBooks.findKey(obj.key))
	  if (!m_replayFilter || orderBook->handler())
	    orderBook->modifyOrder(
		obj.orderID, obj.orderIDScope, obj.transactTime,
		obj.side, obj.rank, obj.price, obj.qty, obj.flags);
      }
      break;
    case Type::CancelOrder:
      {
	const CancelOrder &obj = frame->as<CancelOrder>();
	if (ZmRef<MxMDOrderBook> orderBook = m_allOrderBooks.findKey(obj.key))
	  if (!m_replayFilter || orderBook->handler())
	  orderBook->cancelOrder(
	      obj.orderID, obj.orderIDScope,
	      obj.transactTime, obj.side);
      }
      break;
    case Type::ResetOB:
      {
	const ResetOB &obj = frame->as<ResetOB>();
	if (ZmRef<MxMDOrderBook> orderBook = m_allOrderBooks.findKey(obj.key))
	  if (!m_replayFilter || orderBook->handler())
	    orderBook->reset(obj.transactTime);
      }
      break;
    case Type::AddTrade:
      {
	const AddTrade &obj = frame->as<AddTrade>();
	if (ZmRef<MxMDOrderBook> orderBook = m_allOrderBooks.findKey(obj.key))
	  if (!m_replayFilter || orderBook->handler())
	    orderBook->addTrade(
		obj.tradeID, obj.transactTime, obj.price, obj.qty);
      }
      break;
    case Type::CorrectTrade:
      {
	const CorrectTrade &obj = frame->as<CorrectTrade>();
	if (ZmRef<MxMDOrderBook> orderBook = m_allOrderBooks.findKey(obj.key))
	  if (!m_replayFilter || orderBook->handler())
	    orderBook->correctTrade(
		obj.tradeID, obj.transactTime, obj.price, obj.qty);
      }
      break;
    case Type::CancelTrade:
      {
	const CancelTrade &obj = frame->as<CancelTrade>();
	if (ZmRef<MxMDOrderBook> orderBook = m_allOrderBooks.findKey(obj.key))
	  if (!m_replayFilter || orderBook->handler())
	    orderBook->cancelTrade(
		obj.tradeID, obj.transactTime, obj.price, obj.qty);
      }
      break;
    case Type::RefDataLoaded:
      {
	const RefDataLoaded &obj = frame->as<RefDataLoaded>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  this->loaded(venue);
      }
    default:
      break;
  }
}

ZmRef<MxMDVenue> MxMDCore::venue(MxID id)
{
  ZmRef<MxMDVenue> venue;
  if (ZuLikely(venue = m_venues.findKey(id))) return venue;
  venue = new MxMDVenue(id, m_localFeed, this);
  addVenue(venue);
  return venue;
}

void MxMDCore::startTimer(MxDateTime begin)
{
  ZmTime next = !begin ? ZmTimeNow() : begin.zmTime();
  {
    ReplayGuard guard(m_replayLock);
    m_timerNext = next;
  }
  m_mx->add(ZmFn<>::Member<&MxMDCore::timer>::fn(this), next, &m_timer);
}

void MxMDCore::stopTimer()
{
  m_mx->del(&m_timer);
  ReplayGuard guard(m_replayLock);
  m_timerNext = ZmTime();
}

void MxMDCore::timer()
{
  MxDateTime now, next;
  {
    ReplayGuard guard(m_replayLock);
    now = m_timerNext;
  }
  this->handler()->timer(now, next);
  {
    ReplayGuard guard(m_replayLock);
    m_timerNext = !next ? ZmTime() : next.zmTime();
  }
  if (!next)
    m_mx->del(&m_timer);
  else
    m_mx->add(ZmFn<>::Member<&MxMDCore::timer>::fn(this),
	next.zmTime(), &m_timer);
}
