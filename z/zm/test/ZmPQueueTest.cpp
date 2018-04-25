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

/* ZmPQueue unit test */

#include <ZuLib.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <ZuTraits.hpp>
#include <ZuPair.hpp>

#include <ZmPQueue.hpp>
#include <ZmNoLock.hpp>

typedef ZuPair<uint32_t, unsigned> Msg_Data;
template <typename T> struct Msg_Fn : public T {
  inline uint32_t key() const { return T::ptr()->p1(); }
  inline unsigned length() const { return T::ptr()->p2(); }
  inline unsigned clipHead(unsigned length) {
    T::ptr()->p1() += length;
    return T::ptr()->p2() -= length;
  }
  inline unsigned clipTail(unsigned length) {
    return T::ptr()->p2() -= length;
  }
  template <typename I>
  inline void write(const I &i) { }
};
typedef ZuMixin<Msg_Data, Msg_Fn> Msg;

typedef ZmPQueue<Msg,
	  ZmPQueueLock<ZmNoLock,
	    ZmPQueueBits<1,
	      ZmPQueueLevels<4,
		ZmPQueueNodeIsItem<true> > > > > PQueue;

typedef PQueue::Node QMsg;

void head(PQueue &q, uint32_t seqNo)
{
  printf("head %u\n", (unsigned)seqNo);
  q.head(seqNo);
}
void dequeue(PQueue &q)
{
  while (ZmRef<QMsg> msg = q.dequeue())
    printf("process %u, %u\n", (unsigned)msg->Msg::key(), msg->length());
}
void enqueue(PQueue &q, uint32_t seqNo, unsigned length)
{
  printf("send %u, %u\n", (unsigned)seqNo, length);
  ZmRef<QMsg> msg = q.rotate(ZmRef<QMsg>(new QMsg(ZuMkPair(seqNo, length))));
  printf("send - head %u gap %u, %u\n", (unsigned)q.head(), (unsigned)q.gap().key(), (unsigned)q.gap().length());
  while (msg) {
    printf("send - process %u, %u\n", (unsigned)msg->Msg::key(), msg->length());
    msg = q.dequeue();
    printf("send - head %u gap %u, %u\n", (unsigned)q.head(), (unsigned)q.gap().key(), (unsigned)q.gap().length());
  }
}

int main()
{
  PQueue q(1);

  enqueue(q, 1, 1);
  enqueue(q, 2, 2);
  enqueue(q, 4, 1);

  enqueue(q, 7, 1);
  enqueue(q, 8, 2);
  enqueue(q, 7, 3); // completely overlaps, should be fully clipped (ignored)
  enqueue(q, 9, 2); // should be head-clipped
  enqueue(q, 12, 2);
  enqueue(q, 10, 3); // should be head- and tail-clipped
  enqueue(q, 6, 3); // should be tail-clipped
  enqueue(q, 4, 3); // should be head- and tail-clipped, trigger dequeue

  enqueue(q, 15, 1);
  assert(q.gap() == ZuMkPair(14, 1));
  enqueue(q, 17, 1);
  enqueue(q, 19, 1);
  enqueue(q, 21, 3);
  enqueue(q, 14, 8); // should overwrite 15,17,19 and be clipped by 21

  enqueue(q, 28, 1);
  enqueue(q, 27, 3); // should overwrite 28
  enqueue(q, 24, 10); // should overwrite 27

  head(q, 1);

  enqueue(q, 2, 1);
  enqueue(q, 3, 1);
  enqueue(q, 5, 1);
  enqueue(q, 7, 1);
  enqueue(q, 8, 2);
  enqueue(q, 10, 1);
  enqueue(q, 11, 3);

  head(q, 12); // should leave 12+2 in place
  enqueue(q, 15, 1);
  assert(q.gap() == ZuMkPair(14, 1));
  dequeue(q);
  assert(q.gap() == ZuMkPair(14, 1));
  enqueue(q, 14, 1);
}
