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
#include <mxmd/MxMDLib.hpp>
#endif

#include <zlib/ZmTime.hpp>
#include <zlib/ZmPLock.hpp>
#include <zlib/ZmGuard.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmAtomic.hpp>

#include <zlib/ZtString.hpp>

#include <zlib/ZiFile.hpp>

#include <zlib/ZvCmdHost.hpp>

#include <mxbase/MxEngine.hpp>

#include <mxmd/MxMDTypes.hpp>

class MxMDCore;

class MxMDRecLink;

class MxMDRecord : public MxEngine, public MxEngineApp {
public:
  MxMDCore *core() const;

  void init(MxMDCore *core, ZvCf *cf);
  void final();

  ZuInline unsigned snapThread() const { return m_snapThread; }

  bool record(ZtString path);
  ZtString stopRecording();

protected:
  ZmRef<MxAnyLink> createLink(MxID id);

  // commands
  int recordCmd(void *, ZvCf *args, ZtString &out);

private:
  unsigned	m_snapThread = 0;

  MxMDRecLink	*m_link = 0;
};

class MxMDAPI MxMDRecLink : public MxLink<MxMDRecLink> {
public:
  MxMDRecLink(MxID id) : MxLink<MxMDRecLink>{id} { }

  typedef MxMDRecord Engine;

  ZuInline Engine *engine() const {
    return static_cast<Engine *>(MxAnyLink::engine()); // actually MxAnyTx
  }
  ZuInline MxMDCore *core() const {
    return engine()->core();
  }

  bool ok();
  bool record(ZtString path);
  ZtString stopRecording();

  // MxAnyLink virtual (mostly unused)
  void update(ZvCf *);
  void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo);

  void connect();
  void disconnect();

  // MxLink CRTP (unused)
  ZmTime reconnInterval(unsigned) { return ZmTime{1}; }

  // MxLink Rx CRTP
  void process(MxQMsg *);
  ZmTime reReqInterval() { return ZmTime{1}; } // unused
  void request(const MxQueue::Gap &prev, const MxQueue::Gap &now) { } // unused
  void reRequest(const MxQueue::Gap &now) { } // unused

  // MxLink Tx CRTP (unused)
  void loaded_(MxQMsg *) { }
  void unloaded_(MxQMsg *) { }

  bool send_(MxQMsg *, bool) { return true; }
  bool resend_(MxQMsg *, bool) { return true; }
  void aborted_(MxQMsg *msg) { }

  bool sendGap_(const MxQueue::Gap &, bool) { return true; }
  bool resendGap_(const MxQueue::Gap &, bool) { return true; }

  void archive_(MxQMsg *msg) { archived(msg->id.seqNo + 1); }
  ZmRef<MxQMsg> retrieve_(MxSeqNo, MxSeqNo) { return nullptr; }

private:
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  typedef MxQueueRx<MxMDRecLink> Rx;

  typedef MxMDStream::Msg Msg;

  int write_(const void *ptr, ZeError *e);

  // Rx thread
  void wake();
  void recv(Rx *rx);

public:
  // snap thread
  void snap();
  void *push(unsigned size);
  void *out(void *ptr, unsigned length, unsigned type, int shardID);
  void push2();

private:
  Lock			m_lock;	// serializes record/stopRecording

  MxSeqNo		m_seqNo = 0;

  Lock			m_fileLock;
    ZtString		  m_path;
    ZiFile		  m_file;

  ZuRef<Msg>		m_snapMsg;
};

#endif /* MxMDRecord_HPP */
