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

#include <utility>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>

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

// rolling median, percentiles
namespace {
  // adapted from GNU pb_ds tree_order_statistics_node_update
  using namespace __gnu_pbds;

  template <
    typename Node_CItr, typename Node_Itr, typename Cmp_Fn, typename _Alloc>
  class stats_tree_node_update :
      private detail::branch_policy<Node_CItr, Node_Itr, _Alloc> {
  private:
    typedef detail::branch_policy<Node_CItr, Node_Itr, _Alloc> base_type;

  public:
    using cmp_fn = Cmp_Fn;
    using allocator_type = _Alloc;
    using size_type = typename allocator_type::size_type;
    using key_type = typename base_type::key_type;
    using key_const_reference = typename base_type::key_const_reference;

    using metadata_type = size_type;
    using node_const_iterator = Node_CItr;
    using node_iterator = Node_Itr;
    using const_iterator = typename node_const_iterator::value_type;
    using iterator = typename node_iterator::value_type;

    virtual ~stats_tree_node_update() { }

  private:
    virtual node_const_iterator node_begin() const = 0;
    virtual node_iterator node_begin() = 0;
    virtual node_const_iterator node_end() const = 0;
    virtual node_iterator node_end() = 0;
    virtual cmp_fn &get_cmp_fn() = 0;

  public:
    const_iterator find_by_order(size_type order) const {
      return const_cast<stats_tree_node_update *>(this)->
	find_by_order(order);
    }
    iterator find_by_order(size_type order) {
      node_iterator it = node_begin();
      node_iterator end_it = node_end();

      while (it != end_it) {
	node_iterator l_it = it.get_l_child();
	const size_type o = l_it == end_it ? 0 : l_it.get_metadata();
	const size_type n = (*it)->second;

	if (order < o)
	  it = l_it;
	else if (order < o + n)
	  return *it;
	else {
	  order -= o + n;
	  it = it.get_r_child();
	}
      }
      return base_type::end_iterator();
    }

    size_type order_of_key(key_const_reference r_key) const {
      node_const_iterator it = node_begin();
      node_const_iterator end_it = node_end();

      const auto &r_cmp_fn =
	const_cast<stats_tree_node_update *>(this)->get_cmp_fn();
      size_type ord = 0;
      while (it != end_it) {
	node_const_iterator l_it = it.get_l_child();

	const auto &key = (*it)->first;
	if (r_cmp_fn(r_key, key))
	  it = l_it;
	else if (r_cmp_fn(key, r_key)) {
	  const size_t n = (*it)->second;
	  ord += (l_it == end_it) ? n : n + l_it.get_metadata();
	  it = it.get_r_child();
	} else {
	  ord += (l_it == end_it) ? 0 : l_it.get_metadata();
	  it = end_it;
	}
      }
      return ord;
    }

    // increment repeat count
    void inc(iterator it_) {
      auto node = it_.m_p_nd;
      auto begin = node_begin().m_p_nd;
      do {
	++const_cast<metadata_reference>(node->get_metadata());
      } while (node != begin && (node = node->m_p_parent));
    }

    // decrement repeat count
    void dec(iterator it_) {
      auto node = it_.m_p_nd;
      auto begin = node_begin().m_p_nd;
      do {
	--const_cast<metadata_reference>(node->get_metadata());
      } while (node != begin && (node = node->m_p_parent));
    }

  private:
    typedef typename detail::rebind_traits<_Alloc, metadata_type>::reference
      metadata_reference;

  protected:
    void operator()(
	node_iterator node_it, node_const_iterator end_nd_it) const {
      node_iterator l_it = node_it.get_l_child();
      const size_type l_rank = (l_it == end_nd_it) ? 0 : l_it.get_metadata();

      node_iterator r_it = node_it.get_r_child();
      const size_type r_rank = (r_it == end_nd_it) ? 0 : r_it.get_metadata();

      const size_t n = (*node_it)->second;
      const_cast<metadata_reference>(node_it.get_metadata()) =
	n + l_rank + r_rank;
    }
  };
}

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
  using Tree = pbds::tree<
    ZuFixedVal,
    unsigned,
    std::less<ZuFixedVal>,
    pbds::rb_tree_tag,
    stats_tree_node_update,
    Alloc>;
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
    auto p = m_tree.insert(std::make_pair(v, 1));
    if (!p.second) {
      ++p.first->second;
      m_tree.inc(p.first);
    }
  }
  void del_(Iter iter) {
    Stats::del_(iter->first);
    if (iter->second == 1)
      m_tree.erase(iter);
    else {
      m_tree.dec(iter);
      --iter->second;
    }
  }

private:
  Tree		m_tree;
};

} // namespace Zdf

#endif /* ZdfStats_HPP */
