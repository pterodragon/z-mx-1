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

// MxMD recorder

#ifndef MxMDRecord_HPP
#define MxMDRecord_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <ZiMultiplex.hpp>

#include <MxMultiplex.hpp>
#include <MxEngine.hpp>

#include <MxMDTypes.hpp>

class MxMDCore;

class MxMDRecord : public MxEngineApp, public MxEngine {
public:
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

public:
  MxMDRecord(MxMDCore *core);
  ~MxMDRecord();

  void init();

  bool start(ZtString path);
  ZtString stop();

  ZtString path() const;
  bool running() const;

  // Rx (called from engine's rx thread)
  MxEngineApp::ProcessFn processFn();

  // Tx (called from engine's tx thread) (unused)
  void sent(MxAnyLink *, MxQMsg *) { }
  void aborted(MxAnyLink *, MxQMsg *) { }
  void archive(MxAnyLink *, MxQMsg *) { }
  ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo) { return 0; }

  // FIXME - move file, etc. into link
  class Link : public MxLink<Link> { // dummy Link for recorder (mostly unused)
  public:
    Link(MxID id) : MxLink<Link>{id} { }

    void init(MxEngine *engine) { MxLink<Link>::init(engine); }

    ZuInline MxMDRecord *recorder() {
      return static_cast<MxMDRecord *>(this->engine());
    }

    // MxAnyLink virtual (unused)
    void update(ZvCf *) { }
    void reset(MxSeqNo, MxSeqNo) { }

    void connect() { MxAnyLink::connected(); }
    void disconnect() { MxAnyLink::disconnected(); }

    // MxLink CRTP (unused)
    ZmTime reconnInterval(unsigned) { return ZmTime{1}; }

    // MxLink Rx CRTP (unused)
    ZmTime reReqInterval() { return ZmTime{1}; }
    void request(const MxQueue::Gap &prev, const MxQueue::Gap &now) { }
    void reRequest(const MxQueue::Gap &now) { }

    // MxLink Tx CRTP (unused)
    bool send_(MxQMsg *, bool) { return true; }
    bool resend_(MxQMsg *, bool) { return true; }

    bool sendGap_(const MxQueue::Gap &, bool) { return true; }
    bool resendGap_(const MxQueue::Gap &, bool) { return true; }
  };

private:
  typedef MxQueueRx<Link> Rx;
  typedef MxMDStream::MsgData MsgData;

  bool fileOpen(ZtString path);
  ZtString fileClose();
  int write(const Frame *frame, ZeError *e);

  void recvStart();
  void recvStop();

  void snapStart();
  void snapStop();

  void recv(Rx *rx);
  void recvMsg(Rx *rx);
  void writeMsg(MxQMsg *qmsg);

  void snap();
  Frame *push(unsigned size);
  void *out(void *ptr, unsigned length, unsigned type, ZmTime stamp);
  void push2();
  MxSeqNo snapshotSeqNo();

private:
  MxMDCore		*m_core;
  ZmRef<MxLink<Link> >	m_link;
  int			m_snapThread = -1;

  ZmSemaphore		m_attachSem;
  ZmSemaphore		m_detachSem;

  Lock			m_stateLock;
    bool		  m_running = false;

  Lock			m_ioLock;
    ZtString		  m_path;
    ZiFile		  m_file;
    ZmRef<MsgData>	  m_snapMsg;
    MxSeqNo		  m_snapshotSeqNo = 0;
};

#endif /* MxMDRecord_HPP */
