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
    m_mx->add(ZmFn<>::Member<&MxAnyLink::up>::fn(link));
}

void MxEngine::stop_()
{
  appException(ZeEVENT(Info, "STOP"));

  auto i = m_links.readIterator();
  while (ZmRef<MxAnyLink> link = i.iterateKey())
    m_mx->add(ZmFn<>::Member<&MxAnyLink::down>::fn(link));
  down();

  appDelEngine();
}

void MxEngine::stopped_()
{
}

void MxEngine::start()
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);

  bool start = false;

  switch ((int)m_state) {
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

  appEngineState(m_state);

  if (start) start_();
}

void MxEngine::stop()
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);

  bool stop = false;

  switch ((int)m_state) {
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

  appEngineState(m_state);

  if (stop) stop_();
}

void MxEngine::linkUp(MxAnyLink *link)
{
  appException(ZeEVENT(Info,
    ([id = link->id()](const ZeEvent &, ZmStream &s) {
      s << "link " << id << " up"; })));

  ZmGuard<ZmLock> stateGuard(m_stateLock);

  if (++m_running < m_links.count()) return;

  bool stop = false;

  switch ((int)m_state) {
    case MxEngineState::Starting:
      m_state = MxEngineState::Running;
      break;
    case MxEngineState::StopPending:
      m_state = MxEngineState::Stopping;
      stop = true;
      break;
    default:
      break;
  }

  if (stop) stop_();
}

void MxEngine::linkDown(MxAnyLink *link)
{
  appException(ZeEVENT(Info,
    ([id = link->id()](const ZeEvent &, ZmStream &s) {
      s << "link " << id << " down"; })));

  ZmGuard<ZmLock> stateGuard(m_stateLock);

  if (--m_running) return;

  bool start = false, stopped = false;

  switch ((int)m_state) {
    case MxEngineState::Stopping:
      m_state = MxEngineState::Stopped;
      stopped = true;
      break;
    case MxEngineState::StartPending:
      m_state = MxEngineState::Starting;
      start = true;
      break;
    default:
      break;
  }

  if (stopped) stopped_();
  if (start) start_();
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

void MxAnyLink::up()
{
  bool connect = false;

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    // state machine
    switch ((int)m_state) {
      case MxLinkState::Down:
	m_state = MxLinkState::Connecting;
	connect = true;
	break;
      case MxLinkState::Disconnecting:
	m_state = MxLinkState::ConnectPending;
	break;
      default:
	break;
    }

    if (connect) m_reconnects = 0;
  }

  if (connect)
    mx()->add(ZmFn<>::Member<&MxAnyLink::connect>::fn(this));
}

void MxAnyLink::down()
{
  bool disconnect = false;

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    // state machine
    switch ((int)m_state) {
      case MxLinkState::Up:
	m_state = MxLinkState::Disconnecting;
	disconnect = true;
	break;
      case MxLinkState::Connecting:
	m_state = MxLinkState::DisconnectPending;
	break;
      default:
	break;
    }

    if (disconnect) m_reconnects = 0;
  }

  if (disconnect)
    mx()->add(ZmFn<>::Member<&MxAnyLink::disconnect>::fn(this));
}

void MxAnyLink::reconnect()
{
  bool reconnect = false;

  ZmGuard<ZmLock> stateGuard(m_stateLock);

  // cancel reconnect
  mx()->del(&m_reconnTimer);

  // state machine
  switch ((int)m_state) {
    case MxLinkState::Connecting:
    case MxLinkState::Up:
      reconnect = true;
      break;
    default:
      break;
  }

  if (reconnect) {
    ++m_reconnects;
    m_reconnTime.now(reconnInterval(m_reconnects));
    ZmTime reconnTime = m_reconnTime;
    stateGuard.unlock();
    mx()->add(ZmFn<>::Member<&MxAnyLink::connect>::fn(this),
	reconnTime, &m_reconnTimer);
  }
}

void MxAnyLink::connected()
{
  bool linkUp = false, disconnect = false;

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    // state machine
    switch ((int)m_state) {
      case MxLinkState::Connecting:
	m_state = MxLinkState::Up;
	linkUp = true;
	break;
      case MxLinkState::DisconnectPending:
	m_state = MxLinkState::Disconnecting;
	disconnect = true;
	break;
      default:
	break;
    }

    if (disconnect) m_reconnects = 0;
  }

  if (linkUp) engine()->linkUp(this);
  if (disconnect) 
    mx()->add(ZmFn<>::Member<&MxAnyLink::disconnect>::fn(this));
}

void MxAnyLink::disconnected()
{
  bool linkDown = false, connect = false;

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    // state machine
    switch ((int)m_state) {
      case MxLinkState::Disconnecting:
	m_state = MxLinkState::Down;
	linkDown = true;
	break;
      case MxLinkState::ConnectPending:
	m_state = MxLinkState::Connecting;
	connect = true;
	break;
      default:
	break;
    }

    if (connect) m_reconnects = 0;
  }

  if (linkDown) engine()->linkDown(this);
  if (connect)
    mx()->add(ZmFn<>::Member<&MxAnyLink::connect>::fn(this));
}
