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

/* test program */

#include <ZmHash.hpp>
#include <ZmThread.hpp>
#include <ZmSemaphore.hpp>
#include <ZmTrap.hpp>

struct Connection : public ZmObject { };

typedef ZmHash<int, ZmHashVal<ZmRef<Connection> > > ConnHash;

bool running = true;

struct TestObject {
  TestObject() { connHash = new ConnHash(); }

  void inserter() {
    printf("Starting inserter\n");
    while (running) {
      connHash->findAdd(15, ZmRef<Connection>(new Connection()));
    }
  }
  void remover() {
    printf("Starting remover\n");
    while (running) {
      connHash->del(15);
    }
  }
  void finder() {
    printf("Starting finder\n");
    while (running) {
      ZmRef<Connection> c = connHash->findVal(15);
    }
  }
  ZmRef<ConnHash>	connHash;
};

ZmSemaphore sem;

void stop() {
  running = false;
  sem.post();
}

int main(int argc, char *argv[])
{
  ZmTrap::sigintFn(ZmFn<>::Ptr<&stop>::fn());
  ZmTrap::trap();
  TestObject prog;

  ZmThread inserter(0, ZmFn<>::Member<&TestObject::inserter>::fn(&prog));
  ZmThread remover(0, ZmFn<>::Member<&TestObject::remover>::fn(&prog));
  ZmThread finder(0, ZmFn<>::Member<&TestObject::finder>::fn(&prog));

  sem.wait();

  puts("Caught Ctrl-C\n");

  return 0;
}
