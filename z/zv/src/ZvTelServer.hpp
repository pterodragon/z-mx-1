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

#ifndef ZvTelServer_HPP
#define ZvTelServer_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZvTelemetry.hpp>
#include <zlib/ZvEngine.hpp>

#include <zlib/zcmd_fbs.h>
#include <zlib/telreq_fbs.h>
#include <zlib/telack_fbs.h>

namespace ZvTelemetry {

namespace ReqType {
  ZfbEnumValues(ReqType,
      Heap, HashTbl, Thread, Mx, Queue, Engine, DBEnv, App, Alert);
}

using IOBuf = ZiIOBuf<>;

class AlertFile {
  using BufRef = ZmRef<IOBuf>;
  using BufPtr = IOBuf *;

public:
  AlertFile() { }
  ~AlertFile() { close(); }

private:
  // do not call ZeLOG since that may well recurse back here, print to stderr
  template <typename Message>
  void error(bool index, const Message &message) {
    struct Fmt : public ZtDate::CSVFmt { Fmt() { offset(timezone); } };
    thread_local Fmt dateFmt;
    std::cerr << ZtDateNow().csv(dateFmt)
      << " FATAL " << m_path << (index ? ".idx" : "")
      << ": " << message << '\n' << std::flush;
  }

  void open(ZuString prefix, unsigned date, unsigned flags) {
    m_date = date;
    m_seqNo = 0;
    if (!prefix) return;
    m_path = prefix;
    m_path << '_' << m_date;
    ZeError e;
    int i = m_file.open(m_path, flags, 0666, &e);
    if (i != Zi::OK) { error(false, e); return; }
    i = m_index.open(m_path + ".idx", flags, 0666, &e);
    if (i != Zi::OK) { m_file.close(); error(true, e); return; }
    m_offset = m_file.size();
    m_seqNo = m_index.size() / sizeof(size_t);
  }

public:
  void close() {
    m_file.close();
    m_index.close();
    m_path.null();
    m_date = 0;
    m_offset = 0;
    m_seqNo = 0;
  }

  unsigned date() const { return m_date; }
  size_t offset() const { return m_offset; }
  unsigned seqNo() const { return m_seqNo; }

  bool isOpen() const { return m_file; }

  // returns seqNo
  unsigned alloc(ZuString prefix, unsigned date) {
    if (date != m_date) {
      close();
      open(prefix, date, ZiFile::Create);
    }
    return m_seqNo;
  }
  void write(const BufPtr buf) {
    if (m_file) {
      ZeError e;
      if (m_file.pwrite(m_offset, buf->data(), buf->length, &e) != Zi::OK)
	error(false, e);
      else if (m_index.pwrite(m_seqNo * sizeof(size_t),
	    &m_offset, sizeof(size_t), &e) != Zi::OK)
	error(true, e);
    }
    m_seqNo++;
    m_offset += buf->length;
  }

  template <typename L>
  BufRef read(ZuString prefix, unsigned date, unsigned seqNo, void *bufOwner) {
    if (date != m_date) {
      close();
      open(prefix, date, ZiFile::ReadOnly);
    }
    if (!m_file) return BufRef{};
    if (seqNo >= m_seqNo) return BufRef{};
    size_t offset, next;
    ZeError e;
    if (m_index.pread(seqNo * sizeof(size_t),
	  &offset, sizeof(size_t), &e) != Zi::OK) {
      error(true, e);
      return BufRef{};
    }
    if (offset >= m_offset) {
      error(true, "corrupt");
      return BufRef{};
    }
    if (seqNo == m_seqNo - 1)
      next = m_offset;
    else {
      if (m_index.pread((seqNo + 1) * sizeof(size_t),
	    &next, sizeof(size_t), &e) != Zi::OK) {
	error(true, e);
	return BufRef{};
      }
    }
    if (next < offset || next > m_offset) {
      error(true, "corrupt");
      return BufRef{};
    }
    ZmRef<IOBuf> buf = new IOBuf(bufOwner, next - offset);
    if (m_file.pread(offset, buf->data(), buf->length, &e) != Zi::OK) {
      error(false, e);
      return BufRef{};
    }
    return buf;
  }

private:
  unsigned		m_date = 0; // YYYYMMDD
  size_t		m_offset = 0;
  unsigned		m_seqNo = 0;
  ZtString		m_path;
  ZiFile		m_file;
  ZiFile		m_index;
};

using AlertRing = ZmDRing<ZmRef<IOBuf> >;

template <typename App_, typename Link_>
class Server : public ZvEngineMgr {
public:
  using App = App_;
  using Link = Link_;

private:
  using MxTbl =
    ZmRBTree<ZmRef<ZvMultiplex>,
      ZmRBTreeIndex<ZvMultiplex::IDAccessor,
	ZmRBTreeObject<ZuNull,
	  ZmRBTreeLock<ZmRWLock> > > >;

public:
  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

  ZuInline static App *app(const Link *link) {
    return static_cast<const App *>(link->app());
  }

  void init(ZiMultiplex *mx, ZvCf *cf) {
    m_mx = mx;
    if (auto thread = cf->get("telemetry:thread", false))
      m_thread = mx->tid(thread);
    else
      m_thread = mx->txThread();

    if (ZmRef<ZvCf> mxCf = cf->subset("mx", false, true)) {
      ZvCf::Iterator i(mxCf);
      ZuString key;
      while (ZmRef<ZvCf> mxCf_ = i.subset(key))
	m_mxTbl.add(new ZvMultiplex(key, mxCf_));
    }

    m_minInterval = cf->getInt("telemetry:minInterval", 10, 1000000, 100);
    m_alertPrefix = cf->get("telemetry:alertPrefix", false, "alerts");
    m_alertMaxReplay = cf->getInt("telemetry:alertMaxReplay", 1, 1000, 10);
  }
  void final() {
    m_alertFile.close();
    m_mxTbl.clean();
    m_queues.clean();
    m_engines.clean();
    m_dbEnvFn = DBEnvFn{};
  }

  void start() {
    m_mx->invoke(m_thread, this, [](Server *server) { server->start_(); });
  }
  void stop() {
    m_mx->invoke(m_thread, this, [](Server *server) { server->stop_(); });
  }

  ZuInline ZvMultiplex *mx() const { return m_mx; }
  ZuInline ZvMultiplex *mx(const ZuID &id) const {
    return m_mxTbl.findKey(id);
  }
  template <typename L>
  ZuInline void allMx(L l) const {
    auto i = m_mxTbl.readIterator();
    while (MxTbl::Node *node = i.iterate()) l(node->key());
  }

  void process(Link *link, const fbs::Request *in) {
    int reqType = in->type();
    unsigned interval = in->interval();
    if (interval < m_minInterval) interval = m_minInterval;
    switch (reqType) {
      case fbs::ReqType_Heap:
	heapQuery(link, in->subscribe(), in->filter(), interval);
	break;
      case fbs::ReqType_HashTbl:
	hashQuery(link, in->subscribe(), in->filter(), interval);
	break;
      case fbs::ReqType_Thread:
	threadQuery(link, in->subscribe(), in->filter(), interval);
	break;
      case fbs::ReqType_Mx:
	mxQuery(link, in->subscribe(), in->filter(), interval);
	break;
      case fbs::ReqType_Queue:
	queueQuery(link, in->subscribe(), in->filter(), interval);
	break;
      case fbs::ReqType_Engine:
	engineQuery(link, in->subscribe(), in->filter(), interval);
	break;
      case fbs::ReqType_DBEnv:
	dbEnvQuery(link, in->subscribe(), in->filter(), interval);
	break;
      case fbs::ReqType_App:
	appQuery(link, in->subscribe(), in->filter(), interval);
	break;
      case fbs::ReqType_Alert:
	alertQuery(link, in->subscribe(), in->filter(), interval);
	break;
      default:
	break;
    }
  }
  void disconnected(Link *link);

  // EngineMgr functions

  void updEngine(ZvEngine *engine) const {
    m_mx->invoke(m_thread, this, [engine = ZmMkRef(engine)](Server *server) {
      server->engineScan(engine);
    });
  }
  void updLink(ZvAnyLink *link) const {
    m_mx->invoke(m_thread, this, [link = ZmMkRef(link)](Server *server) {
      server->linkScan(link);
    });
  }

  void addEngine(ZvEngine *engine) {
    m_mx->invoke(m_thread, this,
	[engine = ZmMkRef(engine)](Server *server) mutable {
	  if (!server->m_engines.find(engine->id()))
	    server->m_engines.add(ZuMv(engine));
	});
  }
  void delEngine(ZvEngine *engine) {
    m_mx->invoke(m_thread, this, [id = engine->id()](Server *server) {
      delete server->m_engines.del(id);
    });
  }
  void addQueue(unsigned type, ZuID id, QueueFn queueFn) {
    m_mx->invoke(m_thread, this,
	[type, id, queueFn = ZuMv(queueFn)](Server *server) mutable {
	  auto key = ZuMkPair(type, id);
	  if (!server->m_queues.find(key))
	    server->m_queues.add(key, ZuMv(queueFn));
	});
  }
  void delQueue(unsigned type, ZuID id) {
    m_mx->invoke(m_thread, this, [type, id](Server *server) {
      auto key = ZuMkPair(type, id);
      delete server->m_queues.del(key);
    });
  }

  // ZdbEnv registration
 
  void addDBEnv(DBEnvFn fn) {
    m_mx->invoke(m_thread, this, [fn = ZuMv(fn)](Server *server) mutable {
      server->m_dbEnvFn = ZuMv(fn);
    });
  }
  void delDBEnv() {
    m_mx->invoke(m_thread, this, [](Server *server) {
      server->m_dbEnvFn = DBEnvFn{};
    });
  }

  // app RAG updates

  void appUpdated() {
    m_mx->invoke(m_thread, this, [](Server *server) {
      server->m_appUpdated = true;
    });
  }

  // alerts

  void alert(ZmRef<ZeEvent> e) {
    m_mx->invoke(m_thread, this, [e = ZuMv(e)](Server *server) mutable {
      server->alert_(ZuMv(e));
    });
  }

private:
  void start_() {
    for (unsigned i = 0; i < ReqType::N; i++) {
      auto &list = m_watchLists[i];
      ZmFn<> fn;
      switch (i) {
	case fbs::ReqType_Heap:
	  fn = ZmFn<>{this, [](Server *server) { server->heapScan(); }};
	  break;
	case fbs::ReqType_HashTbl:
	  fn = ZmFn<>{this, [](Server *server) { server->hashScan(); }};
	  break;
	case fbs::ReqType_Thread:
	  fn = ZmFn<>{this, [](Server *server) { server->threadScan(); }};
	  break;
	case fbs::ReqType_Mx:
	  fn = ZmFn<>{this, [](Server *server) { server->mxScan(); }};
	  break;
	case fbs::ReqType_Queue:
	  fn = ZmFn<>{this, [](Server *server) { server->queueScan(); }};
	  break;
	case fbs::ReqType_Engine:
	  fn = ZmFn<>{this, [](Server *server) { server->engineScan(); }};
	  break;
	case fbs::ReqType_DBEnv:
	  fn = ZmFn<>{this, [](Server *server) { server->dbEnvScan(); }};
	  break;
	case fbs::ReqType_App:
	  fn = ZmFn<>{this, [](Server *server) { server->appScan(); }};
	  break;
	case fbs::ReqType_Alert:
	  fn = ZmFn<>{this, [](Server *server) { server->alertScan(); }};
	  break;
      }
      if (fn)
	m_mx->run(m_thread, ZuMv(fn),
	    ZmTimeNow() + ZmTime{ZmTime::Nano, ((int64_t)list.interval) * 1000},
	    ZmScheduler::Advance, &list.timer);
    }
  }

  void stop_() {
    for (unsigned i = 0; i < ReqType::N; i++) {
      auto &list = m_watchLists[i];
      m_mx->del(&list.timer);
    }
  }

  void alert_(ZmRef<ZeEvent> alert) {
    m_alertBuf.length(0);
    m_alertBuf << alert->message();
    ZtDate date{alert->time()};
    unsigned yyyymmdd = date.yyyymmdd();
    unsigned seqNo = m_alertFile.alloc(m_alertPrefix, yyyymmdd);
    using namespace Zfb::Save;
    m_fbb.Clear();
    m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	  fbs::TelData_Alert,
	  fbs::CreateAlert(m_fbb,
	    date(m_fbb, date), seqNo, alert->tid(),
	    static_cast<fbs::Severity>(alert->severity()),
	    str(m_fbb, m_alertBuf)).Union()));
    ZmRef<IOBuf> buf = m_fbb.buf();
    m_alertFile.write(buf);
    m_alertRing.push(ZuMv(buf));
  }

  using Queues =
    ZmRBTree<ZuPair<ZuID, bool>,
      ZmRBTreeVal<QueueFn,
	ZmRBTreeObject<ZuNull,
	  ZmRBTreeLock<ZmNoLock> > > >;

  struct EngineIDAccessor : public ZuAccessor<ZvEngine *, ZuID> {
    static ZuID value(const ZvEngine *engine) { return engine->id(); }
  };
  using Engines =
    ZmRBTree<ZmRef<ZvEngine>,
      ZmRBTreeIndex<EngineIDAccessor,
	ZmRBTreeObject<ZuNull,
	  ZmRBTreeLock<ZmNoLock> > > >;

  struct Watch_ {
    Link	*link = nullptr;
    ZmIDString	filter;
  };
  using WatchList_ =
    ZmList<Watch_,
      ZmListObject<ZuNull,
	ZmListNodeIsItem<true,
	  ZmListLock<ZmNoLock> > > >;
  using Watch = typename WatchList_::Node;
  struct WatchList {
    WatchList_		list;
    unsigned		interval = 0;	// in microseconds
    ZmScheduler::Timer	timer;

    template <typename ...Args>
    Watch *add(Args &&... args) {
      auto watch = new Watch{ZuFwd<Args>(args)...};
      list.add(watch);
      return watch;
    }
    void del(const Link *link) {
      auto i = list.iterator();
      while (auto watch = i.iterate())
	if (watch->link == link) { i.del(); break; }
    }
  };

  void subscribe(WatchList &list, Watch *watch, unsigned interval, ZmFn<> fn) {
    bool reschedule = false;
    if (!list.interval || interval < list.interval)
      if (list.interval != interval) {
	reschedule = true;
	list.interval = interval;
      }
    list.push(watch);
    if (reschedule)
      m_mx->run(m_thread, ZuMv(fn),
	  ZmTimeNow() + ZmTime{ZmTime::Nano, ((int64_t)list.interval) * 1000},
	  ZmScheduler::Advance, &list.timer);
  }

  void unsubscribe(WatchList &list, Link *link, ZuString filter) {
    auto i = list.iterate();
    while (auto watch = i.iterateNode())
      if (watch->link == link && (!filter || watch->filter == filter))
	i.del();
    if (!list.count() && list.interval) {
      list.interval = 0;
      m_mx->del(&list.timer);
    }
  }

  // heap processing

  void heapQuery(Link *link,
      bool subscribe, ZuString filter, unsigned interval) {
    auto &list = m_watchLists[ReqType::Heap];
    if (interval && !subscribe) { unsubscribe(list, link, filter); return; }
    auto watch = new Watch{ filter, link };
    if (interval)
      subscribe(list, watch, interval,
	  ZmFn<>{this, [](Server *server) { server->heapScan(); }});
    ZmHeapMgr::all(ZmFn<ZmHeapCache *>{
      watch, [](Watch *watch, ZmHeapCache *heap) {
	watch->link->app()->heapQuery_(watch, heap);
      }});
    if (!interval) delete watch;
  }
  void heapQuery_(Watch *watch, const ZmHeapCache *heap) {
    Heap data;
    heap->telemetry(data);
    if (!match(watch->filter, data.id)) return;
    m_fbb.Clear();
    m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	  fbs::TelData_Heap, data.save(m_fbb).Union()));
    m_fbb.PushElement(
	ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
    watch->link->send(m_fbb.buf());
  }

  void heapScan() {
    if (!m_watchLists[ReqType::Heap].list.count()) return;
    ZmHeapMgr::all(ZmFn<ZmHeapCache *>{
      this, [](Server *server, ZmHeapCache *heap) {
	server->heapScan(heap);
      }});
  }
  void heapScan(const ZmHeapCache *heap) {
    Heap data;
    heap->telemetry(data);
    auto i = m_watchLists[ReqType::HashTbl].list.readIterate();
    while (auto watch = i.iterateNode()) {
      if (!match(watch->filter, data.id)) continue;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_Heap, data.saveDelta(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
    }
  }

  // hash table processing

  void hashQuery(Link *link,
      bool subscribe, ZuString filter, unsigned interval) {
    auto &list = m_watchLists[ReqType::HashTbl];
    if (interval && !subscribe) { unsubscribe(list, link, filter); return; }
    auto watch = new Watch{ filter, link };
    if (interval)
      subscribe(list, watch, interval,
	  ZmFn<>{this, [](Server *server) { server->hashScan(); }});
    ZmHashMgr::all(ZmFn<ZmAnyHash *>{
      watch, [](Watch *watch, ZmAnyHash *tbl) {
	watch->link->app()->hashQuery_(watch, tbl);
      }});
    if (!interval) delete watch;
  }
  void hashQuery_(Watch *watch, const ZmAnyHash *tbl) {
    HashTbl data;
    tbl->telemetry(data);
    if (!match(watch->filter, data.id)) return;
    m_fbb.Clear();
    m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	  fbs::TelData_HashTbl, data.save(m_fbb).Union()));
    m_fbb.PushElement(
	ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
    watch->link->send(m_fbb.buf());
  }

  void hashScan() {
    if (!m_watchLists[ReqType::HashTbl].list.count()) return;
    ZmHashMgr::all(ZmFn<ZmAnyHash *>{
      this, [](Server *server, ZmAnyHash *tbl) {
	server->hashScan(tbl);
      }});
  }
  void hashScan(const ZmAnyHash *tbl) {
    HashTbl data;
    tbl->telemetry(data);
    auto i = m_watchLists[ReqType::HashTbl].list.readIterate();
    while (auto watch = i.iterateNode()) {
      if (!match(watch->filter, data.id)) continue;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_HashTbl, data.saveDelta(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
    }
  }

  // thread processing

  void threadQuery(Link *link,
      bool subscribe, ZuString filter, unsigned interval) {
    auto &list = m_watchLists[ReqType::Thread];
    if (interval && !subscribe) { unsubscribe(list, link, filter); return; }
    auto watch = new Watch{ filter, link };
    if (interval)
      subscribe(list, watch, interval,
	  ZmFn<>{this, [](Server *server) { server->threadScan(); }});
    ZmSpecific<ZmThreadContext>::all(ZmFn<ZmThreadContext *>{
      watch, [](Watch *watch, ZmThreadContext *tc) {
	watch->link->app()->threadQuery_(watch, tc);
      }});
    if (!interval) delete watch;
  }
  void threadQuery_(Watch *watch, const ZmThreadContext *tc) {
    Thread data;
    tc->telemetry(data);
    if (!matchThread(watch->filter, data.name, data.tid)) return;
    m_fbb.Clear();
    m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	  fbs::TelData_Thread, data.save(m_fbb).Union()));
    m_fbb.PushElement(
	ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
    watch->link->send(m_fbb.buf());
  }

  void threadScan() {
    if (!m_watchLists[ReqType::Thread].list.count()) return;
    ZmSpecific<ZmThreadContext>::all(ZmFn<ZmThreadContext *>{
      this, [](Server *server, ZmThreadContext *tc) {
	server->threadScan(tc);
      }});
  }
  void threadScan(const ZmThreadContext *tc) {
    Thread data;
    tc->telemetry(data);
    auto i = m_watchLists[ReqType::Thread].list.readIterate();
    while (auto watch = i.iterateNode()) {
      if (!match(watch->filter, data.name, data.tid)) continue;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_Thread, data.saveDelta(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
    }
  }

  // mx processing

  void mxQuery(Link *link,
      bool subscribe, ZuString filter, unsigned interval) {
    auto &list = m_watchLists[ReqType::Mx];
    if (interval && !subscribe) { unsubscribe(list, link, filter); return; }
    auto watch = new Watch{ filter, link };
    if (interval)
      subscribe(list, watch, interval,
	  ZmFn<>{this, [](Server *server) { server->mxScan(); }});
    allMx(ZmFn<ZvMultiplex *>{watch, [](Watch *watch, ZvMultiplex *mx) {
	watch->link->app()->mxQuery_(watch, mx);
      }});
    if (!interval) delete watch;
  }
  void mxQuery_(Watch *watch, const ZvMultiplex *mx) {
    Mx data;
    mx->telemetry(data);
    if (!match(watch->filter, data.id)) return;
    m_fbb.Clear();
    m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	  fbs::TelData_Mx, data.save(m_fbb).Union()));
    m_fbb.PushElement(
	ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
    watch->link->send(m_fbb.buf());
    {
      using namespace Zfb::Save;
      uint64_t inCount, inBytes, outCount, outBytes;
      for (unsigned tid = 1, n = mx->params().nThreads(); tid <= n; tid++) {
	ZmIDString queueID;
	queueID << mx->params().id() << '.'
	  << mx->params().thread(tid).name();
	{
	  const auto &ring = mx->ring(tid);
	  ring.stats(inCount, inBytes, outCount, outBytes);
	  m_fbb.Clear();
	  m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
		fbs::TelData_Queue,
		fbs::CreateQueue(m_fbb,
		  str(m_fbb, queueID), 0, ring.count(),
		  inCount, inBytes, outCount, outBytes,
		  ring.params().size(), ring.full(),
		  QueueType::Thread).Union()));
	  m_fbb.PushElement(
	      ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
	  watch->link->send(m_fbb.buf());
	}
	if (queueID.length() < ZmIDStrSize - 1)
	  queueID << '_';
	else
	  queueID[ZmIDStrSize - 2] = '_';
	{
	  const auto &overRing = mx->overRing(tid);
	  overRing.stats(inCount, outCount);
	  m_fbb.Clear();
	  m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
		fbs::TelData_Queue,
		fbs::CreateQueue(m_fbb,
		  str(m_fbb, queueID), 0, overRing.count(),
		  inCount, inCount * sizeof(ZmFn<>),
		  outCount, outCount * sizeof(ZmFn<>),
		  overRing.size_(), false,
		  QueueType::Thread).Union()));
	  m_fbb.PushElement(
	      ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
	  watch->link->send(m_fbb.buf());
	}
      }
    }
    mx->allCxns([this, watch](ZiConnection *cxn) {
      Socket data;
      cxn->telemetry(data);
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_Socket, data.save(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
    });
  }

  void mxScan() {
    if (!m_watchLists[ReqType::Mx].list.count()) return;
    allMx(ZmFn<ZvMultiplex *>{this, [](Server *server, ZvMultiplex *mx) {
      server->mxScan(mx);
    }});
  }
  void mxScan(ZvMultiplex *mx) {
    Mx data;
    mx->telemetry(data);
    auto i = m_watchLists[ReqType::Mx].list.readIterate();
    while (auto watch = i.iterateNode()) {
      if (!match(watch->filter, data.id)) continue;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_Mx, data.saveDelta(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
      {
	uint64_t inCount, inBytes, outCount, outBytes;
	for (unsigned tid = 1, n = mx->params().nThreads(); tid <= n; tid++) {
	  ZmIDString queueID;
	  queueID << mx->params().id() << '.'
	    << mx->params().thread(tid).name();
	  {
	    const auto &ring = mx->ring(tid);
	    ring.stats(inCount, inBytes, outCount, outBytes);
	    m_fbb.Clear();
	    auto id_ = str(m_fbb, queueID);
	    fbs::QueueBuilder b(m_fbb);
	    b.add_id(id_);
	    b.add_count(ring.count());
	    b.add_inCount(inCount);
	    b.add_inBytes(inBytes);
	    b.add_outCount(outCount);
	    b.add_outBytes(outBytes);
	    b.add_full(ring.full());
	    m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
		  fbs::TelData_Queue, b.Finish().Union()));
	    m_fbb.PushElement(
		ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
	    watch->link->send(m_fbb.buf());
	  }
	  if (queueID.length() < ZmIDStrSize - 1)
	    queueID << '_';
	  else
	    queueID[ZmIDStrSize - 2] = '_';
	  {
	    const auto &overRing = mx->overRing(tid);
	    overRing.stats(inCount, outCount);
	    m_fbb.Clear();
	    auto id_ = str(m_fbb, queueID);
	    fbs::QueueBuilder b(m_fbb);
	    b.add_id(id_);
	    b.add_count(overRing.count());
	    b.add_inCount(inCount);
	    b.add_inBytes(inCount * sizeof(ZmFn<>));
	    b.add_outCount(outCount);
	    b.add_outBytes(outCount * sizeof(ZmFn<>));
	    b.add_full(0);
	    m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
		  fbs::TelData_Queue, b.Finish().Union()));
	    m_fbb.PushElement(
		ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
	    watch->link->send(m_fbb.buf());
	  }
	}
      }
      mx->allCxns([this](ZiConnection *cxn) {
	Socket data;
	cxn->telemetry(data);
	m_fbb.Clear();
	m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	      fbs::TelData_Socket, data.saveDelta(m_fbb).Union()));
	m_fbb.PushElement(
	    ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
	watch->link->send(m_fbb.buf());
      });
    }
  }

  // queue processing
  // LATER - old queue code - used by caller of addQueue()
#if 0
    ZvQueue *queue;
    queue->stats(data.inCount, data.inBytes, data.outCount, data.outBytes);

    // I/O queues (link, etc.)
	  cxn->transmit(queue(
		key.p<0>(), queue_->head(), queue_->count_(),
		inCount, inBytes, outCount, outBytes,
		(uint32_t)0, (uint32_t)0,
		(uint8_t)(key.p<1>() ? QueueType::Tx : QueueType::Rx)));

	    // IPC queues (ZiRing)
	    ring->params().name(), (uint64_t)0, (uint64_t)count,
	    inCount, inBytes, outCount, outBytes,
	    (uint32_t)ring->full(), (uint32_t)ring->params().size(),
	    (uint8_t)QueueType::IPC));
#endif

  void queueQuery(Link *link,
      bool subscribe, ZuString filter, unsigned interval) {
    auto &list = m_watchLists[ReqType::Queue];
    if (interval && !subscribe) { unsubscribe(list, link, filter); return; }
    auto watch = new Watch{ filter, link };
    if (interval)
      subscribe(list, watch, interval,
	  ZmFn<>{this, [](Server *server) { server->queueScan(); }});
    auto i = m_queues.readIterator();
    while (auto node = i.iterate()) queueQuery_(watch, node->val());
    if (!interval) delete watch;
  }
  void queueQuery_(Watch *watch, const QueueFn &fn) {
    Queue data;
    fn(data);
    if (!matchQueue(watch->filter, data.type, data.id)) return;
    m_fbb.Clear();
    m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	  fbs::TelData_Queue, data.save(m_fbb).Union()));
    m_fbb.PushElement(
	ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
    watch->link->send(m_fbb.buf());
  }
  void queueScan() {
    if (!m_watchLists[ReqType::Queue].list.count()) return;
    auto i = m_queues.readIterator();
    while (auto node = i.iterate()) queueScan(node->val());
  }
  void queueScan(const QueueFn &fn) {
    Queue data;
    fn(data);
    auto i = m_watchLists[ReqType::Queue].list.readIterate();
    while (auto watch = i.iterateNode()) {
      if (!matchQueue(watch->filter, data.type, data.id)) continue;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_Queue, data.saveDelta(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
    }
  }

  // engine processing

  void engineQuery(Link *link,
      bool subscribe, ZuString filter, unsigned interval) {
    auto &list = m_watchLists[ReqType::Engine];
    if (interval && !subscribe) { unsubscribe(list, link, filter); return; }
    auto watch = new Watch{ filter, link };
    if (interval)
      subscribe(list, watch, interval,
	  ZmFn<>{this, [](Server *server) { server->engineScan(); }});
    auto i = m_engines.readIterate();
    while (auto node = i.iterate()) engineQuery_(watch, node->val());
    if (!interval) delete watch;
  }
  void engineQuery_(Watch *watch, const ZvEngine *engine) {
    Engine data;
    engine->telemetry(data);
    if (!match(watch->filter, data.id)) return;
    m_fbb.Clear();
    m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	  fbs::TelData_Engine, data.save(m_fbb).Union()));
    m_fbb.PushElement(
	ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
    watch->link->send(m_fbb.buf());
    engine->allLinks<ZvAnyLink>([this, watch](ZvAnyLink *link) -> uintptr_t {
      Link data;
      link->telemetry(data);
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_Link, data.save(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
      return 0;
    });
  }
  void engineScan() {
    if (!m_watchLists[ReqType::Engine].list.count()) return;
    auto i = m_engines.readIterator();
    while (auto node = i.iterate()) engineScan(node->key());
  }
  void engineScan(const ZvEngine *engine) {
    Engine data;
    engine->telemetry(data);
    auto i = m_watchLists[ReqType::Engine].list.readIterator();
    while (auto watch = i.iterateNode()) {
      if (!match(watch->filter, data.id)) continue;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_Engine, data.saveDelta(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
      engine->allLinks<ZvAnyLink>([this, watch](ZvAnyLink *link) -> uintptr_t {
	linkScan(link, watch);
	return 0;
      });
    }
  }
  void linkScan(const ZvAnyLink *link) {
    auto i = m_watchLists[ReqType::Engine].list.readIterator();
    while (auto watch = i.iterateNode()) linkScan(link, watch);
  }
  void linkScan(const ZvAnyLink *link, Watch *watch) {
    ZvTelemetry::Link data;
    link->telemetry(data);
    m_fbb.Clear();
    m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	  fbs::TelData_Link, data.saveDelta(m_fbb).Union()));
    m_fbb.PushElement(
	ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
    watch->link->send(m_fbb.buf());
  }

  // DB processing

  void dbEnvQuery(Link *link,
      bool subscribe, ZuString filter, unsigned interval) {
    auto &list = m_watchLists[ReqType::DBEnv];
    if (interval && !subscribe) { unsubscribe(list, link, filter); return; }
    auto watch = new Watch{ filter, link };
    if (interval)
      subscribe(list, watch, interval,
	  ZmFn<>{this, [](Server *server) { server->dbEnvScan(); }});
    dbEnvQuery_(watch);
    if (!interval) delete watch;
  }
  void dbEnvQuery_(Watch *watch) {
    if (!m_dbEnvFn) return;
    m_dbEnvFn([this, watch](const DBEnv &data) {
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_DBEnv, data.save(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
    }, [this, watch](const DBHost &data) {
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_DBHost, data.save(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
    }, [this, watch](const DB &data) {
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_DB, data.save(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
    });
  }
  void dbEnvScan() {
    if (!m_watchLists[ReqType::DBEnv].list.count()) return;
    if (!m_dbEnvFn) return;
    auto i = m_watchLists[ReqType::DBEnv].list.readIterator();
    while (auto watch = i.iterateNode()) {
      m_dbEnvFn([this, watch](const DBEnv &data) {
	m_fbb.Clear();
	m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	      fbs::TelData_DBEnv, data.saveDelta(m_fbb).Union()));
	m_fbb.PushElement(
	    ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
	watch->link->send(m_fbb.buf());
      }, [this, watch](const DBHost &data) {
	m_fbb.Clear();
	m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	      fbs::TelData_DBHost, data.saveDelta(m_fbb).Union()));
	m_fbb.PushElement(
	    ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
	watch->link->send(m_fbb.buf());
      }, [this, watch](const DB &data) {
	m_fbb.Clear();
	m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	      fbs::TelData_DB, data.saveDelta(m_fbb).Union()));
	m_fbb.PushElement(
	    ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
	watch->link->send(m_fbb.buf());
      });
    }
  }

  // app processing

  void appQuery(Link *link,
      bool subscribe, ZuString filter, unsigned interval) {
    auto &list = m_watchLists[ReqType::App];
    if (interval && !subscribe) { unsubscribe(list, link, filter); return; }
    auto watch = new Watch{ filter, link };
    if (interval)
      subscribe(list, watch, interval,
	  ZmFn<>{this, [](Server *server) { server->appScan(); }});
    appQuery_(watch);
    if (!interval) delete watch;
  }
  void appQuery_(Watch *watch) {
    ZvTelemetry::App data;
    app()->telemetry(data);
    m_fbb.Clear();
    m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	  fbs::TelData_App, data.save(m_fbb).Union()));
    m_fbb.PushElement(
	ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
    watch->link->send(m_fbb.buf());
  }
  void appScan() {
    if (!m_appUpdated) return;
    m_appUpdated = false;
    if (!m_watchLists[ReqType::App].list.count()) return;
    ZvTelemetry::App data;
    app()->telemetry(data);
    auto i = m_watchLists[ReqType::App].list.readIterator();
    while (auto watch = i.iterateNode()) {
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateTelemetry(m_fbb,
	    fbs::TelData_App, data.saveDelta(m_fbb).Union()));
      m_fbb.PushElement(
	  ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Telemetry));
      watch->link->send(m_fbb.buf());
    }
  }

  // alert processing

  void alertQuery(Link *link,
      bool subscribe, ZuString filter, unsigned interval) {
    auto &list = m_watchLists[ReqType::Alert];
    if (interval && !subscribe) { unsubscribe(list, link, filter); return; }
    auto watch = new Watch{ filter, link };
    if (interval)
      subscribe(list, watch, interval,
	  ZmFn<>{this, [](Server *server) { server->alertScan(); }});
    alertQuery_(watch);
    if (!interval) delete watch;
  }
  void alertQuery_(Watch *watch) {
    // parse filter - yyyymmdd:seqNo
    const auto &alertFilter = ZtStaticRegexUTF8("^(\\d{8}):(\\d+)$");
    ZtRegex::Captures c;
    ZuBox<unsigned> date = 0, seqNo = 0;
    if (alertFilter.m(watch->filter, c) == 3) {
      date = c[2];
      seqNo = c[3];
    }
    // ensure date is within range
    unsigned now = ZtDateNow().yyyymmdd();
    ZmRef<IOBuf> buf;
    if (!date)
      date = now;
    else if (date < now - m_alertMaxReplay)
      date = now - m_alertMaxReplay;
    // obtain date and seqNo of in-memory alert ring (today:UINT_MAX if empty)
    unsigned headDate = now;
    unsigned headSeqNo = UINT_MAX;
    {
      if (buf = m_alertRing.head()) {
	auto alert = fbs::GetTelemetry(buf->data())->data_as_Alert();
	if (ZuLikely(alert)) {
	  headDate = Zfb::Load::dateTime(alert->time()).yyyymmdd();
	  headSeqNo = alert->seqNo();
	}
      }
    }
    // replay from file(s) up to alerts available in-memory (if any)
    {
      AlertFile replay;
      while (date < headDate) {
	while (buf = replay.read(m_alertPrefix, date, seqNo++, this))
	  watch->link->send(ZuMv(buf));
	seqNo = 0;
	++date;
      }
      while (buf = replay.read(m_alertPrefix, date, seqNo++, this)) {
	if (seqNo >= headSeqNo) break;
	watch->link->send(ZuMv(buf));
      }
    }
    // replay from memory remaining alerts requested, up to latest
    {
      auto i = m_alertRing.iterator();
      while (buf = i.iterate()) {
	auto alert = fbs::GetTelemetry(buf->data())->data_as_Alert();
	if (ZuLikely(alert)) {
	  unsigned alertDate = Zfb::Load::dateTime(alert->time()).yyyymmdd();
	  unsigned alertSeqNo = alert->seqNo();
	  if (alertDate > date || (alertDate == date && alertSeqNo >= seqNo))
	    watch->link->send(ZuMv(buf));
	}
      }
    }
  }
  void alertScan() {
    // dequeue all alerts in-memory, send to all watchers
    while (ZmRef<IOBuf> buf = m_alertRing.shift()) {
      auto i = m_watchLists[ReqType::HashTbl].list.readIterator();
      while (auto watch = i.iterateNode()) watch->link->send(buf);
    }
  }

  ZiMultiplex		*m_mx = nullptr;
  unsigned		m_thread;
  unsigned		m_minInterval;	// min. refresh interval in microsecs
  ZtString		m_alertPrefix;	// prefix for alert files
  unsigned		m_alertMaxReplay;// max. replay in days

  // telemetry thread exclusive
  Zfb::IOBuilder	m_fbb;
  MxTbl			m_mxTbl;
  Queues		m_queues;
  Engines		m_engines;
  DBEnvFn		m_dbEnvFn;
  WatchList		m_watchLists[ReqType::N];
  AlertRing		m_alertRing;		// in-memory ring of alerts
  AlertFile		m_alertFile;		// current file being written
  ZtString		m_alertBuf;		// alert message buffer
  bool			m_appUpdated = false;
};

} // ZvTelemetry

#endif /* ZvTelServer_HPP */
