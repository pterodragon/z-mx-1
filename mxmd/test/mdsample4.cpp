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

// MxMD sample subscriber application

#include <ZuLib.hpp>

#include <stdio.h>
#include <signal.h>

#include <ZeLog.hpp>

#include <MxMD.hpp>
#include <MxMDCmd.hpp>

ZmSemaphore stop, stopped;	// used to signal exit

extern "C" { void sigint(int); };
void sigint(int sig) { stop.post(); }	// CTRL-C signal handler

void usage() {
  fputs("usage: mdsample4 CONFIG [RICS]\n"
    "    CONFIG - configuration file\n"
    "    RICS - optional file containing RICs to subscribe to\n"
    , stderr);
  exit(1);
}

void exitThread()
{
  stop.wait();
  MxMDLib *md = MxMDLib::instance();
  if (md) md->stop();
  stopped.post();
}

void exception(MxMDLib *, const MxException &e)
{
  std::cerr << Ze::name(e->severity()) << e->message() << '\n';
  fflush(stderr);
}

void l1(MxMDOrderBook *orderBook, const MxMDL1Data &l1Data_, MxSeqNo seqNo)
{
  const MxMDL1Data &l1Data = orderBook->l1Data();
  const MxSymString &ric = orderBook->security()->refData().ric;

  // insert application's L1 handling code here

  MxString<1024> line;
  MxMDFlagsStr flags;
  MxMDL1Flags::print(flags, orderBook->venueID(), l1Data.flags);
  line << ric << ' ' <<
    " stamp: " << ZuBox<int>(l1Data.stamp.hhmmss()).fmt(6, 0, -1, "------") <<
    " status: " << MxTradingStatus::name(l1Data.status) <<
    " last: " << l1Data.last <<
    " lastQty: " << l1Data.lastQty <<
    " bid: " << l1Data.bid <<
    " bidQty: " << l1Data.bidQty <<
    " ask: " << l1Data.ask <<
    " askQty: " << l1Data.askQty <<
    " tickDir: " << MxTickDir::name(l1Data.tickDir) <<
    " high: " << l1Data.high <<
    " low: " << l1Data.low <<
    " accVol: " << l1Data.accVol <<
    " accVolQty: " << l1Data.accVolQty <<
    " match: " << l1Data.match <<
    " matchQty: " << l1Data.matchQty <<
    " surplusQty: " << l1Data.surplusQty <<
    " flags: " << flags;
  puts(line);
}

ZtArray<ZtString> rics;

void referenceDataLoaded(MxMDVenue *venue)
{
  std::cerr << "reference data loaded for " << venue->id() << '\n';
  if (venue->id() == "XASX") {
    MxMDLib *md = venue->md();
    for (unsigned i = 0, n = rics.length(); i < n; i++) {
      ZmRef<MxMDSecurity> security =
	md->securities().findVal(MxSecKey(MxSecIDSrc::RIC, rics[i]));
      if (security)
	security->subscribe(MxMDSecurityHandler(). l1Fn(&l1));
      else
	std::cerr << "RIC " << rics[i] << " not found\n";
    }
  }
}

void subscribe(const MxMDCmd::CmdArgs &args, ZtArray<char> &out)
{
  MxMDLib *md = MxMDLib::instance();
  if (!md) throw ZtZString("MxMDLib::instance() failed");
  ZmRef<MxMDSecurity> security;
  unsigned argc = ZuBox<unsigned>(args.get("#"));
  if (argc < 2) throw MxMDCmd::CmdUsage();
  MxMDCmd::instance(md)->lookup(args, 1, &security, 0);
  if (!security) return;
  security->subscribe(MxMDSecurityHandler().l1Fn(&l1));
  out = "subscribed\n";
}

int main(int argc, char **argv)
{
  if (argc < 2 || argc > 3) usage();
  if (!argv[1]) usage();

  if (argc > 2) { // read RICs from file
    FILE *f = fopen(argv[2], "r");
    if (f) {
      int i = 0;
      do {
	ZtZString ric(32);
	if (!fgets(ric, 31, f)) break;
	ric.calcLength();
	ric.chomp();
	rics.length(i + 1);
	rics[i++] = ric;
      } while (i < 10000);
      fclose(f);
    } else {
      fprintf(stderr, "could not open %s\n", argv[2]);
    }
  }

  // configure and start logging
  ZeLog::init("mdsample4");		// program name
  ZeLog::level(Ze::Debug);		// turn on debug output
  ZeLog::add(ZeLog::fileSink("&2"));	// log errors to stderr
  ZeLog::start();			// start logger thread

  signal(SIGINT, &sigint);		// handle CTRL-C

  try {
    MxMDLib *md = MxMDLib::init(argv[1]); // initialize market data library
    if (!md) return 1;

    md->subscribe(MxMDLibHandler().
	exceptionFn(&exception).
	referenceDataLoadedFn(&referenceDataLoaded));

    MxMDCmd *cmd = MxMDCmd::instance(md);
    cmd->addCmd("subscribe",
	cmd->lookupSyntax(),
	MxMDCmd::CmdFn::fn(&subscribe),
	"subscribe",
	ZtZString("usage: subscribe OPTIONS symbol\n\nOptions:\n") +
	  cmd->lookupOptions());

    ZmThread(0, 0, ZmFn<>::fn(&exitThread), ZmThreadParams().detached(true));

    md->start();			// start all feeds

    stopped.wait();			// wait for stop

    md->final();			// clean up

  } catch (...) { }

  return 0;
}
