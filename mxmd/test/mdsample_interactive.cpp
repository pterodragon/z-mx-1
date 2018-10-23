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

#include <iostream>

ZmSemaphore stop;	// used to signal exit

extern "C" { void sigint(int); };
void sigint(int sig) { stop.post(); }	// CTRL-C signal handler

void usage() {
  std::cerr <<
    "usage: mdsample_interactive CONFIG [RICS]\n"
    "    CONFIG - configuration file\n"
    "    RICS - optional file containing RICs to subscribe to\n"
    << std::flush;
  exit(1);
}

void l1(MxMDOrderBook *ob, const MxMDL1Data &)
{
  using namespace ZuFmt;
  const MxMDL1Data &l1 = ob->l1Data();
  MxMDFlagsStr flags;
  MxMDL1Flags::print(flags, ob->venueID(), l1.flags);
  std::cout << ob->security()->id() <<
    " stamp: " <<
      ZuBox<unsigned>(l1.stamp.hhmmss()).fmt(Right<6>()) << '.' <<
	ZuBox<unsigned>(l1.stamp.nsec()).fmt(Right<9>()) <<
    ' ' << MxTradingStatus::name(l1.status) <<
    ' ' << MxTickDir::name(l1.tickDir) <<
    " last: " << MxValNDP{l1.last, l1.pxNDP}.fmt(FP<-3>()) << '/' <<
      MxValNDP{l1.lastQty, l1.qtyNDP}.fmt(FP<-3>()) <<
    " bid: " << MxValNDP{l1.bid, l1.pxNDP}.fmt(FP<-3>()) << '/' <<
      MxValNDP{l1.bidQty, l1.qtyNDP}.fmt(FP<-3>()) <<
    " ask: " << MxValNDP{l1.ask, l1.pxNDP}.fmt(FP<-3>()) << '/' <<
      MxValNDP{l1.askQty, l1.qtyNDP}.fmt(FP<-3>()) <<
    " high: " << MxValNDP{l1.high, l1.pxNDP}.fmt(FP<-3>()) <<
    " low: " << MxValNDP{l1.low, l1.pxNDP}.fmt(FP<-3>()) <<
    " accVol: " << MxValNDP{l1.accVol, l1.pxNDP}.fmt(FP<-3>()) << '/' <<
      MxValNDP{l1.accVolQty, l1.qtyNDP}.fmt(FP<-3>()) <<
    " match: " << MxValNDP{l1.match, l1.pxNDP}.fmt(FP<-3>()) << '/' <<
      MxValNDP{l1.matchQty, l1.qtyNDP}.fmt(FP<-3>()) <<
    " surplusQty: " << MxValNDP{l1.surplusQty, l1.qtyNDP}.fmt(FP<-3>()) <<
    " flags: " << flags << '\n';
}

void pxLevel(MxMDPxLevel *pxLevel, MxDateTime stamp)
{
  using namespace ZuFmt;
  const MxMDPxLvlData &pxLvlData = pxLevel->data();
  std::cout <<
    ZuBox<unsigned>(stamp.hhmmss()).fmt(Right<6>()) << '.' <<
      ZuBox<unsigned>(stamp.nsec()).fmt(Right<9>()) <<
    (pxLevel->side() == MxSide::Buy ? " bid" : " ask") <<
    " price: " << MxValNDP{pxLevel->price(), pxLevel->pxNDP()}.fmt(FP<-3>()) <<
    " qty: " << MxValNDP{pxLvlData.qty, pxLevel->qtyNDP()}.fmt(FP<-3>()) <<
    " nOrders: " << pxLvlData.nOrders.fmt(FP<-3>()) << '\n';
}

void deletedPxLevel(MxMDPxLevel *pxLevel, MxDateTime stamp)
{
  using namespace ZuFmt;
  // const MxMDPxLvlData &pxLvlData = pxLevel->data();
  std::cout <<
    ZuBox<unsigned>(stamp.hhmmss()).fmt(Right<6>()) << '.' <<
      ZuBox<unsigned>(stamp.nsec()).fmt(Right<9>()) <<
    (pxLevel->side() == MxSide::Buy ? " bid" : " ask") <<
    " price: " << MxValNDP{pxLevel->price(), pxLevel->pxNDP()}.fmt(FP<-3>()) <<
    " DELETED" << '\n';
}

void l2(MxMDOrderBook *ob, MxDateTime stamp)
{
  std::cout << "L2 updated\n";
}

void exception(MxMDLib *, ZmRef<ZeEvent> e) { std::cerr << *e << '\n'; }

typedef ZmLHash<MxSymString> Syms; // hash table of syms
static Syms *syms = 0;

void refDataLoaded(MxMDVenue *venue)
{
  std::cout << "reference data loaded for " << venue->id() << '\n';
}

static ZmRef<MxMDSecHandler> secHandler;

void addSecurity(MxMDSecurity *security, MxDateTime)
{
  if (!syms) return;
  if (!syms->findKey(security->refData().symbol)) return;
  security->subscribe(secHandler);
}

void subscribe(const MxMDCmd::Args &args, ZtArray<char> &out)
{
  MxMDLib *md = MxMDLib::instance();
  if (!md) throw ZtString("MxMDLib::instance() failed");
  ZmRef<MxMDSecurity> security;
  unsigned argc = ZuBox<unsigned>(args.get("#"));
  if (argc < 2) throw MxMDCmd::Usage();
  md->lookupSecurity(args, 1, 1, [&out](MxMDSecurity *sec) {
    sec->subscribe(secHandler);
    out << "subscribed\n";
  });
}

int main(int argc, char **argv)
{
  if (argc < 2 || argc > 3) usage();
  if (!argv[1]) usage();

  syms = new Syms();
  (secHandler = new MxMDSecHandler())->
    l1Fn(MxMDLevel1Fn::Ptr<&l1>::fn()).
    addPxLevelFn(MxMDPxLevelFn::Ptr<&pxLevel>::fn()).
    updatedPxLevelFn(MxMDPxLevelFn::Ptr<&pxLevel>::fn()).
    deletedPxLevelFn(MxMDPxLevelFn::Ptr<&deletedPxLevel>::fn()).
    l2Fn(MxMDOrderBookFn::Ptr<&l2>::fn());

  // read rics from file into hash table
  {
    FILE *f = fopen(argv[2], "r");
    if (f) {
      int i = 0;
      do {
	MxSymString sym;
	if (!fgets(sym, MxSymString::N - 1, f)) break;
	sym.calcLength();
	sym.chomp();
	syms->add(sym);
      } while (i < 10000);
      fclose(f);
    } else {
      std::cerr << "could not open " << argv[2] << '\n';
    }
  }

  signal(SIGINT, &sigint);		// handle CTRL-C

  try {
    MxMDLib *md = MxMDLib::init(argv[1]); // initialize market data library
    if (!md) return 1;

    // add interactive subscribe command
    md->addCmd("subscribe",
	md->lookupSyntax(),
	MxMDCmdFn::Ptr<&subscribe>::fn(),
	"subscribe",
	ZtString() << "usage: subscribe OPTIONS symbol\n\nOptions:\n" <<
	  md->lookupOptions());

    md->subscribe(&((new MxMDLibHandler())->
	exceptionFn(MxMDExceptionFn::Ptr<&exception>::fn()).
	refDataLoadedFn(MxMDVenueFn::Ptr<&refDataLoaded>::fn()).
	addSecurityFn(MxMDSecurityFn::Ptr<&addSecurity>::fn())));

    md->start();			// start all feeds

    stop.wait();			// wait for stop

    md->stop();				// stop
    md->final();			// clean up

  } catch (...) { }

  delete syms;

  return 0;
}
