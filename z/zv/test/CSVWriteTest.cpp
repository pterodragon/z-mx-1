//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <zlib/ZuLib.hpp>

#include <stddef.h>

#include <zlib/ZmList.hpp>

#include <zlib/ZtEnum.hpp>

#include <zlib/ZiFile.hpp>

#include <zlib/ZvCSV.hpp>

namespace Snafus {
  ZtEnumValues(
      Sasha = 1, Grey = 42, Girlfriend = 43, Experience = 44, TigerWoods = 45);
  ZtEnumMap(Map,
      "sasha", 1, "grey", 42, "girlfriend", 43, "experience", 44,
      "tiger-woods", 45);
}

namespace DaFlags {
  ZtEnumValues(S, A, P, SUP, HI);
  ZtEnumFlags(Map, "S", S, "A", A, "P", P, "SUP", SUP, "HI", HI);
}

struct Row {
  static const ZvFieldArray fields() noexcept;

  ZuStringN<24>	foo;
  bool		bar;
  int		bah;
  double	baz;
  ZuFixedVal	bam;
  int		snafu;
  ZtDate	mabbit;
  int		flags;
};
inline const ZvFieldArray Row::fields() noexcept {
  ZvFields(Row,
      (String, foo, 0),
      (Bool, bar, 0),
      (Float, baz, 0, 2),
      (Fixed, bam, 0, 2),
      (Int, snafu, 0),
      (Time, mabbit, 0),
      (Flags, flags, 0, DaFlags::Map));
}

using CSVWrite = ZmList<ZuRef<ZuPOD<Row> > >;

int main()
{
  try {
    ZvCSV<Row> csv;

    CSVWrite filtList;
    CSVWrite unFiltList;

    for (int i = 0; i < 10; i++) {
      ZuRef<ZuPOD<Row> > row = new ZuPOD<Row>();
      Row * r = new (row->ptr()) Row();
      r->foo = ZtSprintf("Sup Homie %d", i).data();
      r->bar = i % 2;
      r->bah = i * 2;
      r->baz = i * 2.2;
      r->bam = ZuFixed{r->baz * 2.2, 2}.value;
      switch(i) {
	case 1: r->snafu = 1; break;
	case 2: r->snafu = 42; break;
	case 3: r->snafu = 43; break;
	case 4: r->snafu = 44; break;
	case 5: r->snafu = 45; break;
	default: r->snafu = 99; break;
      }
      r->mabbit = ZtDateNow();
      switch(i) {
	case 1: r->flags = 0x10 | 0x08; break;
	case 2: r->flags = 0x01 | 0x02; break;
	case 3: r->flags = 0x04 | 0x08; break;
	case 4: r->flags = 0x10; break;
	case 5: r->flags = 0x08; break;
	default: r->flags = ZuCmp<int>::null(); break;
      }
      switch(i) {
	case 1: r->mabbit = ZtDate(2010, 01, 22, 15, 22, 14); break;
	default: break;
      }
      filtList.push(row);
      unFiltList.push(row);
    }

    ZtArray<ZtString> filter;
    filter.push("*");
    {
      auto fn = csv.writeFile("all.written.csv", filter);
      while (ZuRef<ZuPOD<Row>> pod = unFiltList.shift())
	fn(pod->ptr());
      fn(nullptr);
    }
    filter.null();
    filter.push("foo");
    filter.push("flags");
    {
      auto fn = csv.writeFile("filt.written.csv", filter);
      while (ZuRef<ZuPOD<Row>> pod = filtList.shift())
	fn(pod->ptr());
      fn(nullptr);
    }
  } catch (const ZvError &e) {
    std::cerr << "ZvError: " << e << '\n';
    ZmPlatform::exit(1);
  } catch (const ZeError &e) {
    std::cerr << "ZeError: " << e << '\n';
    ZmPlatform::exit(1);
  } catch (...) {
    std::cerr << "unknown exception\n";
    ZmPlatform::exit(1);
  }
  return 0;
}
