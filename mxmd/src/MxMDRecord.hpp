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

#include <ZiFile.hpp>

#include <MxEngine.hpp>

#include <MxMDTypes.hpp>

class MxMDCore;

class MxMDRecord : public MxEngineApp, public MxEngine {
public:
  MxMDRecord() { }
  ~MxMDRecord() { }

  inline MxMDCore *core() const { return static_cast<MxMDCore *>(mgr()); }

  void init(MxMDCore *core);
  void final();

  bool record(ZtString path);
  ZtString stopRecording();

protected:
  ZmRef<MxAnyLink> createLink(MxID id);

  // Rx
  MxEngineApp::ProcessFn processFn();
  void wake();

  // Tx (called from engine's tx thread) (unused)
  void sent(MxAnyLink *, MxQMsg *) { }
  void aborted(MxAnyLink *, MxQMsg *) { }
  void archive(MxAnyLink *, MxQMsg *) { }
  ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo) { return 0; }

private:
  MxMDRecLink	*m_link = 0;
};

class MxMDAPI MxMDRecLink : public MxLink<MxMDRecLink> {
public:
  MxMDRecLink(MxID id) : MxLink<MxMDRecLink>{id} { }

  inline MxMDCore *core() const {
    return static_cast<MxMDCore *>(engine()->mgr());
  }

  bool record(ZtString path);
  ZtString stopRecording();

  // MxAnyLink virtual (mostly unused)
  void update(ZvCf *);
  void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo);

  void connect();
  void disconnect();

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

private:
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  typedef MxQueueRx<MxMDRecLink> Rx;
  typedef MxMDStream::MsgData MsgData;

  int write_(const Frame *frame, ZeError *e);

  // Rx thread
  void recv(Rx *rx);
  void write(MxQMsg *qmsg);

  // snap thread
  void snap();
  Frame *push(unsigned size);
  void *out(void *ptr, unsigned length, unsigned type, ZmTime stamp);
  void push2();

private:
  int			m_snapThread = -1;
  Lock			m_lock;	// serializes record/stopRecording
  Lock			m_fileLock;
    ZtString		  m_path;
    ZiFile		  m_file;
  ZmRef<MsgData>	m_snapMsg;
};

#endif /* MxMDRecord_HPP */
