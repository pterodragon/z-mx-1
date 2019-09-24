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

// CSV parser

#include <stdio.h>
#include <ctype.h>

#include <ZePlatform.hpp>

#include <ZvCSV.hpp>
#include <ZvRegexError.hpp>
#include <ZvDateError.hpp>

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#define RowBufSize 8100

ZvCSV::~ZvCSV()
{
}

void ZvCSV::readFile(
    const char *fileName, ZvCSVAllocFn allocFn, ZvCSVReadFn readFn)
{
  FILE *file;

  if (!(file = fopen(fileName, "r")))
    throw FileIOError(fileName, ZeLastError);

  ColIndex colIndex;
  try {
    ZtString row(ZvCSV_MaxLineSize);
    if (char *data = fgets(row.data(), ZvCSV_MaxLineSize - 1, file)) {
      row[ZvCSV_MaxLineSize - 1] = 0;
      row.calcLength();
      row.chomp();
      header(colIndex, row);
      row.length(0);
      while (data = fgets(row.data(), ZvCSV_MaxLineSize - 1, file)) {
	row[ZvCSV_MaxLineSize - 1] = 0;
	row.calcLength();
	row.chomp();
        if (row.length()) {
          ZuRef<ZuAnyPOD> pod;
          allocFn(pod);
          if (!pod) break;
          parse(colIndex, row, pod);
          if (!!readFn) readFn(pod);
          row.length(0);
        }
      }
    }
  } catch (const ZtRegex::Error &e) {
    fclose(file);
    throw ZvRegexError(e);
  } catch (const ZvDateError &e) {
    fclose(file);
    throw e;
  } catch (...) {
    fclose(file);
    throw;
  }

  fclose(file);
}

void ZvCSV::readData(ZuString data, ZvCSVAllocFn allocFn, ZvCSVReadFn readFn)
{
  ColIndex colIndex;
  try {
    const auto &newLine = ZtStaticRegexUTF8("\\n");
    ZtRegex::Captures rows;
    if (!newLine.split(data, rows)) return;
    ZtString row(ZvCSV_MaxLineSize);
    row.init(rows[0].data(), rows[0].length());
    row.chomp();
    header(colIndex, row);
    row.length(0);
    for (unsigned i = 1; i < rows.length(); i++) {
      row.init(rows[i].data(), rows[i].length());
      row.chomp();
      ZuRef<ZuAnyPOD> pod;
      allocFn(pod);
      if (!pod) break;
      parse(colIndex, row, pod);
      if (!!readFn) readFn(pod);
      row.length(0);
    }
  } catch (const ZtRegex::Error &e) {
    throw ZvRegexError(e);
  } catch (const ZvDateError &e) {
    throw e;
  }
}

ZvCSVWriteFn ZvCSV::writeFile(const char *fileName, const ColNames &columns)
{
  FILE *file;

  if (!strcmp(fileName, "&1"))
    file = stdout;
  else if (!strcmp(fileName, "&2"))
    file = stderr;
  else if (!(file = fopen(fileName, "w")))
    throw FileIOError(fileName, ZeLastError);

  CColArray colArray = this->columns(columns);

  ZtArray<char> *row = new ZtArray<char>(RowBufSize);

  for (unsigned i = 0, n = colArray.length(); i < n; i++) {
    *row << colArray[i]->id();
    if (i < n - 1) *row << ',';
  }
  if (!*row) {
    fclose(file);
    return ZvCSVWriteFn();
  }
  *row << '\n';
  fwrite(row->data(), 1, row->length(), file);

  return ZvCSVWriteFn(
      [this, colArray = ZuMv(colArray), row, file](ZuAnyPOD *pod) {
	this->writeFile_(ZuMv(colArray), row, file, pod);
      });
}

void ZvCSV::writeFile_(
    CColArray colArray, ZtArray<char> *row, FILE *file, ZuAnyPOD *pod)
{
  if (ZuUnlikely(!pod)) {
    delete row;
    fclose(file);
    return;
  }

  row->length(0);
  for (unsigned i = 0, n = colArray.length(); i < n; i++) {
    colArray[i]->place(*row, pod);
    if (i < n - 1) *row << ',';
  }
  *row << '\n';
  fwrite(row->data(), 1, row->length(), file);
}

ZvCSVWriteFn ZvCSV::writeData(ZtString &data, const ColNames &columns)
{
  CColArray colArray = this->columns(columns);

  ZtArray<char> *row = new ZtArray<char>(RowBufSize);

  for (unsigned i = 0, n = colArray.length(); i < n; i++) {
    *row << colArray[i]->id();
    if (i < n - 1) *row << ',';
  }
  if (!*row) return ZvCSVWriteFn();
  *row << '\n';
  data << *row;

  return ZvCSVWriteFn(
      [this, colArray = ZuMv(colArray), row, &data](ZuAnyPOD *pod) {
	this->writeData_(ZuMv(colArray), row, data, pod);
      });
}

void ZvCSV::writeData_(
    CColArray colArray, ZtArray<char> *row, ZtString &data, ZuAnyPOD *pod)
{
  if (ZuUnlikely(!pod)) {
    delete row;
    return;
  }

  row->length(0);
  for (unsigned i = 0, n = colArray.length(); i < n; i++) {
    colArray[i]->place(*row, pod);
    if (i < n - 1) *row << ',';
  }
  *row << '\n';
  data << *row;
}

void ZvCSV::split(ZuString row, ZtArray<ZtArray<char> > &values)
{
  enum { Value = 0, Quoted, Quoted2, EOV, EOL };
  int state;
  char ch;
  int offset = 0, length = row.length();
  int start, end, next;
  bool simple;

  for (;;) {
    start = offset;
    state = Value;
    simple = true;

    for (;;) {
      ch = offset >= length ? 0 : row[offset];
      switch (state) {
	case Value:
	  if (!ch) { end = offset; state = EOL; break; }
	  if (ch == '"') { simple = false; state = Quoted; break; }
	  if (ch == ',') { end = offset; state = EOV; break; }
	  break;
	case Quoted:
	  if (!ch) { end = offset; state = EOL; break; }
	  if (ch == '"') { state = Quoted2; break; }
	  break;
	case Quoted2:
	  if (!ch) { end = offset; state = EOL; break; }
	  if (ch == '"') { state = Quoted; break; }
	  if (ch == ',') { end = offset; state = EOV; break; }
	  state = Value;
	  break;
      }
      if (state == EOV || state == EOL) break;
      offset++;
    }

    if (state == EOV) {
      while (offset < length && isspace(row[++offset]));
      next = offset;
    } else // state == EOL
      next = -1;

    if (simple) {
      new (values.push()) ZtArray<char>(row.data() + start, end - start);
    } else {
      ZtArray<char> value;
      value.size(end - start);
      state = Value;
      offset = start;
      while (offset < end) {
	ch = row[offset++];
	switch (state) {
	  case Value:
	    if (ch == '"') { state = Quoted; break; }
	    value += ch;
	    break;
	  case Quoted:
	    if (ch == '"') { state = Quoted2; break; }
	    value += ch;
	    break;
	  case Quoted2:
	    if (ch == '"') { value += ch; state = Quoted; break; }
	    value += ch;
	    state = Value;
	    break;
	}
      }
      values.push(ZuMv(value));
    }

    if (next < 0) break;
    offset = next;
  }
}

void ZvCSV::writeHeaders(ZtArray<ZuString> &headers)
{
  unsigned n = m_colArray.length();
  headers.size(n);
  for (unsigned i = 0; i < n; i++) headers.push(m_colArray[i]->id());
}
