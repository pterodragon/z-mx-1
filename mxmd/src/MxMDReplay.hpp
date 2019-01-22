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

// MxMD replay

#ifndef MxMDReplay_HPP
#define MxMDReplay_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <ZmTime.hpp>
#include <ZmPLock.hpp>
#include <ZmGuard.hpp>
#include <ZmRef.hpp>

#include <ZtString.hpp>

#include <ZiFile.hpp>

#include <ZvCmd.hpp>

#include <MxEngine.hpp>

#include <MxMDTypes.hpp>

class MxMDCore;

class MxMDReplayLink;

class MxMDAPI MxMDReplay : public MxEngine, public MxEngineApp {
public:
  MxMDCore *core() const;

  void init(MxMDCore *core, ZmRef<ZvCf> cf);
  void final();

  bool replay(ZtString path,
      MxDateTime begin = MxDateTime(),
      bool filter = true);
  ZtString stopReplaying();

protected:
  ZmRef<MxAnyLink> createLink(MxID id);

  // commands
  void replayCmd(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg);

private:
  MxMDReplayLink	*m_link = 0;
};

class MxMDAPI MxMDReplayLink : public MxLink<MxMDReplayLink> {
public:
  MxMDReplayLink(MxID id) : MxLink<MxMDReplayLink>{id} { }

  typedef MxMDReplay Engine;

  ZuInline Engine *engine() const {
    return static_cast<Engine *>(MxAnyLink::engine()); // actually MxAnyTx
  }
  ZuInline MxMDCore *core() const {
    return engine()->core();
  }

  bool ok();
  bool replay(ZtString path, MxDateTime begin, bool filter);
  ZtString stopReplaying();

  // MxAnyLink virtual
  void update(ZvCf *);
  void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo);	 // unused
  bool failover() const { return false; }
  void failover(bool) { }

  void connect();
  void disconnect();

  // MxLink CRTP (unused)
  ZmTime reconnInterval(unsigned) { return ZmTime{1}; }

  // MxLink Rx CRTP (unused)
  void process(MxQMsg *) { }
  ZmTime reReqInterval() { return ZmTime{1}; }
  void request(const MxQueue::Gap &prev, const MxQueue::Gap &now) { }
  void reRequest(const MxQueue::Gap &now) { }

  // MxLink Tx CRTP (unused)
  void loaded_(MxQMsg *) { }
  void unloaded_(MxQMsg *) { }

  bool send_(MxQMsg *, bool) { return true; }
  bool resend_(MxQMsg *, bool) { return true; }

  bool sendGap_(const MxQueue::Gap &, bool) { return true; }
  bool resendGap_(const MxQueue::Gap &, bool) { return true; }

  void archive_(MxQMsg *msg) { archived(msg->id.seqNo + 1); }
  ZmRef<MxQMsg> retrieve_(MxSeqNo) { return nullptr; }

private:
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;

  typedef MxMDStream::Msg Msg;

  typedef ZuPair<ZuBox0(uint16_t), ZuBox0(uint16_t)> Version;

  void read();

private:
  Lock			m_lock;	// serializes replay/stopReplaying
 
  // Rx thread members
  ZtString		m_path;
  ZiFile		m_file;
  ZuRef<Msg>		m_msg;
  ZmTime		m_lastTime;
  ZmTime		m_nextTime;
  bool			m_filter = false;
  Version		m_version;
};

#endif /* MxMDReplay_HPP */
