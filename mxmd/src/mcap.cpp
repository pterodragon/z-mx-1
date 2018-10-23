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

// multicast capture tool

#include <stdio.h>
#include <signal.h>

#include <ZuPOD.hpp>

#include <ZmLock.hpp>
#include <ZmGuard.hpp>
#include <ZmAtomic.hpp>
#include <ZmSemaphore.hpp>
#include <ZmTimeInterval.hpp>
#include <ZmTime.hpp>
#include <ZmTrap.hpp>

#include <ZeLog.hpp>

#include <ZiMultiplex.hpp>
#include <ZiFile.hpp>

#include <ZvCf.hpp>
#include <ZvMultiplexCf.hpp>
#include <ZvHeapCf.hpp>

#include <MxCSV.hpp>

#include <MxMDFrame.hpp>

struct Group {
  uint16_t		id;
  ZiIP			ip;
  ZuBox0(uint16_t)	port;
};

typedef ZvCSVColumn<ZvCSVColType::Int, uint16_t> GroupCol;
typedef ZvCSVColumn<ZvCSVColType::Int, ZuBox0(uint16_t)> PortCol;
typedef MxIPCol IPCol;

class GroupCSV : public ZvCSV {
public:
  typedef ZuPOD<Group> POD;

  GroupCSV() : m_pod(new POD()) {
    new (m_pod->ptr()) Group();
    add(new GroupCol("group", offsetof(Group, id)));
    add(new IPCol("ip", offsetof(Group, ip)));
    add(new PortCol("port", offsetof(Group, port)));
  }

  void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

  template <typename File>
  void read(const File &file, ZvCSVReadFn fn) {
    ZvCSV::readFile(file,
	ZvCSVAllocFn::Member<&GroupCSV::alloc>::fn(this), fn);
  }

private:
  ZuRef<POD>	m_pod;
};

class App;

class Source : public ZmPolymorph {
public:
  inline Source(App *app, const Group &group) : m_app(app), m_group(group) { }
  ~Source() { }

  void connect();
  ZiConnection *connected(const ZiCxnInfo &ci);
  void connectFailed(bool transient);

  inline App *app() const { return m_app; }
  inline const Group &group() const { return m_group; }

private:
  App		*m_app;
  Group		m_group;
};

class Connection : public ZiConnection {
public:
  Connection(Source *source, const ZiCxnInfo &ci);
  ~Connection() { }

  inline App *app() { return m_app; }
  inline const Group &group() const { return m_group; }

  void connected(ZiIOContext &io);
  void disconnected();

  void recv(ZiIOContext &io);

private:
  App		*m_app;
  Group		m_group;
};

class Mx : public ZmObject, public ZiMultiplex {
public:
  Mx(ZvCf *cf) : ZiMultiplex(ZvMultiplexParams(cf)) { }
  ~Mx() { }
};

class App : public ZmPolymorph {
public:
  App(ZvCf *cf);

  int start();
  void stop();

  void wait() { m_sem.wait(); }
  void post() { m_sem.post(); }

  void connect(ZuAnyPOD *group_);
  void write(const MxMDFrame *frame, const char *buf);

  inline const ZtString &path() const { return m_path; }
  inline const ZtString &groups() const { return m_groups; }
  inline bool raw() const { return m_raw; }
  inline ZiIP interface() const { return m_interface; }
  inline unsigned reconnectFreq() const { return m_reconnectFreq; }

  inline Mx *mx() { return m_mx; }
  inline Mx *mx2() { return m_mx2; }

private:
  ZmSemaphore	m_sem;

  ZtString	m_path;		// path of capture file
  ZtString	m_groups;	// CSV file containing multicast groups
  bool		m_raw;		// true if TSE (raw) format
  ZiIP		m_interface;	// interface to capture from
  unsigned	m_reconnectFreq;// reconnect frequency

  ZmLock	m_fileLock;	// lock on capture file
    ZiFile	m_file;		// capture file
    ZiVec	m_vecs[2];

  ZmRef<Mx>	m_mx;		// primary (receiver) multiplexer
  ZmRef<Mx>	m_mx2;		// secondary (file writer) multiplexer
};

template <typename Heap> class Msg_ : public Heap, public ZmPolymorph {
public:
  // UDP over Ethernet maximum payload is 1472 without Jumbo frames
  enum { Size = 1472 };

  template <typename Cxn> inline Msg_(Cxn &&cxn) : m_cxn(ZuFwd<Cxn>(cxn)) { }
  ~Msg_() { }

  void recv(ZiIOContext &io);
  void rcvd(ZiIOContext &io);
  void write();

private:
  ZmRef<Connection>	m_cxn;
  MxMDFrame		m_frame;
  ZiSockAddr		m_addr;
  char			m_buf[Size];
};
struct Msg_HeapID {
  inline static const char *id() { return "Msg"; }
};
typedef ZmHeap<Msg_HeapID, sizeof(Msg_<ZuNull>)> Msg_Heap;
typedef Msg_<Msg_Heap> Msg;

void App::connect(ZuAnyPOD *group_) {
  const Group &group = group_->as<Group>();
  ZmRef<Source> source = new Source(this, group);
  source->connect();
}

void Source::connect()
{
  ZiCxnOptions options;
  options.udp(true);
  options.multicast(true);
  options.mreq(ZiMReq(group().ip, m_app->interface()));
  m_app->mx()->udp(
      ZiConnectFn::Member<&Source::connected>::fn(ZmMkRef(this)),
      ZiFailFn::Member<&Source::connectFailed>::fn(ZmMkRef(this)),
#ifndef _WIN32
      group().ip,
#else
      ZiIP(),
#endif
      group().port, ZiIP(), 0, options);
}

ZiConnection *Source::connected(const ZiCxnInfo &ci)
{
  return new Connection(this, ci);
}

void Source::connectFailed(bool transient)
{
  if (transient) m_app->mx()->add(
      ZmFn<>::Member<&Source::connect>::fn(ZmMkRef(this)),
      ZmTimeNow((int)m_app->reconnectFreq()));
}

Connection::Connection(Source *source, const ZiCxnInfo &ci) :
    ZiConnection(source->app()->mx(), ci),
    m_app(source->app()),
    m_group(source->group())
{
}

void Connection::connected(ZiIOContext &io)
{
  recv(io);
}

void Connection::disconnected()
{
  m_app->post();
}

void Connection::recv(ZiIOContext &io)
{
  ZmRef<Msg> msg = new Msg(this);
  msg->recv(io);
}

template <typename Heap>
void Msg_<Heap>::recv(ZiIOContext &io)
{
  io.init(ZiIOFn::Member<&Msg_::rcvd>::fn(ZmMkRef(this)),
      m_buf, Size, 0, m_addr);
}

template <typename Heap>
void Msg_<Heap>::rcvd(ZiIOContext &io)
{
  ZmTime now(ZmTime::Now);
  m_frame.len = io.offset + io.length;
  m_frame.type = 0;
  m_frame.sec = now.sec();
  m_frame.nsec = now.nsec();
  m_frame.linkID = m_cxn->group().id;

  m_cxn->app()->mx2()->add(ZmFn<>::Member<&Msg_::write>::fn(ZmMkRef(this)));

  m_cxn->recv(io);
}

template <typename Heap>
void Msg_<Heap>::write()
{
  m_cxn->app()->write(&m_frame, m_buf);
}

void App::write(const MxMDFrame *frame, const char *buf)
{
  ZeError e;
  {
    ZmGuard<ZmLock> guard(m_fileLock);
    if (!m_raw) {
      m_vecs[0].iov_base = (void *)frame;
      m_vecs[0].iov_len = sizeof(MxMDFrame);
      m_vecs[1].iov_base = (void *)buf;
      m_vecs[1].iov_len = frame->len;
      if (m_file.writev(m_vecs, 2, &e) != Zi::OK) goto error;
    } else {
      if (m_file.write((void *)buf, frame->len, &e) != Zi::OK) goto error;
    }
  }
  return;

error:
  ZeLOG(Error, ZtString() << '"' << m_path << "\": " << e);
}

App::App(ZvCf *cf) :
  m_path(cf->get("path", true)),
  m_groups(cf->get("groups", true)),
  m_raw(cf->getInt("raw", 0, 1, false, 0)),
  m_interface(cf->get("interface", false, "0.0.0.0")),
  m_reconnectFreq(cf->getInt("reconnect", 0, 3600, false, 0))
{
  m_mx = new Mx(cf->subset("mx", false));
  m_mx2 = new Mx(cf->subset("mx2", false));
}

int App::start()
{
  try {
    ZeError e;
    if (m_file.open(
	  m_path, ZiFile::Create | ZiFile::Append, 0666, &e) != Zi::OK) {
      ZeLOG(Fatal, ZtString() << '"' << m_path << "\": " << e);
      goto error;
    }
    if (m_mx->start() != Zi::OK ||
	m_mx2->start() != Zi::OK) {
      ZeLOG(Fatal, "multiplexer start failed");
      goto error;
    }
    GroupCSV csv;
    csv.read(m_groups, ZvCSVReadFn::Member<&App::connect>::fn(this));
  } catch (const ZvError &e) {
    ZeLOG(Fatal, ZtString() << e);
    goto error;
  } catch (const ZeError &e) {
    ZeLOG(Fatal, ZtString() << e);
    goto error;
  } catch (...) {
    ZeLOG(Fatal, "Unknown Exception");
    goto error;
  }
  return Zi::OK;

error:
  m_mx->stop(true);
  m_mx2->stop(true);
  m_file.close();
  return Zi::IOError;
}

void App::stop()
{
  m_mx->stop(true);
  m_mx2->stop(true);
  m_file.close();
}

void usage()
{
  std::cerr <<
    "usage: mcap [OPTION]... CONFIG\n"
    "  capture IP multicast data as specified in the CONFIG file\n\n"
    "Options:\n"
    << std::flush;
  exit(1);
}

#if 0
static void printHeapStats()
{
  std::cout <<
    "\nHeap Statistics\n"
      "===============\n" <<
    ZmHeapMgr::csv() << std::flush;
}
#endif

int main(int argc, const char *argv[])
{
  const char *cfPath = 0;

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      if (cfPath) usage();
      cfPath = argv[i];
      continue;
    }
    switch (argv[i][1]) {
      default:
	usage();
	break;
    }
  }
  if (!cfPath) usage();

  ZeLog::init("mcap");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink("&2"));
  ZeLog::start();

  {
    ZmRef<App> app;

    try {

      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromFile(cfPath, false);

      if (ZmRef<ZvCf> heapCf = cf->subset("heap", false))
	ZvHeapMgrCf::init(heapCf);

      app = new App(cf);

    } catch (const ZvError &e) {
      ZeLOG(Fatal, ZtString() << e);
      goto error;
    } catch (const ZeError &e) {
      ZeLOG(Fatal, ZtString() << e);
      goto error;
    } catch (...) {
      ZeLOG(Fatal, "Unknown Exception");
      goto error;
    }

    ZmTrap::sigintFn(ZmFn<>::Member<&App::post>::fn(app));
    ZmTrap::trap();

    if (app->start() != Zi::OK) goto error;

    app->wait();

    ZmTrap::sigintFn(ZmFn<>());

    app->stop();
  }

  ZeLog::stop();

  // printHeapStats();

  return 0;

error:
  ZeLog::stop();
  exit(1);
}
