//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

// generic quick sort
// - no STL iterator cruft
// - optimized array memory operations
// - median-of-three pivot
// - insertion sort for partitions of 8 items or less

#ifndef ZuSort_HPP
#define ZuSort_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuCmp.hpp>
#include <zlib/ZuArrayFn.hpp>

namespace ZuSort_ {

enum { isort_N = 8 };	// threshold for insertion sort

template <typename T, typename Cmp>
void isort_(T *base, unsigned n) {
  using Fn = ZuArrayFn<T>;
  T *end = base + n; // past end
  T *minimum = base, *ptr = base;
  // find minimum
  while (++ptr < end)
    if (Cmp::cmp(*minimum, *ptr) > 0)
      minimum = ptr;
  // swap minimum into base
  if (minimum != base) {
    ZuSwap(*minimum, *base);
    minimum = base;
  }
  // standard insertion sort
  while ((ptr = ++minimum) < end) {
    while (Cmp::cmp(*--ptr, *minimum) > 0);
    if (++ptr != minimum) {
      T tmp = ZuMv(*minimum);
      Fn::moveItems(ptr + 1, ptr, minimum - ptr);
      *ptr = ZuMv(tmp);
    }
  }
}

template <typename T, typename Cmp>
void qsort_(T *base, unsigned n) {
loop:
  T *middle = base + (n>>1);
  unsigned i = 0;
  {
    T *end = base + (n - 1); // at end (not past)
    // find pivot - median of base, middle, end
    T *ptr;
    if (Cmp::cmp(*base, *middle) >= 0)
      ptr = base;
    else
      ptr = middle, middle = base;
    if (Cmp::cmp(*ptr, *end) > 0)
      ptr = Cmp::cmp(*middle, *end) >= 0 ? middle : end;
    // swap pivot into base
    if (ptr != base) {
      ZuSwap(*ptr, *base);
      ptr = base;
    }
    // standard quicksort & check for flat partition
    middle = base;
    unsigned j = 1;
    int k;
    while (++ptr <= end) {
      if (!(k = Cmp::cmp(*ptr, *base)))
	j++;
      if (k < 0) {
	if (++middle != ptr)
	  ZuSwap(*middle, *ptr);
	++i;
      }
    }
    if (j == n) return; // exit if flat
  }
  if (base != middle) ZuSwap(*base, *middle);
  ++middle;
  // recurse smaller partition
  if (i < (n>>1)) {
    if (i > isort_N)
      qsort_<T, Cmp>(base, i);
    else if (i > 1)
      isort_<T, Cmp>(base, i);
    i = n - i - 1;
  } else {
    i = n - i - 1;
    if (i > isort_N)
      qsort_<T, Cmp>(middle, i);
    else if (i > 1)
      isort_<T, Cmp>(middle, i);
    i = n - i - 1;
    middle = base;
  }
  // iterate larger partition (unless insertion sort)
  if (i > isort_N) {
    base = middle;
    n = i;
    goto loop;
  } else if (i > 1)
    isort_<T, Cmp>(middle, i);
}

} // namespace ZuSort_

template <typename T, typename Cmp = ZuCmp<T>>
void ZuSort(T *base, unsigned n) {
  using namespace ZuSort_;
  if (n > isort_N)
    qsort_<T, Cmp>(base, n);
  else if (n > 1)
    isort_<T, Cmp>(base, n);
}

#endif /* ZuSort_HPP */
