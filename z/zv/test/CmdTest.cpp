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

#include <ZvCmdServer.hpp>
#include <ZvMultiplexCf.hpp>

class Mx : public ZuPolymorph, public ZiMultiplex {
public:
  inline Mx() : ZiMultiplex(ZvMxParams()) { }
  inline Mx(ZvCf *cf) : ZiMultiplex(ZvMxParams(cf)) { }
};

class ZvCmdTest :
    public ZmPolymorph, public ZvCmdServer<ZvCmdTest> {
public:
  void init(ZiMultiplex *mx, ZvCf *cf) {
    ZvCmdServer::init(mx, cf);
    addCmd("ackme", "", ZvCmdFn{[](void *context_, ZvCf *args, ZtString &out) {
      auto context = static_cast<Context *>(context_);
      if (auto cxn = context->link->tcp())
	std::cout << cxn->info().remoteIP << ':'
	  << ZuBoxed(cxn->info().remotePort) << ' ';
      std::cout << "user: "
	<< context->user->id << ' ' << context->user->name << '\n'
	<< "cmd: " << args->get("0") << '\n';
      out << "this is an ack\n";
    }}, "test ack", "");
    addCmd("nakme", "", ZvCmdFn{[](void *context_, ZvCf *args, ZtString &out) {
      out << "this is a nak\n";
    }}, "test nak", "");
    addCmd("quit", "", ZvCmdFn{[](void *context_, ZvCf *args, ZtString &out) {
      auto context = static_cast<Context *>(context_);
      context->app->post();
      out << "quitting...\n";
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
  ZeLog::sink(ZeLog::fileSink("&2"));
  ZeLog::start();

  server->init(mx, ZmMkRef(new ZvCf())); // FIXME - certs, etc.

  mx->start();
  server->start();

  server->wait();

  server->stop();
  mx->stop(true);
  ZeLog::stop();

  return 0;
}
