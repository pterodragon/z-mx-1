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

// CSV parser/generator

// Microsoft Excel compatible quoting: a, " ,"",",b -> a| ,",|b
// Unlike Excel, leading white space is discarded if not quoted

#ifndef ZvCSV_HPP
#define ZvCSV_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#include <ZuBox.hpp>
#include <ZuString.hpp>

#include <ZuPOD.hpp>
#include <ZuRef.hpp>

#include <ZmObject.hpp>
#include <ZmRBTree.hpp>
#include <ZmFn.hpp>

#include <ZtArray.hpp>
#include <ZtDate.hpp>
#include <ZtString.hpp>
#include <ZtRegex.hpp>

#include <ZePlatform.hpp>

#include <ZvError.hpp>
#include <ZvDateError.hpp>
#include <ZvEnum.hpp>

#define ZvCSV_MaxLineSize	(8<<10)	// 8K

struct ZvCSVColType {
  enum {
    String,
    Int,
    Bool,
    Float,
    Enum,
    Time,
    Flags,
    Func
  };
};

class ZvCSVColumn_ : public ZuObject {
public:
  struct IDAccessor : public ZuAccessor<ZvCSVColumn_ *, const ZtString &> {
    inline static const ZtString &value(const ZvCSVColumn_ *c) {
      return c->id();
    }
  };

  template <typename ID>
  inline ZvCSVColumn_(const ID &id, int offset) :
      m_id(id), m_offset(offset), m_index(-1) { }
  virtual ~ZvCSVColumn_() { }

  inline const ZtString &id() const { return m_id; }
  inline int offset() const { return m_offset; }
  inline int index() const { return m_index; }

  inline void index(int i) { m_index = i; }

  virtual void parse(ZuString value, ZuAnyPOD *pod) const = 0;
  virtual void place(ZtArray<char> &row, ZuAnyPOD *pod) const = 0;

  static void quote(ZtArray<char> &row, ZuString s) {
    row << '"';
    quote2(row, s);
    row << '"';
  }
  static void quote2(ZtArray<char> &row, ZuString s) {
    for (unsigned i = 0, n = s.length(); i < n; i++) {
      char ch = s[i];
      row << ch;
      if (ZuUnlikely(ch == '"')) row << '"'; // double-up quotes within quotes
    }
  }

protected:
  ZtString	m_id;
  int		m_offset;
  int		m_index;
};

template <int ColType_, typename T_> class ZvCSVColumn;
template <typename T_, typename Map_> class ZvCSVEnumColumn;
template <typename T_, typename Flags_> class ZvCSVFlagsColumn;

template <typename T_>
class ZvCSVColumn<ZvCSVColType::String, T_> : public ZvCSVColumn_ {
public:
  typedef T_ T;
  enum { ColType = ZvCSVColType::String };

  template <typename ID>
  inline ZvCSVColumn(ID &&id, int offset) :
    ZvCSVColumn_(ZuFwd<ID>(id), offset) { }

  void parse(ZuString value, ZuAnyPOD *pod) const {
    *((T *)((char *)pod->ptr() + this->m_offset)) = value;
  }
  void place(ZtArray<char> &row, ZuAnyPOD *pod) const {
    ZuString s(*((const T *)((char *)pod->ptr() + this->m_offset)));
    this->quote(row, s);
  }
};

template <typename T_>
class ZvCSVColumn<ZvCSVColType::Int, T_> : public ZvCSVColumn_ {
public:
  typedef T_ T;
  typedef typename ZuBoxT<T>::T Box;
  typedef typename Box::Cmp Cmp;
  enum { ColType = ZvCSVColType::Int };

  template <typename ID>
  inline ZvCSVColumn(ID &&id, int offset, int width = -1,
      T deflt = Cmp::null()) :
      ZvCSVColumn_(ZuFwd<ID>(id), offset), m_deflt(deflt) {
    if (width >= 0) m_fmt.right(width, '0');
  }

  inline ZvCSVColumn &deflt(T value) { m_deflt = value; return *this; }

  void parse(ZuString value, ZuAnyPOD *pod) const {
    T *ptr = (T *)((char *)pod->ptr() + this->m_offset);
    if (!value)
      *ptr = m_deflt;
    else
      *ptr = Box(value);
  }
  void place(ZtArray<char> &row, ZuAnyPOD *pod) const {
    const Box &v = *(const Box *)((const char *)pod->ptr() + this->m_offset);
    if (*v) row << v.vfmt(m_fmt);
  }

private:
  mutable ZuVFmt	m_fmt;
  T			m_deflt;
};

template <typename T_>
class ZvCSVColumn<ZvCSVColType::Bool, T_> : public ZvCSVColumn_ {
public:
  typedef T_ T;
  enum { ColType = ZvCSVColType::Bool };

  template <typename ID>
  inline ZvCSVColumn(ID &&id, int offset, T deflt = false) :
    ZvCSVColumn_(ZuFwd<ID>(id), offset), m_deflt(deflt) { }

  void parse(ZuString value, ZuAnyPOD *pod) const {
    T *ptr = (T *)((char *)pod->ptr() + this->m_offset);
    if (!value)
      *ptr = m_deflt;
    else
      *ptr = value.length() == 1 && value[0] == 'Y';
  }
  void place(ZtArray<char> &row, ZuAnyPOD *pod) const {
    const T &v = *(const T *)((const char *)pod->ptr() + this->m_offset);
    row << (v ? 'Y' : 'N');
  }

private:
  T			m_deflt;
};

template <typename T_>
class ZvCSVColumn<ZvCSVColType::Float, T_> : public ZvCSVColumn_ {
public:
  typedef T_ T;
  typedef typename ZuBoxT<T>::T Box;
  typedef typename Box::Cmp Cmp;
  enum { ColType = ZvCSVColType::Float };

  template <typename ID>
  inline ZvCSVColumn(ID &&id, int offset, int width = -1) :
      ZvCSVColumn_(ZuFwd<ID>(id), offset) {
    if (width >= 0) m_fmt.right(width, '0');
  }

  void parse(ZuString value, ZuAnyPOD *pod) const {
    *((T *)((char *)pod->ptr() + this->m_offset)) = Box(value);
  }
  void place(ZtArray<char> &row, ZuAnyPOD *pod) const {
    const Box &v = *(const Box *)((const char *)pod->ptr() + this->m_offset);
    if (*v) row << v.vfmt(m_fmt);
  }

private:
  mutable ZuVFmt	m_fmt;
};

template <typename T_, typename Map_>
class ZvCSVEnumColumn : public ZvCSVColumn_ {
public:
  typedef T_ T;
  typedef typename ZuBoxT<T>::T Box;
  typedef typename Box::Cmp Cmp;
  typedef Map_ Map;
  enum { ColType = ZvCSVColType::Enum };

  template <typename ID>
  inline ZvCSVEnumColumn(ID &&id, int offset, T deflt = Cmp::null()) :
    ZvCSVColumn_(ZuFwd<ID>(id), offset),
    m_map(ZvEnum<Map>::instance()), m_deflt(deflt) { }

  inline ZvCSVEnumColumn &deflt(T value) { m_deflt = value; return *this; }

  void parse(ZuString value, ZuAnyPOD *pod) const {
    void *ptr = (char *)pod->ptr() + this->m_offset;
    *(T *)ptr = m_map->s2v(id(), value, m_deflt);
  }
  void place(ZtArray<char> &row, ZuAnyPOD *pod) const {
    const Box &v = *(const Box *)((const char *)pod->ptr() + this->m_offset);
    try {
      this->quote(row, m_map->v2s(id(), v));
    } catch (...) { }
  }

private:
  ZvEnum<Map>	*m_map;
  T		m_deflt;
};

template <typename T_>
class ZvCSVColumn<ZvCSVColType::Time, T_> : public ZvCSVColumn_ {
public:
  typedef T_ T;
  enum { ColType = ZvCSVColType::Time };

  // provide a constant (fixed) TZ offset
  template <typename ID, typename TZ>
  inline ZvCSVColumn(ID &&id, int offset, TZ tzOffset,
	typename ZuSame<int, TZ>::T *_ = 0) :
      ZvCSVColumn_(ZuFwd<ID>(id), offset), m_tzOffset(tzOffset), m_tz(0) {
    m_fmt.offset(tzOffset);
  }

  // provide a TZ name (variable offset since dates may differ in DST)
  template <typename ID, typename TZ>
  inline ZvCSVColumn(ID &&id, int offset, TZ &&tz,
	typename ZuIsString<TZ>::T *_ = 0) :
      ZvCSVColumn_(id, offset), m_tzOffset(0), m_tz(ZuFwd<TZ>(tz)) { }

  void parse(ZuString value, ZuAnyPOD *pod) const {
    void *ptr = (char *)pod->ptr() + this->m_offset;
    if (!value) { *(T *)ptr = ZtDate(); return; }
    ZtDate v;
    if (m_tzOffset)
      v = ZtDate(ZtDate::CSV, value, m_tzOffset);
    else
      v = ZtDate(ZtDate::CSV, value, m_tz);
    if (!v) throw ZvDateError(value);
    *(T *)ptr = v;
  }
  void place(ZtArray<char> &row, ZuAnyPOD *pod) const {
    ZtDate d = *(const T *)((const char *)pod->ptr() + this->m_offset);
    if (!d) return;
    if (m_tz) m_fmt.offset(d.offset(m_tz));
    row << d.csv(m_fmt);
  }

private:
  int	  			m_tzOffset;
  ZtString			m_tz;
  mutable ZtDateFmt::CSV	m_fmt;
};

template <typename T_, typename Map_>
class ZvCSVFlagsColumn : public ZvCSVColumn_ {
public:
  typedef T_ T;
  typedef Map_ Map;
  enum { ColType = ZvCSVColType::Flags };

  template <typename ID>
  inline ZvCSVFlagsColumn(ID &&id, int offset, T deflt = 0) :
    ZvCSVColumn_(ZuFwd<ID>(id), offset),
    m_map(ZvFlags<Map>::instance()), m_deflt(deflt) { }

  void parse(ZuString value, ZuAnyPOD *pod) const {
    void *ptr = (char *)pod->ptr() + this->m_offset;
    if (!value) { *(T *)ptr = m_deflt; return; }
    *(T *)ptr = m_map->template scan<T>(id(), value);
  }
  void place(ZtArray<char> &row, ZuAnyPOD *pod) const {
    const T &v = *(const T *)((const char *)pod->ptr() + this->m_offset);
    if (!v) return;
    ZtString s;
    m_map->print(id(), s, v);
    this->quote(row, s);
  }

private:
  ZvFlags<Map>	*m_map;
  T		m_deflt;
};

template <typename T_>
class ZvCSVColumn<ZvCSVColType::Func, T_> : public ZvCSVColumn_ {
public:
  typedef T_ T;
  typedef ZmFn<T *, ZuString> ParseFn;
  typedef ZmFn<ZtArray<char> &, const T *> PlaceFn;

  enum { ColType = ZvCSVColType::Func };

  template <typename ID>
  inline ZvCSVColumn(ID &&id, int offset, ParseFn parseFn, PlaceFn placeFn) :
      ZvCSVColumn_(ZuFwd<ID>(id), offset),
      m_parseFn(parseFn), m_placeFn(placeFn) {
    ZmAssert(!!m_parseFn && !!m_placeFn);
  }

  void parse(ZuString value, ZuAnyPOD *pod) const {
    void *ptr = (char *)pod->ptr() + this->m_offset;
    m_parseFn((T *)ptr, value);
  }
  void place(ZtArray<char> &row, ZuAnyPOD *pod) const {
    const void *ptr = (const char *)pod->ptr() + this->m_offset;
    m_placeFn(row, (const T *)ptr);
  }

private:
  ParseFn	m_parseFn;
  PlaceFn	m_placeFn;
};

typedef ZmFn<ZuRef<ZuAnyPOD> &> ZvCSVAllocFn;
typedef ZmFn<ZuAnyPOD *> ZvCSVReadFn;
typedef ZmFn<ZuAnyPOD *> ZvCSVWriteFn;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

class ZvAPI ZvCSV {
  ZvCSV(const ZvCSV &);
  ZvCSV &operator =(const ZvCSV &);	// prevent mis-use

  typedef ZtArray<ZuString> ColNames;
  typedef ZtArray<const ZvCSVColumn_ *> CColArray;
  typedef ZtArray<ZvCSVColumn_ *> ColArray;
  typedef ZtArray<int> ColIndex;

  struct ColTree_HeapID {
    inline static const char *id() { return "ZvCSV.ColTree"; }
  };
  typedef ZmRBTree<ZuRef<ZvCSVColumn_>,
	    ZmRBTreeIndex<ZvCSVColumn_::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock,
		  ZmRBTreeHeapID<ColTree_HeapID> > > > > ColTree;

public:
  inline ZvCSV() { }
  virtual ~ZvCSV();

  inline void add(const ZuRef<ZvCSVColumn_> &c) {
    c->index(m_colArray.length());
    m_colArray.push(c);
    m_colTree.add(c);
  }

  template <typename ID>
  inline const ZvCSVColumn_ *find(ID &&id) const {
    return m_colTree.findKey(ZuFwd<ID>(id));
  }

  inline const ZvCSVColumn_ *column(int i) const {
    int n = m_colArray.length();
    if (i < 0 || i >= n) return 0;
    return m_colArray[i];
  }

private:
  void split(ZuString row, ZtArray<ZtArray<char> > &a);

  void header(ColIndex &colIndex, ZuString hdr) {
    ZtArray<ZtArray<char> > a;
    split(hdr, a);
    unsigned n = m_colArray.length();
    colIndex.null();
    colIndex.length(n);
    for (unsigned i = 0; i < n; i++) colIndex[i] = -1;
    n = a.length();
    for (unsigned i = 0; i < n; i++)
      if (const ZvCSVColumn_ *col = find(a[i]))
	colIndex[col->index()] = i;
  }

  void parse(const ColIndex &colIndex, ZuString row, ZuAnyPOD *pod) {
    ZtArray<ZtArray<char> > a;
    split(row, a);
    unsigned n = colIndex.length();
    for (unsigned i = 0; i < n; i++) {
      int j;
      if ((j = colIndex[i]) < 0 || j >= (int)a.length())
        m_colArray[i]->parse(ZuString(), pod);
    }
    for (unsigned i = 0; i < n; i++) {
      int j;
      if ((j = colIndex[i]) >= 0 && j < (int)a.length())
	m_colArray[i]->parse(a[j], pod);
    }
  }

  CColArray columns(const ZtArray<ZtString> &names) const {
    if (!names.length() || (names.length() == 1 && names[0] == "*"))
      return m_colArray;
    CColArray colArray;
    unsigned n = names.length();
    colArray.size(n);
    for (unsigned i = 0; i < n; i++)
      if (const ZvCSVColumn_ *col = find(names[i])) colArray.push(col);
    return colArray;
  }

public:
  class ZvAPI FileIOError : public ZvError {
  public:
    template <typename FileName>
    FileIOError(const FileName &fileName, ZeError e) :
      m_fileName(fileName), m_error(e) { }

    void print_(ZmStream &s) const {
      s << "\"" << m_fileName << "\" " << m_error;
    }

  private:
    ZtString	m_fileName;
    ZeError	m_error;
  };

  void readFile(const char *fileName,
      ZvCSVAllocFn allocFn, ZvCSVReadFn readFn = ZvCSVReadFn());
  void readData(ZuString data,
      ZvCSVAllocFn allocFn, ZvCSVReadFn readFn = ZvCSVReadFn());

  ZvCSVWriteFn writeFile(const char *fileName, const ColNames &columns);
  inline ZvCSVWriteFn writeFile(const char *fileName) {
    return writeFile(fileName, ColNames());
  }
  ZvCSVWriteFn writeData(ZtString &data, const ColNames &columns);
  inline ZvCSVWriteFn writeData(ZtString &data) {
    return writeData(data, ColNames());
  }

  void writeHeaders(ColNames &headers);

private:
  void writeFile_(
      CColArray colArray, ZtArray<char> *row,
      FILE *file, ZuAnyPOD *pod);
  void writeData_(
      CColArray colArray, ZtArray<char> *row,
      ZtString &data, ZuAnyPOD *pod);

  ColArray	m_colArray;
  ColTree	m_colTree;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZvCSV_HPP */
