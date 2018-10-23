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

// multicast replay tool

#include <stdio.h>
#include <signal.h>

#include <ZuPOD.hpp>

#include <ZmAtomic.hpp>
#include <ZmSemaphore.hpp>
#include <ZmTime.hpp>
#include <ZmTrap.hpp>
#include <ZmHash.hpp>

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

class Dest : public ZmPolymorph {
public:
  inline Dest(App *app, const Group &group) : m_app(app), m_group(group) { }
  ~Dest() { }

  void connect();
  ZiConnection *connected(const ZiConnectionInfo &ci);
  void connectFailed(bool transient);

  inline App *app() const { return m_app; }
  inline const Group &group() const { return m_group; }

private:
  App	*m_app;
  Group	m_group;
};

class Connection : public ZiConnection {
public:
  struct GroupIDAccessor : public ZuAccessor<Connection *, uint16_t> {
    inline static uint16_t value(const Connection *c) { return c->groupID(); }
  };

  Connection(Dest *dest, const ZiConnectionInfo &ci);
  ~Connection() { }

  inline App *app() const { return m_app; }
  inline uint16_t groupID() const { return m_groupID; }
  inline const ZiSockAddr &dest() const { return m_dest; }

  void connected(ZiIOContext &);
  void disconnected();

private:
  App		*m_app;
  uint16_t	m_groupID;
  ZiSockAddr	m_dest;
};

class Mx : public ZmObject, public ZiMultiplex {
public:
  Mx(ZvCf *cf) : ZiMultiplex(ZvMultiplexParams(cf)) { }
  ~Mx() { }
};

class App : public ZmPolymorph {
  typedef ZmHash<ZmRef<Connection>,
	    ZmHashObject<ZuNull,
	      ZmHashIndex<Connection::GroupIDAccessor> > > Cxns;

public:
  App(ZvCf *cf);
  ~App() { }

  int start();
  void stop();

  void wait() { m_sem.wait(); }
  void post() { m_sem.post(); }

  void connect(ZuAnyPOD *pod);
  void read();

  inline const ZtString &replay() const { return m_replay; }
  inline const ZtString &groups() const { return m_groups; }
  inline ZuBox<double> speed() const { return m_speed; }
  inline ZuBox<double> interval() const { return m_interval; }
  inline ZiIP interface() const { return m_interface; }
  inline int ttl() const { return m_ttl; }
  inline bool loopBack() const { return m_loopBack; }

  inline Mx *mx() { return m_mx; }

  void connected_(Connection *cxn) { m_cxns.add(cxn); }
  void disconnected_(Connection *cxn) { delete m_cxns.del(cxn->groupID()); }
  inline int nCxns() { return m_cxns.count(); }

private:
  ZmSemaphore	m_sem;

  ZtString	m_replay;	// path of capture file to replay
  ZtString	m_groups;	// CSV file containing multicast groups
  ZuBox<double>	m_speed;	// replay speed multiplier
  ZuBox<double>	m_interval;	// delay interval between ticks
  ZiIP		m_interface;	// interface to send to
  int		m_ttl;		// broadcast TTL
  bool		m_loopBack;	// broadcast loopback

  ZmLock	m_fileLock;	// lock on capture file
    ZiFile	m_file;		// capture file

  ZmRef<Mx>	m_mx;		// multiplexer

  Cxns		m_cxns;
  ZmTime	m_prev;
};

template <typename Heap> class Msg_ : public Heap, public ZmPolymorph {
public:
  // UDP over Ethernet maximum payload is 1472 without Jumbo frames
  enum { Size = 1472 };

  inline Msg_(App *app) : m_app(app) { }
  inline ~Msg_() { }

  int read(ZiFile *);
  inline uint32_t group() { return m_frame.linkID; }

  void send(Connection *);
  void send_(ZiIOContext &);
  void sent_(ZiIOContext &);

  inline ZmTime stamp() const {
    return ZmTime((time_t)m_frame.sec, (int32_t)m_frame.nsec);
  }

private:
  App			*m_app;
  ZmRef<Connection>	m_cxn;
  MxMDFrame		m_frame;
  char			m_buf[Size];
};
struct Msg_HeapID {
  inline static const char *id() { return "Msg"; }
};
typedef ZmHeap<Msg_HeapID, sizeof(Msg_<ZuNull>)> Msg_Heap;
typedef Msg_<Msg_Heap> Msg;

void App::connect(ZuAnyPOD *pod)
{
  const Group &group = pod->as<Group>();
  ZmRef<Dest> dest = new Dest(this, group);
  dest->connect();
}

void Dest::connect()
{
  ZiCxnOptions options;
  options.udp(true);
  options.multicast(true);
  options.mif(m_app->interface());
  options.ttl(m_app->ttl());
  options.loopBack(m_app->loopBack());
  m_app->mx()->udp(
      ZiConnectFn::Member<&Dest::connected>::fn(ZmMkRef(this)),
      ZiFailFn::Member<&Dest::connectFailed>::fn(ZmMkRef(this)),
      ZiIP(), m_group.port, ZiIP(), 0, options);
}

ZiConnection *Dest::connected(const ZiConnectionInfo &ci)
{
  return new Connection(this, ci);
}

void Dest::connectFailed(bool transient)
{
  if (transient)
    m_app->mx()->add(
	ZmFn<>::Member<&Dest::connect>::fn(this),
	ZmTimeNow(1));
  else
    m_app->post();
}

Connection::Connection(Dest *dest, const ZiConnectionInfo &ci) :
    ZiConnection(dest->app()->mx(), ci),
    m_app(dest->app()),
    m_groupID(dest->group().id),
    m_dest(dest->group().ip, dest->group().port)
{
}

void Connection::connected(ZiIOContext &io)
{
  io.complete();
  m_app->connected_(this);
}

void Connection::disconnected()
{
  m_app->disconnected_(this);
}

void App::read()
{
  ZmRef<Msg> msg = new Msg(this);

  {
    ZmGuard<ZmLock> guard(m_fileLock);
    if (msg->read(&m_file) != Zi::OK) { post(); return; }
  }

  if (ZmRef<Connection> cxn = m_cxns.findKey(msg->group()))
    msg->send(cxn);

  ZuBox<double> delay;

  {
    ZmTime next = msg->stamp();

    if (next) {
      delay = !m_prev ? 0.0 : (next - m_prev).dtime() / m_speed;
      m_prev = next;
    } else
      delay = 0;
  }

  delay += m_interval;

  if (delay.feq(0))
    mx()->add(ZmFn<>::Member<&App::read>::fn(this));
  else
    mx()->add(ZmFn<>::Member<&App::read>::fn(this), ZmTimeNow(delay));
}

template <typename Heap>
int Msg_<Heap>::read(ZiFile *file)
{
  ZeError e;
  int n;

  n = file->read(&m_frame, sizeof(MxMDFrame), &e);
  if (n == Zi::IOError) goto error;
  if (n == Zi::EndOfFile || (unsigned)n < sizeof(MxMDFrame)) goto eof;

  if (m_frame.len > Size) goto lenerror;
  n = file->read(m_buf, m_frame.len, &e);
  if (n == Zi::IOError) goto error;
  if (n == Zi::EndOfFile || (unsigned)n < m_frame.len) goto eof;

  return Zi::OK;

eof:
  ZeLOG(Info, ZtString() << '"' << m_app->replay() << "\": EOF");
  return Zi::EndOfFile;

error:
  ZeLOG(Error, ZtString() << '"' << m_app->replay() << "\": read() - " <<
      Zi::resultName(n) << " - " << e);
  return Zi::IOError;

lenerror:
  {
    uint64_t offset = file->offset();
    offset -= sizeof(MxMDFrame);
    ZeLOG(Error, ZtString() << '"' << m_app->replay() << "\": "
	"message length >" << ZuBoxed(Size) <<
	" at offset " << ZuBoxed(offset));
  }
  return Zi::IOError;
}

template <typename Heap>
void Msg_<Heap>::send(Connection *cxn)
{
  (m_cxn = cxn)->send(ZiIOFn::Member<&Msg_<Heap>::send_>::fn(ZmMkRef(this)));
}
template <typename Heap>
void Msg_<Heap>::send_(ZiIOContext &io)
{
  io.init(ZiIOFn::Member<&Msg_<Heap>::sent_>::fn(ZmMkRef(this)),
      m_buf, m_frame.len, 0, m_cxn->dest());
  m_cxn = 0;
}
template <typename Heap>
void Msg_<Heap>::sent_(ZiIOContext &io)
{
  if (ZuUnlikely((io.offset += io.length) < io.size)) return;
  io.complete();
}

App::App(ZvCf *cf) :
  m_replay(cf->get("replay", true)),
  m_groups(cf->get("groups", true)),
  m_speed(cf->getDbl("speed", 0, ZuBox<double>::inf(), false, 1)),
  m_interval(cf->getDbl("interval", 0, 1, false, 0)),
  m_interface(cf->get("interface", false, "0.0.0.0")),
  m_ttl(cf->getInt("ttl", 0, INT_MAX, false, 1)),
  m_loopBack(cf->getInt("loopBack", 0, 1, false, 0))
{
  m_mx = new Mx(cf->subset("mx", false));
}

int App::start()
{
  try {
    ZeError e;
    if (m_file.open(m_replay, ZiFile::ReadOnly, 0666, &e) != Zi::OK) {
      ZeLOG(Fatal, ZtString() << '"' << m_replay << "\": " << e);
      goto error;
    }
    if (m_mx->start() != Zi::OK) {
      ZeLOG(Fatal, "multiplexer start failed");
      goto error;
    }
    GroupCSV csv;
    csv.read(m_groups, ZvCSVReadFn::Member<&App::connect>::fn(this));
    m_mx->add(ZmFn<>::Member<&App::read>::fn(this));
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
  m_file.close();
  return Zi::IOError;
}

void App::stop()
{
  m_mx->stop(true);
  m_file.close();
  m_cxns.clean();
}

void usage()
{
  std::cerr <<
    "usage: mcreplay [OPTION]... CONFIG\n"
    "  replay IP multicast data as specified in the CONFIG file\n\n"
    "Options:\n"
    << std::flush;
  exit(1);
}

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

  ZeLog::init("mcreplay");
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
  return 0;

error:
  ZeLog::stop();
  exit(1);
}
