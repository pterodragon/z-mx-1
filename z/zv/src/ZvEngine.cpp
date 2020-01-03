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

// ZvEngine - Connectivity Framework

#include <zlib/ZvEngine.hpp>

void ZvEngine::start_()
{
  appAddEngine();

  // appException(ZeEVENT(Info, "START"));

  auto i = m_links.readIterator();
  while (ZmRef<ZvAnyLink> link = i.iterateKey())
    rxRun(ZmFn<>{link, [](ZvAnyLink *link) { link->up_(false); }});
}

void ZvEngine::stop_()
{
  // appException(ZeEVENT(Info, "STOP"));

  auto i = m_links.readIterator();
  while (ZmRef<ZvAnyLink> link = i.iterateKey())
    rxRun(ZmFn<>{link, [](ZvAnyLink *link) { link->down_(false); }});

  appDelEngine();
}

void ZvEngine::start()
{
  int prev, next;

  {
    StateGuard stateGuard(m_stateLock);

    bool start = false;

    prev = m_state.load_();
    switch (prev) {
      case EngineState::Stopped:
	m_state.store_(EngineState::Starting);
	start = true;
	break;
      case EngineState::Stopping:
	m_state.store_(EngineState::StartPending);
	break;
      default:
	break;
    }
    next = m_state.load_();

    if (start) start_();
  }

  if (next != prev) appUpdEngine()
}

void ZvEngine::stop()
{
  int prev, next;

  {
    StateGuard stateGuard(m_stateLock);

    bool stop = false;

    prev = m_state.load_();
    switch (prev) {
      case EngineState::Running:
	m_state.store_(EngineState::Stopping);
	stop = true;
	break;
      case EngineState::Starting:
	m_state.store_(EngineState::StopPending);
	break;
      default:
	break;
    }
    next = m_state.load_();

    if (stop) stop_();
  }

  if (next != prev) appUpdEngine();
}

void ZvEngine::linkState(ZvAnyLink *link, int prev, int next)
{
  if (ZuUnlikely(next == prev)) return;
  switch (prev) {
    case LinkState::Connecting:
    case LinkState::Disconnecting:
    case LinkState::ConnectPending:
    case LinkState::DisconnectPending:
      switch (next) {
	case LinkState::Connecting:
	case LinkState::Disconnecting:
	case LinkState::ConnectPending:
	case LinkState::DisconnectPending:
	  return;
      }
      break;
  }

#if 0
  appException(ZeEVENT(Info,
    ([id = link->id(), prev, next](const ZeEvent &, ZmStream &s) {
      s << "link " << id << ' ' <<
      LinkState::name(prev) << "->" << LinkState::name(next); })));
#endif
  appUpdLink(link);

  int enginePrev, engineNext;

  {
    StateGuard stateGuard(m_stateLock);

    switch (prev) {
      case LinkState::Down:
	--m_down;
	break;
      case LinkState::Disabled:
	--m_disabled;
	break;
      case LinkState::Connecting:
      case LinkState::Disconnecting:
      case LinkState::ConnectPending:
      case LinkState::DisconnectPending:
	--m_transient;
	break;
      case LinkState::Up:
	--m_up;
	break;
      case LinkState::ReconnectPending:
      case LinkState::Reconnecting:
	--m_reconn;
	break;
      case LinkState::Failed:
	--m_failed;
	break;
      default:
	break;
    }

    switch (next) {
      case LinkState::Down:
	++m_down;
	break;
      case LinkState::Disabled:
	++m_disabled;
	break;
      case LinkState::Connecting:
      case LinkState::Disconnecting:
      case LinkState::ConnectPending:
      case LinkState::DisconnectPending:
	++m_transient;
	break;
      case LinkState::Up:
	++m_up;
	break;
      case LinkState::ReconnectPending:
      case LinkState::Reconnecting:
	++m_reconn;
	break;
      case LinkState::Failed:
	++m_failed;
	break;
      default:
	break;
    }

    bool start = false, stop = false;

    enginePrev = m_state.load_();
    switch (enginePrev) {
      case EngineState::Starting:
	switch (prev) {
	  case LinkState::Down:
	  case LinkState::Connecting:
	  case LinkState::Disconnecting:
	  case LinkState::ConnectPending:
	  case LinkState::DisconnectPending:
	    if (!(m_down + m_transient))
	      m_state.store_(EngineState::Running);
	    break;
	}
	break;
      case EngineState::StopPending:
	switch (prev) {
	  case LinkState::Down:
	  case LinkState::Connecting:
	  case LinkState::Disconnecting:
	  case LinkState::ConnectPending:
	  case LinkState::DisconnectPending:
	    if (!(m_down + m_transient)) {
	      m_state.store_(EngineState::Stopping);
	      stop = true;
	    }
	    break;
	}
	break;
      case EngineState::Stopping:
	switch (prev) {
	  case LinkState::Connecting:
	  case LinkState::Disconnecting:
	  case LinkState::ConnectPending:
	  case LinkState::DisconnectPending:
	  case LinkState::Up:
	    if (!(m_up + m_transient))
	      m_state.store_(EngineState::Stopped);
	    break;
	}
	break;
      case EngineState::StartPending:
	switch (prev) {
	  case LinkState::Connecting:
	  case LinkState::Disconnecting:
	  case LinkState::ConnectPending:
	  case LinkState::DisconnectPending:
	  case LinkState::Up:
	    if (!(m_up + m_transient)) {
	      m_state.store_(EngineState::Starting);
	      start = true;
	    }
	    break;
	}
	break;
    }
    engineNext = m_state.load_();

    if (start) start_();
    if (stop) stop_();
  }

  if (engineNext != enginePrev) appUpdEngine();
}

void ZvEngine::final()
{
  m_links.clean();
  m_txPools.clean();
}

void ZvEngine::telemetry(Telemetry &data) const
{
  data.id = m_id;
  data.mxID = m_mx->params().id();
  {
    StateReadGuard guard(m_stateLock);
    data.down = m_down;
    data.disabled = m_disabled;
    data.transient = m_transient;
    data.up = m_up;
    data.reconn = m_reconn;
    data.failed = m_failed;
    data.state = m_state.load_();
  }
  {
    ReadGuard guard(m_lock);
    data.nLinks = m_links.count();
  }
  data.rxThread = m_rxThread;
  data.txThread = m_txThread;
}

// connection state management

ZvAnyTx::ZvAnyTx(ZvID id) : m_id(id)
{
}

void ZvAnyTx::init(ZvEngine *engine)
{
  m_engine = engine;
  m_mx = engine->mx();
}

ZvAnyLink::ZvAnyLink(ZvID id) :
  ZvAnyTx(id),
  m_state(LinkState::Down),
  m_reconnects(0)
{
}

void ZvAnyLink::up_(bool enable)
{
  int prev, next;
  bool connect = false;
  bool running = false;

  switch ((int)engine()->state()) {
    case EngineState::Starting:
    case EngineState::Running:
      running = true;
      break;
  }

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  {
    StateGuard stateGuard(m_stateLock);

    if (enable) m_enabled = true;

    // state machine
    prev = m_state.load_();
    switch (prev) {
      case LinkState::Disabled:
      case LinkState::Down:
      case LinkState::Failed:
	if (running) {
	  m_state.store_(LinkState::Connecting);
	  connect = true;
	} else
	  m_state.store_(LinkState::Down);
	break;
      case LinkState::Disconnecting:
	if (running && m_enabled)
	  m_state.store_(LinkState::ConnectPending);
	break;
      case LinkState::DisconnectPending:
	if (m_enabled)
	  m_state.store_(LinkState::Connecting);
	break;
      default:
	break;
    }
    next = m_state.load_();
  }

  if (next != prev) engine()->linkState(this, prev, next);
  if (connect)
    engine()->rxInvoke(this, [](ZvAnyLink *link) { link->connect(); });
}

void ZvAnyLink::down_(bool disable)
{
  int prev, next;
  bool disconnect = false;

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  {
    StateGuard stateGuard(m_stateLock);

    if (disable) m_enabled = false;

    // state machine
    prev = m_state.load_();
    switch (prev) {
      case LinkState::Down:
	if (!m_enabled) m_state.store_(LinkState::Disabled);
	break;
      case LinkState::Up:
      case LinkState::ReconnectPending:
      case LinkState::Reconnecting:
	m_state.store_(LinkState::Disconnecting);
	disconnect = true;
	break;
      case LinkState::Connecting:
	m_state.store_(LinkState::DisconnectPending);
	break;
      case LinkState::ConnectPending:
	m_state.store_(LinkState::Disconnecting);
	break;
      default:
	break;
    }
    next = m_state.load_();
  }

  if (next != prev) engine()->linkState(this, prev, next);
  if (disconnect)
    engine()->rxInvoke(this, [](ZvAnyLink *link) { link->disconnect(); });
}

void ZvAnyLink::connected()
{
  int prev, next;
  bool disconnect = false;

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  {
    StateGuard stateGuard(m_stateLock);

    // state machine
    prev = m_state.load_();
    switch (prev) {
      case LinkState::Connecting:
      case LinkState::ReconnectPending:
      case LinkState::Reconnecting:
	m_state.store_(LinkState::Up);
      case LinkState::Up:
	m_reconnects.store_(0);
	break;
      case LinkState::DisconnectPending:
	m_state.store_(LinkState::Disconnecting);
	disconnect = true;
	m_reconnects.store_(0);
	break;
      default:
	break;
    }
    next = m_state.load_();
  }

  if (next != prev) engine()->linkState(this, prev, next);
  if (disconnect) 
    engine()->rxRun(
	ZmFn<>{this, [](ZvAnyLink *link) { link->disconnect(); }});
}

void ZvAnyLink::disconnected()
{
  int prev, next;
  bool connect = false;

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  {
    StateGuard stateGuard(m_stateLock);

    // state machine
    prev = m_state.load_();
    switch (prev) {
      case LinkState::Connecting:
      case LinkState::DisconnectPending:
      case LinkState::ReconnectPending:
      case LinkState::Reconnecting:
      case LinkState::Up:
	m_state.store_(m_enabled ? LinkState::Failed : LinkState::Disabled);
	m_reconnects.store_(0);
	break;
      case LinkState::Disconnecting:
	m_state.store_(m_enabled ? LinkState::Down : LinkState::Disabled);
	m_reconnects.store_(0);
	break;
      case LinkState::ConnectPending:
	if (m_enabled) {
	  m_state.store_(LinkState::Connecting);
	  connect = true;
	} else
	  m_state.store_(LinkState::Disabled);
	m_reconnects.store_(0);
	break;
      default:
	break;
    }
    next = m_state.load_();
  }

  if (next != prev) engine()->linkState(this, prev, next);
  if (connect)
    engine()->rxRun(
	ZmFn<>{this, [](ZvAnyLink *link) { link->connect(); }});
}

void ZvAnyLink::reconnecting()
{
  int prev, next;

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  {
    StateGuard stateGuard(m_stateLock);

    // state machine
    prev = m_state.load_();
    switch (prev) {
      case LinkState::Up:
	m_state.store_(LinkState::Connecting);
	break;
      default:
	break;
    }
    next = m_state.load_();
  }

  if (next != prev) engine()->linkState(this, prev, next);
}

void ZvAnyLink::reconnect(bool immediate)
{
  int prev, next;
  bool reconnect = false, disconnect = false;
  ZmTime reconnTime;

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  {
    StateGuard stateGuard(m_stateLock);

    // state machine
    prev = m_state.load_();
    switch (prev) {
      case LinkState::Connecting:
      case LinkState::Reconnecting:
      case LinkState::Up:
	m_state.store_(LinkState::ReconnectPending);
	reconnect = true;
	break;
      case LinkState::DisconnectPending:
	disconnect = true;
	break;
      default:
	break;
    }
    next = m_state.load_();

    if (reconnect) {
      m_reconnects.store_(m_reconnects.load_() + 1);
      reconnTime.now(reconnInterval(m_reconnects.load_()));
    }
  }

  if (next != prev) engine()->linkState(this, prev, next);
  if (reconnect) {
    if (immediate)
      engine()->rxRun(
	  ZmFn<>{this, [](ZvAnyLink *link) { link->reconnect_(); }});
    else
      engine()->rxRun(
	  ZmFn<>{this, [](ZvAnyLink *link) { link->reconnect_(); }},
	  reconnTime, &m_reconnTimer);
  }
  if (disconnect) 
    engine()->rxRun(
	ZmFn<>{this, [](ZvAnyLink *link) { link->disconnect(); }});
}

void ZvAnyLink::reconnect_()
{
  int prev, next;
  bool connect = false;

  {
    StateGuard stateGuard(m_stateLock);

    // state machine
    prev = m_state.load_();
    switch (prev) {
      case LinkState::ReconnectPending:
	m_state.store_(LinkState::Reconnecting);
	connect = true;
	break;
      default:
	break;
    }
    next = m_state.load_();
  }

  if (next != prev) engine()->linkState(this, prev, next);
  if (connect)
    engine()->rxRun(
	ZmFn<>{this, [](ZvAnyLink *link) { link->connect(); }});
}

void ZvAnyLink::telemetry(Telemetry &data) const
{
  data.id = id();
  data.rxSeqNo = rxSeqNo();
  data.txSeqNo = txSeqNo();
  data.reconnects = m_reconnects.load_();
  data.state = m_state.load_();
}
