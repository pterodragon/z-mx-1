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

/* ZmPQueueTx unit test */

#include <ZuLib.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <ZuTraits.hpp>
#include <ZuPair.hpp>

#include <ZmPQueue.hpp>
#include <ZmNoLock.hpp>
#include <ZmTime.hpp>
#include <ZmHash.hpp>

typedef ZuPair<uint32_t, unsigned> Msg_Data;
struct Msg_ : public Msg_Data {
  using Msg_Data::Msg_Data;
  using Msg_Data::operator =;
  inline Msg_(const Msg_Data &v) : Msg_Data(v) { }
  inline Msg_(Msg_Data &&v) : Msg_Data(ZuMv(v)) { }
  inline uint32_t key() const { return p1(); }
  inline unsigned length() const { return p2(); }
  inline unsigned clipHead(unsigned length) {
    p1() += length;
    return p2() -= length;
  }
  inline unsigned clipTail(unsigned length) {
    return p2() -= length;
  }
  template <typename I>
  inline void write(const I &i) { }
};

typedef ZmPQueue<Msg_,
	  ZmPQueueLock<ZmNoLock,
	    ZmPQueueNodeIsItem<true> > > Queue;

class App : public ZmPQTx<App, Queue, ZmNoLock> {
public:
  typedef ZmPQTx<App, Queue, ZmNoLock> Tx;
  typedef Queue::Node Msg;
  typedef Queue::Gap Gap;
  typedef Queue::Fn Fn;

  App(uint32_t head) : m_queue(head) { }

  /* -- test interface -- */

  bool runSend() {
    if (!m_sends) return false;
    puts("run send");
    --m_sends;
    this->Tx::send();
    return true;
  }

  bool runResend() {
    if (!m_resends) return false;
    puts("run resend");
    --m_resends;
    this->resend();
    return true;
  }

  bool runArchive() {
    if (!m_archives) return false;
    puts("run archive");
    --m_archives;
    this->archive();
    return true;
  }

  /* -- sender callback interface -- */

  inline Queue &txQueue() { return m_queue; }

  // send message
  bool send_(Msg *msg, bool) {
    printf("send %u, %u\n", (unsigned)msg->key(), msg->length());
    m_sent = msg;
    return true;
  }
  bool resend_(Msg *msg, bool) { // resend
    printf("resend %u, %u\n", (unsigned)msg->key(), msg->length());
    m_resent = msg;
    return true;
  }

  // send gap (optional)
  bool sendGap_(const Gap &gap, bool) {
    printf("sendGap %u, %u\n", (unsigned)gap.key(), (unsigned)gap.length());
    m_sentGap = gap;
    return true;
  }
  bool resendGap_(const Gap &gap, bool) { // resend
    printf("resendGap %u, %u\n", (unsigned)gap.key(), (unsigned)gap.length());
    m_resentGap = gap;
    return true;
  }

  // archive message (once ackd by receiver(s))
  void archive_(Msg *msg) {
    printf("ackd %u, %u\n", (unsigned)msg->key(), msg->length());
    m_ackd = msg;
    archived(msg->key() + msg->length());
  }

  // retrieve message from archive
  ZmRef<Msg> retrieve_(Key key) {
    printf("retrieve %u\n", (unsigned)key);
    if (!m_ackd) return 0;
    Fn item(m_ackd->item());
    if (key >= item.key() && (key - item.key()) < item.length())
      return m_ackd;
    return 0;
  }

  // schedule send() to be called (possibly from different thread)
  void scheduleSend() {
    puts("schedule send");
    ++m_sends;
  }
  void rescheduleSend() { scheduleSend(); }
  void idleSend() { }

  // schedule resend() to be called (possibly from different thread)
  void scheduleResend() {
    puts("schedule resend");
    ++m_resends;
  }
  void rescheduleResend() { scheduleResend(); }
  void idleResend() { }

  // schedule archive() to be called (possibly from different thread)
  void scheduleArchive() {
    puts("schedule archive");
    ++m_archives;
  }
  void rescheduleArchive() { scheduleArchive(); }
  void idleArchive() { }

  bool checkSent(Msg *msg)
    { bool b = m_sent == msg; m_sent = 0; return b; }
  bool checkSentGap(const Gap &gap)
    { bool r = m_sentGap == gap; m_sentGap = Gap(); return r; }
  bool checkResent(Msg *msg)
    { bool b = m_resent == msg; m_resent = 0; return b; }
  bool checkResentGap(const Gap &gap)
    { bool r = m_resentGap == gap; m_resentGap = Gap(); return r; }
  bool checkArchived(Msg *msg) {
    return m_ackd == msg;
  }

protected:
  Queue				m_queue;
  unsigned			m_sends = 0;
  unsigned			m_resends = 0;
  unsigned			m_archives = 0;
  ZmRef<Msg>			m_sent;
  Gap				m_sentGap;
  ZmRef<Msg>			m_resent;
  Gap				m_resentGap;
  ZmRef<Msg>			m_ackd;
};

int main(int argc, char **argv)
{
  ZmHeapMgr::init("ZmPQueue", 0, ZmHeapConfig{0, 100});

  App a(1);
  ZmRef<App::Msg> msg, msg2;

  a.start();

  // basic load, send, resend, ack, resend test
  msg = new App::Msg(App::Gap(1, 1));
  a.send(msg);
  while (a.runSend());
  assert(a.checkSent(msg));
  a.resend(App::Gap(1, 1));
  while (a.runResend());
  assert(a.checkResent(msg));
  a.ackd(2);
  while (a.runArchive());
  assert(a.checkArchived(msg));
  a.resend(App::Gap(1, 1));
  while (a.runResend());
  assert(a.checkResent(msg));

  // load, send, resend, ack, resend test with gap
  msg = new App::Msg(App::Gap(3, 1));
  a.send(msg);
  while (a.runSend());
  assert(a.checkSentGap(App::Gap(2, 1)));
  assert(a.checkSent(msg));
  a.resend(App::Gap(2, 2));
  while (a.runResend());
  assert(a.checkResentGap(App::Gap(2, 1)));
  assert(a.checkResent(msg));
  a.ackd(4);
  while (a.runArchive());
  assert(a.checkArchived(msg));
  a.resend(App::Gap(2, 2));
  while (a.runResend());
  assert(a.checkResentGap(App::Gap(2, 1)));
  assert(a.checkResent(msg));
 
  // load, send, resend, ack, resend test with
  // misaligned partially overlapping resend requests, including gaps
  a.txReset(1);
  msg = new App::Msg(App::Gap(3, 3));
  a.send(msg);
  while (a.runSend());
  assert(a.checkSentGap(App::Gap(1, 2)));
  assert(a.checkSent(msg));
  msg2 = new App::Msg(App::Gap(8, 3));
  a.send(msg2);
  while (a.runSend());
  assert(a.checkSentGap(App::Gap(6, 2)));
  assert(a.checkSent(msg2));
  a.resend(App::Gap(4, 5));
  while (a.runResend());
  assert(a.checkResentGap(App::Gap(6, 2)));
  assert(a.checkResent(msg2));
  a.ackd(4);
  while (a.runArchive());
  assert(a.checkArchived(msg));
  a.resend(App::Gap(4, 5));
  while (a.runResend());
  assert(a.checkResentGap(App::Gap(6, 2)));
  assert(a.checkResent(msg2));

  // resend request spanning unsent data
  a.txReset(1);
  msg = new App::Msg(App::Gap(3, 3));
  a.send(msg);
  msg2 = new App::Msg(App::Gap(8, 3));
  a.send(msg2);
  a.resend(App::Gap(1, 12));
  a.runResend();
  assert(a.checkResentGap(App::Gap(1, 2)));
  assert(a.checkResent(msg));
  a.runResend();
  assert(a.checkResentGap(App::Gap(6, 2)));
  assert(a.checkResent(msg2));
  a.runResend();
  assert(a.checkResentGap(App::Gap(11, 2)));

  std::cout << ZmHeapMgr::csv();
}
