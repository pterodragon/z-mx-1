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
// rolling median, percentiles (using statistics tree)

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

#include <zlib/ZuCmp.hpp>
#include <zlib/ZuFP.hpp>

namespace Zdf {

namespace pbds = __gnu_pbds;

// rolling count, total, mean, variance, standard deviation
template <typename Key_>
class Stats {
public:
  using Key = Key_;

  ZuInline unsigned count() const { return m_count; }
  ZuInline Key total() const { return m_total; }
  ZuInline double mean() const {
    if (ZuUnlikely(!m_count)) return 0.0;
    return static_cast<double>(m_total) / static_cast<double>(m_count);
  }
  ZuInline double var() const {
    if (ZuUnlikely(!m_count)) return 0.0;
    return static_cast<double>(m_var) / static_cast<double>(m_count);
  }
  ZuInline double std() const { return sqrt(var()); }

  ZuInline void clean() {
    m_count = 0;
    m_total = 0;
    m_var = 0.0;
  }

  void add(Key k) {
    if (!m_count) {
      m_total = k;
    } else {
      double n = m_count;
      auto prev = static_cast<double>(m_total) / n;
      auto mean = static_cast<double>(m_total += k) / (n + 1);
      double k_ = k;
      m_var += (k_ - prev) * (k_ - mean);
    }
    ++m_count;
  }

  void del(Key k) {
    if (ZuUnlikely(!m_count)) return;
    if (m_count == 1) {
      m_total = 0;
    } else if (m_count == 2) {
      m_total -= k;
      m_var = 0.0;
    } else {
      double n = m_count;
      auto prev = static_cast<double>(m_total) / n;
      auto mean = static_cast<double>(m_total -= k) / (n - 1);
      double k_ = k;
      m_var -= (k_ - prev) * (k_ - mean);
    }
    --m_count;
  }

private:
  unsigned	m_count = 0;
  Key		m_total = 0;
  double	m_var = 0.0;	// accumulated variance
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

template <typename Key_, typename Cmp_ = ZuCmp<Key_>>
class StatsTree : public Stats<Key_> {
  using Base = Stats<Key_>;
public:
  using Key = Key_;
  using Cmp = Cmp_;
private:
  template <typename T, typename Cmp> struct Less {
    constexpr bool operator()(const T &v1, const T &v2) const {
      return Cmp::less(v1, v2);
    }
  };
  using Tree = pbds::tree<
    Key,
    unsigned,
    Less<Key, Cmp>,
    pbds::rb_tree_tag,
    stats_tree_node_update>;
  static Tree &tree_();
public:
  using Iter = decltype(tree_().begin());

public:
  ZuInline void add(Key k) {
    add_(k);
    ++m_tree.insert(std::make_pair(k, 0)).first->second;
  }
  template <typename T>
  ZuInline ZuIsNot<typename ZuDecay<T>::T, Iter>::T del(T &&v) {
    auto iter = m_tree.find(ZuFwd<T>(v));
    if (iter != m_tree.end()) del_(iter);
  }
  template <typename T>
  ZuInline ZuIs<T, Iter>::T del(T iter) {
    if (iter != m_tree.end()) del_(iter);
  }

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
    return static_cast<double>(iter->first);
  }
  ZuInline auto medianIter() const {
    return m_tree.find_by_order(this->count()>>1);
  }
  ZuInline double median() const {
    auto iter = medianIter();
    if (iter == m_tree.end()) return ZuFP<double>::nan();
    return static_cast<double>(iter->first);
  }

  template <typename T>
  ZuInline auto find(T &&v) const { return m_tree.find(ZuFwd<T>(v)); }

  ZuInline auto begin() const { return m_tree.begin(); }
  ZuInline auto end() const { return m_tree.end(); }

  ZuInline void clean() {
    Base::clean();
    m_tree.clear();
  }

private:
  void add_(Key k) {
    Base::add(k);
  }
  void del_(Iter iter) {
    Base::del(iter->first);
    if (!--iter->second) m_tree.erase(iter);
  }

private:
  Tree		m_tree;
};

} // namespace Zdf

#endif /* ZdfStats_HPP */
