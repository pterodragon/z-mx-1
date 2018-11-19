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

// Mx Telemetry

#ifndef MxTelemetry_HPP
#define MxTelemetry_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxBaseLib_HPP
#include <MxBaseLib.hpp>
#endif

#include <ZuConversion.hpp>

#include <ZmFn.hpp>

#include <ZiIP.hpp>
#include <ZiMultiplex.hpp>

#include <MxBase.hpp>
#include <MxMultiplex.hpp>

#pragma pack(push, 1)

namespace MxTelemetry {
  namespace Type {
    MxEnumValues(Heap, Thread, Multiplexer, HashTbl, Queue,
	Engine, Link,
	DBEnv, DBHost, DB);
    MxEnumNames("Heap", "Thread", "Multiplexer", "HashTbl", "Queue",
	"Engine", "Link",
	"DBEnv", "DBHost", "DB");
  }

  struct HdrData {
    uint32_t	len = 0;
    uint32_t	type = 0;
  };

  struct Hdr : public HdrData {
    ZuInline Hdr() { }
    ZuInline Hdr(uint32_t len, uint32_t type) : HdrData{len, type} { }

    ZuInline void *ptr() { return (void *)this; }
    ZuInline const void *ptr() const { return (const void *)this; }

    ZuInline void *body() { return (void *)&this[1]; }
    ZuInline const void *body() const { return (const void *)&this[1]; }

    template <typename T> ZuInline T &as() {
      T *ZuMayAlias(ptr) = (T *)&this[1];
      return *ptr;
    }
    template <typename T> ZuInline const T &as() const {
      const T *ZuMayAlias(ptr) = (const T *)&this[1];
      return *ptr;
    }

    template <typename T> ZuInline void pad() {
      if (ZuUnlikely(len < sizeof(T)))
	memset(((char *)&this[1]) + len, 0, sizeof(T) - len);
    }

    bool scan(unsigned length) const {
      return sizeof(Hdr) + len > length;
    }
  };

  typedef uint64_t Bitmap;
 
  struct Heap {
    enum { Code = Type::Heap };

    MxIDString	id;
    uint64_t	cacheSize;
    Bitmap	cpuset;
    uint64_t	cacheAllocs;
    uint64_t	heapAllocs;
    uint64_t	frees;
    uint64_t	allocated;
    uint64_t	maxAllocated;
    uint32_t	size;
    uint16_t	partition;
    uint8_t	alignment;
  };

  struct Thread {
    enum { Code = Type::Thread };

    MxIDString	name;
    uint64_t	tid;
    uint64_t	stackSize;
    Bitmap	cpuset;
    int32_t	id;		// thread mgr ID, -ve if unset
    int32_t	priority;
    uint16_t	partition;
    uint8_t	main;
    uint8_t	detached;
  };

  struct MxThread {
    MxIDString	id;		// ID of both thread and queue
  };
  struct Multiplexer {	
    enum { Code = Type::Multiplexer };

    MxID	id;
    MxThread	threads[16];
    Bitmap	affinity[16];
    Bitmap	isolation;
    uint32_t	stackSize;
    uint32_t	rxBufSize;
    uint32_t	txBufSize;
    uint32_t	nCxns;
    uint16_t	rxThread;
    uint16_t	txThread;
    uint16_t	partition;
    uint8_t	state;
    uint8_t	priority;
    uint8_t	nThreads;
  };

  struct HashTbl {
    enum { Code = Type::HashTbl };

    MxIDString	id;
    uint32_t	nodeSize;
    uint32_t	loadFactor;	// (double)N / 16.0
    uint32_t	count;
    uint32_t	effLoadFactor;	// (double)N / 16.0
    uint32_t	resized;
    uint8_t	bits;
    uint8_t	cBits;
    uint8_t	linear;
  };

  namespace QueueType {
    MxEnumValues(
	Thread,		// ZmRing (in ZmScheduler)
	IPC,		// ZiRing (e.g. MxMD broadcast)
	Rx,		// MxQueue (Rx)
	Tx		// MxQueue (Tx)
	);
    MxEnumNames("Thread", "IPC", "Rx", "Tx");
  }
  struct Queue {
    enum { Code = Type::Queue };

    MxIDString	id;		// is same as Link id for Rx/Tx
    uint64_t	seqNo;		// 0 for Thread, IPC
    uint64_t	count;		// due to overlaps, may not equal in - out
    uint64_t	inCount;
    uint64_t	inBytes;
    uint64_t	outCount;
    uint64_t	outBytes;
    uint32_t	size;		// 0 for Rx, Tx
    uint8_t	type;		// QueueType
  };

  struct Link {
    enum { Code = Type::Link };

    MxID	id;
    uint64_t	rxSeqNo;
    uint64_t	txSeqNo;
    uint32_t	reconnects;
    uint8_t	state;
  };

  // followed by
  //   TxPool[nTxPools]
  //   Link[nLinks]
  struct Engine {
    enum { Code = Type::Engine };

    MxID	id;
    MxID	mxID;
    uint32_t	down;
    uint32_t	disabled;
    uint32_t	transient;
    uint32_t	up;
    uint32_t	reconn;
    uint32_t	failed;
    uint16_t	nLinks;
    uint8_t	rxThread;
    uint8_t	txThread;
    uint8_t	state;
  };

  struct DB {
    enum { Code = Type::DB };

    MxTxtString	path;
    uint64_t	fileSize;
    uint64_t	allocRN;
    uint64_t	fileRN;
    uint32_t	id;
    uint32_t	preAlloc;
    uint32_t	recSize;
    uint32_t	fileRecs;
    uint32_t	cacheSize;
    uint32_t	filesMax;
    uint8_t	compress;
    uint8_t	replicate;
    uint8_t	noIndex;
    uint8_t	noLock;
  };

  struct DBHost {
    enum { Code = Type::DBHost };

    ZiIP	ip;
    uint32_t	id;
    uint32_t	priority;
    uint16_t	port;
    uint8_t	voted;
    uint8_t	state;
  };

  // followed by
  //   DBHost[nHosts]
  //   DB[nDBs]
  struct DBEnv {
    enum { Code = Type::DBEnv };

    uint32_t	nCxns;
    uint32_t	heartbeatFreq;
    uint32_t	heartbeatTimeout;
    uint32_t	reconnectFreq;
    uint32_t	electionTimeout;
    uint32_t	self;		// host ID 
    uint32_t	master;		// ''
    uint32_t	prev;		// ''
    uint32_t	next;		// ''
    uint16_t	writeThread;
    uint8_t	nHosts;
    uint8_t	nPeers;
    uint8_t	nDBs;
    uint8_t	state;		// same as hosts[hostID].state
    uint8_t	active;
    uint8_t	recovering;
    uint8_t	replicating;
  };

  typedef ZuLargest<
     Heap, Thread, Multiplexer, HashTbl, Queue,
     Engine, Link,
     DBEnv, DBHost, DB>::T Largest;

  struct Buf {
    char	data[sizeof(Largest)];
  };

  struct MsgData : public Hdr, public Buf { };
}

#pragma pack(pop)

namespace MxTelemetry {

  struct Msg_HeapID {
    ZuInline static const char *id() { return "MxMDStream.Msg"; }
  };

  template <typename Heap>
  struct Msg_ : public Heap, public ZmPolymorph, public MsgData {
    ZuInline const Hdr &hdr() const {
      return static_cast<const Hdr &>(*this);
    }
    ZuInline Hdr &hdr() {
      return static_cast<Hdr &>(*this);
    }

    ZuInline void calcLength() { length = sizeof(Hdr) + this->len; }
    ZuInline constexpr unsigned size() const { return sizeof(MsgData); }

    ZiSockAddr	addr;
    unsigned	length = 0;
  };

  typedef Msg_<ZmHeap<Msg_HeapID, sizeof(Msg_<ZuNull>)> > Msg;

#ifdef FnDeclare
#undef FnDeclare
#endif
#define FnDeclare(Fn, Type) \
  template <typename ...Args> \
  inline ZmRef<Msg> Fn(Args &&... args) { \
    ZmRef<Msg> msg = new Msg(); \
    new (msg->ptr()) Hdr{sizeof(Type), Type::Code}; \
    new (msg->body()) Type{ZuFwd<Args>(args)...}; \
    msg->calcLength(); \
    return msg; \
  }

  FnDeclare(heap, Heap)
  FnDeclare(thread, Thread)
  FnDeclare(multiplexer, Multiplexer)
  FnDeclare(hashTbl, HashTbl)
  FnDeclare(queue, Queue)
  FnDeclare(link, Link)
  FnDeclare(engine, Engine)
  FnDeclare(db, DB)
  FnDeclare(dbHost, DBHost)
  FnDeclare(dbEnv, DBEnv)

#undef FnDeclare

  template <typename Cxn, typename L> struct IOMvLambda_ {
    typedef void (*Fn)(Cxn *, ZmRef<Msg>, ZiIOContext &);
    enum { OK = ZuConversion<L, Fn>::Exists };
  };
  template <typename Cxn, typename L, bool = IOMvLambda_<Cxn, L>::OK>
  struct IOMvLambda;
  template <typename Cxn, typename L> struct IOMvLambda<Cxn, L, true> {
    typedef void T;
    ZuInline static void invoke(ZiIOContext &io) {
      (*(L *)(void *)0)(
	  static_cast<Cxn *>(io.cxn), io.fn.mvObject<Msg>(), io);
    }
  };

  namespace UDP {
    template <typename Cxn>
    inline void send(Cxn *cxn, ZmRef<Msg> msg, const ZiSockAddr &addr) {
      msg->addr = addr;
      cxn->send(ZiIOFn{[](Msg *msg, ZiIOContext &io) {
	io.init(ZiIOFn{[](Msg *msg, ZiIOContext &io) {
	  if (ZuUnlikely((io.offset += io.length) < io.size)) return;
	}, io.fn.mvObject<Msg>()},
	msg->ptr(), msg->length, 0, msg->addr);
      }, ZuMv(msg)});
    }

    template <typename Cxn, typename L>
    inline typename IOMvLambda<Cxn, L>::T recv(
	ZmRef<Msg> msg, ZiIOContext &io, L l) {
      Msg *msg_ = msg.ptr();
      io.init(ZiIOFn{[](Msg *msg, ZiIOContext &io) {
	msg->length = (io.offset += io.length);
	msg->addr = io.addr;
	IOMvLambda<Cxn, L>::invoke(io);
      }, ZuMv(msg)},
      msg_->ptr(), msg_->size(), 0);
    }
  }

  class MxBaseAPI Client {
    typedef ZmPLock Lock;
    typedef ZmGuard<Lock> Guard;
    typedef ZmReadGuard<Lock> ReadGuard;

  public:
    void init(MxMultiplex *mx, ZvCf *cf);
    void final();

    inline ZiMultiplex *mx() const { return m_mx; }

    void start();
    void stop();

    class Cxn;
  friend class Cxn;
    class MxBaseAPI Cxn : public ZiConnection {
      friend class Client;

    public:
      Cxn(Client *client, const ZiConnectionInfo &info) :
	ZiConnection(client->mx(), info), m_client(client) { }

      ZuInline Client *client() const { return m_client; }

    private:
      void connected(ZiIOContext &io) { m_client->connected(this, io); }
      void disconnected() { m_client->disconnected(this); }

      void recv(ZiIOContext &io) {
	ZmRef<Msg> msg = new Msg();
	UDP::recv<Cxn>(ZuMv(msg), io,
	    [](Cxn *cxn, ZmRef<Msg> msg, ZiIOContext &io) mutable {
	      if (!msg->hdr().scan(msg->length))
		cxn->client()->process(ZuMv(msg));
	      cxn->recv(io);
	    });
      }

      Client		*m_client = 0;
    };

    virtual void process(ZmRef<Msg> msg) = 0;

    virtual void error(ZmRef<ZeEvent> e) { ZeLog::log(ZuMv(e)); }

  private:
    void connected(Cxn *, ZiIOContext &);
    void disconnected(Cxn *);

    ZiMultiplex		*m_mx = 0;

    ZiIP		m_interface;
    ZiIP		m_ip;
    uint16_t		m_port = 0;

    Lock		m_connLock;
      ZmRef<Cxn>	  m_cxn;
  };

  class MxBaseAPI Server {
    typedef ZmPLock Lock;
    typedef ZmGuard<Lock> Guard;
    typedef ZmReadGuard<Lock> ReadGuard;

  public:
    void init(MxMultiplex *mx, ZvCf *cf);
    void final();

    inline ZiMultiplex *mx() const { return m_mx; }

    void start();
    void stop();

    class Cxn;
  friend class Cxn;
    class MxBaseAPI Cxn : public ZiConnection {
    public:
      Cxn(Server *server, const ZiConnectionInfo &info) :
	ZiConnection(server->mx(), info), m_server(server) { }

      ZuInline Server *server() const { return m_server; }

      void transmit(ZmRef<Msg> msg) { // named to avoid colliding with send()
	UDP::send(this, ZuMv(msg), m_server->m_addr);
      }

    private:
      void connected(ZiIOContext &) { m_server->connected(this); }
      void disconnected() { m_server->disconnected(this); }

      Server		*m_server = 0;
    };

    virtual void run(Cxn *) = 0; // app should repeatedly call cxn->send(msg)

    virtual void error(ZmRef<ZeEvent> e) { ZeLog::log(ZuMv(e)); }

  private:
    void run_();
    void scheduleRun();

    void connected(Cxn *);
    void disconnected(Cxn *);

    ZiMultiplex		*m_mx = 0;

    ZiIP		m_interface;
    ZiIP		m_ip;
    uint16_t		m_port = 0;
    unsigned		m_ttl = 0;
    bool		m_loopBack = false;
    unsigned		m_freq = 0;	// in microseconds

    ZiSockAddr		m_addr;
    ZmScheduler::Timer	m_timer;

    Lock		m_connLock;
      ZmRef<Cxn>	  m_cxn;
  };
}

#endif /* MxTelemetry_HPP */
