//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

#ifndef ZvTelemetry_HPP
#define ZvTelemetry_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <stdlib.h>
#include <time.h>

#include <zlib/ZuPrint.hpp>
#include <zlib/ZuBox.hpp>
#include <zlib/ZuID.hpp>
#include <zlib/ZuString.hpp>
#include <zlib/ZuStringN.hpp>

#include <zlib/ZmHeap.hpp>
#include <zlib/ZmHashMgr.hpp>
#include <zlib/ZmThread.hpp>
#include <zlib/ZmScheduler.hpp>
#include <zlib/ZmRWLock.hpp>
#include <zlib/ZmRBTree.hpp>

#include <zlib/ZtEnum.hpp>

#include <zlib/ZePlatform.hpp>

#include <zlib/ZiFile.hpp>

#include <zlib/Zfb.hpp>

#include <zlib/ZvFields.hpp>
#include <zlib/ZvMultiplex.hpp>

#include <zlib/telemetry_fbs.h>
#include <zlib/telreq_fbs.h>

class ZvEngine;
class ZvAnyLink;

namespace ZvTelemetry {

namespace RAG {
  ZfbEnumValues(RAG, Off, Red, Amber, Green);
}

namespace ThreadPriority {
  ZfbEnumValues(ThreadPriority, RealTime, High, Normal, Low);
}

namespace MxState {
  ZfbEnumValues(MxState,
      Stopped, Starting, Running, Draining, Drained, Stopping);

  int rag(int i) {
    using namespace RAG;
    if (i < 0 || i >= N) return Off;
    static const int values[N] =
      { Red, Amber, Green, Amber, Amber, Amber };
    return values[i];
  }
}

namespace SocketType {
  ZfbEnumValues(SocketType, TCPIn, TCPOut, UDP);
}

namespace QueueType {
  ZfbEnumValues(QueueType, Thread, IPC, Rx, Tx);
}

namespace LinkState {
  ZfbEnumValues(LinkState, 
    Down,
    Disabled,
    Deleted,
    Connecting,
    Up,
    ReconnectPending,
    Reconnecting,
    Failed,
    Disconnecting,
    ConnectPending,
    DisconnectPending)

  int rag(int i) {
    using namespace RAG;
    if (i < 0 || i >= N) return Off;
    static const int values[N] =
      { Red, Off, Off, Amber, Green, Amber, Amber, Red, Amber, Amber, Amber };
    return values[i];
  }
}

namespace EngineState {
  ZfbEnumValues(EngineState,
      Stopped, Starting, Running, Stopping,
      StartPending,		// started while stopping
      StopPending);		// stopped while starting

  int rag(int i) {
    using namespace RAG;
    if (i < 0 || i >= N) return Off;
    static const int values[N] =
      { Red, Amber, Green, Amber, Amber, Amber };
    return values[i];
  }
}

namespace DBCacheMode {
  ZfbEnumValues(DBCacheMode, Normal, FullCache)
}

namespace DBHostState {
  ZfbEnumValues(DBHostState,
      Instantiated,
      Initialized,
      Stopped,
      Electing,
      Activating,
      Active,
      Deactivating,
      Inactive,
      Stopping)

  int rag(int i) {
    using namespace RAG;
    if (i < 0 || i >= N) return Off;
    static const int values[N] =
      { Off, Amber, Red, Amber, Amber, Green, Amber, Amber, Amber };
    return values[i];
  }
}

namespace AppRole {
  ZfbEnumValues(AppRole, Dev, Test, Prod)
}

namespace Severity {
  ZfbEnumValues(Severity, Debug, Info, Warning, Error, Fatal)
}

using Heap_ = ZmHeapTelemetry;
struct Heap : public Heap_, public ZvFieldTuple<Heap> {
  static const ZvFields<Heap> fields() noexcept;
  using Key = ZuTuple<ZmIDString, unsigned, unsigned>;
  Key key() const { return Key{id, partition, size}; }
  static Key key(const fbs::Heap *heap_) {
    return Key{Zfb::Load::str(heap_->id()), heap_->partition(), heap_->size()};
  }

  uint64_t allocated() const { return (cacheAllocs + heapAllocs) - frees; }
  void allocated(uint64_t) { } // unused

  Zfb::Offset<fbs::Heap> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateHeap(fbb,
	str(fbb, id), cacheSize, bitmap(fbb, cpuset),
	cacheAllocs, heapAllocs, frees, size, partition, sharded, alignment);
  }

  Zfb::Offset<fbs::Heap> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    auto id_ = str(fbb, id);
    fbs::HeapBuilder b(fbb);
    b.add_id(id_);
    b.add_cacheAllocs(cacheAllocs);
    b.add_heapAllocs(heapAllocs);
    b.add_frees(frees);
    return b.Finish();
  }
  void loadDelta(const fbs::Heap *heap_) {
    cacheAllocs = heap_->cacheAllocs();
    heapAllocs = heap_->heapAllocs();
    frees = heap_->frees();
  }

  struct KeyPrint : public ZuPrintDelegate {
    template <typename S>
    static void print(S &s, const Key &k) {
      s << k.p<0>() << ':' << k.p<1>() << ':' << k.p<2>();
    }
  };
  friend KeyPrint ZuPrintType(const Key *);
};
inline const ZvFields<Heap> Heap::fields() noexcept {
  ZvMkFields(Heap,
      (String, id, 0),
      (Scalar, size, 0),
      (Scalar, alignment, 0),
      (Scalar, partition, 0),
      (Bool, sharded, 0),
      (Scalar, cacheSize, 0),
      (String, cpuset, 0),
      (Scalar, cacheAllocs, Dynamic | Cumulative),
      (Scalar, heapAllocs, Dynamic | Cumulative),
      (Scalar, frees, Dynamic | Cumulative),
      (ScalarFn, allocated, Synthetic | Dynamic));
}
struct Heap_load : public Heap {
  Heap_load(const fbs::Heap *heap_) : Heap{ {
    .id = Zfb::Load::str(heap_->id()),
    .cacheSize = heap_->cacheSize(),
    .cpuset = Zfb::Load::bitmap(heap_->cpuset()),
    .cacheAllocs = heap_->cacheAllocs(),
    .heapAllocs = heap_->heapAllocs(),
    .frees = heap_->frees(),
    .size = heap_->size(),
    .partition = heap_->partition(),
    .sharded = heap_->sharded(),
    .alignment = heap_->alignment()
  } } { }
};

using HashTbl_ = ZmHashTelemetry;
struct HashTbl : public HashTbl_, public ZvFieldTuple<HashTbl> {
  static const ZvFields<HashTbl> fields() noexcept;
  using Key = ZuPair<ZmIDString, uintptr_t>;
  Key key() const { return Key{id, addr}; }
  static Key key(const fbs::HashTbl *hash_) {
    return Key{Zfb::Load::str(hash_->id()), hash_->addr()};
  }

  Zfb::Offset<fbs::HashTbl> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateHashTbl(fbb,
	str(fbb, id), addr, loadFactor, effLoadFactor, nodeSize, count,
	resized, bits, cBits, linear);
  }
  Zfb::Offset<fbs::HashTbl> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    auto id_ = str(fbb, id);
    fbs::HashTblBuilder b(fbb);
    b.add_id(id_);
    b.add_count(count);
    b.add_effLoadFactor(effLoadFactor);
    return b.Finish();
  }
  void loadDelta(const fbs::HashTbl *hash_) {
    count = hash_->count();
    effLoadFactor = hash_->effLoadFactor();
  }

  struct KeyPrint : public ZuPrintDelegate {
    template <typename S>
    static void print(S &s, const Key &k) {
      s << k.p<0>() << ':' << ZuBoxed(k.p<1>()).hex();
    }
  };
  friend KeyPrint ZuPrintType(const Key *);
};
inline const ZvFields<HashTbl> HashTbl::fields() noexcept {
  ZvMkFields(HashTbl,
      (String, id, 0),
      (Hex, addr, 0),
      (Bool, linear, 0),
      (Scalar, bits, 0),
      (Scalar, cBits, 0),
      (Scalar, loadFactor, 0),
      (Scalar, nodeSize, 0),
      (Scalar, count, Dynamic),
      (Scalar, effLoadFactor, Dynamic),
      (Scalar, loadFactor, 0));
}
struct HashTbl_load : public HashTbl {
  HashTbl_load(const fbs::HashTbl *hash_) : HashTbl{ {
    .id = Zfb::Load::str(hash_->id()),
    .addr = hash_->addr(),
    .loadFactor = hash_->loadFactor(),
    .effLoadFactor = hash_->effLoadFactor(),
    .nodeSize = hash_->nodeSize(),
    .count = hash_->count(),
    .resized = hash_->resized(),
    .bits = hash_->bits(),
    .cBits = hash_->cBits(),
    .linear = hash_->linear()
  } } { }
};

using Thread_ = ZmThreadTelemetry;
struct Thread : public Thread_, public ZvFieldTuple<Thread> {
  static const ZvFields<Thread> fields() noexcept;
  const auto &key() const { return tid; }
  static auto key(const fbs::Thread *thread_) { return thread_->tid(); }

  Zfb::Offset<fbs::Thread> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateThread(fbb,
	str(fbb, name), tid, stackSize, bitmap(fbb, cpuset),
	cpuUsage, sysPriority, index, partition,
	static_cast<fbs::ThreadPriority>(priority), main, detached);
  }
  Zfb::Offset<fbs::Thread> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    fbs::ThreadBuilder b(fbb);
    b.add_tid(tid);
    b.add_cpuUsage(cpuUsage);
    return b.Finish();
  }
  void loadDelta(const fbs::Thread *thread_) {
    cpuUsage = thread_->cpuUsage();
  }
};
inline const ZvFields<Thread> Thread::fields() noexcept {
  ZvMkFields(Thread,
      (String, name, 0),
      (Scalar, tid, 0),
      (Scalar, cpuUsage, Dynamic),
      (String, cpuset, 0),
      (Enum, priority, 0, ThreadPriority::Map),
      (Scalar, sysPriority, 0),
      (Scalar, stackSize, 0),
      (Scalar, partition, 0),
      (Bool, main, 0),
      (Bool, detached, 0));
}
struct Thread_load : public Thread {
  Thread_load(const fbs::Thread *thread_) : Thread{ {
    .name = Zfb::Load::str(thread_->name()),
    .tid = thread_->tid(),
    .stackSize = thread_->stackSize(),
    .cpuset = Zfb::Load::bitmap(thread_->cpuset()),
    .cpuUsage = thread_->cpuUsage(),
    .sysPriority = thread_->sysPriority(),
    .index = thread_->index(),
    .partition = thread_->partition(),
    .priority = (int8_t)thread_->priority(),
    .main = thread_->main(),
    .detached = thread_->detached()
  } } { }
};

using Mx_ = ZiMxTelemetry;
struct Mx : public Mx_, public ZvFieldTuple<Mx> {
  static const ZvFields<Mx> fields() noexcept;
  const auto &key() const { return id; }
  static auto key(const fbs::Mx *mx_) {
    return ZuID{Zfb::Load::str(mx_->id())};
  }

  int rag() const { return MxState::rag(state); }
  void rag(int) { } // unused

  Zfb::Offset<fbs::Mx> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateMx(fbb,
	str(fbb, id), stackSize, queueSize, spin, timeout,
	rxBufSize, txBufSize, rxThread, txThread, partition,
	static_cast<fbs::MxState>(state), ll, priority, nThreads);
  }
  Zfb::Offset<fbs::Mx> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    auto id_ = str(fbb, id);
    fbs::MxBuilder b(fbb);
    b.add_id(id_);
    b.add_state(static_cast<fbs::MxState>(state));
    return b.Finish();
  }
  void loadDelta(const fbs::Mx *mx_) {
    state = mx_->state();
  }
};
inline const ZvFields<Mx> Mx::fields() noexcept {
  ZvMkFields(Mx,
      (String, id, 0),
      (Enum, state, Dynamic, MxState::Map),
      (Scalar, nThreads, 0),
      (Scalar, rxThread, 0),
      (Scalar, txThread, 0),
      (Scalar, priority, 0),
      (Scalar, stackSize, 0),
      (Scalar, partition, 0),
      (Scalar, rxBufSize, 0),
      (Scalar, txBufSize, 0),
      (Scalar, queueSize, 0),
      (Bool, ll, 0),
      (Scalar, spin, 0),
      (Scalar, timeout, 0),
      (EnumFn, rag, Dynamic | Synthetic | ColorRAG, RAG::Map));
}
struct Mx_load : public Mx {
  Mx_load(const fbs::Mx *mx_) : Mx{ {
    .id = Zfb::Load::str(mx_->id()),
    .stackSize = mx_->stackSize(),
    .queueSize = mx_->queueSize(),
    .spin = mx_->spin(),
    .timeout = mx_->timeout(),
    .rxBufSize = mx_->rxBufSize(),
    .txBufSize = mx_->txBufSize(),
    .rxThread = mx_->rxThread(),
    .txThread = mx_->txThread(),
    .partition = mx_->partition(),
    .state = (int8_t)mx_->state(),
    .ll = mx_->ll(),
    .priority = mx_->priority(),
    .nThreads = mx_->nThreads()
  } } { }
};

using Socket_ = ZiCxnTelemetry;
struct Socket : public Socket_, public ZvFieldTuple<Socket> {
  static const ZvFields<Socket> fields() noexcept;
  const auto &key() const { return socket; }
  static auto key(const fbs::Socket *socket_) { return socket_->socket(); }

  Zfb::Offset<fbs::Socket> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateSocket(fbb,
	str(fbb, mxID), socket,
	rxBufSize, rxBufLen, txBufSize, txBufLen,
	mreqAddr, mreqIf, mif, ttl,
	localIP, remoteIP, localPort, remotePort, flags,
	static_cast<fbs::SocketType>(type));
  }
  Zfb::Offset<fbs::Socket> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    fbs::SocketBuilder b(fbb);
    b.add_socket(socket);
    b.add_rxBufLen(rxBufLen);
    b.add_txBufLen(txBufLen);
    return b.Finish();
  }
  void loadDelta(const fbs::Socket *socket_) {
    rxBufLen = socket_->rxBufLen();
    txBufLen = socket_->txBufLen();
  }
};
inline const ZvFields<Socket> Socket::fields() noexcept {
  ZvMkFields(Socket,
      (String, mxID, 0),
      (Enum, type, 0, SocketType::Map),
      (String, remoteIP, 0),
      (Scalar, remotePort, 0),
      (String, localIP, 0),
      (Scalar, localPort, 0),
      (Scalar, socket, 0),
      (Scalar, flags, 0),
      (String, mreqAddr, 0),
      (String, mreqIf, 0),
      (String, mif, 0),
      (Scalar, ttl, 0),
      (Scalar, rxBufSize, 0),
      (Scalar, rxBufLen, Dynamic),
      (Scalar, txBufSize, 0),
      (Scalar, txBufLen, Dynamic));
}
struct Socket_load : public Socket {
  Socket_load(const fbs::Socket *socket_) : Socket{ {
    .mxID = Zfb::Load::str(socket_->mxID()),
    .socket = socket_->socket(),
    .rxBufSize = socket_->rxBufSize(),
    .rxBufLen = socket_->rxBufLen(),
    .txBufSize = socket_->txBufSize(),
    .txBufLen = socket_->txBufLen(),
    .mreqAddr = socket_->mreqAddr(),
    .mreqIf = socket_->mreqIf(),
    .mif = socket_->mif(),
    .ttl = socket_->ttl(),
    .localIP = socket_->localIP(),
    .remoteIP = socket_->remoteIP(),
    .localPort = socket_->localPort(),
    .remotePort = socket_->remotePort(),
    .flags = socket_->flags(),
    .type = (uint8_t)socket_->type()
  } } { }
};

// display sequence:
//   id, type, size, full, count, seqNo,
//   inCount, inBytes, outCount, outBytes
// RAG for queues - count > 50% size - amber; 75% - red
struct Queue : public ZvFieldTuple<Queue> {
  ZuID		id;		// primary key - same as Link id for Rx/Tx
  uint64_t	seqNo;		// 0 for Thread, IPC
  uint64_t	count;		// dynamic - may not equal in - out
  uint64_t	inCount;	// dynamic (*)
  uint64_t	inBytes;	// dynamic
  uint64_t	outCount;	// dynamic (*)
  uint64_t	outBytes;	// dynamic
  uint32_t	size;		// 0 for Rx, Tx
  uint32_t	full;		// dynamic - how many times queue overflowed
  uint8_t	type;		// primary key - QueueType

  int rag() const {
    if (ZuLikely(!size)) return RAG::Off;
    if (ZuLikely(count < (size>>1))) return RAG::Green;
    if (ZuLikely(count < (size>>1) + (size>>2))) return RAG::Amber;
    return RAG::Red;
  }
  void rag(int) { } // unused

  static const ZvFields<Queue> fields() noexcept;
  using Key = ZuPair<ZuID, unsigned>;
  Key key() const { return Key{id, type}; }
  static Key key(const fbs::Queue *queue_) {
    return Key{Zfb::Load::str(queue_->id()), queue_->type()}; 
  }

  Zfb::Offset<fbs::Queue> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateQueue(fbb,
	str(fbb, id), seqNo,
	count, inCount, inBytes, outCount, outBytes, size, full,
	static_cast<fbs::QueueType>(type));
  }
  Zfb::Offset<fbs::Queue> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    auto id_ = str(fbb, id);
    fbs::QueueBuilder b(fbb);
    b.add_id(id_);
    b.add_count(count);
    b.add_inCount(inCount);
    b.add_inBytes(inBytes);
    b.add_outCount(outCount);
    b.add_outBytes(outBytes);
    b.add_full(full);
    return b.Finish();
  }
  void loadDelta(const fbs::Queue *queue_) {
    count = queue_->count();
    inCount = queue_->inCount();
    inBytes = queue_->inBytes();
    outCount = queue_->outCount();
    outBytes = queue_->outBytes();
    full = queue_->full();
  }

  struct KeyPrint : public ZuPrintDelegate {
    template <typename S>
    static void print(S &s, const Key &k) {
      s << k.p<0>() << ':' << QueueType::name(k.p<1>());
    }
  };
  friend KeyPrint ZuPrintType(const Key *);
};
inline const ZvFields<Queue> Queue::fields() noexcept {
  ZvMkFields(Queue,
      (String, id, 0),
      (Enum, type, 0, QueueType::Map),
      (Scalar, size, 0),
      (Scalar, full, Dynamic | Cumulative),
      (Scalar, count, Dynamic),
      (Scalar, seqNo, 0),
      (Scalar, inCount, Dynamic | Cumulative),
      (Scalar, inBytes, Dynamic | Cumulative),
      (Scalar, outCount, Dynamic | Cumulative),
      (Scalar, outBytes, Dynamic | Cumulative),
      (EnumFn, rag, Dynamic | Synthetic | ColorRAG, RAG::Map));
}
struct Queue_load : public Queue {
  Queue_load(const fbs::Queue *queue_) : Queue{
    .id = Zfb::Load::str(queue_->id()),
    .seqNo = queue_->seqNo(),
    .count = queue_->count(),
    .inCount = queue_->inCount(),
    .inBytes = queue_->inBytes(),
    .outCount = queue_->outCount(),
    .outBytes = queue_->outBytes(),
    .size = queue_->size(),
    .full = queue_->full(),
    .type = (uint8_t)queue_->type()
  } { }
};

// display sequence:
//   id, state, reconnects, rxSeqNo, txSeqNo
struct Link : public ZvFieldTuple<Link> {
  ZuID		id;
  uint64_t	rxSeqNo;
  uint64_t	txSeqNo;
  uint32_t	reconnects;
  int8_t	state;

  static const ZvFields<Link> fields() noexcept;
  const auto &key() const { return id; }
  static auto key(const fbs::Link *link_) {
    return ZuID{Zfb::Load::str(link_->id())};
  }

  int rag() const { return LinkState::rag(state); }
  void rag(int) { } // unused

  Zfb::Offset<fbs::Link> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateLink(fbb,
	str(fbb, id), rxSeqNo, txSeqNo, reconnects,
	static_cast<fbs::LinkState>(state));
  }
  Zfb::Offset<fbs::Link> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    auto id_ = str(fbb, id);
    fbs::LinkBuilder b(fbb);
    b.add_id(id_);
    b.add_rxSeqNo(rxSeqNo);
    b.add_txSeqNo(txSeqNo);
    b.add_reconnects(reconnects);
    b.add_state(static_cast<fbs::LinkState>(state));
    return b.Finish();
  }
  void loadDelta(const fbs::Link *link_) {
    rxSeqNo = link_->rxSeqNo();
    txSeqNo = link_->txSeqNo();
    reconnects = link_->reconnects();
    state = link_->state();
  }
};
inline const ZvFields<Link> Link::fields() noexcept {
  ZvMkFields(Link,
      (String, id, 0),
      (Enum, state, Dynamic, LinkState::Map),
      (Scalar, reconnects, Dynamic | Cumulative),
      (Scalar, rxSeqNo, Dynamic | Cumulative),
      (Scalar, txSeqNo, Dynamic | Cumulative),
      (EnumFn, rag, Dynamic | Synthetic | ColorRAG, RAG::Map));
}
struct Link_load : public Link {
  Link_load(const fbs::Link *link_) : Link{
    .id = Zfb::Load::str(link_->id()),
    .rxSeqNo = link_->rxSeqNo(),
    .txSeqNo = link_->txSeqNo(),
    .reconnects = link_->reconnects(),
    .state = (int8_t)link_->state()
  } { }
};

struct Engine : public ZvFieldTuple<Engine> {
  ZuID		id;		// primary key
  ZuID		type;
  ZuID		mxID;
  uint16_t	down;
  uint16_t	disabled;
  uint16_t	transient;
  uint16_t	up;
  uint16_t	reconn;
  uint16_t	failed;
  uint16_t	nLinks;
  uint16_t	rxThread;
  uint16_t	txThread;
  int8_t	state;

  static const ZvFields<Engine> fields() noexcept;
  const auto &key() const { return id; }
  static auto key(const fbs::Engine *engine_) {
    return ZuID{Zfb::Load::str(engine_->id())};
  }

  int rag() const { return EngineState::rag(state); }
  void rag(int) { } // unused

  Zfb::Offset<fbs::Engine> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateEngine(fbb,
	str(fbb, id), str(fbb, type), str(fbb, mxID),
	down, disabled, transient, up, reconn, failed, nLinks,
	rxThread, txThread, static_cast<fbs::EngineState>(state));
  }
  Zfb::Offset<fbs::Engine> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    auto id_ = str(fbb, id);
    fbs::EngineBuilder b(fbb);
    b.add_id(id_);
    b.add_down(down);
    b.add_disabled(disabled);
    b.add_transient(transient);
    b.add_up(up);
    b.add_reconn(reconn);
    b.add_failed(failed);
    b.add_state(static_cast<fbs::EngineState>(state));
    return b.Finish();
  }
  void loadDelta(const fbs::Engine *engine_) {
    down = engine_->down();
    disabled = engine_->disabled();
    transient = engine_->transient();
    up = engine_->up();
    reconn = engine_->reconn();
    failed = engine_->failed();
    state = engine_->state();
  }
};
inline const ZvFields<Engine> Engine::fields() noexcept {
  ZvMkFields(Engine,
      (String, id, 0),
      (String, type, 0),
      (Enum, state, Dynamic, EngineState::Map),
      (Scalar, nLinks, 0),
      (Scalar, up, Dynamic),
      (Scalar, down, Dynamic),
      (Scalar, disabled, Dynamic),
      (Scalar, transient, Dynamic),
      (Scalar, reconn, Dynamic),
      (Scalar, failed, Dynamic),
      (String, mxID, 0),
      (Scalar, rxThread, 0),
      (Scalar, txThread, 0),
      (EnumFn, rag, Dynamic | Synthetic | ColorRAG, RAG::Map));
}
struct Engine_load : public Engine {
  Engine_load(const fbs::Engine *engine_) : Engine{
    .id = Zfb::Load::str(engine_->id()),
    .type = Zfb::Load::str(engine_->type()),
    .mxID = Zfb::Load::str(engine_->mxID()),
    .down = engine_->down(),
    .disabled = engine_->disabled(),
    .transient = engine_->transient(),
    .up = engine_->up(),
    .reconn = engine_->reconn(),
    .failed = engine_->failed(),
    .nLinks = engine_->nLinks(),
    .rxThread = engine_->rxThread(),
    .txThread = engine_->txThread(),
    .state = (int8_t)engine_->state() } { }
};

// display sequence: 
//   name, id, recSize, compress, cacheMode, cacheSize,
//   path, fileSize, fileRecs, filesMax, preAlloc,
//   minRN, nextRN, fileRN,
//   cacheLoads, cacheMisses, fileLoads, fileMisses
struct DB : public ZvFieldTuple<DB> {
  using Path = ZuStringN<124>;
  using Name = ZuStringN<28>;

  Path		path;
  Name		name;		// primary key
  uint64_t	fileSize;
  uint64_t	minRN;		// dynamic
  uint64_t	nextRN;		// dynamic
  uint64_t	fileRN;		// dynamic
  uint64_t	cacheLoads;	// dynamic (*)
  uint64_t	cacheMisses;	// dynamic (*)
  uint64_t	fileLoads;	// dynamic
  uint64_t	fileMisses;	// dynamic
  uint32_t	id;
  uint32_t	preAlloc;
  uint32_t	recSize;
  uint32_t	fileRecs;
  uint32_t	cacheSize;
  uint32_t	filesMax;
  uint8_t	compress;
  uint8_t	cacheMode;	// ZdbCacheMode

  static const ZvFields<DB> fields() noexcept;
  const auto &key() const { return id; }
  static auto key(const fbs::DB *db_) { return db_->id(); }

  Zfb::Offset<fbs::DB> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateDB(fbb,
	str(fbb, path), str(fbb, name), fileSize,
	minRN, nextRN, fileRN, cacheLoads, cacheMisses, fileLoads, fileMisses,
	id, preAlloc, recSize, fileRecs, cacheSize, filesMax,
	compress, static_cast<fbs::DBCacheMode>(cacheMode));
  }
  Zfb::Offset<fbs::DB> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    auto name_ = str(fbb, name);
    fbs::DBBuilder b(fbb);
    b.add_name(name_);
    b.add_minRN(minRN);
    b.add_nextRN(nextRN);
    b.add_fileRN(fileRN);
    b.add_cacheLoads(cacheLoads);
    b.add_cacheMisses(cacheMisses);
    b.add_fileLoads(fileLoads);
    b.add_fileMisses(fileMisses);
    return b.Finish();
  }
  void loadDelta(const fbs::DB *db_) {
    minRN = db_->minRN();
    nextRN = db_->nextRN();
    fileRN = db_->fileRN();
    cacheLoads = db_->cacheLoads();
    cacheMisses = db_->cacheMisses();
    fileLoads = db_->fileLoads();
    fileMisses = db_->fileMisses();
  }
};
inline const ZvFields<DB> DB::fields() noexcept {
  ZvMkFields(DB,
      (String, name, 0),
      (Scalar, id, 0),
      (Scalar, recSize, 0),
      (Scalar, compress, 0),
      (Enum, cacheMode, 0, DBCacheMode::Map),
      (Scalar, cacheSize, 0),
      (String, path, 0),
      (Scalar, fileSize, 0),
      (Scalar, fileRecs, 0),
      (Scalar, filesMax, 0),
      (Scalar, preAlloc, 0),
      (Scalar, minRN, 0),
      (Scalar, nextRN, Dynamic | Cumulative),
      (Scalar, fileRN, Dynamic | Cumulative),
      (Scalar, cacheLoads, Dynamic | Cumulative),
      (Scalar, cacheMisses, Dynamic | Cumulative),
      (Scalar, fileLoads, Dynamic | Cumulative),
      (Scalar, fileMisses, Dynamic | Cumulative));
}
struct DB_load : public DB {
  DB_load(const fbs::DB *db_) : DB{
    .path = Zfb::Load::str(db_->path()),
    .name = Zfb::Load::str(db_->name()),
    .fileSize = db_->fileSize(),
    .minRN = db_->minRN(),
    .nextRN = db_->nextRN(),
    .fileRN = db_->fileRN(),
    .cacheLoads = db_->cacheLoads(),
    .cacheMisses = db_->cacheMisses(),
    .fileLoads = db_->fileLoads(),
    .fileMisses = db_->fileMisses(),
    .id = db_->id(),
    .preAlloc = db_->preAlloc(),
    .recSize = db_->recSize(),
    .fileRecs = db_->fileRecs(),
    .cacheSize = db_->cacheSize(),
    .filesMax = db_->filesMax(),
    .compress = db_->compress(),
    .cacheMode = (uint8_t)db_->cacheMode()
  } { }
};

// display sequence:
//   id, priority, state, voted, ip, port
struct DBHost : public ZvFieldTuple<DBHost> {
  ZiIP		ip;
  uint32_t	id;
  uint32_t	priority;
  uint16_t	port;
  int8_t	state; // RAG: Instantiated - Red; Active - Green; * - Amber
  uint8_t	voted;

  int rag() const { return DBHostState::rag(state); }
  void rag(int) { } // unused

  static const ZvFields<DBHost> fields() noexcept;
  const auto &key() const { return id; }
  static auto key(const fbs::DBHost *host_) { return host_->id(); }

  Zfb::Offset<fbs::DBHost> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateDBHost(fbb,
	ip, id, priority, port,
	static_cast<fbs::DBHostState>(state), voted);
  }
  Zfb::Offset<fbs::DBHost> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    fbs::DBHostBuilder b(fbb);
    b.add_id(id);
    b.add_state(static_cast<fbs::DBHostState>(state));
    b.add_voted(voted);
    return b.Finish();
  }
  void loadDelta(const fbs::DBHost *host_) {
    state = host_->state();
    voted = host_->voted();
  }
};
inline const ZvFields<DBHost> DBHost::fields() noexcept {
  ZvMkFields(DBHost,
      (Scalar, id, 0),
      (Scalar, priority, 0),
      (Enum, state, Dynamic, DBHostState::Map),
      (Bool, voted, Dynamic),
      (String, ip, 0),
      (Scalar, port, 0),
      (EnumFn, rag, Dynamic | Synthetic | ColorRAG, RAG::Map));
}
struct DBHost_load : public DBHost {
  DBHost_load(const fbs::DBHost *host_) : DBHost{
    .ip = host_->ip(),
    .id = host_->id(),
    .priority = host_->priority(),
    .port = host_->port(),
    .state = (int8_t)host_->state(),
    .voted = host_->voted()
  } { }
};

// display sequence: 
//   self, master, prev, next, state, active, recovering, replicating,
//   nDBs, nHosts, nPeers, nCxns,
//   heartbeatFreq, heartbeatTimeout, reconnectFreq, electionTimeout,
//   writeThread
struct DBEnv : public ZvFieldTuple<DBEnv> {
  uint32_t	nCxns;
  uint32_t	heartbeatFreq;
  uint32_t	heartbeatTimeout;
  uint32_t	reconnectFreq;
  uint32_t	electionTimeout;
  uint32_t	self;		// primary key - host ID 
  uint32_t	master;		// ''
  uint32_t	prev;		// ''
  uint32_t	next;		// ''
  uint16_t	writeThread;
  uint8_t	nHosts;
  uint8_t	nPeers;
  uint8_t	nDBs;
  int8_t	state;		// same as hosts[hostID].state
  uint8_t	active;
  uint8_t	recovering;
  uint8_t	replicating;

  int rag() const { return DBHostState::rag(state); }
  void rag(int) { } // unused

  static const ZvFields<DBEnv> fields() noexcept;

  Zfb::Offset<fbs::DBEnv> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateDBEnv(fbb,
	nCxns,
	heartbeatFreq, heartbeatTimeout, reconnectFreq, electionTimeout,
	self, master, prev, next,
	writeThread,
	nHosts, nPeers, nDBs,
	static_cast<fbs::DBHostState>(state),
	active, recovering, replicating);
  }
  Zfb::Offset<fbs::DBEnv> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    fbs::DBEnvBuilder b(fbb);
    b.add_nCxns(nCxns);
    b.add_state(state);
    return b.Finish();
  }
  void loadDelta(const fbs::DBEnv *env_) {
    nCxns = env_->nCxns();
    state = env_->state();
  }
};
inline const ZvFields<DBEnv> DBEnv::fields() noexcept {
  ZvMkFields(DBEnv,
      (Scalar, self, 0),
      (Scalar, master, 0),
      (Scalar, prev, 0),
      (Scalar, next, 0),
      (Enum, state, Dynamic, DBHostState::Map),
      (Scalar, active, 0),
      (Scalar, recovering, 0),
      (Scalar, replicating, 0),
      (Scalar, nDBs, 0),
      (Scalar, nHosts, 0),
      (Scalar, nPeers, 0),
      (Scalar, nCxns, Dynamic),
      (Scalar, heartbeatFreq, 0),
      (Scalar, heartbeatTimeout, 0),
      (Scalar, reconnectFreq, 0),
      (Scalar, electionTimeout, 0),
      (Scalar, writeThread, 0),
      (EnumFn, rag, Dynamic | Synthetic | ColorRAG, RAG::Map));
}
struct DBEnv_load : public DBEnv {
  DBEnv_load(const fbs::DBEnv *env_) : DBEnv{
    .nCxns = env_->nCxns(),
    .heartbeatFreq = env_->heartbeatFreq(),
    .heartbeatTimeout = env_->heartbeatTimeout(),
    .reconnectFreq = env_->reconnectFreq(),
    .electionTimeout = env_->electionTimeout(),
    .self = env_->self(),
    .master = env_->master(),
    .prev = env_->prev(),
    .next = env_->next(),
    .writeThread = env_->writeThread(),
    .nHosts = env_->nHosts(),
    .nPeers = env_->nPeers(),
    .nDBs = env_->nDBs(),
    .state = (int8_t)env_->state(),
    .active = env_->active(),
    .recovering = env_->recovering(),
    .replicating = env_->replicating()
  } { }
};

// display sequence:
//   id, role, RAG, uptime, version
struct App : public ZvFieldTuple<App> {
  ZmIDString	id;
  ZmIDString	version;
  ZtDate	uptime;
  uint8_t	role;
  uint8_t	rag;

  static const ZvFields<App> fields() noexcept;

  Zfb::Offset<fbs::App> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateApp(fbb,
	str(fbb, id), str(fbb, version), dateTime(fbb, uptime),
	static_cast<fbs::AppRole>(role),
	static_cast<fbs::RAG>(rag));
  }
  Zfb::Offset<fbs::App> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    auto id_ = str(fbb, id);
    fbs::AppBuilder b(fbb);
    b.add_id(id_);
    b.add_rag(static_cast<fbs::RAG>(rag));
    return b.Finish();
  }
  void loadDelta(const fbs::App *app_) {
    rag = app_->rag();
  }
};
inline const ZvFields<App> App::fields() noexcept {
  ZvMkFields(App,
      (String, id, 0),
      (String, version, 0),
      (Time, uptime, 0),
      (Enum, role, 0, AppRole::Map),
      (Enum, rag, 0, RAG::Map));
}
struct App_load : public App {
  App_load(const fbs::App *app_) : App{
    .id = Zfb::Load::str(app_->id()),
    .version = Zfb::Load::str(app_->version()),
    .uptime = Zfb::Load::dateTime(app_->uptime()),
    .role = (uint8_t)app_->role(),
    .rag = (uint8_t)app_->rag()
  } { }
};

// display sequence:
//   time, severity, tid, message
struct Alert : public ZvFieldTuple<Alert> {
  ZtDate	time;
  uint32_t	seqNo;
  uint32_t	tid;
  uint8_t	severity;
  ZtString	message;

  static const ZvFields<Alert> fields() noexcept;

  Zfb::Offset<fbs::Alert> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateAlert(fbb,
	dateTime(fbb, time), seqNo, tid,
	static_cast<fbs::Severity>(severity),
	str(fbb, message));
  }
  Zfb::Offset<fbs::Alert> saveDelta(Zfb::Builder &fbb) const {
    return save(fbb);
  }
  void loadDelta(const fbs::Alert *);
};
inline const ZvFields<Alert> Alert::fields() noexcept {
  ZvMkFields(Alert,
      (Time, time, 0),
      (Scalar, seqNo, 0),
      (Scalar, tid, 0),
      (Enum, severity, 0, Severity::Map),
      (String, message, 0));
}
struct Alert_load : public Alert {
  Alert_load(const fbs::Alert *alert_) : Alert{
    .time = Zfb::Load::dateTime(alert_->time()),
    .seqNo = alert_->seqNo(),
    .tid = alert_->tid(),
    .severity = (uint8_t)alert_->severity(),
    .message = Zfb::Load::str(alert_->message())
  } { }
};
void Alert::loadDelta(const fbs::Alert *alert_) {
  this->~Alert();
  new (static_cast<void *>(this)) Alert_load{alert_};
}

namespace ReqType {
  ZfbEnumValues(ReqType,
      Heap, HashTbl, Thread, Mx, Queue, Engine, DBEnv, App, Alert);
}

namespace TelData {
  ZfbEnumValues(TelData,
      Heap, HashTbl, Thread, Mx, Socket, Queue, Engine, Link,
      DB, DBHost, DBEnv, App, Alert);
}

} // ZvTelemetry

#endif /* ZvTelemetry_HPP */
