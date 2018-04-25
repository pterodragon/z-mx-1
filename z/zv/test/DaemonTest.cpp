//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZmPlatform.hpp>
#include <ZmTime.hpp>
#include <ZmSemaphore.hpp>
#include <ZmTrap.hpp>

#include <ZeLog.hpp>

#include <ZvDaemon.hpp>
#include <ZvCf.hpp>

namespace {
  void usage() {
    puts("usage: DaemonTest [username [password]] [-d|--daemonize]");
    exit(1);
  }

  void notify(const char *text) {
    ZeLOG(Info, ZtSprintf("PID %d: %s", (int)ZmPlatform::getPID(), text));
  }

  ZmSemaphore done;

  void sigint() {
    ZeLOG(Info, "SIGINT");
    done.post();
  }
} // namespace

#ifdef _MSC_VER
#pragma warning(disable:4800)
#endif

int main(int argc, char **argv)
{
  static ZvOpt opts[] = {
    { "daemonize", "d", ZvOptFlag },
    { "help", 0, ZvOptFlag },
    { 0 }
  };

  ZmRef<ZvCf> cf = new ZvCf();
  ZtZString username, password;
  bool daemonize;

  ZeLog::init("DaemonTest");
  ZeLog::level(0);
  ZeLog::add(ZeLog::debugSink());

  ZmTrap::sigintFn(ZmFn<>::Ptr<&sigint>::fn());
  ZmTrap::trap();

  try {
    cf->fromArgs(opts, argc, argv);

    // if (cf->getInt("help", 0, 1, 0)) usage();

    username = cf->get("1");
    password = cf->get("2");
    daemonize = cf->getInt("daemonize", 0, 1, 0);
  } catch (const ZvError &e) {
    ZeLog::start();
    ZeLOG(Error, e.message());
    ZeLog::stop();
    exit(1);
  }

  int r = ZvDaemon::init(username, password, 0, daemonize, "DaemonTest.pid");

  ZeLog::start();

  switch (r) {
    case ZvDaemon::OK:
      notify("OK");
      break;
    case ZvDaemon::Running:
      notify("already running");
      ZeLog::stop();
      exit(1);
      break;
    case ZvDaemon::Error:
      notify("error");
      ZeLog::stop();
      exit(1);
      break;
  }

  done.wait();

  ZeLog::stop();
  return 0;
}
