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

// Data Series Statistics

// rolling mean, variance and standard deviation
// rolling median, percentiles (using order statistics tree)

#ifndef ZdfStats_HPP
#define ZdfStats_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZdfLib_HPP
#include <zlib/ZdfLib.hpp>
#endif

#include <math.h>

#include <stats_tree.hpp>

#include <zlib/ZuNull.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuFP.hpp>

#include <zlib/ZuFixed.hpp>

#include <zlib/ZmHeap.hpp>
#include <zlib/ZmAllocator.hpp>

namespace Zdf {

namespace pbds = __gnu_pbds;

// rolling count, total, mean, variance, standard deviation
class Stats {
public:
  Stats() = default;
  Stats(const Stats &) = delete;
  Stats &operator =(const Stats &) = delete;
  Stats(Stats &&) = default;
  Stats &operator =(Stats &&) = default;
  ~Stats() = default;

  ZuInline double fp(ZuFixedVal v) const {
    return ZuFixed{v, m_exponent}.fp();
  }

  ZuInline unsigned count() const { return m_count; }
  ZuInline double total() const { return m_total; }
  ZuInline double mean() const {
    if (ZuUnlikely(!m_count)) return 0.0;
    return m_total / static_cast<double>(m_count);
  }
  ZuInline double var() const {
    if (ZuUnlikely(!m_count)) return 0.0;
    return m_var / static_cast<double>(m_count);
  }
  ZuInline double std() const { return sqrt(var()); }
  ZuInline unsigned exponent() const { return m_exponent; }

  void exponent(unsigned exp) {
    m_exponent = exp;
  }

  void add(const ZuFixed &v) {
    exponent(v.exponent);
    add_(v.value);
  }
protected:
  void add_(ZuFixedVal v_) {
    auto v = fp(v_);
    if (!m_count) {
      m_total = v;
    } else {
      double n = m_count;
      auto prev = m_total / n;
      auto mean = (m_total += v) / (n + 1.0);
      m_var += (v - prev) * (v - mean);
    }
    ++m_count;
  }

public:
  void del(const ZuFixed &v) {
    del_(v.adjust(m_exponent));
  }
protected:
  void del_(ZuFixedVal v_) {
    auto v = fp(v_);
    if (ZuUnlikely(!m_count)) return;
    if (m_count == 1) {
      m_total = 0.0;
    } else if (m_count == 2) {
      m_total -= v;
      m_var = 0.0;
    } else {
      double n = m_count;
      auto prev = m_total / n;
      auto mean = (m_total -= v) / (n - 1.0);
      m_var -= (v - prev) * (v - mean);
    }
    --m_count;
  }

  ZuInline void clean() {
    m_count = 0;
    m_total = 0.0;
    m_var = 0.0;
  }

private:
  unsigned	m_count = 0;
  double	m_total = 0.0;
  double	m_var = 0.0;	// accumulated variance
  unsigned	m_exponent = 0;
};

// NTP defaults
struct StatsTree_Defaults {
  struct HeapID {
    static constexpr const char *id() { return "Zdf.StatsTree"; }
  };
};

// StatsTreeHeapID - the heap ID
template <class HeapID_, class NTP = StatsTree_Defaults>
struct StatsTreeHeapID : public NTP {
  using HeapID = HeapID_;
};

template <class NTP = StatsTree_Defaults>
class StatsTree : public Stats {
public:
  using HeapID = typename NTP::HeapID;
  using Alloc = ZmAllocator<std::pair<ZuFixedVal, unsigned>, HeapID>;

private:
  using Tree = pbds::stats_tree<ZuFixedVal, unsigned, Alloc>;
  static Tree &tree_();
  static const Tree &ctree_();
public:
  using Iter = decltype(tree_().begin());
  using CIter = decltype(ctree_().begin());

public:
  StatsTree() = default;
  StatsTree(const StatsTree &) = delete;
  StatsTree &operator =(const StatsTree &) = delete;
  StatsTree(StatsTree &&) = default;
  StatsTree &operator =(StatsTree &&) = default;
  ~StatsTree() = default;

  ZuInline unsigned exponent() { return Stats::exponent(); }
  void exponent(unsigned newExp) {
    auto exp = exponent();
    if (ZuUnlikely(newExp != exp)) {
      if (count()) {
	if (newExp > exp)
	  shiftLeft(ZuDecimalFn::pow10_64(newExp - exp));
	else
	  shiftRight(ZuDecimalFn::pow10_64(exp - newExp));
      }
      Stats::exponent(newExp);
    }
  }

  void shiftLeft(uint64_t f) {
    for (auto i = begin(); i != end(); ++i)
      const_cast<ZuFixedVal &>(i->first) *= f;
  }
  void shiftRight(uint64_t f) {
    for (auto i = begin(); i != end(); ++i)
      const_cast<ZuFixedVal &>(i->first) /= f;
  }

  ZuInline void add(const ZuFixed &v_) {
    exponent(v_.exponent);
    auto v = v_.value;
    add_(v);
  }
  ZuInline void del(const ZuFixed &v_) {
    auto v = v_.adjust(exponent());
    auto iter = m_tree.find(v);
    if (iter != end()) del_(iter);
  }
  template <typename T>
  ZuInline typename ZuIs<T, Iter>::T del(T iter) {
    if (iter != end()) del_(iter);
  }

  ZuInline auto begin() const { return m_tree.begin(); }
  ZuInline auto end() const { return m_tree.end(); }

  ZuInline double fp(CIter iter) const {
    if (iter == end()) return ZuFP<double>::nan();
    return Stats::fp(iter->first);
  }

  ZuInline double minimum() const { return fp(begin()); }
  ZuInline double maximum() const {
    if (!count()) return ZuFP<double>::nan();
    return fp(--end());
  }

  template <typename T>
  ZuInline auto find(T &&v) const { return m_tree.find(ZuFwd<T>(v)); }

  ZuInline auto order(unsigned n) const {
    return m_tree.find_by_order(n);
  }
  // 0 <= n < 1
  ZuInline auto rankIter(double n) const {
    return m_tree.find_by_order(n * static_cast<double>(this->count()));
  }
  // 0 <= n < 1
  ZuInline double rank(double n) const {
    auto iter = rankIter(n);
    if (iter == m_tree.end()) return ZuFP<double>::nan();
    return fp(iter);
  }
  ZuInline auto medianIter() const {
    return m_tree.find_by_order(this->count()>>1);
  }
  ZuInline double median() const {
    auto iter = medianIter();
    if (iter == m_tree.end()) return ZuFP<double>::nan();
    return fp(iter);
  }

  ZuInline void clean() {
    Stats::clean();
    m_tree.clear();
  }

private:
  void add_(ZuFixedVal v) {
    Stats::add_(v);
    m_tree.insert(v);
  }
  void del_(Iter iter) {
    Stats::del_(iter->first);
    m_tree.erase(iter);
  }

private:
  Tree		m_tree;
};

} // namespace Zdf

#endif /* ZdfStats_HPP */
