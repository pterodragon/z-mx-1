//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <zlib/ZuLib.hpp>

#include <stdio.h>
#include <string.h>

#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmQueue.hpp>

typedef ZmQueue<int, ZmQueueID<unsigned char> > Queue;
typedef ZmQueue<int> Queue2;

void check(Queue &queue, int l)
{
  Queue::Iterator i(queue);
  int j = 0, k;
  while (!ZuCmp<int>::null(k = i.iterate())) {
    if (k != l) printf("BAD %d: %d != %d\n", j, k, l);
    j++, l++;
  }
}

void check(Queue &queue, int *l, int m)
{
  Queue::Iterator i(queue);
  int j = 0, k;
  while (!ZuCmp<int>::null(k = i.iterate())) {
    if (j >= m) {
      printf("BAD %d >= %d: %d\n", j, m, k);
    } else {
      if (k != l[j]) printf("BAD %d: %d != %d\n", j, k, l[j]);
    }
    j++;
  }
}

int main(int argc, char **argv)
{
  {
    Queue queue;

    for (int n = 0; n < 10; n++) {
      puts("push 42...");
      for (int i = 0; i < 42; i++) queue.push(i);
      check(queue, 0);
      puts("shift 21...");
      for (int i = 0; i < 21; i++) queue.shift();
      check(queue, 21);
      puts("unshift 21...");
      for (int i = 20; i >= 0; --i) queue.unshift(i);
      check(queue, 0);
      puts("del every 3rd...");
      for (int i = 0; i < 42; i += 3) queue.del(i);
      {
	int l[] = { 1, 2, 4, 5, 7, 8, 10, 11, 13, 14, 16, 17, 19, 20, 22, 23,
		    25, 26, 28, 29, 31, 32, 34, 35, 37, 38, 40, 41 };
	check(queue, l, sizeof(l) / sizeof(l[0]));
      }
      puts("iterator.del every 2nd...");
      {
	Queue::Iterator i(queue);
	int j = 0, k;
	while (!ZuCmp<int>::null(k = i.iterate())) if (!(j++ & 1)) i.del();
      }
      {
	int l[] = { 2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41 };
	check(queue, l, sizeof(l) / sizeof(l[0]));
      }
      puts("iterator.del all...");
      {
	Queue::Iterator i(queue);
	int k;
	while (!ZuCmp<int>::null(k = i.iterate())) i.del();
      }
      printf("head: %d tail: %d\n", (int)queue.head(), (int)queue.tail());
    }
  }

  {
    Queue2 q2;

    for (int i = 0; i < 10000; i++) q2.push(i);
    puts("q2 push() done"); fflush(stdout);
    for (int i = 0; i < 5000; i++)
      if (i != q2.shift()) {
	printf("shift() failed @%d\n", i);
	break;
      }
    for (int i = 5000; i < 10000; i++)
      if (i != q2.shift()) {
	printf("shift() failed @%d\n", i);
	break;
      }
    puts("q2 shift() done"); fflush(stdout);
  }
}
