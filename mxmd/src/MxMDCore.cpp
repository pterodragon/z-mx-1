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
      "recorder {\n"
        "id recorder\n"
	"rxThread 3\n"
	"txThread 3\n"
      "}\n"
      "publisher {\n"
        "id publisher\n"
	"rxThread 3\n"
	"txThread 3\n"
      "}\n"
      "subscriber {\n"
        "id subscriber\n"
	"rxThread 3\n"
	"txThread 3\n"
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
  return ZmSingleton<MxMDCore, 0>::instance();
}

MxMDCore::MxMDCore(ZmRef<Mx> mx) :
  MxMDLib(mx),
  m_mx(ZuMv(mx)),
  m_broadcast(this),
  m_snapper(this),
  m_recorder(this),
  m_publisher(this),
  m_subscriber(this)
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

  m_recordCfPath = cf->get("record");

  if (ZmRef<ZvCf> ringCf = cf->subset("ring", false))
    m_broadcast.init(ringCf);

  if (ZmRef<ZvCf> threadsCf = cf->subset("threads", false)) {
    if (ZmRef<ZvCf> threadCf = threadsCf->subset("recReceiver", false))
      m_recReceiverParams.init(threadCf);
    if (ZmRef<ZvCf> threadCf = threadsCf->subset("recSnapper", false))
      m_recSnapperParams.init(threadCf);
    if (ZmRef<ZvCf> threadCf = threadsCf->subset("snapper", false))
      m_snapperParams.init(threadCf);
  }

  m_replayCfPath = cf->get("replay");

  if (ZmRef<ZvCf> cmdCf = cf->subset("cmd", false)) {
    m_cmd = new MxMDCore::CmdServer(this);
    m_cmd->init(cmdCf);
    initCmds();
  }

  m_recorder.init();
  m_publisher.init();
  m_subscriber.init();

  ZeLOG(Info, "MxMDLib - initialized...");
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

  if (m_cmd) {
    raise(ZeEVENT(Info, "starting cmd server..."));
    m_cmd->start();
  }

  if (m_recordCfPath) record(m_recordCfPath);
  if (m_replayCfPath) replay(m_replayCfPath);

  if (cf()->subset("publisher", false, true)->get("multicastAddr")) publish();
  if (cf()->subset("subscriber", false, true)->get("username")) subscribe();

  raise(ZeEVENT(Info, "starting feeds..."));
  allFeeds([](MxMDFeed *feed) { try { feed->start(); } catch (...) { } });
}

void MxMDCore::stop()
{
  Guard guard(m_stateLock);

  raise(ZeEVENT(Info, "stopping feeds..."));
  allFeeds([](MxMDFeed *feed) { try { feed->stop(); } catch (...) { } });

  if (m_mx) {
    stopReplaying();
    stopRecording();
    stopPublish();
    stopSubscribe();
    stopStreaming();
    broadcast().close();
    
    
    std::cout << "after stopPublish" << std::endl;
    
    
    
  }

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
    ZuString timezone = cf->get("timezone"); // default to system tz
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

void MxMDCmd::addCmd(ZuString name, ZuString syntax,
    CmdFn fn, ZtString brief, ZtString usage)
{
  MxMDCore::CmdServer *server = static_cast<MxMDCore *>(this)->m_cmd;
  if (!server) return;
  {
    ZmRef<ZvCf> cf = server->m_syntax->subset(name, true);
    cf->fromString(syntax, false);
    cf->set("help:type", "flag");
  }
  if (MxMDCore::CmdServer::Cmds::NodeRef cmd = server->m_cmds.find(name))
    cmd->val() = ZuMkTuple(ZuMv(fn), ZuMv(brief), ZuMv(usage));
  else
    server->m_cmds.add(name, ZuMkTuple(ZuMv(fn), ZuMv(brief), ZuMv(usage)));
}

void MxMDCore::CmdServer::rcvd(
  ZvCmdLine *line, const ZvInvocation &inv, ZvAnswer &ans)
{
  ZmRef<ZvCf> cf = new ZvCf();
  ZuString name;
  Cmds::NodeRef cmd;
  try {
    cf->fromCLI(m_syntax, inv.cmd());
    name = cf->get("0");
    cmd = m_cmds.find(name);
    if (!cmd) throw ZtString("unknown command");
    if (cf->getInt("help", 0, 1, false, 0))
      ans.make(ZvCmd::Success, Ze::Info,
	       ZvAnswerArgs, cmd->val().p3());
    else {
      CmdArgs args;
      ZvCf::Iterator i(cf);
      {
	ZuString arg;
	const ZtArray<ZtString> *values;
	while (values = i.getMultiple(arg, 0, INT_MAX)) args.add(arg, values);
      }
      ZtArray<char> out;
      (cmd->val().p1())(args, out);
      ans.make(ZvCmd::Success, out);
    }
  } catch (const CmdUsage &) {
    ans.make(ZvCmd::Fail, Ze::Warning, ZvAnswerArgs, cmd->val().p3().data());
  } catch (const ZvError &e) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtString() << '"' << name << "\": " << e);
  } catch (const ZeError &e) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtString() << '"' << name << "\": " << e);
  } catch (const ZtString &s) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtString() << '"' << name << "\": " << s);
  } catch (const ZtArray<char> &s) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtString() << '"' << name << "\": " << s);
  } catch (...) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtString() << '"' << name << "\": unknown exception");
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
    auto i = m_cmds.readIterator();
    while (Cmds::NodeRef cmd = i.iterate()) {
      help += cmd->key();
      help += " -- ";
      help += cmd->val().p2();
      help += "\n";
    }
  }
}

ZtString MxMDCmd::lookupSyntax()
{
  return 
    "S src src { type scalar } "
    "m market market { type scalar } "
    "s segment segment { type scalar }";
}

ZtString MxMDCmd::lookupOptions()
{
  return
    "    -S --src=SRC\t- symbol ID source is SRC\n"
    "\t(CUSIP|SEDOL|QUIK|ISIN|RIC|EXCH|CTA|BSYM|BBGID|FX|CRYPTO)\n"
    "    -m --market=MIC\t - market MIC, e.g. XTKS\n"
    "    -s --segment=SEGMENT\t- market segment SEGMENT\n";
}

void MxMDCmd::lookupSecurity(
    MxMDLib *md, const MxMDCmd::CmdArgs &args, unsigned index,
    bool secRequired, ZmFn<MxMDSecurity *> fn)
{
  ZuString symbol = args.get(ZuStringN<16>(ZuBoxed(index)));
  MxID venue = args.get("market");
  MxID segment = args.get("segment");
  thread_local ZmSemaphore sem;
  bool notFound = 0;
  if (ZuString src_ = args.get("src")) {
    MxEnum src = MxSecIDSrc::lookup(src_);
    MxSecSymKey key{src, symbol};
    md->secInvoke(key,
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
    if (!*venue) throw MxMDCmd::CmdUsage();
    MxSecKey key{venue, segment, symbol};
    md->secInvoke(key,
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
    throw ZtString() << "order book " <<
	MxSecKey{venue, segment, args.get(ZuStringN<16>(ZuBoxed(index)))} <<
	" not found";
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
  std::cout << "start recording" << std::endl;
	
	
  switch (m_mx->stateLocked([this, path](int state) {
    if (state != ZmScheduler::Running && state != ZmScheduler::Starting) {
      m_recordCfPath = path;
      return 0;
    }
    if (!m_recorder.start(path)) return -1;
    return 1;
  })) {
    case -1:
      fileERROR(path, "failed to start recording");
      break;
    case 1:
      fileINFO(path, "started recording");
      break;
  }
}

void MxMDCore::stopRecording()
{
  m_recorder.stop();
  raise(ZeEVENT(Info, "stopped recording"));
}

void MxMDCore::publish()
{
  raise(ZeEVENT(Info, "start publishing"));
  m_publisher.start();
}

void MxMDCore::stopPublish()
{
  m_publisher.stop();
  raise(ZeEVENT(Info, "stopped publishing"));
}

void MxMDCore::subscribe()
{
  raise(ZeEVENT(Info, "start subscribing"));
  m_subscriber.start();
}

void MxMDCore::stopSubscribe()
{
  m_subscriber.stop();
  raise(ZeEVENT(Info, "stopped subscribing"));
}

void MxMDCore::stopStreaming()
{
  m_snapper.stop();
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
  if (ZuString id_ = args.get("stop")) {
    ZuBox<int> id = ZvCf::toInt("spin", id_, 0, 63);
    m_broadcast.reqDetach(id);
    m_broadcast.close();
    out << "detached " << id << "\n";
    return;
  }
  if (argc != 2) throw CmdUsage();
  ZiRingParams ringParams(args.get("1"), m_broadcast.config());
  if (ZuString spin = args.get("spin"))
    ringParams.spin(ZvCf::toInt("spin", spin, 0, INT_MAX));
  if (ZuString timeout = args.get("timeout"))
    ringParams.timeout(ZvCf::toInt("timeout", timeout, 0, 3600));
  if (!m_broadcast.open())
    throw ZtString() << '"' << m_broadcast.config().name() <<
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
  ZuString path = args.get("1");
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

void MxMDCore::apply(const Frame *frame)
{
  using namespace MxMDStream;

  switch ((int)frame->type) {
    case Type::AddTickSizeTbl:
      {
	const AddTickSizeTbl &obj = frame->as<AddTickSizeTbl>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  venue->addTickSizeTbl(obj.id, obj.pxNDP);
      }
      break;
    case Type::ResetTickSizeTbl:
      {
	const ResetTickSizeTbl &obj = frame->as<ResetTickSizeTbl>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  if (ZmRef<MxMDTickSizeTbl> tbl = venue->tickSizeTbl(obj.id))
	    tbl->reset();
      }
      break;
    case Type::AddTickSize:
      {
	const AddTickSize &obj = frame->as<AddTickSize>();
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
	const AddSecurity &obj = frame->as<AddSecurity>();
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
	const UpdateSecurity &obj = frame->as<UpdateSecurity>();
	MxMDLib::secInvoke(obj.key, [refData = obj.refData,
	    transactTime = obj.transactTime](MxMDSecurity *sec) {
	  if (sec) sec->update(refData, transactTime);
	});
      }
      break;
    case Type::AddOrderBook:
      {
	const AddOrderBook &obj = frame->as<AddOrderBook>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.key.venue()))
	  if (ZmRef<MxMDTickSizeTbl> tbl =
		  venue->tickSizeTbl(obj.tickSizeTbl))
	    MxMDLib::secInvoke(obj.security, [
		key = obj.key, tbl = ZuMv(tbl),
		qtyNDP = obj.qtyNDP, lotSizes = obj.lotSizes,
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
	const DelOrderBook &obj = frame->as<DelOrderBook>();
	obInvoke(obj.key, [transactTime = obj.transactTime](
	      MxMDOrderBook *ob) {
	  if (ob) ob->security()->delOrderBook(
	      ob->venueID(), ob->segment(), transactTime);
	});
      }
      break;
    case Type::AddCombination:
      {
	const AddCombination &obj = frame->as<AddCombination>();
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
	const DelCombination &obj = frame->as<DelCombination>();
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
	const UpdateOrderBook &obj = frame->as<UpdateOrderBook>();
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
	const TradingSession &obj = frame->as<TradingSession>();
	if (ZmRef<MxMDVenue> venue = this->venue(obj.venue))
	  venue->tradingSession(
	      MxMDSegment{obj.segment, obj.session, obj.stamp});
      }
      break;
    case Type::L1:
      {
	const L1 &obj = frame->as<L1>();
	obInvoke(obj.key, [
	    data = obj.data,
	    replayFilter = m_replayFilter](MxMDOrderBook *ob) mutable {
	  // inconsistent NDP handled within MxMDOrderBook::l1()
	  if (ob && (!replayFilter || ob->handler())) ob->l1(data);
	});
      }
      break;
    case Type::PxLevel:
      {
	const PxLevel &obj = frame->as<PxLevel>();
	obInvoke(obj.key, [
	    side = obj.side, transactTime = obj.transactTime,
	    delta = obj.delta, pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty,
	    nOrders = obj.nOrders, flags = obj.flags,
	    replayFilter = m_replayFilter](MxMDOrderBook *ob) mutable {
	  if (ob && (!replayFilter || ob->handler())) {
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
	const L2 &obj = frame->as<L2>();
	obInvoke(obj.key, [
	    stamp = obj.stamp, updateL1 = obj.updateL1,
	    replayFilter = m_replayFilter](MxMDOrderBook *ob) {
	  if (ob && (!replayFilter || ob->handler()))
	    ob->l2(stamp, updateL1);
	});
      }
      break;
    case Type::AddOrder:
      {
	const AddOrder &obj = frame->as<AddOrder>();
	obInvoke(obj.key, [
	    orderID = obj.orderID, transactTime = obj.transactTime,
	    side = obj.side, rank = obj.rank,
	    pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty, flags = obj.flags,
	    replayFilter = m_replayFilter](MxMDOrderBook *ob) mutable {
	  if (ob && (!replayFilter || ob->handler())) {
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
	const ModifyOrder &obj = frame->as<ModifyOrder>();
	obInvoke(obj.key, [
	    orderID = obj.orderID, transactTime = obj.transactTime,
	    side = obj.side, rank = obj.rank,
	    pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty, flags = obj.flags,
	    replayFilter = m_replayFilter](MxMDOrderBook *ob) mutable {
	  if (ob && (!replayFilter || ob->handler())) {
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
	const CancelOrder &obj = frame->as<CancelOrder>();
	obInvoke(obj.key, [
	    orderID = obj.orderID,
	    transactTime = obj.transactTime,
	    side = obj.side,
	    replayFilter = m_replayFilter](MxMDOrderBook *ob) {
	  if (ob && (!replayFilter || ob->handler()))
	    ob->cancelOrder(orderID, transactTime, side);
	});
      }
      break;
    case Type::ResetOB:
      {
	const ResetOB &obj = frame->as<ResetOB>();
	obInvoke(obj.key, [
	    transactTime = obj.transactTime,
	    replayFilter = m_replayFilter](MxMDOrderBook *ob) {
	  if (ob && (!replayFilter || ob->handler()))
	    ob->reset(transactTime);
	});
      }
      break;
    case Type::AddTrade:
      {
	const AddTrade &obj = frame->as<AddTrade>();
	obInvoke(obj.key, [
	    tradeID = obj.tradeID, transactTime = obj.transactTime,
	    pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty,
	    replayFilter = m_replayFilter](MxMDOrderBook *ob) mutable {
	  if (ob && (!replayFilter || ob->handler())) {
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
	const CorrectTrade &obj = frame->as<CorrectTrade>();
	obInvoke(obj.key, [
	    tradeID = obj.tradeID, transactTime = obj.transactTime,
	    pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty,
	    replayFilter = m_replayFilter](MxMDOrderBook *ob) mutable {
	  if (ob && (!replayFilter || ob->handler())) {
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
	const CancelTrade &obj = frame->as<CancelTrade>();
	obInvoke(obj.key, [
	    tradeID = obj.tradeID, transactTime = obj.transactTime,
	    pxNDP = obj.pxNDP, qtyNDP = obj.qtyNDP,
	    price = obj.price, qty = obj.qty,
	    replayFilter = m_replayFilter](MxMDOrderBook *ob) mutable {
	  if (ob && (!replayFilter || ob->handler())) {
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
	const RefDataLoaded &obj = frame->as<RefDataLoaded>();
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

MxMDCore::Publisher::TCP::TCP(MxMDCore::Publisher *publisher, const ZiCxnInfo &ci) :
    ZiConnection(publisher->m_core->mx(), ci), m_publisher(publisher)
{
  using namespace MxMDPubSub::TCP;
  m_in = new InMsg();
}

void MxMDCore::Publisher::TCP::connected(ZiIOContext &io)
{
  m_publisher->m_core->raise(ZeEVENT(Info, "Publisher TCP connected"));

  using namespace MxMDPubSub::TCP;
  m_publisher->tcpConnected2(this);
  m_in->fn() = InMsg::Fn::Member<&MxMDCore::Publisher::TCP::process>::fn(this);
  m_in->recv(io);
}

void MxMDCore::Publisher::tcpConnected2(MxMDCore::Publisher::TCP *tcp)
{
  {
    ZiIP ip = tcp->info().remoteIP;
    uint16_t port = tcp->info().remotePort;
    m_core->raise(ZeEVENT(Info,
	      ([ip, port](const ZeEvent &, ZmStream &s) {
		s << "TCP connected "
		  << ip << ':' << ZuBoxed(port);
	      })));
  }
  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);
    m_tcp = tcp;
  }
}

void MxMDCore::Publisher::TCP::process(MxMDPubSub::TCP::InMsg *, ZiIOContext &io)
{
  //using namespace MxMDPubSub::TCP;
  using namespace MxMDStream;

  Frame &hdr = m_in->as<Frame>();

  switch ((int)hdr.type) {
    case Type::Login: {
      m_publisher->m_core->raise(ZeEVENT(Info, "Publisher TCP Login recv"));

      const Login &data = hdr.as<Login>();
      std::cout << "username: " << data.username << std::endl;
      std::cout << "password: " << data.password << std::endl;
      
      //// process login.....

      m_publisher->snapStart();

      break;
    }
    default:
      m_publisher->m_core->raise(ZeEVENT(Error,
          [type = (int)hdr.type](const ZeEvent &, ZmStream &s) {
	    s << "unexpected message type '" << type << '\'';
	  }));
  }
  m_in->recv(io);
}

void MxMDCore::Publisher::TCP::send(const Frame *frame)
{
  m_publisher->m_core->raise(ZeEVENT(Info, "Publisher TCP send"));
	
  using namespace MxMDStream;

  switch ((int)frame->type) {
    case Type::AddTickSizeTbl:	send_<AddTickSizeTbl>(frame); break;
    case Type::ResetTickSizeTbl: send_<ResetTickSizeTbl>(frame); break;
    case Type::AddTickSize:	send_<AddTickSize>(frame); break;
    case Type::AddSecurity:	send_<AddSecurity>(frame); break;
    case Type::UpdateSecurity:	send_<UpdateSecurity>(frame); break;
    case Type::AddOrderBook:	send_<AddOrderBook>(frame); break;
    case Type::DelOrderBook:	send_<DelOrderBook>(frame); break;
    case Type::AddCombination:	send_<AddCombination>(frame); break;
    case Type::DelCombination:	send_<DelCombination>(frame); break;
    case Type::UpdateOrderBook:	send_<UpdateOrderBook>(frame); break;
    case Type::TradingSession:	send_<TradingSession>(frame); break;
    case Type::L1:		send_<L1>(frame); break;
    case Type::PxLevel:		send_<PxLevel>(frame); break;
    case Type::L2:		send_<L2>(frame); break;
    case Type::AddOrder:	send_<AddOrder>(frame); break;
    case Type::ModifyOrder:	send_<ModifyOrder>(frame); break;
    case Type::CancelOrder:	send_<CancelOrder>(frame); break;
    case Type::ResetOB:		send_<ResetOB>(frame); break;
    case Type::AddTrade:	send_<AddTrade>(frame); break;
    case Type::CorrectTrade:	send_<CorrectTrade>(frame); break;
    case Type::CancelTrade:	send_<CancelTrade>(frame); break;
    case Type::RefDataLoaded:	send_<RefDataLoaded>(frame); break;
    
    case Type::EndOfSnapshot:
      send_<EndOfSnapshot>(frame);
      std::cout << "Publisher TCP send EndOfSnapshot +++++++" << std::endl;
      break;
    
    default:
      m_publisher->m_core->raise(ZeEVENT(Info, "Publisher TCP send default"));
    
      break;
  }
}

void MxMDCore::Publisher::TCP::disconnect()
{
  ZiConnection::disconnect();
}

MxMDCore::Publisher::UDP::UDP(MxMDCore::Publisher *publisher, const ZiCxnInfo &ci, const ZiSockAddr &dest) :
    ZiConnection(publisher->m_core->mx(), ci),
    m_publisher(publisher),
    m_remoteAddr(dest) { }

void MxMDCore::Publisher::UDP::connected(ZiIOContext &io)
{
  m_publisher->udpConnected2(this, io);
}

void MxMDCore::Publisher::UDP::disconnect()
{
  ZiConnection::disconnect();
}

ZiConnection *MxMDCore::Publisher::tcpConnected(const ZiCxnInfo &ci)
{
  m_core->raise(ZeEVENT(Info, "Publisher tcpConnected"));
  return new TCP(this, ci);
}

void MxMDCore::Publisher::udpConnect()
{
  ZmRef<ZvCf> cf = m_core->cf()->subset("publisher", false, true);

  ZiIP ip = cf->get("multicastAddr", true).data();
  auto port = static_cast<uint16_t>(strtoul(cf->get("multicastPort", true).data(), nullptr, 10));

  ZiCxnOptions options;
  options.udp(true);
  options.multicast(true);
  
  options.mreq(ZiMReq(ip, "127.0.0.1"));

  ZiIP mif("127.0.0.1");
  options.mif(mif);
  
  options.loopBack(true);
  
  
  m_dest = ZiSockAddr("127.0.0.1", port);
  
  

  m_core->raise(ZeEVENT(Info, "************ Publisher udpConnect ************"));
  options.print(std::cout);
  std::cout << std::endl;
  
  

  m_core->mx()->udp(
    ZiConnectFn::Member<&MxMDCore::Publisher::udpConnected>::fn(this),
    ZiFailFn::Member<&MxMDCore::Publisher::udpConnectFailed>::fn(this),
    ZiIP(), 27412, ZiIP(), 0, options);
}

ZiConnection *MxMDCore::Publisher::udpConnected(const ZiCxnInfo &ci)
{
  m_core->raise(ZeEVENT(Info, "Publisher udpConnected"));
  return new UDP(this, ci, m_dest);
}

void MxMDCore::Publisher::udpConnected2(MxMDCore::Publisher::UDP *udp, ZiIOContext &io)
{
  m_core->raise(ZeEVENT(Info, "Publisher udpConnected2"));
  ZmGuard<ZmLock> stateGuard(m_stateLock);
  m_udp = udp;
  
  
  
  m_link->txInvoke([](Tx *tx) {//////////////////////////////////////////////
	tx->start();
	
      });
  
}

void MxMDCore::Publisher::UDP::send(const Frame *frame)
{
  //m_publisher->m_core->raise(ZeEVENT(Info, "Publisher UDP send"));

  using namespace MxMDStream;

  switch ((int)frame->type) {
    case Type::AddTickSizeTbl:	send_<AddTickSizeTbl>(frame); break;
    case Type::ResetTickSizeTbl: send_<ResetTickSizeTbl>(frame); break;
    case Type::AddTickSize:	send_<AddTickSize>(frame); break;
    case Type::AddSecurity:	send_<AddSecurity>(frame); break;
    case Type::UpdateSecurity:	send_<UpdateSecurity>(frame); break;
    case Type::AddOrderBook:	send_<AddOrderBook>(frame); break;
    case Type::DelOrderBook:	send_<DelOrderBook>(frame); break;
    case Type::AddCombination:	send_<AddCombination>(frame); break;
    case Type::DelCombination:	send_<DelCombination>(frame); break;
    case Type::UpdateOrderBook:	send_<UpdateOrderBook>(frame); break;
    case Type::TradingSession:	send_<TradingSession>(frame); break;
    case Type::L1:		send_<L1>(frame); break;
    case Type::PxLevel:		send_<PxLevel>(frame); break;
    case Type::L2:		send_<L2>(frame); break;
    case Type::AddOrder:	send_<AddOrder>(frame); break;
    case Type::ModifyOrder:	send_<ModifyOrder>(frame); break;
    case Type::CancelOrder:	send_<CancelOrder>(frame); break;
    case Type::ResetOB:		send_<ResetOB>(frame); break;
    case Type::AddTrade:	send_<AddTrade>(frame); break;
    case Type::CorrectTrade:	send_<CorrectTrade>(frame); break;
    case Type::CancelTrade:	send_<CancelTrade>(frame); break;
    case Type::RefDataLoaded:	send_<RefDataLoaded>(frame); break;
    default: 
      m_publisher->m_core->raise(ZeEVENT(Info, "Publisher UDP send default"));
      
      std::cout << "**** " << (int)frame->type << std::endl;
      
      break;
  }
}

MxMDCore::Subscriber::TCP::TCP(Subscriber *subscriber, const ZiCxnInfo &ci) :
  ZiConnection(subscriber->m_core->mx(), ci), m_subscriber(subscriber)
{
  using namespace MxMDStream::TCP;
  m_in = new InMsg();
}

void MxMDCore::Subscriber::TCP::connected(ZiIOContext &io)
{
  using namespace MxMDPubSub::TCP;
  m_subscriber->tcpConnected2(this);
  
  m_subscriber->m_core->raise(ZeEVENT(Info, "Subscriber TCP connected"));

  m_in->fn() = InMsg::Fn::Member<&MxMDCore::Subscriber::TCP::process>::fn(this);
  m_in->recv(io);
}

void MxMDCore::Subscriber::TCP::sendLogin()
{
  using namespace MxMDStream;
  ZmRef<MxMDPubSub::TCP::OutMsg> msg = new MxMDPubSub::TCP::OutMsg();
  m_subscriber->loginReq(new (msg->out<Login>()) Login());
  msg->send(this);
  
  m_subscriber->m_core->raise(ZeEVENT(Info, "Subscriber TCP login sent"));
}

void MxMDCore::Subscriber::loginReq(MxMDStream::Login *login)
{
  ZmRef<ZvCf> cf = m_core->cf()->subset("subscriber", false, true);
  login->username = cf->get("username", true).data();
  login->password = cf->get("password", true).data();
}

void MxMDCore::Subscriber::TCP::disconnect()
{
  ZiConnection::disconnect();
}

void MxMDCore::Subscriber::TCP::process(MxMDPubSub::TCP::InMsg *, ZiIOContext &io)
{
  m_subscriber->m_core->raise(ZeEVENT(Info, "Subscriber TCP process"));
	
  using namespace MxMDPubSub::TCP;
  using namespace MxMDStream;

  Frame &hdr = m_in->as<Frame>();
  
  
  
  std::cout << "tcp seqNo: " << hdr.seqNo << std::endl;
      std::cout << "tcp len: " << hdr.len << std::endl;
      std::cout << "tcp type: " << (int)hdr.type << std::endl;
  
  
  
  
  

  switch ((int)hdr.type) {
    case Type::EndOfSnapshot:
      m_subscriber->m_core->raise(ZeEVENT(Info, "Subscriber TCP EndOfSnapshot+++"));
      std::cout << m_subscriber->m_seqNo << " +++++" << std::endl;

      m_subscriber->running();
      m_subscriber->m_link->rxInvoke(
        [subscriber = m_subscriber](Rx *rx) {
		
	  std::cout << "$$$$$$ rx stopQueuing $$$$$$$$$$$$$$$$$" << std::endl;
		
	  rx->stopQueuing(subscriber->m_seqNo); });
	  

      disconnect();
      return;
    default:
      m_subscriber->m_core->raise(ZeEVENT(Info, "Subscriber TCP apply"));
      
      
      std::cout << hdr.seqNo << " +++++ apply" << std::endl;
      
    
      m_subscriber->m_core->apply(&hdr);
  }

  m_in->recv(io);
}

MxMDCore::Subscriber::UDP::UDP(Subscriber *subscriber, const ZiCxnInfo &ci) :
    ZiConnection(subscriber->m_core->mx(), ci), m_subscriber(subscriber)
{
  using namespace MxMDPubSub::UDP;
  m_in = new InMsg();
}

void MxMDCore::Subscriber::UDP::connected(ZiIOContext &io)
{
  std::cout << "Subscriber UDP connected" << std::endl;
	
  m_subscriber->udpConnected2(this);
  
  
  
  
  using namespace MxMDPubSub::UDP;
  
  
  
  
  //ZmRef<InMsg> msg = new InMsg();
  m_in->fn() = InMsg::Fn::Member<&MxMDCore::Subscriber::UDP::process>::fn(this);
  
  m_in->recv(io);
  
}

void MxMDCore::Subscriber::udpConnected2(Subscriber::UDP *udp)
{
  ZmRef<TCP> tcp;
  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);
    if (!(tcp = m_tcp)) { // paranoia
      stateGuard.unlock();
      udp->disconnect();
      return;
    }
    m_udp = udp;
  }

  m_core->raise(ZeEVENT(Info, "Subscriber udpConnected2"));

  
  
  
  
  
  m_link->rxInvoke([](Rx *rx) {
	static_cast<Subscriber *>(
	    static_cast<Link *>(rx)->engine())->recv(rx);
      });////////////////////////////////////

  //recv(rx, io);
      
      
      
      
      
      
      
      
  tcp->sendLogin();
}

void MxMDCore::Subscriber::recv(Rx *rx) {
	
	
  std::cout << "Subscriber recv" << std::endl;
	
	
  rx->startQueuing();

  m_rx = rx;

  
  //m_udp->recv();
  /*m_link->rxRun([&io](Rx *rx) {
	static_cast<Subscriber *>(
	    static_cast<Link *>(rx)->engine())->m_udp->recv(io);
      });*/
  
  
  
  std::cout << "$$$$$$$ Subscriber recv end $$$$$$$$$$$$$$$" << std::endl;
  
}

//void MxMDCore::Subscriber::UDP::recv()
//{
 // std::cout << "Subscriber UDP recv" << std::endl;
	
	/*
  using namespace MxMDPubSub::UDP;
  ZmRef<InMsg> msg = new InMsg();
  msg->fn() = InMsg::Fn::Member<&MxMDCore::Subscriber::UDP::process>::fn(this);*/

  
  
  //ZiSockAddr addr;///////////
  //addr.init({"239.255.90.105"}, 27413);//////////////
  
  /*
  msg->recv(io);///////////////////////////////
  m_subscriber->m_link->rxInvoke([&io, msg](Rx *) {
	  
	  msg->recv(io);
	  
	
      });*/
  
  
  
  
//  std::cout << "$$$$$$$$$$ Subscriber UDP recv end $$$$$$$$$$$$$$$" << std::endl;
  
  
//}

void MxMDCore::Subscriber::pushMsg(MxQMsg *qmsg) {
  auto frame = qmsg->template as<Msg>().frame();
  
  
  std::cout << "----- Subscriber apply seqNo: " << frame->seqNo << std::endl;
  
  m_core->apply(frame);
}

void MxMDCore::Subscriber::UDP::disconnect()
{
  ZiConnection::disconnect();
}

void MxMDCore::Subscriber::up()
{
  tcpConnect();
}

void MxMDCore::Subscriber::tcpConnect()
{
  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);
    ++m_reconnects;
  }

  ZmRef<ZvCf> cf = m_core->cf()->subset("subscriber", false, true);

  ZiIP ip{cf->get("remoteIP", true).data()};
  auto port = static_cast<uint16_t>(strtoul(cf->get("remoteTcpPort", true).data(), nullptr, 10));
  
  m_core->raise(ZeEVENT(Info, "Subscriber tcpConnect"));

  m_core->mx()->connect(
      ZiConnectFn::Member<&MxMDCore::Subscriber::tcpConnected>::fn(this),
      ZiFailFn::Member<&MxMDCore::Subscriber::tcpConnectFailed>::fn(this),
      ZiIP(), 0, ip, port);
}

ZiConnection *MxMDCore::Subscriber::tcpConnected(const ZiCxnInfo &ci)
{
  if (pendingStop()) return 0;
  
  m_core->raise(ZeEVENT(Info, "Subscriber tcpConnected"));
  
  return new TCP(this, ci);
}

void MxMDCore::Subscriber::tcpConnectFailed(bool transient)
{
  if (transient) restart();
}

void MxMDCore::Subscriber::restart()
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);
  restart_();
}

void MxMDCore::Subscriber::restart_()
{
  bool stop = m_state & State::PendingStop;
  m_state = (m_state & ~(State::Mask | State::PendingStop)) | State::Running;
  if (stop)
    m_core->mx()->add(ZmFn<>::Member<&MxMDCore::Subscriber::stop_>::fn(this));
  else {
    ZmRef<ZvCf> cf = m_core->cf()->subset("subscriber", false, true);
    auto reconnectInterval = static_cast<int>(strtoul(cf->get("reconnectInterval", true).data(), nullptr, 10));

    m_core->mx()->add(ZmFn<>::Member<&MxMDCore::Subscriber::start_>::fn(this),
	ZmTimeNow(reconnectInterval), ZmScheduler::Advance, &m_restartTimer);
  }
}

void MxMDCore::Subscriber::stop_()
{
  disconnect();
  stopped();
}

void MxMDCore::Subscriber::disconnect()
{
  ZmRef<TCP> tcp;
  ZmRef<UDP> udp;
  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);
    tcp = ZuMv(m_tcp);
    udp = ZuMv(m_udp);
  }
  if (tcp) tcp->disconnect();
  if (udp) udp->disconnect();
}

void MxMDCore::Subscriber::stopped()
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);
  if (pendingStart_()) return;
  m_state = (m_state & ~(State::Mask | State::PendingStart)) | State::Stopped;
}

void MxMDCore::Subscriber::stop()
{
  // cancel restart
  m_core->mx()->del(&m_restartTimer);

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    // state machine
    int state = m_state & State::Mask;
    switch (state) {
      case State::Stopping:
	m_state &= ~State::PendingStart;
	return;
      case State::Stopped:
	return;
      case State::Starting:
	m_state |= State::PendingStop;
	return;
      default:
	break;
    }

    // stopping...
    m_state =
      (m_state & ~(State::Mask | State::PendingStop)) | State::Stopping;
  }

  m_core->mx()->add(ZmFn<>::Member<&MxMDCore::Subscriber::stop_>::fn(this));
}

void MxMDCore::Subscriber::running()
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);
  if (pendingStop_()) return;
  m_state = (m_state & ~State::Mask) | State::Running;
}

bool MxMDCore::Subscriber::pendingStart()
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);
  return pendingStart_();
}

bool MxMDCore::Subscriber::pendingStart_()
{
  if (!(m_state & State::PendingStart)) return false;
  m_state =
    (m_state & ~(State::Mask | State::PendingStart)) | State::Starting;
  m_core->mx()->add(ZmFn<>::Member<&MxMDCore::Subscriber::start_>::fn(this));
  return true;
}

bool MxMDCore::Subscriber::pendingStop()
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);
  return pendingStop_();
}

bool MxMDCore::Subscriber::pendingStop_()
{
  if (!(m_state & State::PendingStop)) return false;
  m_state =
    (m_state & ~(State::Mask | State::PendingStop)) | State::Stopping;
  m_core->mx()->add(ZmFn<>::Member<&MxMDCore::Subscriber::stop_>::fn(this));
  return true;
}

bool MxMDCore::Subscriber::tcpError(TCP *tcp, ZiIOContext *io)
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);
  if (io)
    io->disconnect();
  else if (tcp)
    m_core->mx()->add(ZmFn<>::Member<&MxMDCore::Subscriber::TCP::disconnect>::fn(tcp));
  if (tcp && tcp != m_tcp) return false;
  restart_();
  return true;
}

bool MxMDCore::Subscriber::udpError(UDP *udp, ZiIOContext *io)
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);
  if (io)
    io->disconnect();
  else if (udp)
    m_core->mx()->add(ZmFn<>::Member<&MxMDCore::Subscriber::UDP::disconnect>::fn(udp));
  if (udp && udp != m_udp) return false;
  restart_();
  return true;
}

void MxMDCore::Subscriber::tcpConnected2(MxMDCore::Subscriber::TCP *tcp)
{
  {
    ZiIP ip = tcp->info().remoteIP;
    uint16_t port = tcp->info().remotePort;
    m_core->raise(ZeEVENT(Info,
	      ([ip, port](const ZeEvent &, ZmStream &s) {
		s << "TCP connected "
		  << ip << ':' << ZuBoxed(port);
	      })));
  }
  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);
    m_tcp = tcp;
  }
  m_core->mx()->add(ZmFn<>::Member<&MxMDCore::Subscriber::udpConnect>::fn(this));

  // TCP sendLogin() is called once UDP is receiving/queuing
}

void MxMDCore::Subscriber::udpConnect()
{
  ZmRef<ZvCf> cf = m_core->cf()->subset("subscriber", false, true);

  //ZiIP ip{cf->get("multicastAddr", true).data()};
  auto port = static_cast<uint16_t>(strtoul(cf->get("multicastPort", true).data(), nullptr, 10));

  ZiCxnOptions options;
  options.udp(true);
  options.multicast(true);
  //options.mreq(ZiMReq(ip, "127.0.0.1"));
  
  options.loopBack(true);
  
  ZiIP mif("127.0.0.1");
  options.mif(mif);
  
  
  
  m_core->raise(ZeEVENT(Info, "***************** Subscriber udpConnect ********"));
  options.print(std::cout);
  std::cout << std::endl;
  
  

  m_core->mx()->udp(
    ZiConnectFn::Member<&MxMDCore::Subscriber::udpConnected>::fn(this),
    ZiFailFn::Member<&MxMDCore::Subscriber::udpConnectFailed>::fn(this),
    ZiIP(), port, ZiIP(), 0, options);
}

ZiConnection *MxMDCore::Subscriber::udpConnected(const ZiCxnInfo &ci)
{
  if (pendingStop()) return 0;
  m_core->raise(ZeEVENT(Info, "Subscriber udpConnected"));
  return new UDP(this, ci);
}

void MxMDCore::Subscriber::udpConnectFailed(bool transient)
{
  if (transient) restart();
}
