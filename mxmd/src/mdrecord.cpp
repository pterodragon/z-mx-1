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

// MxMD recording application

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
    "usage: mdrecord CONFIG RECFILE [SYMBOLS]\n"
    "    CONFIG\t- configuration file\n"
    "    RECFILE\t- recording file\n"
    "    SYMBOLS\t- optional file containing symbols to subscribe to\n"
    << std::flush;
  exit(1);
}

void exception(MxMDLib *, ZmRef<ZeEvent> e) { std::cerr << *e << '\n'; }

void l1(MxMDOrderBook *, const MxMDL1Data &) { }
void l2(MxMDOrderBook *, MxDateTime) { }

typedef ZmLHash<MxSymString> Syms; // hash table of syms
static Syms *syms = 0;

void refDataLoaded(MxMDVenue *venue)
{
  MxMDLib *md = venue->md();
  md->dumpTickSizes(MxTxtString() << venue->id() << "_tickSizes.csv");
  md->dumpSecurities(MxTxtString() << venue->id() << "_securities.csv");
  md->dumpOrderBooks(MxTxtString() << venue->id() << "_orderBooks.csv");
}

static ZmRef<MxMDSecHandler> secHandler;

void addSecurity(MxMDSecurity *security, MxDateTime)
{
  if (!syms) return;
  if (!syms->findKey(security->refData().symbol)) return;
  security->subscribe(secHandler);
}

int main(int argc, char **argv)
{
  if (argc < 3 || argc > 4) usage();
  if (!argv[1] || !argv[2]) usage();

  syms = new Syms();
  (secHandler = new MxMDSecHandler())->
    l1Fn(MxMDLevel1Fn::Ptr<&l1>::fn()).
    l2Fn(MxMDLevel2Fn::Ptr<&l2>::fn());

  if (argc > 3) { // read symbols from file
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

    md->subscribe(&((new MxMDLibHandler())->
	exceptionFn(MxMDExceptionFn::Ptr<&exception>::fn()).
	refDataLoadedFn(MxMDVenueFn::Ptr<&refDataLoaded>::fn()).
	addSecurityFn(MxMDSecurityFn::Ptr<&addSecurity>::fn())));

    md->record(argv[2]);		// start recording
    md->start();			// start all feeds

    stop.wait();			// wait for SIGINT

    md->stopRecording();		// stop recording
    md->stop();				// stop
    md->final();			// clean up

  } catch (...) { }

  delete syms;

  return 0;
}
