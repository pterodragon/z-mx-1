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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// Z Database

// Notes on replication and failover:

// voted (connected, associated and heartbeated) hosts are sorted in
// priority order (i.e. dbState then priority):
//   first-ranked is master
//   second-ranked is master's next
//   third-ranked is second-ranked's next
//   etc.

// a new next is identified and recovery/replication restarts when
// * an election ends
// * a new host heartbeats for first time after election completes
// * an existing host disconnects

// a new master is identified (and the local instance may activate/deactivate)
// when:
// * an election ends
// * a new host heartbeats for first time after election completes
//   - possible deactivation of local instance only -
//   - if self is master and the new host < this one, we just heartbeat it
// * an existing host disconnects (if that is master, a new election begins)

// if replicating from primary to DR and a down secondary comes back up,
// then primary's m_next will be DR and DR's m_next will be secondary

// if master and not replicating, then no host is a replica, so master runs
// as standalone until peers have recovered

#include <Zdb.hpp>

#include <ZiDir.hpp>

#include <assert.h>
#include <errno.h>

#include <ZmBitmap.hpp>

ZdbEnv::ZdbEnv(unsigned maxDBID) :
  m_mx(0), m_stateCond(m_lock),
  m_appActive(false), m_self(0), m_master(0), m_prev(0), m_next(0),
  m_nextCxn(0), m_recovering(false), m_nPeers(0)
{
  m_dbs.length(maxDBID + 1);
}

ZdbEnv::~ZdbEnv()
{
}

void ZdbEnv::init(const ZdbEnvConfig &config, ZiMultiplex *mx,
    ZmFn<> activeFn, ZmFn<> inactiveFn)
{
  Guard guard(m_lock);
  if (state() != Zdb_Host::Instantiated) {
    ZeLOG(Fatal, "ZdbEnv::init called out of order");
    return;
  }
  if (!config.writeThread ||
      config.writeThread > mx->nThreads() ||
      config.writeThread == mx->rxThread() ||
      config.writeThread == mx->txThread()) {
    ZeLOG(Fatal, ZtString() <<
	"Zdb writeThread misconfigured: " << config.writeThread);
    return;
  }
  m_config = config;
  m_mx = mx;
  m_cxns = new CxnHash(m_config.cxnHash);
  unsigned n = m_config.hostCfs.length();
  for (unsigned i = 0; i < n; i++)
    m_hosts.add(ZmRef<Zdb_Host>(new Zdb_Host(this, &m_config.hostCfs[i])));
  m_self = m_hosts.findKey(m_config.hostID).ptr();
  if (!m_self) {
    ZeLOG(Fatal, ZtString() <<
	"Zdb own host ID " << m_config.hostID << "not in hosts table");
    return;
  }
  state(Zdb_Host::Initialized);
  m_activeFn = activeFn;
  m_inactiveFn = inactiveFn;
  guard.unlock();
  m_stateCond.broadcast();
}

void ZdbEnv::final()
{
  Guard guard(m_lock);
  if (state() != Zdb_Host::Initialized) {
    ZeLOG(Fatal, "ZdbEnv::final called out of order");
    return;
  }
  state(Zdb_Host::Instantiated);
  {
    unsigned i, n = m_dbs.length();
    for (i = 0; i < n; i++) if (Zdb_ *db = m_dbs[i]) db->final();
  }
  m_activeFn = ZmFn<>();
  m_inactiveFn = ZmFn<>();
  guard.unlock();
  m_stateCond.broadcast();
}

void ZdbEnv::add(Zdb_ *db)
{
  Guard guard(m_lock);
  ZdbID id = db->id();
  if (state() != Zdb_Host::Initialized) {
    ZeLOG(Fatal, ZtString() <<
	"ZdbEnv::add called out of order for DBID " << id);
    return;
  }
  if (id >= (ZdbID)m_dbs.length()) {
    ZeLOG(Fatal, ZtString() <<
	"ZdbEnv::add called with invalid DBID " << id <<
	" (dbCount = " << m_dbs.length() << ')');
    return;
  }
  unsigned i, n = m_config.dbCfs.length();
  for (i = 0; i < n; i++)
    if (id == (ZdbID)m_config.dbCfs[i].id) {
      db->init(&m_config.dbCfs[i]);
      m_dbs[id] = db;
      return;
    }
  ZeLOG(Fatal, ZtString() <<
      "ZdbEnv::add called with invalid DBID " << id);
}

void ZdbEnv::open()
{
  Guard guard(m_lock);
  if (state() != Zdb_Host::Initialized) {
    ZeLOG(Fatal, "ZdbEnv::open called out of order");
    return;
  }
  {
    unsigned i, n = m_dbs.length();
    for (i = 0; i < n; i++) if (Zdb_ *db = m_dbs[i]) db->open();
  }
  dbStateRefresh_();
  state(Zdb_Host::Stopped);
  guard.unlock();
  m_stateCond.broadcast();
}

void ZdbEnv::close()
{
  Guard guard(m_lock);
  if (state() != Zdb_Host::Stopped) {
    ZeLOG(Fatal, "ZdbEnv::close called out of order");
    return;
  }
  {
    unsigned i, n = m_dbs.length();
    for (i = 0; i < n; i++) if (Zdb_ *db = m_dbs[i]) db->close();
  }
  state(Zdb_Host::Initialized);
  guard.unlock();
  m_stateCond.broadcast();
}

void ZdbEnv::checkpoint()
{
  Guard guard(m_lock);
  switch (state()) {
    case Zdb_Host::Instantiated:
    case Zdb_Host::Initialized:
      ZeLOG(Fatal, "ZdbEnv::checkpoint called out of order");
      return;
  }
  {
    unsigned i, n = m_dbs.length();
    for (i = 0; i < n; i++) if (Zdb_ *db = m_dbs[i]) db->checkpoint();
  }
}

void ZdbEnv::start()
{
  {
    Guard guard(m_lock);

retry:
    switch (state()) {
      case Zdb_Host::Instantiated:
      case Zdb_Host::Initialized:
	ZeLOG(Fatal, "ZdbEnv::start called out of order");
	return;
      case Zdb_Host::Stopped:
	break;
      case Zdb_Host::Stopping:
	do { m_stateCond.wait(); } while (state() == Zdb_Host::Stopping);
	goto retry;
      default:
	return;
    }

    state(Zdb_Host::Electing);
    stopReplication();
    if (m_nPeers = m_hosts.count() - 1) {
      dbStateRefresh_();
      m_mx->add(ZmFn<>::Member<&ZdbEnv::hbSend>::fn(this),
	  m_hbSendTime = ZmTimeNow(), &m_hbSendTimer);
      m_mx->add(ZmFn<>::Member<&ZdbEnv::holdElection>::fn(this),
	  ZmTimeNow((int)m_config.electionTimeout), &m_electTimer);
    }
    guard.unlock();
    m_stateCond.broadcast();
  }

  ZeLOG(Info, "Zdb starting");

  if (m_hosts.count() == 1) {
    holdElection();
    return;
  }

  listen();

  {
    auto i = m_hosts.readIterator<ZmRBTreeLess>(m_config.hostID);
    while (Zdb_Host *host = i.iterateKey()) host->connect();
  }
}

void ZdbEnv::stop()
{
  ZeLOG(Info, "Zdb stopping");

  {
    Guard guard(m_lock);

retry:
    switch (state()) {
      case Zdb_Host::Instantiated:
      case Zdb_Host::Initialized:
	ZeLOG(Fatal, "ZdbEnv::stop called out of order");
	return;
      case Zdb_Host::Stopped:
	return;
      case Zdb_Host::Activating:
	do { m_stateCond.wait(); } while (state() == Zdb_Host::Activating);
	goto retry;
      case Zdb_Host::Deactivating:
	do { m_stateCond.wait(); } while (state() == Zdb_Host::Deactivating);
	goto retry;
      case Zdb_Host::Stopping:
	do { m_stateCond.wait(); } while (state() == Zdb_Host::Stopping);
	goto retry;
      default:
	break;
    }

    state(Zdb_Host::Stopping);
    stopReplication();
    guard.unlock();
    m_mx->del(&m_hbSendTimer);
    m_mx->del(&m_electTimer);
    m_stateCond.broadcast();
  }

  // cancel reconnects
  {
    auto i = m_hosts.readIterator<ZmRBTreeLess>(m_config.hostID);
    while (Zdb_Host *host = i.iterateKey()) host->cancelConnect();
  }

  stopListening();

  // close all connections (and wait for them to be disconnected)
  if (disconnectAll()) {
    Guard guard(m_lock);
    while (m_nPeers > 0) m_stateCond.wait();
    m_nPeers = 0; // paranoia
  }

  // final clean up
  {
    Guard guard(m_lock);

    state(Zdb_Host::Stopped);
    guard.unlock();
    m_stateCond.broadcast();
  }
}

bool ZdbEnv::disconnectAll()
{
  m_lock.lock();
  unsigned n = m_cxns->count();
  ZtArray<ZmRef<Zdb_Cxn> > cxns(n);
  unsigned i = 0;
  {
    ZmRef<Zdb_Cxn> cxn;
    CxnHash::ReadIterator j(*m_cxns);
    while (i < n && (cxn = j.iterateKey())) if (cxn->up()) ++i, cxns.push(cxn);
  }
  m_lock.unlock();
  for (unsigned j = 0; j < i; j++)
    cxns[j]->disconnect();
  return i;
}

void ZdbEnv::listen()
{
  m_mx->listen(
      ZiListenFn::Member<&ZdbEnv::listening>::fn(this),
      ZiFailFn::Member<&ZdbEnv::listenFailed>::fn(this),
      ZiConnectFn::Member<&ZdbEnv::accepted>::fn(this),
      m_self->ip(), m_self->port(), m_config.nAccepts);
}

void ZdbEnv::listening(const ZiListenInfo &)
{
  ZeLOG(Info, ZtString() << "Zdb listening on (" <<
      m_self->ip() << ':' << m_self->port() << ')');
}

void ZdbEnv::listenFailed(bool transient)
{
  ZtString warning;
  warning << "Zdb listen failed on (" <<
      m_self->ip() << ':' << m_self->port() << ')';
  if (transient && running()) {
    warning << " - retrying...";
    m_mx->add(ZmFn<>::Member<&ZdbEnv::listen>::fn(this),
      ZmTimeNow((int)m_config.reconnectFreq));
  }
  ZeLOG(Warning, ZuMv(warning));
}

void ZdbEnv::stopListening()
{
  ZeLOG(Info, "Zdb stop listening");
  m_mx->stopListening(m_self->ip(), m_self->port());
}

void ZdbEnv::holdElection()
{
  bool won, appActive;
  Zdb_Host *oldMaster;

  m_mx->del(&m_electTimer);

  {
    Guard guard(m_lock);
    if (state() != Zdb_Host::Electing) return;
    appActive = m_appActive;
    oldMaster = setMaster();
    if (won = m_master == m_self) {
      state(Zdb_Host::Activating);
      m_appActive = true;
      m_prev = 0;
      if (!m_nPeers)
	ZeLOG(Warning, "Zdb activating standalone");
      else
	hbSend_(0); // announce new master
    } else {
      state(Zdb_Host::Deactivating);
      m_appActive = false;
    }
    guard.unlock();
    m_stateCond.broadcast();
  }

  if (won) {
    if (!appActive) {
      ZeLOG(Info, "Zdb ACTIVE");
      if (ZtString cmd = m_self->config().up) {
	if (oldMaster) cmd << ' ' << oldMaster->config().ip;
	ZeLOG(Info, ZtString() << "Zdb invoking \"" << cmd << '\"');
	::system(cmd);
      }
      m_activeFn();
    }
  } else {
    if (appActive) {
      ZeLOG(Info, "Zdb INACTIVE");
      if (ZuString cmd = m_self->config().down) {
	ZeLOG(Info, ZtString() << "Zdb invoking \"" << cmd << '\"');
	::system(cmd);
      }
      m_inactiveFn();
    }
  }

  {
    Guard guard(m_lock);
    state(won ? Zdb_Host::Active : Zdb_Host::Inactive);
    setNext();
    guard.unlock();
    m_stateCond.broadcast();
  }
}

void ZdbEnv::deactivate()
{
  bool appActive;

  {
    Guard guard(m_lock);
retry:
    switch (state()) {
      case Zdb_Host::Instantiated:
      case Zdb_Host::Initialized:
      case Zdb_Host::Stopped:
      case Zdb_Host::Stopping:
	ZeLOG(Fatal, "ZdbEnv::inactive called out of order");
	return;
      case Zdb_Host::Deactivating:
      case Zdb_Host::Inactive:
	return;
      case Zdb_Host::Activating:
	do { m_stateCond.wait(); } while (state() == Zdb_Host::Activating);
	goto retry;
      default:
	break;
    }
    state(Zdb_Host::Deactivating);
    appActive = m_appActive;
    m_self->voted(false);
    setMaster();
    m_self->voted(true);
    m_appActive = false;
    guard.unlock();
    m_stateCond.broadcast();
  }

  if (appActive) {
    ZeLOG(Info, "Zdb INACTIVE");
    if (ZuString cmd = m_self->config().down) {
      ZeLOG(Info, ZtString() << "Zdb invoking \"" << cmd << '\"');
      ::system(cmd);
    }
    m_inactiveFn();
  }

  {
    Guard guard(m_lock);
    state(Zdb_Host::Inactive);
    setNext();
    guard.unlock();
    m_stateCond.broadcast();
  }
}

void Zdb_Host::reactivate()
{
  m_env->reactivate(this);
}

void ZdbEnv::reactivate(Zdb_Host *host)
{
  if (ZmRef<Zdb_Cxn> cxn = host->cxn()) cxn->hbSend();
  ZeLOG(Info, "Zdb dual active detected, remaining master");
  if (ZtString cmd = m_self->config().up) {
    cmd << ' ' << host->config().ip;
    ZeLOG(Info, ZtString() << "Zdb invoking \'" << cmd << '\'');
    ::system(cmd);
  }
}

Zdb_Host::Zdb_Host(ZdbEnv *env, const ZdbHostConfig *config) :
  m_env(env),
  m_config(config),
  m_mx(env->mx()),
  m_state(Instantiated),
  m_voted(false)
{
  unsigned n = env->dbCount();
  m_dbState.length(env->dbCount());
  for (unsigned i = 0; i < n; i++) m_dbState[i] = 0;
}

void Zdb_Host::connect()
{
  if (!m_env->running() || m_cxn) return;

  ZeLOG(Info, ZtString() << "Zdb connecting to host " << id() <<
      " (" << m_config->ip << ':' << m_config->port << ')');

  m_mx->connect(
      ZiConnectFn::Member<&Zdb_Host::connected>::fn(this),
      ZiFailFn::Member<&Zdb_Host::connectFailed>::fn(this),
      ZiIP(), 0, m_config->ip, m_config->port);
}

void Zdb_Host::connectFailed(bool transient)
{
  ZtString warning;
  warning << "Zdb failed to connect to host " << id() <<
      " (" << m_config->ip << ':' << m_config->port << ')';
  if (transient && m_env->running()) {
    warning << " - retrying...";
    reconnect();
  }
  ZeLOG(Warning, ZuMv(warning));
}

ZiConnection *Zdb_Host::connected(const ZiCxnInfo &ci)
{
  ZeLOG(Info, ZtString() <<
      "Zdb connected to host " << id() <<
      " (" << ci.remoteIP << ':' << ci.remotePort << "): " <<
      ci.localIP << ':' << ci.localPort);

  if (!m_env->running()) return 0;

  return new Zdb_Cxn(m_env, this, ci);
}

ZiConnection *ZdbEnv::accepted(const ZiCxnInfo &ci)
{
  ZeLOG(Info, ZtString() << "Zdb accepted cxn on (" <<
      ci.localIP << ':' << ci.localPort << "): " <<
      ci.remoteIP << ':' << ci.remotePort);

  if (!running()) return 0;

  return new Zdb_Cxn(this, 0, ci);
}

Zdb_Cxn::Zdb_Cxn(ZdbEnv *env, Zdb_Host *host, const ZiCxnInfo &ci) :
  ZiConnection(env->mx(), ci),
  m_env(env),
  m_mx(env->mx()),
  m_host(host),
  m_hbSendVec(0)
{
  memset(&m_hbSendHdr, 0, sizeof(Zdb_Msg_Hdr));
}

void Zdb_Cxn::connected(ZiIOContext &io)
{
  if (!m_env->running()) { io.disconnect(); return; }

  m_env->connected(this);

  m_mx->add(ZmFn<>::Member<&Zdb_Cxn::hbTimeout>::fn(this),
      ZmTimeNow((int)m_env->config().heartbeatTimeout),
      ZmScheduler::Defer, &m_hbTimer);

  msgRead(io);
}

void ZdbEnv::connected(Zdb_Cxn *cxn)
{
  m_cxns->add(cxn);

  Guard guard(m_lock);

  if (Zdb_Host *host = cxn->host()) associate(cxn, host);

  hbSend_(cxn);
}

void ZdbEnv::associate(Zdb_Cxn *cxn, int hostID)
{
  Guard guard(m_lock);

  Zdb_Host *host = m_hosts.findKey(hostID);

  if (!host) {
    ZeLOG(Error, ZtString() <<
	"Zdb cannot associate incoming cxn: host ID " << hostID <<
	" not found");
    return;
  }

  if (host == m_self) {
    ZeLOG(Error, ZtString() <<
	"Zdb cannot associate incoming cxn: host ID " << hostID <<
	" is same as self");
    return;
  }

  if (cxn->host() == host) return;

  associate(cxn, host);
}

void ZdbEnv::associate(Zdb_Cxn *cxn, Zdb_Host *host)
{
  ZeLOG(Info, ZtString() << "Zdb host " << host->id() << " CONNECTED");

  cxn->host(host);

  host->associate(cxn);

  host->voted(false);
}

void Zdb_Host::associate(Zdb_Cxn *cxn)
{
  Guard guard(m_lock);

  if (ZuUnlikely(m_cxn && m_cxn.ptr() != cxn)) {
    m_cxn->host(0);
    m_cxn->m_mx->add(ZmFn<>::Member<&ZiConnection::disconnect>::fn(m_cxn));
  }
  m_cxn = cxn;
}

void Zdb_Host::reconnect()
{
  m_mx->add(ZmFn<>::Member<&Zdb_Host::reconnect2>::fn(this),
      ZmTimeNow((int)m_env->config().reconnectFreq),
      ZmScheduler::Defer, &m_connectTimer);
}

void Zdb_Host::reconnect2()
{
  connect();
}

void Zdb_Host::cancelConnect()
{
  m_mx->del(&m_connectTimer);
}

void Zdb_Cxn::hbTimeout()
{
  ZeLOG(Info, ZtString() << "Zdb heartbeat timeout on host " <<
      ZuBoxed(m_host ? (int)m_host->id() : -1) << " (" <<
      info().remoteIP << ':' << info().remotePort << ')');
  disconnect();
}

void Zdb_Cxn::disconnected()
{
  ZeLOG(Info, ZtString() << "Zdb disconnected from host " <<
      ZuBoxed(m_host ? (int)m_host->id() : -1) << " (" <<
      info().remoteIP << ':' << info().remotePort << ')');
  m_mx->del(&m_hbTimer);
  m_env->disconnected(this);
}

void ZdbEnv::disconnected(Zdb_Cxn *cxn)
{
  delete m_cxns->del(cxn);

  if (cxn == m_nextCxn) m_nextCxn = 0;

  Zdb_Host *host = cxn->host();

  if (!host || host->cxn() != cxn) return;

  {
    Guard guard(m_lock);

    if (state() == Zdb_Host::Stopping && --m_nPeers <= 0) {
      guard.unlock();
      m_stateCond.broadcast();
    }
  }

  host->disconnected();
  ZeLOG(Info, ZtString() << "Zdb host " << host->id() << " DISCONNECTED");

  {
    Guard guard(m_lock);

    host->state(Zdb_Host::Instantiated);
    host->voted(false);

    switch (state()) {
      case Zdb_Host::Activating:
      case Zdb_Host::Active:
      case Zdb_Host::Deactivating:
      case Zdb_Host::Inactive:
	break;
      default:
	goto ret;
    }

    if (host == m_prev) m_prev = 0;

    if (host == m_master) {
  retry:
      switch (state()) {
	case Zdb_Host::Deactivating:
	  do { m_stateCond.wait(); } while (state() == Zdb_Host::Deactivating);
	  goto retry;
	case Zdb_Host::Inactive:
	  state(Zdb_Host::Electing);
	  guard.unlock();
	  m_stateCond.broadcast();
	  holdElection();
	  break;
      }
      goto ret;
    }

    if (host == m_next) setNext();
  }

ret:
  if (running() && host->id() < m_config.hostID) host->reconnect();
}

void Zdb_Host::disconnected()
{
  m_cxn = 0;
}

Zdb_Host *ZdbEnv::setMaster()
{
  Zdb_Host *oldMaster = m_master;
  dbStateRefresh_();
  m_master = 0;
  m_nPeers = 0;
  {
    auto i = m_hosts.readIterator();
    ZmRef<Zdb_Host> host;

    ZdbDEBUG(this, ZtString() << "setMaster()\n" << 
	"  self " << m_self << '\n' <<
	"  prev " << m_prev << '\n' <<
	"  next " << m_next << '\n' <<
	"  recovering " << m_recovering << " replicating " << !!m_nextCxn);
    while (host = i.iterateKey()) {
      if (host->voted()) {
	if (host != m_self) ++m_nPeers;
	if (!m_master || host->cmp(m_master) > 0) m_master = host;
      }
      ZdbDEBUG(this, ZtString() <<
	  "  host" << *host << '\n' <<
	  "  master" << m_master);
    }
  }
  ZeLOG(Info, ZtString() << "Zdb host " << m_master->id() << " is master");
  return oldMaster;
}

void ZdbEnv::setNext(Zdb_Host *host)
{
  m_next = host;
  m_recovering = false;
  m_nextCxn = 0;
  if (m_next) startReplication();
}

void ZdbEnv::setNext()
{
  m_next = 0;
  {
    auto i = m_hosts.readIterator();
    ZdbDEBUG(this, ZtString() << "setNext()\n" <<
	"  self " << m_self << '\n' <<
	"  master " << m_master << '\n' <<
	"  prev " << m_prev << '\n' <<
	"  next " << m_next << '\n' <<
	"  recovering " << m_recovering << " replicating " << !!m_nextCxn);
    while (Zdb_Host *host = i.iterateKey()) {
      if (host != m_self && host != m_prev && host->voted() &&
	  host->cmp(m_self) < 0 && (!m_next || host->cmp(m_next) > 0))
	m_next = host;
      ZdbDEBUG(this, ZtString() <<
	  "  host" << host << '\n' <<
	  "  next" << m_next);
    }
  }
  m_recovering = false;
  m_nextCxn = 0;
  if (m_next) startReplication();
}

void ZdbEnv::startReplication()
{
  ZeLOG(Info, ZtString() <<
	"Zdb host " << m_next->id() << " is next in line");
  m_nextCxn = m_next->m_cxn;	// starts replication
  dbStateRefresh_();		// must be called after m_nextCxn assignment
  ZdbDEBUG(this, ZtString() << "startReplication()\n" <<
      "  self " << m_self << '\n' <<
      "  master " << m_master << '\n' <<
      "  prev " << m_prev << '\n' <<
      "  next " << m_next << '\n' <<
      "  recovering " << m_recovering << " replicating " << !!m_nextCxn);
  if (m_next->dbState().cmp(m_self->dbState()) < 0 && !m_recovering) {
    // ZeLOG(Info, "startReplication() initiating recovery");
    m_recovering = true;
    m_recover = m_next->dbState();
    m_recoverEnd = m_self->dbState();
    m_mx->run(m_mx->txThread(), ZmFn<>::Member<&ZdbEnv::recSend>::fn(this));
  }
}

void ZdbEnv::stopReplication()
{
  m_master = 0;
  m_prev = 0;
  m_next = 0;
  m_recovering = 0;
  m_nextCxn = 0;
  {
    auto i = m_hosts.readIterator();
    while (Zdb_Host *host = i.iterateKey()) host->voted(false);
  }
  m_self->voted(true);
  m_nPeers = 1;
}

void Zdb_Cxn::msgRead(ZiIOContext &io)
{
  io.init(ZiIOFn::Member<&Zdb_Cxn::msgRcvd>::fn(this),
      &m_readHdr, sizeof(Zdb_Msg_Hdr), 0);
}

void Zdb_Cxn::msgRcvd(ZiIOContext &io)
{
  if ((io.offset += io.length) < io.size) return;

  switch (m_readHdr.type) {
    case Zdb_Msg::HB:	hbRcvd(io); break;
    case Zdb_Msg::Rep:	repRcvd(io); break;
    case Zdb_Msg::Rec:	repRcvd(io); break;
    default:
      ZeLOG(Error, ZtString() <<
	  "Zdb received garbled message from host " <<
	  ZuBoxed(m_host ? (int)m_host->id() : -1));
      io.disconnect();
      return;
  }

  m_mx->add(ZmFn<>::Member<&Zdb_Cxn::hbTimeout>::fn(this),
      ZmTimeNow((int)m_env->config().heartbeatTimeout), &m_hbTimer);
}

void Zdb_Cxn::hbRcvd(ZiIOContext &io)
{
  const Zdb_Msg_HB &hb = m_readHdr.u.hb;
  unsigned dbCount = m_env->dbCount();

  if (dbCount != hb.dbCount) {
    ZeLOG(Fatal, ZtString() <<
	"Zdb inconsistent remote configuration detected (local dbCount " <<
	dbCount << " != host " << hb.hostID << " dbCount " << hb.dbCount <<
	')');
    io.disconnect();
    return;
  }

  if (!m_host) m_env->associate(this, hb.hostID);

  if (!m_host) {
    io.disconnect();
    return;
  }

  hbDataRead(io);
}

// read heartbeat data
void Zdb_Cxn::hbDataRead(ZiIOContext &io)
{
  const Zdb_Msg_HB &hb = m_readHdr.u.hb;

  m_readData.length(hb.dbCount * sizeof(ZdbRN));

  io.init(ZiIOFn::Member<&Zdb_Cxn::hbDataRcvd>::fn(this),
      m_readData.data(), m_readData.length(), 0);
}

// process received heartbeat (connection level)
void Zdb_Cxn::hbDataRcvd(ZiIOContext &io)
{
  if ((io.offset += io.length) < io.size) return;

  m_env->hbDataRcvd(m_host, m_readHdr.u.hb, (ZdbRN *)m_readData.data());

  msgRead(io);
}

// process received heartbeat
void ZdbEnv::hbDataRcvd(Zdb_Host *host, const Zdb_Msg_HB &hb, ZdbRN *dbState)
{
  Guard guard(m_lock);

  ZdbDEBUG(this, ZtString() << "hbDataRcvd()\n" << 
	"  host " << host << '\n' <<
	"  self " << m_self << '\n' <<
	"  master " << m_master << '\n' <<
	"  prev " << m_prev << '\n' <<
	"  next " << m_next << '\n' <<
	"  recovering " << m_recovering <<
	"  replicating " << !!m_nextCxn);

  host->state(hb.state);
  memcpy((void *)(host->dbState().data()), dbState,
      m_dbs.length() * sizeof(ZdbRN));

  int state = this->state();

  switch (state) {
    case Zdb_Host::Electing:
      if (!host->voted()) {
	host->voted(true);
	if (--m_nPeers <= 0) {
	  guard.unlock();
	  m_mx->add(ZmFn<>::Member<&ZdbEnv::holdElection>::fn(this));
	}
      }
      return;
    case Zdb_Host::Activating:
    case Zdb_Host::Active:
    case Zdb_Host::Deactivating:
    case Zdb_Host::Inactive:
      break;
    default:
      return;
  }

  // check for duplicate master (dual active)
  switch (state) {
    case Zdb_Host::Activating:
    case Zdb_Host::Active:
      switch (host->state()) {
	case Zdb_Host::Activating:
	case Zdb_Host::Active:
	  vote(host);
	  if (host->cmp(m_self) > 0)
	    m_mx->add(ZmFn<>::Member<&ZdbEnv::deactivate>::fn(this));
	  else
	    m_mx->add(ZmFn<>::Member<&Zdb_Host::reactivate>::fn(ZmMkRef(host)));
	  return;
      }
  }

  // check for new host joining after election
  if (!host->voted()) {
    ++m_nPeers;
    vote(host);
  }
}

// check if new host should be our next in line
void ZdbEnv::vote(Zdb_Host *host)
{
  host->voted(true);
  dbStateRefresh_();
  if (host != m_next && host != m_prev && host->cmp(m_self) < 0 &&
      (!m_next || host->cmp(m_next) > 0))
    setNext(host);
}

// send recovery message to next-in-line (continues repeatedly until completed)
void ZdbEnv::recSend()
{
  Guard guard(m_lock);
  if (!m_self) {
    ZeLOG(Fatal, "ZdbEnv::recSend called out of order");
    return;
  }
  if (!m_recovering) return;
  ZmRef<Zdb_Cxn> cxn = m_nextCxn;
  if (!cxn) return;
  unsigned i, n = m_dbs.length();
  if (n != m_recover.length() || n != m_recoverEnd.length()) {
    ZeLOG(Fatal, ZtString() <<
	"ZdbEnv::recSend encountered inconsistent dbCount (local dbCount " <<
	n << " != one of " <<
	m_recover.length() << ", "  << m_recoverEnd.length() << ')');
    return;
  }
  for (i = 0; i < n; i++)
    if (Zdb_ *db = m_dbs[i])
      if (m_recover[i] < m_recoverEnd[i]) {
	ZmRef<ZdbAnyPOD> pod;
	ZdbRN rn = m_recover[i]++;
	if (pod = db->get__(rn)) {
	  if (pod->committed())
	    cxn->repSend(
		ZuMv(pod), Zdb_Msg::Rec, ZdbRange(0, db->dataSize()),
		ZdbOp::New, db->config().compress);
	  else
	    cxn->repSend(
		ZuMv(pod), Zdb_Msg::Rec, ZdbRange(), ZdbOp::Delete, false);
	} else {
	  db->alloc(pod);
	  pod->init(rn, ZdbDeleted);
	  cxn->repSend(
	      ZuMv(pod), Zdb_Msg::Rec, ZdbRange(), ZdbOp::Delete, false);
	}
	return;
      }
  m_recovering = 0;
}

// send replication message to next-in-line
void ZdbEnv::repSend(
    ZdbAnyPOD *pod, int type, ZdbRange range, int op, bool compress)
{
  if (ZmRef<Zdb_Cxn> cxn = m_nextCxn)
    cxn->repSend(pod, type, range, op, compress);
}

// send replication message (directed)
void Zdb_Cxn::repSend(
    ZdbAnyPOD *pod, int type, ZdbRange range, int op, bool compress)
{
  ZmRef<ZdbAnyPOD_Send> send =
    new ZdbAnyPOD_Send(pod, type, range, op, compress);
  send->send(this);
}

// prepare replication data for sending
void ZdbAnyPOD_Send__::init(int type, ZdbRange range, int op, bool compress)
{
  ZdbDEBUG(m_pod->db()->env(), ZtString() << "ZdbAnyPOD_Send__::init(" <<
      type << ", " << range << ", " << ZdbOp::name(op) << ", " <<
      (int)compress << ')');
  m_hdr.type = type;
  Zdb_Msg_Rep &rep = m_hdr.u.rep;
  rep.db = m_pod->db()->id();
  rep.rn = m_pod->rn();
  rep.prevRN = m_pod->prevRN();
  rep.range = range;
  rep.op = op;

  if (compress && range) {
    m_compressed = m_pod->compress();
    if (ZuUnlikely(!m_compressed)) goto uncompressed;
    int n = m_compressed->compress(
	(const char *)m_pod->ptr() + range.off(), range.len());
    if (ZuUnlikely(n < 0)) goto uncompressed;
    rep.clen = n;
    ZiVec_ptr(m_vecs[0]) = (ZiVecPtr)&m_hdr;
    ZiVec_len(m_vecs[0]) = sizeof(Zdb_Msg_Hdr);
    ZiVec_ptr(m_vecs[1]) = (ZiVecPtr)m_compressed->ptr();
    ZiVec_len(m_vecs[1]) = n;
    return;
  }

uncompressed:
  m_compressed = 0;
  rep.clen = 0;
  ZiVec_ptr(m_vecs[0]) = (ZiVecPtr)&m_hdr;
  ZiVec_len(m_vecs[0]) = sizeof(Zdb_Msg_Hdr);
  ZiVec_ptr(m_vecs[1]) = (ZiVecPtr)((const char *)m_pod->ptr() + range.off());
  ZiVec_len(m_vecs[1]) = range.len();
}

// send replication message
void ZdbAnyPOD_Send__::send(Zdb_Cxn *cxn)
{
  cxn->send(ZiIOFn::Member<&ZdbAnyPOD_Send__::send_>::fn(ZmMkRef(this)));
}
void ZdbAnyPOD_Send__::send_(ZiIOContext &io)
{
  m_vec = 0;
  io.init(ZiIOFn::Member<&ZdbAnyPOD_Send__::sent>::fn(
	io.fn.mvObject<ZdbAnyPOD_Send__>()),
      ZiVec_ptr(m_vecs[0]), ZiVec_len(m_vecs[0]), 0);
}
void ZdbAnyPOD_Send__::sent(ZiIOContext &io)
{
  if ((io.offset += io.length) < io.size) return;
  unsigned i;
  if ((i = ++m_vec) <= 1 && ZiVec_len(m_vecs[i])) {
    io.init(ZiIOFn::Member<&ZdbAnyPOD_Send__::sent>::fn(
	  io.fn.mvObject<ZdbAnyPOD_Send__>()),
	ZiVec_ptr(m_vecs[i]), ZiVec_len(m_vecs[i]), 0);
    return;
  }
  if (ZuUnlikely(m_hdr.type == Zdb_Msg::Rec)) {
    ZiMultiplex *mx = io.cxn->mx();
    ZdbEnv *env = m_pod->db()->env();
    mx->run(mx->txThread(), ZmFn<>::Member<&ZdbEnv::recSend>::fn(env));
  }
  io.complete();
}

int ZdbAnyPOD_Compressed::compress(const char *src, unsigned size)
{
  return LZ4_compress((const char *)src, (char *)m_ptr, size);
}

// broadcast heartbeat
void ZdbEnv::hbSend()
{
  Guard guard(m_lock);
  hbSend_(0);
  m_mx->add(ZmFn<>::Member<&ZdbEnv::hbSend>::fn(this),
    m_hbSendTime += (time_t)m_config.heartbeatFreq,
    ZmScheduler::Defer, &m_hbSendTimer);
}

// send heartbeat (either directed, or broadcast if cxn_ is 0)
void ZdbEnv::hbSend_(Zdb_Cxn *cxn_)
{
  if (!m_self) {
    ZeLOG(Fatal, "ZdbEnv::hbSend_ called out of order");
    return;
  }
  dbStateRefresh_();
  if (cxn_) {
    cxn_->hbSend();
    return;
  }
  ZmRef<Zdb_Cxn> cxn;
  unsigned i = 0, n = m_cxns->count();
  ZtArray<ZmRef<Zdb_Cxn> > cxns(n);
  {
    CxnHash::ReadIterator j(*m_cxns);
    while (i < n && (cxn = j.iterateKey())) if (cxn->up()) ++i, cxns.push(cxn);
  }
  for (unsigned j = 0; j < i; j++) {
    cxns[j]->hbSend();
    cxns[j] = 0;
  }
}

// send heartbeat on a specific connection
void Zdb_Cxn::hbSend()
{
  this->ZiConnection::send(ZiIOFn::Member<&Zdb_Cxn::hbSend_>::fn(this));
}
void Zdb_Cxn::hbSend_(ZiIOContext &io)
{
  Zdb_Host *self = m_env->self();
  if (ZuUnlikely(!self)) {
    ZeLOG(Fatal, "Zdb_Cxn::hbSend called out of order");
    io.complete();
    return;
  }
  m_hbSendHdr.type = Zdb_Msg::HB;
  Zdb_Msg_HB &hb = m_hbSendHdr.u.hb;
  hb.hostID = self->id();
  hb.state = m_env->state();
  hb.dbCount = self->dbState().length();
  m_hbSendVec = 0;
  ZiVec_ptr(m_hbSendVecs[0]) = (ZiVecPtr)&m_hbSendHdr;
  ZiVec_len(m_hbSendVecs[0]) = sizeof(Zdb_Msg_Hdr);
  ZiVec_ptr(m_hbSendVecs[1]) = (ZiVecPtr)self->dbState().data();
  ZiVec_len(m_hbSendVecs[1]) = self->dbState().length() * sizeof(ZdbRN);
  io.init(ZiIOFn::Member<&Zdb_Cxn::hbSent>::fn(this),
      ZiVec_ptr(m_hbSendVecs[0]), ZiVec_len(m_hbSendVecs[0]), 0);
  ZdbDEBUG(m_env, ZtString() << "hbSend()\n" <<
	"  self[ID:" << hb.hostID << " S:" << hb.state <<
	" N:" << hb.dbCount << "] " << self->dbState());
}
void Zdb_Cxn::hbSent(ZiIOContext &io)
{
  if ((io.offset += io.length) < io.size) return;
  unsigned i;
  if ((i = ++m_hbSendVec) <= 1) {
    io.init(ZiIOFn::Member<&Zdb_Cxn::hbSent>::fn(this),
	ZiVec_ptr(m_hbSendVecs[i]), ZiVec_len(m_hbSendVecs[i]), 0);
    return;
  }
  io.complete();
}

// refresh db state vector (locked)
void ZdbEnv::dbStateRefresh()
{
  Guard guard(m_lock);
  if (!m_self) {
    ZeLOG(Fatal, "ZdbEnv::dbStateRefresh called out of order");
    return;
  }
  dbStateRefresh_();
}

// refresh db state vector (unlocked)
void ZdbEnv::dbStateRefresh_()
{
  if (!m_self) {
    ZeLOG(Fatal, "ZdbEnv::dbStateRefresh_ called out of order");
    return;
  }
  Zdb_DBState &dbState = m_self->dbState();
  unsigned i, n = m_dbs.length();
  for (i = 0; i < n; i++) {
    Zdb_ *db = m_dbs[i];
    dbState[i] = db ? db->allocRN() : 0;
  }
}

// process received replication header
void Zdb_Cxn::repRcvd(ZiIOContext &io)
{
  if (!m_host) {
    ZeLOG(Fatal, "Zdb received replication message before heartbeat");
    io.disconnect();
    return;
  }

  const Zdb_Msg_Rep &rep = m_readHdr.u.rep;
  Zdb_ *db = m_env->db(rep.db);

  if (!db) {
    ZeLOG(Fatal, ZtString() <<
	"Zdb unknown remote DBID " << rep.db << " received");
    io.disconnect();
    return;
  }

  repDataRead(io);
}

// read replication data
void Zdb_Cxn::repDataRead(ZiIOContext &io)
{
  const Zdb_Msg_Rep &rep = m_readHdr.u.rep;
  Zdb_ *db = m_env->db(rep.db);
  if (ZuUnlikely(!db)) {
    ZeLOG(Fatal, "Zdb_Cxn::repDataRead internal error");
    return;
  }
  ZdbRange range(rep.range);
  if (!range) {
    m_env->repDataRcvd(m_host, this, m_readHdr.type, rep, nullptr);
    msgRead(io);
  } else {
    m_readData2.length(rep.clen ? (unsigned)rep.clen : (unsigned)range.len());
    io.init(ZiIOFn::Member<&Zdb_Cxn::repDataRcvd>::fn(this),
	m_readData2.data(), m_readData2.length(), 0);
  }
}

// pre-process received replication data, decompress as needed
void Zdb_Cxn::repDataRcvd(ZiIOContext &io)
{
  if (!m_host || m_host->cxn().ptr() != this) { io.disconnect(); return; }

  Zdb_Msg_Rep &rep = m_readHdr.u.rep;

  if (rep.clen) {
    Zdb_ *db = m_env->db(rep.db);
    m_readData.length(db->recSize());
    int n = LZ4_uncompress(
	m_readData2.data(), m_readData.data(), db->recSize());
    if (ZuUnlikely(n < 0)) {
      ZeLOG(Fatal, ZtHexDump(ZtString() << 
	    "decompress failed with rcode " << n << " (RN: " << rep.rn <<
	    ") RecSize: " << db->recSize() << " CLen " << rep.clen <<
	    "Data:\n", m_readData.data(), db->recSize()));
      msgRead(io);
      return;
    }
    m_env->repDataRcvd(m_host, this,
	m_readHdr.type, rep, (void *)m_readData.data());
  } else {
    m_env->repDataRcvd(m_host, this,
	m_readHdr.type, rep, (void *)m_readData2.data());
  }
  msgRead(io);
}

// process received replication data
void ZdbEnv::repDataRcvd(Zdb_Host *host,
    Zdb_Cxn *cxn, int type, const Zdb_Msg_Rep &rep, void *ptr)
{
  ZdbRange range(rep.range);
  ZdbDEBUG(this, ZtHexDump(ZtString() << "DBID:" << rep.db <<
	" RN:" << rep.rn << " R:" << range << " FROM:" << host,
	ptr, range.len()));
  Zdb_ *db = this->db(rep.db);
  if (ZuUnlikely(!db)) {
    ZeLOG(Error, ZtString() <<
	  "Zdb bad incoming replication data from host " << host->id() <<
	  " - unknown DBID " << rep.db);
    return;
  }
  {
    Guard guard(m_lock);
    Zdb_DBState &dbState = host->dbState();
    if (ZuUnlikely(rep.db >= (ZdbID)dbState.length())) {
      ZeLOG(Fatal, ZtString() <<
	  "ZdbEnv::repDataRcvd encountered inconsistent DBID "
	  "(ID " << rep.db << " >= " << dbState.length() << ')');
      return;
    }
    if ((active() || host == m_next) && rep.rn < dbState[rep.db]) return;
    if (rep.rn >= dbState[rep.db]) dbState[rep.db] = rep.rn + 1;
    if (!m_prev) {
      m_prev = host;
      ZeLOG(Info, ZtString() <<
	  "Zdb host " << m_prev->id() << " is previous in line");
    }
  }
  ZmRef<ZdbAnyPOD> pod = db->replicated(rep.rn, rep.prevRN, ptr, range, rep.op);
  if (pod) repSend(pod, Zdb_Msg::Rep, range, rep.op, db->config().compress);
}

// process replicated record
ZmRef<ZdbAnyPOD> Zdb_::replicated(
    ZdbRN rn, ZdbRN prevRN, void *ptr, ZdbRange range, int op)
{
  ZmRef<ZdbAnyPOD> pod = replicated_(rn, prevRN, op);
  if (!pod) return 0;
  replicate(pod, ptr, range, op);
  m_env->write(pod, range, op);
  return pod;
}

ZmRef<ZdbAnyPOD> Zdb_::replicated_(ZdbRN rn, ZdbRN prevRN, int op)
{
  ZmRef<ZdbAnyPOD> pod;
  Guard guard(m_lock);
  if (ZuUnlikely(pod = m_cache->del(prevRN))) {
    m_lru.del(pod);
    pod->update(rn, prevRN);
    if (op != ZdbOp::Delete) {
      pod->commit();
      cache_(pod);
    } else
      pod->del();
  } else {
    if (prevRN < m_allocRN) {
      Zdb_FileRec rec = rn2file(prevRN, false);
      if (rec) pod = read_(rec);
    } else
      alloc(pod);
    if (ZuUnlikely(!pod)) return nullptr;
    if (rn >= m_allocRN) m_allocRN = rn + 1;
    pod->update(rn, prevRN);
    if (op != ZdbOp::Delete) {
      pod->commit();
      cache(pod);
    } else
      pod->del();
  }
  return pod;
}

Zdb_::Zdb_(ZdbEnv *env, ZdbID id, ZdbHandler handler,
    unsigned recSize, unsigned dataSize) :
  m_env(env), m_id(id), m_handler(ZuMv(handler)),
  m_recSize(recSize), m_dataSize(dataSize)
{
  if (!m_recSize || !m_dataSize) {
    ZeLOG(Fatal, ZtString() <<
	"Zdb misconfiguration for DBID " << m_id << " - record/data size is 0");
    m_fileRecs = 0;
    return;
  }
  m_env->add(this);
  if (!m_config) {
    ZeLOG(Fatal, ZtString() <<
	"Zdb misconfiguration for DBID " << m_id << " - ZdbEnv::add() failed");
    m_fileRecs = 0;
    return;
  }
  if (m_config->fileSize < ((uint64_t)m_recSize<<3)) {
    ZeLOG(Warning, ZtString() <<
	"Zdb misconfiguration for DBID " << m_id <<
	" - file size " << m_config->fileSize <<
	" < 8x record size " << recSize);
    m_config->fileSize = (uint64_t)m_recSize<<3;
    m_fileRecs = 8;
  } else
    m_fileRecs = (unsigned)(m_config->fileSize / (uint64_t)m_recSize);
}

Zdb_::~Zdb_()
{
  close();
}

void Zdb_::init(ZdbConfig *config)
{
  m_config = config;
  m_cache = new Zdb_Cache(m_config->cache);
  {
    ZmHashStats s;
    m_cache->report(s);
    m_cacheSize = ((double)(((uint64_t)1)<<s.bits) * s.loadFactor);
  }
  m_files = new FileHash(m_config->fileHash);
  {
    ZmHashStats s;
    m_files->report(s);
    m_filesMax = ((double)(((uint64_t)1)<<s.bits) * s.loadFactor);
  }
}

void Zdb_::final()
{
  m_handler = ZdbHandler{};
}

void Zdb_::recover()
{
  ZeError e;
  ZiDir::Path subName;
  ZmBitmap subDirs;
  {
    ZiDir dir;
    if (dir.open(m_config->path) != Zi::OK) {
      ZeError e;
      if (ZiFile::mkdir(m_config->path, &e) != Zi::OK) {
	ZeLOG(Fatal, ZtString() << "could not create directory " <<
	    m_config->path);
      }
      return;
    }
    while (dir.read(subName) == Zi::OK) {
      const auto &r = ZtStaticRegexUTF8("^[0-9a-f]{5}$");
      if (!r.m(subName)) continue;
      ZuBox<unsigned> subIndex;
      subIndex.scan(ZuFmt::Hex<>(), subName);
      subDirs << (int)subIndex;
    }
    dir.close();
  }
  for (int i = subDirs.first(); i >= 0; i = subDirs.next(i)) {
    subName = ZuBox<unsigned>(i).hex(ZuFmt::Right<5>());
    subName = ZiFile::append(m_config->path, subName);
    ZiDir::Path fileName;
    ZmBitmap files;
    {
      ZiDir subDir;
      if (subDir.open(subName, &e) != Zi::OK) {
	ZeLOG(Error, ZtString() << subName << ": " << e);
	continue;
      }
      while (subDir.read(fileName) == Zi::OK) {
	const auto &r = ZtStaticRegexUTF8("^[0-9a-f]{5}\\.zdb$");
	if (!r.m(fileName)) continue;
	ZuBox<unsigned> fileIndex;
	fileIndex.scan(ZuFmt::Hex<>(), fileName);
	files << (int)fileIndex;
      }
      subDir.close();
    }
    for (int j = files.first(); j >= 0; j = files.next(j)) {
      fileName = ZuBox<unsigned>(j).hex(ZuFmt::Right<5>());
      fileName << ".zdb";
      unsigned index = (((unsigned)i)<<20U) | ((unsigned)j);
      fileName = ZiFile::append(subName, fileName);
      ZmRef<Zdb_File> file = new Zdb_File(index, m_fileRecs);
      if (file->open(
	    fileName, ZiFile::GC, 0666, m_config->fileSize, &e) != Zi::OK) {
	ZeLOG(Error, ZtString() << fileName << ": " << e);
	continue;
      }
      recover(file);
    }
  }
}

void Zdb_::recover(Zdb_File *file)
{
  ZdbRN rn = file->index() * m_fileRecs;
  for (unsigned j = 0; j < m_fileRecs; j++, rn++) {
    ZmRef<ZdbAnyPOD> pod = read_(Zdb_FileRec(file, j * m_recSize));
    if (!pod || !pod->magic()) return;
    if (rn != pod->rn()) {
      ZeLOG(Error, ZtString() <<
	  "Zdb recovered corrupt record from \"" <<
	  fileName(file->index()) <<
	  "\" at offset " << (j * m_recSize) << ' ' <<
	  ZuBoxed(rn) << " != " << ZuBoxed(pod->rn()));
      continue;
    }
    if (m_allocRN <= rn) m_allocRN = rn + 1;
    if (m_fileRN <= rn) m_fileRN = rn + 1;
    switch (pod->magic()) {
      case ZdbCommitted:
	cache(pod);
	this->recover(pod);
	file->alloc();
	break;
      case ZdbDeleted:
	file->skip();
	break;
      default:
	return;
    }
  }
}

void Zdb_::open()
{
  if (ZuUnlikely(!m_fileRecs)) return;

  recover();

  unsigned n = m_config->preAlloc;
  if (!n) return;
  unsigned i = m_allocRN / (ZdbRN)m_fileRecs;
  for (;;) {
    ZmRef<Zdb_File> file = getFile(i, true);
    if (!file) return;
    unsigned j = file->unallocated();
    if (n <= j) return;
    n -= j;
    ++i;
  }
}

void Zdb_::close()
{
  FSGuard guard(m_fsLock);
  m_files->clean();
}

void Zdb_::checkpoint()
{
  m_env->mx()->run(
      m_env->config().writeThread,
      ZmFn<>::Member<&Zdb_::checkpoint_>::fn(ZmMkRef(this)));
}

void Zdb_::checkpoint_()
{
  FSGuard guard(m_fsLock);
  auto i = m_files->readIterator();
  while (Zdb_File *file = i.iterate())
    file->checkpoint();
}

ZmRef<ZdbAnyPOD> Zdb_::push()
{
  if (!m_fileRecs) return 0;
  if (!m_env->active()) {
    ZeLOG(Error, ZtString() <<
	"Zdb inactive application attempted push on DBID " << m_id);
    return 0;
  }
  ZmRef<ZdbAnyPOD> pod = push_();
  if (!pod) return 0;
  return pod;
}

ZmRef<ZdbAnyPOD> Zdb_::push_()
{
  ZmRef<ZdbAnyPOD> pod;
  alloc(pod);
  if (ZuUnlikely(!pod)) return 0;
  Guard guard(m_lock);
  pod->init(m_allocRN++);
  return pod;
}

ZmRef<ZdbAnyPOD> Zdb_::get(ZdbRN rn)
{
  ZmRef<ZdbAnyPOD> pod;
  Guard guard(m_lock);
  if (ZuUnlikely(!m_fileRecs)) return nullptr;
  if (ZuUnlikely(rn >= m_allocRN)) return nullptr;
  if (ZuLikely(pod = m_cache->find(rn))) {
    m_lru.del(pod);
    m_lru.push(pod);
    return pod;
  }
  {
    Zdb_FileRec rec = rn2file(rn, false);
    if (rec) pod = read_(rec);
  }
  if (ZuUnlikely(!pod || !pod->committed())) return nullptr;
  cache(pod);
  return pod;
}

ZmRef<ZdbAnyPOD> Zdb_::get_(ZdbRN rn)
{
  Guard guard(m_lock);
  if (ZuUnlikely(!m_fileRecs)) return nullptr;
  if (ZuUnlikely(rn >= m_allocRN)) return nullptr;
  ZmRef<ZdbAnyPOD> pod = get__(rn);
  if (ZuUnlikely(!pod || !pod->committed())) return nullptr;
  return pod;
}

ZmRef<ZdbAnyPOD> Zdb_::get__(ZdbRN rn)
{
  ZmRef<ZdbAnyPOD> pod;
  if (ZuLikely(pod = m_cache->find(rn))) return pod;
  {
    Zdb_FileRec rec = rn2file(rn, false);
    if (rec) pod = read_(rec);
  }
  return pod;
}

void Zdb_::cache(ZdbAnyPOD *pod)
{
  if (m_cache->count() >= m_cacheSize) {
    ZmRef<ZdbLRUNode> lru_ = m_lru.shiftNode();
    if (ZuLikely(lru_)) {
      ZdbAnyPOD *lru = static_cast<ZdbAnyPOD *>(lru_.ptr());
      if (ZuLikely(!lru->m_writeCount))
	m_cache->del(lru->rn());
      else {
	m_lru.unshift(lru_);
	m_cacheSize = m_cache->count() + 1;
      }
    }
  }
  cache_(pod);
}

void Zdb_::cache_(ZdbAnyPOD *pod)
{
  m_cache->add(pod);
  m_lru.push(pod);
}

void Zdb_::cacheDel_(ZdbAnyPOD *pod)
{
  m_cache->del(pod->rn());
  m_lru.del(pod);
}

void Zdb_::put(ZdbAnyPOD *pod, bool copy)
{
  if (ZuUnlikely(!m_fileRecs)) return;
  if (ZuUnlikely(!m_env->active())) {
    ZeLOG(Error, ZtString() <<
	"Zdb inactive application attempted put on DBID " << m_id);
    return;
  }
  pod->commit();
  {
    Guard guard(m_lock);
    cache(pod);
    ++pod->m_writeCount;
  }
  ZdbRange range(0, m_dataSize);
  if (copy) this->copy(pod, range, ZdbOp::New);
  m_env->write(pod, range, ZdbOp::New);
}

void Zdb_::update(ZdbAnyPOD *pod, ZdbRange range, bool copy)
{
  if (ZuUnlikely(!m_fileRecs)) return;
  if (ZuUnlikely(!m_env->active())) {
    ZeLOG(Error, ZtString() <<
	"Zdb inactive application attempted update on DBID " << m_id);
    return;
  }
  if (!range) range.init(0, m_dataSize);
  {
    Guard guard(m_lock);
    cacheDel_(pod);
    pod->update(m_allocRN++, pod->rn());
    cache_(pod);
    ++pod->m_writeCount;
  }
  if (copy) this->copy(pod, range, ZdbOp::Update);
  m_env->write(pod, range, ZdbOp::Update);
}

void Zdb_::del(ZdbAnyPOD *pod, bool copy)
{
  if (ZuUnlikely(!m_fileRecs)) return;
  if (ZuUnlikely(!m_env->active())) {
    ZeLOG(Error, ZtString() <<
	"Zdb inactive application attempted del on DBID " << m_id);
    return;
  }
  bool abort = !pod->committed();
  pod->del();
  {
    Guard guard(m_lock);
    if (!abort) {
      cacheDel_(pod);
      pod->update(m_allocRN++, pod->rn());
    }
    ++pod->m_writeCount;
  }
  if (copy) this->copy(pod, ZdbRange(), ZdbOp::Delete);
  m_env->write(pod, ZdbRange(), ZdbOp::Delete);
}

void ZdbEnv::write(ZdbAnyPOD *pod, ZdbRange range, int op)
{
  ZmRef<ZdbAnyPOD_Write> write = new ZdbAnyPOD_Write(pod, range, op);
  m_mx->run(m_config.writeThread,
      ZmFn<>::Member<&ZdbAnyPOD_Write__::write>::fn(write));
}

void ZdbAnyPOD_Write__::write()
{
  m_pod->db()->write(m_pod, m_range, m_op);
}

void Zdb_::write(ZdbAnyPOD *pod, ZdbRange range, int op)
{
  if (m_config->replicate)
    m_env->repSend(pod, Zdb_Msg::Rep, range, op, pod->db()->config().compress);
  write_(pod->rn(), pod->prevRN(), pod->ptr(), range, op);
  {
    Guard guard(m_lock);
    --pod->m_writeCount;
  }
}

Zdb_FileRec Zdb_::rn2file(ZdbRN rn, bool write)
{
  unsigned index = rn / (ZdbRN)m_fileRecs;
  ZmRef<Zdb_File> file = getFile(index, write);
  if (!file) return Zdb_FileRec();
  ZiFile::Offset offset = (rn % m_fileRecs) * m_recSize;
  return Zdb_FileRec(ZuMv(file), offset);
}

ZmRef<Zdb_File> Zdb_::getFile(unsigned index, bool create)
{
  FSGuard guard(m_fsLock);
  ZmRef<Zdb_File> file;
  if (file = m_files->find(index)) {
    m_filesLRU.del(file);
    m_filesLRU.push(file);
    return file;
  }
  file = openFile(index, create);
  if (ZuUnlikely(!file)) return nullptr;
  if (m_files->count() >= m_filesMax)
    if (ZmRef<Zdb_File> lru = m_filesLRU.shiftNode())
      m_files->del(lru->index());
  m_files->add(file);
  return file;
}

ZmRef<Zdb_File> Zdb_::openFile(unsigned index, bool create)
{
  ZiFile::Path name = dirName(index);
  if (create) ZiFile::mkdir(name); // pre-emptive idempotent
  name = fileName(name, index);
  ZmRef<Zdb_File> file = new Zdb_File(index, m_fileRecs);
  if (file->open(name, ZiFile::GC, 0666, m_config->fileSize, 0) == Zi::OK)
    return file;
  if (!create) return nullptr;
  ZeError e;
  if (file->open(name, ZiFile::Create | ZiFile::GC,
	0666, m_config->fileSize, &e) != Zi::OK) {
    ZeLOG(Fatal, ZtString() <<
	"Zdb could not open or create \"" << name << "\": " << e);
    return nullptr; 
  }
  return file;
}

void Zdb_::delFile(Zdb_File *file)
{
  {
    FSGuard guard(m_fsLock);
    m_filesLRU.del(file);
    m_files->del(file->index());
  }
  file->close();
  ZiFile::remove(fileName(file->index()));
}

ZmRef<ZdbAnyPOD> Zdb_::read_(const Zdb_FileRec &rec)
{
  ZmRef<ZdbAnyPOD> pod;
  alloc(pod);
  int r;
  ZeError e;
  if (ZuUnlikely((r = rec.file()->pread(
	  rec.off(), pod->ptr(), m_recSize)) < (int)m_recSize)) {
    if (r < 0) {
      ZeLOG(Error, ZtString() <<
	  "Zdb pread() failed on \"" << fileName(rec.file()->index()) <<
	  "\" at offset " << rec.off() <<  ": " << e);
    } else {
      ZeLOG(Error, ZtString() <<
	  "Zdb pread() truncated on \"" << fileName(rec.file()->index()) <<
	  "\" at offset " << rec.off());
    }
    return 0;
  }
  return pod;
}

void Zdb_::write_(
    ZdbRN rn, ZdbRN prevRN, const void *ptr, ZdbRange range, int op)
{
  int r;
  ZeError e;
  Zdb_FileRec rec;
  unsigned trailerOffset = m_recSize - sizeof(ZdbTrailer);

  {
    ZdbRN gap = m_fileRN;
    if (m_fileRN <= rn) m_fileRN = rn + 1;
    while (gap < rn) {
      rec = rn2file(gap, true);
      if (!rec) return; // error is logged by getFile/openFile
      ZdbTrailer trailer{gap, gap, ZdbDeleted};
      if (ZuUnlikely((r = rec.file()->pwrite(rec.off() + trailerOffset,
	  &trailer, sizeof(ZdbTrailer), &e)) != Zi::OK))
	fileError_(rec, e);
      ++gap;
    }
  }

  if (!(rec = rn2file(rn, true))) return; // error is logged by getFile/openFile

  if (range.off() + range.len() >= m_dataSize) {
    if (ZuUnlikely((r = rec.file()->pwrite(rec.off() + range.off(),
	    (const char *)ptr + range.off(),
	    m_recSize - range.off(), &e)) != Zi::OK))
      fileError_(rec, e);
  } else {
    if (range) {
      if (ZuUnlikely((r = rec.file()->pwrite(rec.off() + range.off(),
		(const char *)ptr + range.off(), range.len(), &e)) != Zi::OK))
	fileError_(rec, e);
    }
    if (ZuUnlikely((r = rec.file()->pwrite(
	      rec.off() + trailerOffset,
	      (const char *)ptr + trailerOffset,
	      sizeof(ZdbTrailer), &e)) != Zi::OK))
      fileError_(rec, e);
  }

  if (op == ZdbOp::New)
    rec.file()->alloc();
  else if (op == ZdbOp::Delete && rec.file()->del() == m_fileRecs)
    delFile(rec.file());

  if (prevRN == rn) return;

  if (!(rec = rn2file(prevRN, false))) return;

  {
    uint32_t magic = ZdbDeleted;
    if ((r = rec.file()->pwrite(
	    rec.off() + trailerOffset + offsetof(ZdbTrailer, magic),
	    &magic, 4, &e)) != Zi::OK)
      fileError_(rec, e);
  }
  if (rec.file()->del() == m_fileRecs)
    delFile(rec.file());
}

void Zdb_::fileError_(const Zdb_FileRec &rec, ZeError e)
{
  ZeLOG(Error, ZtString() <<
      "Zdb pwrite() failed on \"" << fileName(rec.file()->index()) <<
      "\" at offset " << rec.off() <<  ": " << e);
}
