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
#include <zlib/ZuAssert.hpp>

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

#include <zlib/ZvField.hpp>
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

template <typename> struct load;

using Heap_ = ZmHeapTelemetry;
struct Heap : public Heap_, public ZvFieldTuple<Heap> {
  using FBS = fbs::Heap;
  static const ZvFieldArray fields() noexcept;
  using Key = ZuTuple<decltype(id), decltype(partition), decltype(size)>;
  Key key() const { return Key{id, partition, size}; }
  static Key key(const fbs::Heap *heap_) {
    return Key{Zfb::Load::str(heap_->id()), heap_->partition(), heap_->size()};
  }

  int rag() const {
    if (!cacheSize) return RAG::Off;
    if (allocated() > cacheSize) return RAG::Red;
    if (heapAllocs) return RAG::Amber;
    return RAG::Green;
  }
  void rag(int) { } // unused

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
    b.add_partition(partition);
    b.add_size(size);
    b.add_cacheAllocs(cacheAllocs);
    b.add_heapAllocs(heapAllocs);
    b.add_frees(frees);
    return b.Finish();
  }
  void loadDelta(const fbs::Heap *);
};
inline const ZvFieldArray Heap::fields() noexcept {
  ZvFields(Heap,
      (String, id, 0),
      (Int, size, 0),
      (Int, alignment, 0),
      (Int, partition, 0),
      (Bool, sharded, 0),
      (Int, cacheSize, 0),
      (Stream, cpuset, 0),
      (Int, cacheAllocs, Series | Delta),
      (Int, heapAllocs, Series | Delta),
      (Int, frees, Series | Delta),
      (IntFn, allocated, Synthetic | Series));
}
template <> struct load<Heap> : public Heap {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::Heap *heap_) : Heap{ {
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
inline void Heap::loadDelta(const fbs::Heap *heap_) {
  if (Zfb::IsFieldPresent(heap_, fbs::Heap::VT_CACHESIZE)) {
    this->~Heap();
    new (this) load<Heap>{heap_};
    return;
  }
  cacheAllocs = heap_->cacheAllocs();
  heapAllocs = heap_->heapAllocs();
  frees = heap_->frees();
}

using HashTbl_ = ZmHashTelemetry;
struct HashTbl : public HashTbl_, public ZvFieldTuple<HashTbl> {
  using FBS = fbs::HashTbl;
  static const ZvFieldArray fields() noexcept;
  using Key = ZuTuple<decltype(id), decltype(addr)>;
  Key key() const { return Key{id, addr}; }
  static Key key(const fbs::HashTbl *hash_) {
    return Key{Zfb::Load::str(hash_->id()), hash_->addr()};
  }

  int rag() const {
    if (resized) return RAG::Red;
    if (effLoadFactor >= loadFactor * 0.8) return RAG::Amber;
    return RAG::Green;
  }
  void rag(int) { } // unused

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
  void loadDelta(const fbs::HashTbl *);
};
inline const ZvFieldArray HashTbl::fields() noexcept {
  ZvFields(HashTbl,
      (String, id, 0),
      (Hex, addr, 0),
      (Bool, linear, 0),
      (Int, bits, 0),
      (Int, cBits, 0),
      (Int, loadFactor, 0),
      (Int, nodeSize, 0),
      (Int, count, Series),
      (Float, effLoadFactor, Series, 2),
      (Int, loadFactor, 0));
}
template <> struct load<HashTbl> : public HashTbl {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::HashTbl *hash_) : HashTbl{ {
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
inline void HashTbl::loadDelta(const fbs::HashTbl *hash_) {
  if (Zfb::IsFieldPresent(hash_, fbs::HashTbl::VT_NODESIZE)) {
    this->~HashTbl();
    new (this) load<HashTbl>{hash_};
    return;
  }
  count = hash_->count();
  effLoadFactor = hash_->effLoadFactor();
}

using Thread_ = ZmThreadTelemetry;
struct Thread : public Thread_, public ZvFieldTuple<Thread> {
  using FBS = fbs::Thread;
  static const ZvFieldArray fields() noexcept;
  using Key = ZuTuple<decltype(tid)>;
  Key key() const { return Key{tid}; }
  static Key key(const fbs::Thread *thread_) {
    return Key{thread_->tid()};
  }

  int rag() const {
    if (cpuUsage >= 0.8) return RAG::Red;
    if (cpuUsage >= 0.5) return RAG::Amber;
    return RAG::Green;
  }
  void rag(int) { } // unused

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
  void loadDelta(const fbs::Thread *);
};
inline const ZvFieldArray Thread::fields() noexcept {
  ZvFields(Thread,
      (String, name, 0),
      (Int, tid, 0),
      (Float, cpuUsage, Series, 2),
      (Stream, cpuset, 0),
      (Enum, priority, 0, ThreadPriority::Map),
      (Int, sysPriority, 0),
      (Int, stackSize, 0),
      (Int, partition, 0),
      (Bool, main, 0),
      (Bool, detached, 0));
}
template <> struct load<Thread> : public Thread {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::Thread *thread_) : Thread{ {
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
inline void Thread::loadDelta(const fbs::Thread *thread_) {
  if (Zfb::IsFieldPresent(thread_, fbs::Thread::VT_STACKSIZE)) {
    this->~Thread();
    new (this) load<Thread>{thread_};
    return;
  }
  cpuUsage = thread_->cpuUsage();
}

using Mx_ = ZiMxTelemetry;
struct Mx : public Mx_, public ZvFieldTuple<Mx> {
  using FBS = fbs::Mx;
  static const ZvFieldArray fields() noexcept;
  using Key = ZuTuple<decltype(id)>;
  Key key() const { return Key{id}; }
  static Key key(const fbs::Mx *mx_) {
    return Key{Zfb::Load::str(mx_->id())};
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
  void loadDelta(const fbs::Mx *);
};
inline const ZvFieldArray Mx::fields() noexcept {
  ZvFields(Mx,
      (String, id, 0),
      (Enum, state, Series, MxState::Map),
      (Int, nThreads, 0),
      (Int, rxThread, 0),
      (Int, txThread, 0),
      (Int, priority, 0),
      (Int, stackSize, 0),
      (Int, partition, 0),
      (Int, rxBufSize, 0),
      (Int, txBufSize, 0),
      (Int, queueSize, 0),
      (Bool, ll, 0),
      (Int, spin, 0),
      (Int, timeout, 0));
}
template <> struct load<Mx> : public Mx {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::Mx *mx_) : Mx{ {
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
inline void Mx::loadDelta(const fbs::Mx *mx_) {
  if (Zfb::IsFieldPresent(mx_, fbs::Mx::VT_STACKSIZE)) {
    this->~Mx();
    new (this) load<Mx>{mx_};
    return;
  }
  state = mx_->state();
}

using Socket_ = ZiCxnTelemetry;
struct Socket : public Socket_, public ZvFieldTuple<Socket> {
  using FBS = fbs::Socket;
  static const ZvFieldArray fields() noexcept;
  using Key = ZuTuple<decltype(socket)>;
  Key key() const { return Key{socket}; }
  static Key key(const fbs::Socket *socket_) {
    return Key{socket_->socket()};
  }

  int rag() const {
    if (rxBufLen * 10 >= (rxBufSize<<3) ||
	txBufLen * 10 >= (txBufSize<<3)) return RAG::Red;
    if ((rxBufLen<<1) >= rxBufSize ||
	(txBufLen<<1) >= txBufSize) return RAG::Amber;
    return RAG::Green;
  }
  void rag(int) { } // unused

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
  void loadDelta(const fbs::Socket *);
};
inline const ZvFieldArray Socket::fields() noexcept {
  ZvFields(Socket,
      (String, mxID, 0),
      (Enum, type, 0, SocketType::Map),
      (Stream, remoteIP, 0),
      (Int, remotePort, 0),
      (Stream, localIP, 0),
      (Int, localPort, 0),
      (Int, socket, 0),
      (Int, flags, 0),
      (Stream, mreqAddr, 0),
      (Stream, mreqIf, 0),
      (Stream, mif, 0),
      (Int, ttl, 0),
      (Int, rxBufSize, 0),
      (Int, rxBufLen, Series),
      (Int, txBufSize, 0),
      (Int, txBufLen, Series));
}
template <> struct load<Socket> : public Socket {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::Socket *socket_) : Socket{ {
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
    .type = static_cast<int8_t>(socket_->type())
  } } { }
};
inline void Socket::loadDelta(const fbs::Socket *socket_) {
  if (Zfb::IsFieldPresent(socket_, fbs::Socket::VT_RXBUFSIZE)) {
    this->~Socket();
    new (this) load<Socket>{socket_};
    return;
  }
  rxBufLen = socket_->rxBufLen();
  txBufLen = socket_->txBufLen();
}

// display sequence:
//   id, type, size, full, count, seqNo,
//   inCount, inBytes, outCount, outBytes
struct Queue : public ZvFieldTuple<Queue> {
  ZuID		id;		// primary key - same as Link id for Rx/Tx
  uint64_t	seqNo = 0;	// 0 for Thread, IPC
  uint64_t	count = 0;	// dynamic - may not equal in - out
  uint64_t	inCount = 0;	// dynamic (*)
  uint64_t	inBytes = 0;	// dynamic
  uint64_t	outCount = 0;	// dynamic (*)
  uint64_t	outBytes = 0;	// dynamic
  uint32_t	size = 0;	// 0 for Rx, Tx
  uint32_t	full = 0;	// dynamic - how many times queue overflowed
  int8_t	type = -1;	// primary key - QueueType

  using FBS = fbs::Queue;
  static const ZvFieldArray fields() noexcept;
  using Key = ZuTuple<decltype(id), decltype(type)>;
  Key key() const { return Key{id, type}; }
  static Key key(const fbs::Queue *queue_) {
    return Key{Zfb::Load::str(queue_->id()), queue_->type()}; 
  }

  // RAG for queues - count > 50% size - amber; 80% - red
  int rag() const {
    if (!size) return RAG::Off;
    if (count * 10 >= (size<<3)) return RAG::Red;
    if ((count<<1) >= size) return RAG::Amber;
    return RAG::Green;
  }
  void rag(int) { } // unused

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
  void loadDelta(const fbs::Queue *);
};
inline const ZvFieldArray Queue::fields() noexcept {
  ZvFields(Queue,
      (String, id, 0),
      (Enum, type, 0, QueueType::Map),
      (Int, size, 0),
      (Int, full, Series | Delta),
      (Int, count, Series),
      (Int, seqNo, 0),
      (Int, inCount, Series | Delta),
      (Int, inBytes, Series | Delta),
      (Int, outCount, Series | Delta),
      (Int, outBytes, Series | Delta));
}
template <> struct load<Queue> : public Queue {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::Queue *queue_) : Queue{
    .id = Zfb::Load::str(queue_->id()),
    .seqNo = queue_->seqNo(),
    .count = queue_->count(),
    .inCount = queue_->inCount(),
    .inBytes = queue_->inBytes(),
    .outCount = queue_->outCount(),
    .outBytes = queue_->outBytes(),
    .size = queue_->size(),
    .full = queue_->full(),
    .type = static_cast<int8_t>(queue_->type())
  } { }
};
inline void Queue::loadDelta(const fbs::Queue *queue_) {
  if (Zfb::IsFieldPresent(queue_, fbs::Queue::VT_SIZE)) {
    this->~Queue();
    new (this) load<Queue>{queue_};
    return;
  }
  count = queue_->count();
  inCount = queue_->inCount();
  inBytes = queue_->inBytes();
  outCount = queue_->outCount();
  outBytes = queue_->outBytes();
  full = queue_->full();
}

// display sequence:
//   id, state, reconnects, rxSeqNo, txSeqNo
struct Link : public ZvFieldTuple<Link> {
  ZuID		id;
  ZuID		engineID;
  uint64_t	rxSeqNo = 0;
  uint64_t	txSeqNo = 0;
  uint32_t	reconnects = 0;
  int8_t	state = 0;

  using FBS = fbs::Link;
  static const ZvFieldArray fields() noexcept;
  using Key = ZuTuple<decltype(id)>;
  Key key() const { return Key{id}; }
  static Key key(const fbs::Link *link_) {
    return Key{Zfb::Load::str(link_->id())};
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
    auto engineID_ = str(fbb, engineID);
    fbs::LinkBuilder b(fbb);
    b.add_id(id_);
    b.add_engineID(engineID_);
    b.add_rxSeqNo(rxSeqNo);
    b.add_txSeqNo(txSeqNo);
    b.add_reconnects(reconnects);
    b.add_state(static_cast<fbs::LinkState>(state));
    return b.Finish();
  }
  void loadDelta(const fbs::Link *);
};
inline const ZvFieldArray Link::fields() noexcept {
  ZvFields(Link,
      (String, id, 0),
      (String, engineID, 0),
      (Enum, state, Series, LinkState::Map),
      (Int, reconnects, Series | Delta),
      (Int, rxSeqNo, Series | Delta),
      (Int, txSeqNo, Series | Delta));
}
template <> struct load<Link> : public Link {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::Link *link_) : Link{
    .id = Zfb::Load::str(link_->id()),
    .engineID = Zfb::Load::str(link_->engineID()),
    .rxSeqNo = link_->rxSeqNo(),
    .txSeqNo = link_->txSeqNo(),
    .reconnects = link_->reconnects(),
    .state = (int8_t)link_->state()
  } { }
};
inline void Link::loadDelta(const fbs::Link *link_) {
  if (Zfb::IsFieldPresent(link_, fbs::Link::VT_ENGINEID)) {
    this->~Link();
    new (this) load<Link>{link_};
    return;
  }
  rxSeqNo = link_->rxSeqNo();
  txSeqNo = link_->txSeqNo();
  reconnects = link_->reconnects();
  state = link_->state();
}

struct Engine : public ZvFieldTuple<Engine> {
  ZuID		id;		// primary key
  ZuID		type;
  ZuID		mxID;
  uint16_t	down = 0;
  uint16_t	disabled = 0;
  uint16_t	transient = 0;
  uint16_t	up = 0;
  uint16_t	reconn = 0;
  uint16_t	failed = 0;
  uint16_t	nLinks = 0;
  uint16_t	rxThread = 0;
  uint16_t	txThread = 0;
  int8_t	state = -1;

  using FBS = fbs::Engine;
  static const ZvFieldArray fields() noexcept;
  using Key = ZuTuple<decltype(id)>;
  Key key() const { return Key{id}; }
  static Key key(const fbs::Engine *engine_) {
    return Key{Zfb::Load::str(engine_->id())};
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
  void loadDelta(const fbs::Engine *);
};
inline const ZvFieldArray Engine::fields() noexcept {
  ZvFields(Engine,
      (String, id, 0),
      (String, type, 0),
      (Enum, state, Series, EngineState::Map),
      (Int, nLinks, 0),
      (Int, up, Series),
      (Int, down, Series),
      (Int, disabled, Series),
      (Int, transient, Series),
      (Int, reconn, Series),
      (Int, failed, Series),
      (String, mxID, 0),
      (Int, rxThread, 0),
      (Int, txThread, 0));
}
template <> struct load<Engine> : public Engine {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::Engine *engine_) : Engine{
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
inline void Engine::loadDelta(const fbs::Engine *engine_) {
  if (Zfb::IsFieldPresent(engine_, fbs::Engine::VT_TYPE)) {
    this->~Engine();
    new (this) load<Engine>{engine_};
    return;
  }
  down = engine_->down();
  disabled = engine_->disabled();
  transient = engine_->transient();
  up = engine_->up();
  reconn = engine_->reconn();
  failed = engine_->failed();
  state = engine_->state();
}

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
  uint64_t	fileSize = 0;
  uint64_t	minRN = 0;	// dynamic
  uint64_t	nextRN = 0;	// dynamic
  uint64_t	fileRN = 0;	// dynamic
  uint64_t	cacheLoads = 0;	// dynamic (*)
  uint64_t	cacheMisses = 0;// dynamic (*)
  uint64_t	fileLoads = 0;	// dynamic
  uint64_t	fileMisses = 0;	// dynamic
  uint32_t	id = 0;
  uint32_t	preAlloc = 0;
  uint32_t	recSize = 0;
  uint32_t	fileRecs = 0;
  uint32_t	cacheSize = 0;
  uint32_t	filesMax = 0;
  uint8_t	compress = 0;
  int8_t	cacheMode = -1;	// ZdbCacheMode

  using FBS = fbs::DB;
  static const ZvFieldArray fields() noexcept;
  using Key = ZuTuple<decltype(id)>;
  Key key() const { return Key{id}; }
  static Key key(const fbs::DB *db_) { return Key{db_->id()}; }

  int rag() const {
    unsigned total = cacheLoads + cacheMisses;
    if (!total) return RAG::Off;
    if (cacheMisses * 10 > (total<<3)) return RAG::Red;
    if ((cacheMisses<<1) > total) return RAG::Amber;
    return RAG::Green;
  }
  void rag(int) { } // unused

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
  void loadDelta(const fbs::DB *);
};
inline const ZvFieldArray DB::fields() noexcept {
  ZvFields(DB,
      (String, name, 0),
      (Int, id, 0),
      (Int, recSize, 0),
      (Int, compress, 0),
      (Enum, cacheMode, 0, DBCacheMode::Map),
      (Int, cacheSize, 0),
      (String, path, 0),
      (Int, fileSize, 0),
      (Int, fileRecs, 0),
      (Int, filesMax, 0),
      (Int, preAlloc, 0),
      (Int, minRN, 0),
      (Int, nextRN, Series | Delta),
      (Int, fileRN, Series | Delta),
      (Int, cacheLoads, Series | Delta),
      (Int, cacheMisses, Series | Delta),
      (Int, fileLoads, Series | Delta),
      (Int, fileMisses, Series | Delta));
}
template <> struct load<DB> : public DB {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::DB *db_) : DB{
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
    .cacheMode = static_cast<int8_t>(db_->cacheMode())
  } { }
};
inline void DB::loadDelta(const fbs::DB *db_) {
  if (Zfb::IsFieldPresent(db_, fbs::DB::VT_PATH)) {
    this->~DB();
    new (this) load<DB>{db_};
    return;
  }
  minRN = db_->minRN();
  nextRN = db_->nextRN();
  fileRN = db_->fileRN();
  cacheLoads = db_->cacheLoads();
  cacheMisses = db_->cacheMisses();
  fileLoads = db_->fileLoads();
  fileMisses = db_->fileMisses();
}

// display sequence:
//   id, priority, state, voted, ip, port
struct DBHost : public ZvFieldTuple<DBHost> {
  ZiIP		ip;
  uint32_t	id = 0;
  uint32_t	priority = 0;
  uint16_t	port = 0;
  int8_t	state = 0;// RAG: Instantiated - Red; Active - Green; * - Amber
  uint8_t	voted = 0;

  using FBS = fbs::DBHost;
  static const ZvFieldArray fields() noexcept;
  using Key = ZuTuple<decltype(id)>;
  Key key() const { return Key{id}; }
  static Key key(const fbs::DBHost *host_) { return Key{host_->id()}; }

  int rag() const { return DBHostState::rag(state); }
  void rag(int) { } // unused

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
  void loadDelta(const fbs::DBHost *);
};
inline const ZvFieldArray DBHost::fields() noexcept {
  ZvFields(DBHost,
      (Int, id, 0),
      (Int, priority, 0),
      (Enum, state, Series, DBHostState::Map),
      (Bool, voted, Series),
      (Stream, ip, 0),
      (Int, port, 0));
}
template <> struct load<DBHost> : public DBHost {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::DBHost *host_) : DBHost{
    .ip = host_->ip(),
    .id = host_->id(),
    .priority = host_->priority(),
    .port = host_->port(),
    .state = (int8_t)host_->state(),
    .voted = host_->voted()
  } { }
};
inline void DBHost::loadDelta(const fbs::DBHost *host_) {
  if (Zfb::IsFieldPresent(host_, fbs::DBHost::VT_PRIORITY)) {
    this->~DBHost();
    new (this) load<DBHost>{host_};
    return;
  }
  state = host_->state();
  voted = host_->voted();
}

// display sequence: 
//   self, master, prev, next, state, active, recovering, replicating,
//   nDBs, nHosts, nPeers, nCxns,
//   heartbeatFreq, heartbeatTimeout, reconnectFreq, electionTimeout,
//   writeThread
struct DBEnv : public ZvFieldTuple<DBEnv> {
  uint32_t	nCxns = 0;
  uint32_t	heartbeatFreq = 0;
  uint32_t	heartbeatTimeout = 0;
  uint32_t	reconnectFreq = 0;
  uint32_t	electionTimeout = 0;
  uint32_t	self = 0;		// primary key - host ID 
  uint32_t	master = 0;		// ''
  uint32_t	prev = 0;		// ''
  uint32_t	next = 0;		// ''
  uint16_t	writeThread = 0;
  uint8_t	nHosts = 0;
  uint8_t	nPeers = 0;
  uint8_t	nDBs = 0;
  int8_t	state = -1;		// same as hosts[hostID].state
  uint8_t	active = 0;
  uint8_t	recovering = 0;
  uint8_t	replicating = 0;

  using FBS = fbs::DBEnv;
  static const ZvFieldArray fields() noexcept;

  int rag() const { return DBHostState::rag(state); }
  void rag(int) { } // unused

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
    b.add_master(master);
    b.add_prev(prev);
    b.add_next(next);
    b.add_state(state);
    b.add_active(active);
    b.add_recovering(recovering);
    b.add_replicating(replicating);
    return b.Finish();
  }
  void loadDelta(const fbs::DBEnv *);
};
inline const ZvFieldArray DBEnv::fields() noexcept {
  ZvFields(DBEnv,
      (Int, self, 0),
      (Int, master, 0),
      (Int, prev, 0),
      (Int, next, 0),
      (Enum, state, Series, DBHostState::Map),
      (Int, active, 0),
      (Int, recovering, 0),
      (Int, replicating, 0),
      (Int, nDBs, 0),
      (Int, nHosts, 0),
      (Int, nPeers, 0),
      (Int, nCxns, Series),
      (Int, heartbeatFreq, 0),
      (Int, heartbeatTimeout, 0),
      (Int, reconnectFreq, 0),
      (Int, electionTimeout, 0),
      (Int, writeThread, 0));
}
template <> struct load<DBEnv> : public DBEnv {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::DBEnv *env_) : DBEnv{
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
inline void DBEnv::loadDelta(const fbs::DBEnv *env_) {
  if (Zfb::IsFieldPresent(env_, fbs::DBEnv::VT_NHOSTS)) {
    this->~DBEnv();
    new (this) load<DBEnv>{env_};
    return;
  }
  nCxns = env_->nCxns();
  master = env_->master();
  prev = env_->prev();
  next = env_->next();
  state = env_->state();
  active = env_->active();
  recovering = env_->recovering();
  replicating = env_->replicating();
}

// display sequence:
//   id, role, RAG, uptime, version
struct App : public ZvFieldTuple<App> {
  ZmIDString	id;
  ZmIDString	version;
  ZtDate	uptime;
  int8_t	role = -1;
  int8_t	rag_ = -1;

  using FBS = fbs::App;
  static const ZvFieldArray fields() noexcept;

  int rag() const { return rag_; }
  void rag(int v) { rag_ = v; }

  Zfb::Offset<fbs::App> save(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    return fbs::CreateApp(fbb,
	str(fbb, id), str(fbb, version), dateTime(fbb, uptime),
	static_cast<fbs::AppRole>(role),
	static_cast<fbs::RAG>(rag_));
  }
  Zfb::Offset<fbs::App> saveDelta(Zfb::Builder &fbb) const {
    using namespace Zfb::Save;
    auto id_ = str(fbb, id);
    auto uptime_ = dateTime(fbb, uptime);
    fbs::AppBuilder b(fbb);
    b.add_id(id_);
    b.add_uptime(uptime_);
    b.add_rag(static_cast<fbs::RAG>(rag_));
    return b.Finish();
  }
  void loadDelta(const fbs::App *);
};
inline const ZvFieldArray App::fields() noexcept {
  ZvFields(App,
      (String, id, 0),
      (String, version, 0),
      (Time, uptime, 0),
      (Enum, role, 0, AppRole::Map),
      (Enum, rag_, 0, RAG::Map));
}
template <> struct load<App> : public App {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::App *app_) : App{
    .id = Zfb::Load::str(app_->id()),
    .version = Zfb::Load::str(app_->version()),
    .uptime = Zfb::Load::dateTime(app_->uptime()),
    .role = static_cast<int8_t>(app_->role()),
    .rag_ = static_cast<int8_t>(app_->rag())
  } { }
};
inline void App::loadDelta(const fbs::App *app_) {
  if (Zfb::IsFieldPresent(app_, fbs::App::VT_VERSION)) {
    this->~App();
    new (this) load<App>{app_};
    return;
  }
  uptime = Zfb::Load::dateTime(app_->uptime());
  rag_ = app_->rag();
}

// display sequence:
//   time, severity, tid, message
struct Alert : public ZvFieldTuple<Alert> {
  ZtDate	time;
  uint32_t	seqNo = 0;
  uint32_t	tid = 0;
  int8_t	severity = -1;
  ZtString	message;

  using FBS = fbs::Alert;
  static const ZvFieldArray fields() noexcept;

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
inline const ZvFieldArray Alert::fields() noexcept {
  ZvFields(Alert,
      (Time, time, 0),
      (Int, seqNo, 0),
      (Int, tid, 0),
      (Enum, severity, 0, Severity::Map),
      (String, message, 0));
}
template <> struct load<Alert> : public Alert {
  load() = default;
  load(const load &) = default;
  load(load &&) = default;
  load(const fbs::Alert *alert_) : Alert{
    .time = Zfb::Load::dateTime(alert_->time()),
    .seqNo = alert_->seqNo(),
    .tid = alert_->tid(),
    .severity = static_cast<int8_t>(alert_->severity()),
    .message = Zfb::Load::str(alert_->message())
  } { }
};
inline void Alert::loadDelta(const fbs::Alert *alert_) {
  this->~Alert();
  new (static_cast<void *>(this)) load<Alert>{alert_};
}

namespace ReqType {
  ZfbEnumValues(ReqType,
      Heap, HashTbl, Thread, Mx, Queue, Engine, DBEnv, App, Alert);
}

namespace TelData {
  ZfbEnumUnion(TelData,
      Heap, HashTbl, Thread, Mx, Socket, Queue, Engine, Link,
      DB, DBHost, DBEnv, App, Alert);
}

using TypeList = ZuTypeList<
  Heap, HashTbl, Thread, Mx, Socket, Queue, Engine, Link,
  DB, DBHost, DBEnv, App, Alert>;

} // ZvTelemetry

#endif /* ZvTelemetry_HPP */
