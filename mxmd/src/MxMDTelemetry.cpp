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

// MxMD telemetry

#include <MxMDTelemetry.hpp>

#include <MxMDCore.hpp>

void MxMDTelemetry::init(MxMDCore *core, ZvCf *cf)
{
  using namespace MxTelemetry;

  MxMultiplex *mx = core->mx(cf->get("mx", false, "telemetry"));
  if (!mx) throw ZvCf::Required("telemetry:mx");
  m_core = core;
  Server::init(mx, cf);
}

void MxMDTelemetry::final()
{
  Guard guard(m_lock);
  m_engines.clean();
  m_links.clean();
  m_queues.clean();
  m_dbEnv = nullptr;
}

void MxMDTelemetry::run(MxTelemetry::Server::Cxn *cxn)
{
  using namespace MxTelemetry;

  // heaps
  ZmHeapMgr::all(ZmFn<ZmHeapCache *>{
      cxn, [](Cxn *cxn, ZmHeapCache *h) {
	if (!h->telCount()) cxn->transmit(heap(*h));
      }});

  // hash tables
  ZmHashMgr::all(ZmFn<ZmAnyHash *>{
      cxn, [](Cxn *cxn, ZmAnyHash *h) {
	if (!h->telCount()) cxn->transmit(hashTbl(*h));
      }});

  // threads
  ZmSpecific<ZmThreadContext>::all(ZmFn<ZmThreadContext *>{
      cxn, [](Cxn *cxn, ZmThreadContext *tc) {
	cxn->transmit(thread(*tc));
      }});

  // mutiplexers, thread queues, sockets
  m_core->allMx(ZmFn<MxMultiplex *>{
      cxn, [](Cxn *cxn, MxMultiplex *mx) {
	if (mx->telCount()) return;
	cxn->transmit(multiplexer(*mx));
	{
	  ZmThreadName name;
	  uint64_t inCount, inBytes, outCount, outBytes;
	  for (unsigned tid = 1, n = mx->nThreads(); tid <= n; tid++) {
	    mx->threadName(tid, name);
	    const ZmScheduler::Ring &ring = mx->ring(tid);
	    ring.stats(inCount, inBytes, outCount, outBytes);
	    cxn->transmit(queue(
		  name, (uint64_t)0, (uint64_t)ring.count(),
		  inCount, inBytes, outCount, outBytes,
		  (uint32_t)ring.params().size(), (uint8_t)QueueType::Thread));
	  }
	}
	mx->allCxns([cxn](ZiConnection *cxn_) {
	    cxn->transmit(MxTelemetry::socket(*cxn_)); });
      }});

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
      auto i = m_engines.readIterator();
      while (ZmRef<MxEngine> engine = i.iterateVal())
	cxn->transmit(MxTelemetry::engine(*engine));
    }
    // I/O Links
    {
      auto i = m_links.readIterator();
      while (ZmRef<MxAnyLink> link = i.iterateVal())
	cxn->transmit(MxTelemetry::link(*link));
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
    // Databases
    if (m_dbEnv) {
      cxn->transmit(dbEnv(*m_dbEnv));
      m_dbEnv->allHosts([cxn](const ZdbHost *host) {
	  cxn->transmit(dbHost(*host)); });
      m_dbEnv->allDBs([cxn](const ZdbAny *db_) {
	  cxn->transmit(db(*db_)); });
    }
  }
}

void MxMDTelemetry::engine(MxEngine *engine)
{
  auto key = engine->id();
  Guard guard(m_lock);
  if (!m_engines.find(key)) m_engines.add(key, engine);
}

void MxMDTelemetry::link(MxAnyLink *link)
{
  auto key = ZuMkPair(link->engine()->id(), link->id());
  Guard guard(m_lock);
  if (!m_links.find(key)) m_links.add(key, link);
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
  delete m_queues.del(key);
}

void MxMDTelemetry::addDBEnv(ZdbEnv *env)
{
  Guard guard(m_lock);
  m_dbEnv = env;
}
