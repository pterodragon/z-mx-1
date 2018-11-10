//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stddef.h>

#include <ZmList.hpp>

#include <ZtEnum.hpp>

#include <ZiFile.hpp>

#include <ZvCSV.hpp>

// 2011/11/11 12:00:00
// 2011/11/11 12:12:12
static const char *testdata =
  "string,int,bool,float,enum,time,flags,func,A,B,C,,\n"
  "string,199,Y,1.234,sasha,2011/11/11 12:12:12,Flag1,A,B,C,D,,,\n"
  "string2,23,N,0.00042,grey,2011/11/11 12:12:12.1234,SUP,,,\n"
  "\"-,>\"\"<,-\",2,,0.0000002,\"\"\"\"girlfriend,,Flag1|Flag2,,,\n"
  "-->\",\"<--,3,N,3.1415926,\"experience\"\"\",,Flag1,,,\n";

struct Row {
  ZuStringN<24>	m_string;
  int		m_int;
  int		m_bool;
  double	m_float;
  int		m_enum;
  ZtDate	m_time;
  int		m_flags;
  int	        m_func;
};

struct CSVWrite {
  typedef ZmList<ZuRef<ZuPOD<Row> > > VList;
  CSVWrite() { }

  VList m_list;
};

struct RowSet {
  void alloc(ZuRef<ZuAnyPOD> &obj) {
    ZuRef<ZuPOD<Row> > row = new ZuPOD<Row>();
    new (row->ptr()) Row();
    m_rows.push(row);
    obj = row;
  }
  ZmList<ZuRef<ZuPOD<Row> > >	m_rows;
};

namespace Enums {
  ZtEnumValues(Sasha = 1, Grey, Girlfriend = 43, Experience, __);
  ZtEnumMap(Map,
      "sasha", 1, "grey", 42, "\"girlfriend", 43, "experience\"", 44, "", 45);
}

namespace DaFlags {
  ZtEnumValues(Flag1, Flag2, P, SUP);
  ZtEnumFlags(Map, "Flag1", 0, "Flag2", 1, "P", 2, "SUP", 3);
}

int main()
{
  try {
    {
      ZiFile file;
      ZeError e;
      if (file.open("in.csv", ZiFile::Create | ZiFile::Truncate,
		    0777, &e) != Zi::OK)
	throw e;
      if (file.write((void *)testdata, (int)strlen(testdata), &e) != Zi::OK)
	throw e;
    }
    {
      ZvCSV csv;

      csv.add(new ZvCSVColumn<ZvCSVColType::String, ZuStringN<24> >(
	    "string", offsetof(Row, m_string)));
      csv.add(new ZvCSVColumn<ZvCSVColType::Int, int>(
	    "int", offsetof(Row, m_int)));
      csv.add(new ZvCSVColumn<ZvCSVColType::Bool, bool>(
	    "bool", offsetof(Row, m_bool)));
      csv.add(new ZvCSVColumn<ZvCSVColType::Float, double>(
	    "float", offsetof(Row, m_float)));
      csv.add(new ZvCSVEnumColumn<int, Enums::Map>(
	    "enum", offsetof(Row, m_enum)));
      csv.add(new ZvCSVColumn<ZvCSVColType::Time, ZtDate>(
	    "time", offsetof(Row, m_time), ""));
      csv.add(new ZvCSVFlagsColumn<int, DaFlags::Map>(
	    "flags", offsetof(Row, m_flags)));
      // csv.add(new ZvCSVColumn<ZvCSVColType::Func,
      // 			      ZtDate>("func", offsetof(Row, m_func), ""));

      RowSet rowSet;
      csv.readFile("in.csv",
	  ZvCSVAllocFn::Member<&RowSet::alloc>::fn(&rowSet));
      ZuRef<ZuPOD<Row> > row;

      CSVWrite filtList;
      CSVWrite unFiltList;

      while (row = rowSet.m_rows.shift()) {
	printf("%s, %d, %c, %f, %s (%d:%d) %i\n", 
	       row->ptr()->m_string.data(),
	       (int)row->ptr()->m_int,
	       row->ptr()->m_bool ? 'Y' : 'N',
	       (double)row->ptr()->m_float,
	       (const char *)ZvEnum<Enums::Map>::instance()->v2s(
		 "enum", row->ptr()->m_enum),
	       (row->ptr()->m_time).yyyymmdd(),
	       (row->ptr()->m_time).hhmmss(),
	       (int)row->ptr()->m_flags);
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
      filter.push("string");
      filter.push("flags");
      if (ZvCSVWriteFn fn = csv.writeFile("filt.written.csv", filter)) {
	while (ZuRef<ZuAnyPOD> pod = filtList.m_list.shift()) fn(pod);
	fn((ZuAnyPOD *)0);
      }
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
