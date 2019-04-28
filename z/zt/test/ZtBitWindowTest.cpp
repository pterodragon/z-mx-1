#include <stdio.h>
#include <limits.h>

#include <ZmRandom.hpp>
#include <ZmAssert.hpp>

#include <ZtBitWindow.hpp>

unsigned head[1000];
unsigned tail[1000];

void shuffle(unsigned *sequence)
{
  for (unsigned i = 0; i < 100000; i++) {
    unsigned j = ZmRand::randInt(999);
    unsigned k = ZmRand::randInt(999);
    unsigned o = sequence[j];
    sequence[j] = sequence[k];
    sequence[k] = o;
  }
}

int main()
{
  for (unsigned i = 0; i < 1000; i++) head[i] = tail[i] = i;
  shuffle(head);
  shuffle(tail);
  ZtBitWindow<1> map;
  for (unsigned i = 0; i < 10000; i++) map.set(i);
  for (int i = 0; i < INT_MAX; i += 1000) {
    for (unsigned j = 0; j < 1000; j++) {
      map.set(i + 10000 + head[j], 2);
      ZmAssert(map.val(i + 10000 + head[j]) == 2);
      map.clr(i + tail[j]);
      ZmAssert(!map.val(i + tail[j]));
    }
    if (!(i % 1000000)) {
      unsigned head = map.head();
      unsigned tail = map.tail();
      printf("head: %d tail: %d size: %d\n", head, tail, tail - head);
    }
  }
}
