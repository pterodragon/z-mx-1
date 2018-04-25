//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stddef.h>

#include <ZmList.hpp>

#include <ZtEnum.hpp>

#include <ZiFile.hpp>

#include <ZvCSV.hpp>

struct Row {
  ZuStringN<24>	m_foo;
  bool		m_bar;
  int		m_bah;
  double	m_baz;
  int		m_snafu;
  ZtDate	m_mabbit;
  int		m_flags;
};

struct CSVWrite {
  typedef ZmList<ZuRef<ZuPOD<Row> > > VList;
  CSVWrite() { }

  void pop(ZuRef<ZuAnyPOD> &pod) {
    pod = m_list.shift();
  }

  VList m_list;
};

#ifdef _MSC_VER
#pragma warning(disable:4800)
#endif

struct Snafus {
  ZtEnumValues(
      Sasha = 1, Grey = 42, Girlfriend = 43, Experience = 44, TigerWoods = 45);
  ZtEnumMap(Map,
      "sasha", 1, "grey", 42, "girlfriend", 43, "experience", 44,
      "tiger-woods", 45);
};

struct DaFlags {
  ZtEnumValues(S, A, P, SUP, HI);
  ZtEnumFlags(Map, "S", S, "A", A, "P", P, "SUP", SUP, "HI", HI);
};

int main()
{
  try {
    ZvCSV csv;
    csv.add(new ZvCSVColumn<ZvCSVColType::String, ZuStringN<24> >(
	  "foo", offsetof(Row, m_foo)));
    csv.add(new ZvCSVColumn<ZvCSVColType::Bool, bool>(
	  "bar", offsetof(Row, m_bar)));
    csv.add(new ZvCSVColumn<ZvCSVColType::Int, int>(
	  "bah", offsetof(Row, m_bah)));
    csv.add(new ZvCSVColumn<ZvCSVColType::Float, double>(
	  "baz", offsetof(Row, m_baz), 2));
    csv.add(new ZvCSVEnumColumn<int, Snafus::Map>(
	  "snafu", offsetof(Row, m_snafu)));
    csv.add(new ZvCSVColumn<ZvCSVColType::Time,ZtDate>(
	  "mabbit", offsetof(Row, m_mabbit), ""));
    csv.add(new ZvCSVFlagsColumn<int, DaFlags::Map>(
	  "flags", offsetof(Row, m_flags)));

    CSVWrite filtList;
    CSVWrite unFiltList;

    for (int i = 0; i < 10; i++) {
      ZuRef<ZuPOD<Row> > row = new ZuPOD<Row>();
      Row * r = new (row->ptr()) Row();
      r->m_foo = ZtSprintf("Sup Homie %d", i).data();
      r->m_bar = i % 2;
      r->m_bah = i * 2;
      r->m_baz = i * 2.2;
      switch(i) {
	case 1: r->m_snafu = 1; break;
	case 2: r->m_snafu = 42; break;
	case 3: r->m_snafu = 43; break;
	case 4: r->m_snafu = 44; break;
	case 5: r->m_snafu = 45; break;
	default: r->m_snafu = 99; break;
      }
      r->m_mabbit = ZtDateNow();
      switch(i) {
	case 1: r->m_flags = 0x10 | 0x08; break;
	case 2: r->m_flags = 0x01 | 0x02; break;
	case 3: r->m_flags = 0x04 | 0x08; break;
	case 4: r->m_flags = 0x10; break;
	case 5: r->m_flags = 0x08; break;
	default: r->m_flags = ZuCmp<int>::null(); break;
      }
      switch(i) {
	case 1: r->m_mabbit = ZtDate(2010, 01, 22, 15, 22, 14); break;
	default: break;
      }
      filtList.m_list.push(row);
      unFiltList.m_list.push(row);
    }

    ZtArray<ZtString> filter;
    filter.push("*");
    if (ZvCSVWriteFn fn = csv.writeFile("all.written.csv", filter)) {
      while (ZuRef<ZuAnyPOD> pod = unFiltList.m_list.shift()) fn(pod);
      fn((ZuAnyPOD *)0);
    }
    filter.null();
    filter.push("foo");
    filter.push("flags");
    if (ZvCSVWriteFn fn = csv.writeFile("filt.written.csv", filter)) {
      while (ZuRef<ZuAnyPOD> pod = filtList.m_list.shift()) fn(pod);
      fn((ZuAnyPOD *)0);
    }
  } catch (const ZvError &e) {
    fputs(e.message().data(), stderr);
    fputc('\n', stderr);
    exit(1);
  } catch (const ZeError &e) {
    fputs(e.message(), stderr);
    fputc('\n', stderr);
    exit(1);
  } catch (...) {
    fputs("unknown exception\n", stderr);
    exit(1);
  }
  return 0;
}
