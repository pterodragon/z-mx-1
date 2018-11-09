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
#include <ZuPolymorph.hpp>

#include <ZmTrap.hpp>

#include <ZvCmd.hpp>
#include <ZvMultiplexCf.hpp>

class Mx : public ZuPolymorph, public ZiMultiplex {
public:
  inline Mx() : ZiMultiplex(ZvMultiplexParams()) { }
  inline Mx(ZvCf *cf) : ZiMultiplex(ZvMultiplexParams(cf)) { }
};

class ZvCmdTest : public ZmPolymorph, public ZvCmdServer {
public:
  void init(ZiMultiplex *mx, ZvCf *cf) {
    ZvCmdServer::init(mx, cf);
    addCmd("ackme", "", ZvCmdFn{[](ZvCmdServerCxn *cxn,
	  ZvCf *args, ZmRef<ZvCmdMsg> in, ZmRef<ZvCmdMsg> &out) {
	  std::cout << cxn->info().remoteIP << ':' << 
	    ZuBoxed(cxn->info().remotePort) <<
	    " cmd: " << args->get("0") << '\n' <<
	    ZtHexDump("data:", in->data().data(), in->data().length());
	  out = new ZvCmdMsg(in->seqNo(), 0, "this is an ack\n");
	}}, "test ack", "");
    addCmd("nakme", "", ZvCmdFn{[](ZvCmdServerCxn *cxn,
	    ZvCf *args, ZmRef<ZvCmdMsg> in, ZmRef<ZvCmdMsg> &out) {
	  out = new ZvCmdMsg(in->seqNo(), -1, "this is a nak\n");
	}}, "test nak", "");
    addCmd("quit", "", ZvCmdFn{[](ZvCmdServerCxn *cxn,
	    ZvCf *args, ZmRef<ZvCmdMsg> in, ZmRef<ZvCmdMsg> &out) {
	  static_cast<ZvCmdTest *>(cxn->mgr())->post();
	  out = new ZvCmdMsg(in->seqNo(), -1, "quitting...\n");
	}}, "quit", "");
  }

  void wait() { m_done.wait(); }
  void post() { m_done.post(); }

private:

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

  server->init(mx, ZmMkRef(new ZvCf()));

  mx->start();
  server->start();

  server->wait();

  server->stop();
  mx->stop(true);
  ZeLog::stop();

  return 0;
}
