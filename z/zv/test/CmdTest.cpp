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

#include <ZuTuple.hpp>
#include <ZuPrint.hpp>
#include <ZuMixin.hpp>
#include <ZuPolymorph.hpp>

#include <ZmTrap.hpp>

#include <ZvCmd.hpp>

class Mx : public ZuPolymorph, public ZiMultiplex {
public:
  inline Mx() : ZiMultiplex(ZvMultiplexParams()) { }
  inline Mx(ZvCf *cf) : ZiMultiplex(ZvMultiplexParams(cf)) { }
};

class ZvCmdTest : public ZvCmdServer {
public:
  inline ZvCmdTest() { }
  virtual ~ZvCmdTest() { }

  void init(ZvCf *cf, ZiMultiplex *mx) {
    ZvCmdServer::init(cf, mx,
	ZvCmdDiscFn::Member<&ZvCmdTest::disconnected>::fn(this),
	ZvCmdRcvdFn::Member<&ZvCmdTest::cmdRcvd>::fn(this));
  }

  void wait() { m_done.wait(); }
  void post() { m_done.post(); }

private:
  void cmdRcvd(ZvCmdLine *line,
      const ZvInvocation &inv, ZvAnswer &ans) {
    const ZtZString &cmd = inv.cmd();
    printf("%s:%d cmd: %s\n",
	line->info().remoteIP.string().data(),
	(int)line->info().remotePort, cmd.data());
    std::cout << ZtHexDump(ZtSprintf("%s:%d data:",
	  line->info().remoteIP.string().data(),
	  (int)line->info().remotePort),
	inv.data().data(), inv.data().length());
    if (cmd == "ackme") {
      ans.make(ZvCmd::Success, Ze::Info, ZvAnswerArgs, "this is an ack");
    } else if (cmd == "nakme") {
      ans.make(ZvCmd::Fail, Ze::Info, ZvAnswerArgs, "this is a nak");
    } else {
      ans.make(ZvCmd::Success, Ze::Info, ZvAnswerArgs,
	  ZtZString() << "code 1 " << cmd);
    }
  }

  void disconnected(ZvCmdLine *line) {
    printf("%s:%d disconnected\n",
	   line->info().remoteIP.string().data(),
	   (int)line->info().remotePort);
  }

  ZmSemaphore m_done;
};

int main(int argc, char **argv)
{
  ZmRef<Mx> mx = new Mx();
  ZmRef<ZvCmdTest> server = new ZvCmdTest();

  ZmTrap::sigintFn(ZmFn<>::Member<&ZvCmdTest::post>::fn(server));
  ZmTrap::trap();

  ZeLog::init("ZvCmdTest");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink("&2"));
  ZeLog::start();

  server->init(ZmRef<ZvCf>(new ZvCf()), mx);

  mx->start();
  server->start();

  server->wait();

  server->stop();
  mx->stop(true);
  ZeLog::stop();

  return 0;
}
