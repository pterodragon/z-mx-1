//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

// a HTTP header ends with two CR/LFs
bool httpHeaderEnd(void *ptr, unsigned offset, unsigned length) {
  int n = offset + length - 3;
  if (n <= 0) return false;
  int i = offset - 3;
  if (i < 0) i = 0;
  while (i < n) {
    if (((char *)ptr)[i]      == '\r' && ((char *)ptr)[i + 1] == '\n' &&
	((char *)ptr)[i += 2] == '\r' && ((char *)ptr)[i + 1] == '\n') {
      m_headerLen = i + 2;
      return true;
    }
    ++i;
  }
  return false;
}
