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

// MxEngine - Connectivity Framework

#include <MxEngine.hpp>

void MxEngine::start_()
{
  appAddEngine();

  appException(ZeEVENT(Info, "START"));

  up();
  auto i = m_links.readIterator();
  while (ZmRef<MxAnyLink> link = i.iterateKey())
    m_mx->add(ZmFn<>([](MxAnyLink *link) { link->up_(); }, link));
}

void MxEngine::stop_()
{
  appException(ZeEVENT(Info, "STOP"));

  auto i = m_links.readIterator();
  while (ZmRef<MxAnyLink> link = i.iterateKey())
    m_mx->add(ZmFn<>([](MxAnyLink *link) { link->down_(); }, link));
  down();

  appDelEngine();
}

void MxEngine::stopped_()
{
}

void MxEngine::start()
{
  int prev, next;

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    bool start = false;

    prev = m_state;
    switch (prev) {
      case MxEngineState::Stopped:
	m_state = MxEngineState::Starting;
	start = true;
	break;
      case MxEngineState::Stopping:
	m_state = MxEngineState::StartPending;
	break;
      default:
	break;
    }
    next = m_state;

    if (start) start_();
  }

  if (next != prev) appEngineState(prev, next);
}

void MxEngine::stop()
{
  int prev, next;

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    bool stop = false;

    prev = m_state;
    switch (prev) {
      case MxEngineState::Running:
	m_state = MxEngineState::Stopping;
	stop = true;
	break;
      case MxEngineState::Starting:
	m_state = MxEngineState::StopPending;
	break;
      default:
	break;
    }
    next = m_state;

    if (stop) stop_();
  }

  if (next != prev) appEngineState(prev, next);
}

void MxEngine::linkState(MxAnyLink *link, int prev, int next)
{
  if (ZuUnlikely(next == prev)) return;
  switch (prev) {
    case MxLinkState::Connecting:
    case MxLinkState::Disconnecting:
    case MxLinkState::ConnectPending:
    case MxLinkState::DisconnectPending:
      switch (next) {
	case MxLinkState::Connecting:
	case MxLinkState::Disconnecting:
	case MxLinkState::ConnectPending:
	case MxLinkState::DisconnectPending:
	  return;
      }
      break;
  }

#if 0
  appException(ZeEVENT(Info,
    ([id = link->id(), prev, next](const ZeEvent &, ZmStream &s) {
      s << "link " << id << ' ' <<
      MxLinkState::name(prev) << "->" << MxLinkState::name(next); })));
#endif
  appLinkState(link, prev, next);

  int enginePrev, engineNext;

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    switch (prev) {
      case MxLinkState::Down:
	--m_down;
	break;
      case MxLinkState::Disabled:
	--m_disabled;
	break;
      case MxLinkState::Connecting:
      case MxLinkState::Disconnecting:
      case MxLinkState::ConnectPending:
      case MxLinkState::DisconnectPending:
	--m_transient;
	break;
      case MxLinkState::Up:
	--m_up;
	break;
      case MxLinkState::Reconnecting:
	--m_reconn;
	break;
      case MxLinkState::Failed:
	--m_failed;
	break;
      default:
	break;
    }

    switch (next) {
      case MxLinkState::Down:
	++m_down;
	break;
      case MxLinkState::Disabled:
	++m_disabled;
	break;
      case MxLinkState::Connecting:
      case MxLinkState::Disconnecting:
      case MxLinkState::ConnectPending:
      case MxLinkState::DisconnectPending:
	++m_transient;
	break;
      case MxLinkState::Up:
	++m_up;
	break;
      case MxLinkState::Reconnecting:
	++m_reconn;
	break;
      case MxLinkState::Failed:
	++m_failed;
	break;
      default:
	break;
    }

    bool start = false, stop = false, stopped = false;

    enginePrev = m_state;
    switch (enginePrev) {
      case MxEngineState::Starting:
	switch (prev) {
	  case MxLinkState::Down:
	  case MxLinkState::Connecting:
	  case MxLinkState::Disconnecting:
	  case MxLinkState::ConnectPending:
	  case MxLinkState::DisconnectPending:
	    if (!(m_down + m_transient))
	      m_state = MxEngineState::Running;
	    break;
	}
	break;
      case MxEngineState::StopPending:
	switch (prev) {
	  case MxLinkState::Down:
	  case MxLinkState::Connecting:
	  case MxLinkState::Disconnecting:
	  case MxLinkState::ConnectPending:
	  case MxLinkState::DisconnectPending:
	    if (!(m_down + m_transient)) {
	      m_state = MxEngineState::Stopping;
	      stop = true;
	    }
	    break;
	}
	break;
      case MxEngineState::Stopping:
	switch (prev) {
	  case MxLinkState::Connecting:
	  case MxLinkState::Disconnecting:
	  case MxLinkState::ConnectPending:
	  case MxLinkState::DisconnectPending:
	  case MxLinkState::Up:
	    if (!(m_up + m_transient)) {
	      m_state = MxEngineState::Stopped;
	      stopped = true;
	    }
	    break;
	}
	break;
      case MxEngineState::StartPending:
	switch (prev) {
	  case MxLinkState::Connecting:
	  case MxLinkState::Disconnecting:
	  case MxLinkState::ConnectPending:
	  case MxLinkState::DisconnectPending:
	  case MxLinkState::Up:
	    if (!(m_up + m_transient)) {
	      m_state = MxEngineState::Starting;
	      start = true;
	    }
	    break;
	}
	break;
    }
    engineNext = m_state;

    if (start) start_();
    if (stop) stop_();
    if (stopped) stopped_();
  }

  if (engineNext != enginePrev) appEngineState(enginePrev, engineNext);
}

void MxEngine::final()
{
  m_links.clean();
  m_txPools.clean();
}

// connection state management

MxAnyTx::MxAnyTx(MxID id) : m_id(id)
{
}

void MxAnyTx::init(MxEngine *engine)
{
  m_engine = engine;
  m_mx = engine->mx();
}

MxAnyLink::MxAnyLink(MxID id) :
  MxAnyTx(id),
  m_state(MxLinkState::Down),
  m_reconnects(0)
{
}

void MxAnyLink::up_(bool enable)
{
  int prev, next;
  bool connect = false;

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    if (enable)
      m_enabled = true;
    else
      if (!m_enabled) return;

    // state machine
    prev = m_state;
    switch (prev) {
      case MxLinkState::Disabled:
      case MxLinkState::Down:
      case MxLinkState::Failed:
	m_state = MxLinkState::Connecting;
	connect = true;
	break;
      case MxLinkState::Disconnecting:
	m_state = MxLinkState::ConnectPending;
	break;
      case MxLinkState::DisconnectPending:
	m_state = MxLinkState::Connecting;
	break;
      default:
	break;
    }
    next = m_state;
  }

  engine()->linkState(this, prev, next);
  if (connect)
    mx()->add(ZmFn<>::Member<&MxAnyLink::connect>::fn(this));
}

void MxAnyLink::down_(bool disable)
{
  int prev, next;
  bool disconnect = false;

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    if (disable) m_enabled = false;

    // state machine
    prev = m_state;
    switch (prev) {
      case MxLinkState::Down:
	if (!m_enabled) m_state = MxLinkState::Disabled;
	break;
      case MxLinkState::Up:
      case MxLinkState::Reconnecting:
	m_state = MxLinkState::Disconnecting;
	disconnect = true;
	break;
      case MxLinkState::Connecting:
	m_state = MxLinkState::DisconnectPending;
	break;
      case MxLinkState::ConnectPending:
	m_state = MxLinkState::Disconnecting;
	break;
      default:
	break;
    }
    next = m_state;
  }

  engine()->linkState(this, prev, next);
  if (disconnect)
    mx()->add(ZmFn<>::Member<&MxAnyLink::disconnect>::fn(this));
}

void MxAnyLink::connected()
{
  int prev, next;
  bool disconnect = false;

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    // state machine
    prev = m_state;
    switch (prev) {
      case MxLinkState::Connecting:
      case MxLinkState::Reconnecting:
	m_state = MxLinkState::Up;
      case MxLinkState::Up:
	m_reconnects = 0;
	break;
      case MxLinkState::DisconnectPending:
	m_state = MxLinkState::Disconnecting;
	disconnect = true;
	m_reconnects = 0;
	break;
      default:
	break;
    }
    next = m_state;
  }

  engine()->linkState(this, prev, next);
  if (disconnect) 
    mx()->add(ZmFn<>::Member<&MxAnyLink::disconnect>::fn(this));
}

void MxAnyLink::disconnected()
{
  int prev, next;
  bool connect = false;

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    // state machine
    prev = m_state;
    switch (prev) {
      case MxLinkState::Connecting:
      case MxLinkState::DisconnectPending:
      case MxLinkState::Reconnecting:
      case MxLinkState::Up:
	m_state = m_enabled ? MxLinkState::Failed : MxLinkState::Disabled;
	m_reconnects = 0;
	break;
      case MxLinkState::Disconnecting:
	m_state = m_enabled ? MxLinkState::Down : MxLinkState::Disabled;
	m_reconnects = 0;
	break;
      case MxLinkState::ConnectPending:
	if (m_enabled) {
	  m_state = MxLinkState::Connecting;
	  connect = true;
	} else
	  m_state = MxLinkState::Disabled;
	m_reconnects = 0;
	break;
      default:
	break;
    }
    next = m_state;
  }

  engine()->linkState(this, prev, next);
  if (connect)
    mx()->add(ZmFn<>::Member<&MxAnyLink::connect>::fn(this));
}

void MxAnyLink::reconnect()
{
  int prev, next;
  bool reconnect = false, disconnect = false;
  ZmTime reconnTime;

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    // state machine
    prev = m_state;
    switch (prev) {
      case MxLinkState::Connecting:
      case MxLinkState::Up:
	m_state = MxLinkState::Reconnecting;
      case MxLinkState::Reconnecting:
	reconnect = true;
	break;
      case MxLinkState::DisconnectPending:
	disconnect = true;
	break;
      default:
	break;
    }
    next = m_state;

    if (reconnect) {
      ++m_reconnects;
      reconnTime.now(reconnInterval(m_reconnects));
    }
  }

  engine()->linkState(this, prev, next);
  if (reconnect)
    mx()->add(ZmFn<>::Member<&MxAnyLink::connect>::fn(this),
	reconnTime, &m_reconnTimer);
  if (disconnect) 
    mx()->add(ZmFn<>::Member<&MxAnyLink::disconnect>::fn(this));
}
