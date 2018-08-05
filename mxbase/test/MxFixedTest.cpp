#include <ZuStringN.hpp>

#include <MxBase.hpp>

#include <iostream>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

int main()
{
  // check basic string scan
  CHECK((double)(MxFixedNDP{"1000.42", 4}.floating()) == 1000.42);
  CHECK((double)(MxFixedNDP{"-1000.42", 4}.floating()) == -1000.42);
  // check basic value/ndp scanning
  {
    auto v = MxFixedNDP{"1000.42", 4};
    CHECK(v.value == 10004200);
    CHECK(v.ndp == 4);
    CHECK((double)(v.floating()) == 1000.42);
    v = MxFixedNDP{"-1000.42", 1};
    CHECK(v.value == -10004);
    CHECK(v.ndp == 1);
    CHECK((double)(v.floating()) == -1000.4);
  }
  // check leading/trailing zeros
  CHECK((double)(MxFixedNDP{"001", 10}.floating()) == 1.0);
  CHECK((double)(MxFixedNDP{"1.000", 10}.floating()) == 1.0);
  CHECK((double)(MxFixedNDP{"001.000", 10}.floating()) == 1.0);
  CHECK((double)(MxFixedNDP{"00.100100100", 18}.floating()) == .1001001);
  CHECK((double)(MxFixedNDP{"0.10010010", 18}.floating()) == .1001001);
  CHECK((double)(MxFixedNDP{".1001001", 18}.floating()) == .1001001);
  {
    // check basic multiply
    MxFloat v = (MxFixedNDP{"1000.42", 4} * MxFixedNDP{2.5, 2}).floating();
    CHECK((double)v == 2501.05);
    v = (MxFixedNDP{"-1000.42", 3} * MxFixedNDP{2.5, 3}).floating();
    CHECK((double)v == -2501.05);
    v = (MxFixedNDP{"1000.42", 2} * MxFixedNDP{-2.5, 4}).floating();
    CHECK((double)v == -2501.05);
    v = (MxFixedNDP{"-1000.42", 1} * MxFixedNDP{-2.5, 5}).floating();
    CHECK((double)v == 2501.0);
  }
  {
    // check overflow multiply
    auto v = (MxFixedNDP{"10", 16} * MxFixedNDP{"10", 16}).value;
    CHECK(!*v);
    v = (MxFixedNDP{"10", 15} * MxFixedNDP{"10", 15}).value;
    CHECK((double)(MxFixedNDP{v, 15}.floating()) == 100.0);
  }
  {
    // check underflow multiply
    auto v = (MxFixedNDP{".1", 1} * MxFixedNDP{".1", 1}).value;
    CHECK(!v);
    v = (MxFixedNDP{".1", 2} * MxFixedNDP{".1", 2}).value;
    CHECK((double)(MxFixedNDP{v, 2}.floating()) == .01);
  }
  // check overflow strings
  CHECK((!MxFixedNDP{"", 4}));
  CHECK((!*MxFixedNDP{"1", 19}));
  CHECK((!*MxFixedNDP{"100", 17}));
  // check underflow strings
  CHECK((!MxFixedNDP{"1", 18}));
  CHECK((!MxFixedNDP{".1", 0}.value));
  CHECK((!MxFixedNDP{"000.010", 1}.value));
  // check formatted printing
  {
    ZuStringN<32> s;
    s << MxFixedNDP{"42000.42", 2}.fmt(ZuFmt::Comma<>());
    CHECK(s == "42,000.42");
  }
}
