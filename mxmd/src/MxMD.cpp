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

// MxMD library

#include <MxMD.hpp>
#include <MxMDCore.hpp>

void MxMDTickSizeTbl::reset()
{
  m_venue->md()->resetTickSizeTbl(this);
}

void MxMDTickSizeTbl::addTickSize(
    MxValue minPrice, MxValue maxPrice, MxValue tickSize)
{
  m_venue->md()->addTickSize(this, minPrice, maxPrice, tickSize);
}

void MxMDPxLevel_::updateAbs(
    MxDateTime transactTime, MxValue qty, MxUInt nOrders, MxFlags flags,
    MxValue &d_qty, MxUInt &d_nOrders)
{
  m_data.transactTime = transactTime;
  updateAbs_(qty, nOrders, flags, d_qty, d_nOrders);
  if (!m_data.qty) m_data.nOrders = 0;
}

void MxMDPxLevel_::updateAbs_(
    MxValue qty, MxUInt nOrders, MxFlags flags,
    MxValue &d_qty, MxUInt &d_nOrders)
{
  d_qty = qty - m_data.qty;
  m_data.qty = qty;
  d_nOrders = nOrders - m_data.nOrders;
  m_data.nOrders = nOrders;
  m_data.flags = flags;
}

void MxMDPxLevel_::updateDelta(
    MxDateTime transactTime, MxValue qty, MxUInt nOrders, MxFlags flags)
{
  m_data.transactTime = transactTime;
  updateDelta_(qty, nOrders, flags);
  if (!m_data.qty) m_data.nOrders = 0;
}

void MxMDPxLevel_::updateDelta_(MxValue qty, MxUInt nOrders, MxFlags flags)
{
  m_data.qty += qty;
  m_data.nOrders += nOrders;
  if (qty)
    m_data.flags |= flags;
  else
    m_data.flags &= ~flags;
}

void MxMDPxLevel_::update(
    MxDateTime transactTime, bool delta,
    MxValue qty, MxUInt nOrders, MxFlags flags,
    MxValue &d_qty, MxUInt &d_nOrders)
{
  m_data.transactTime = transactTime;
  if (!delta)
    updateAbs_(qty, nOrders, flags, d_qty, d_nOrders);
  else {
    d_qty = qty, d_nOrders = nOrders;
    updateDelta_(qty, nOrders, flags);
  }
  if (!m_data.qty) m_data.nOrders = 0;
}

void MxMDPxLevel_::addOrder(MxMDOrder *order)
{
  if (obSide()->orderBook()->venue()->flags() &
      (1U<<MxMDVenueFlags::UniformRanks)) {
    MxUInt rank = order->m_data.rank;
    auto i = m_orders.iterator<ZmRBTreeGreaterEqual>(rank);
    typename Orders::Node *node;
    while ((node = i.iterate()) && node->key()->m_data.rank == rank)
      rank = ++node->key()->m_data.rank;
  }
  m_orders.add(order);
}

void MxMDPxLevel_::delOrder(MxUInt rank)
{
  m_orders.delVal(rank);
  if (obSide()->orderBook()->venue()->flags() &
      (1U<<MxMDVenueFlags::UniformRanks)) {
    auto i = m_orders.iterator<ZmRBTreeGreaterEqual>(rank++);
    typename Orders::Node *node;
    while ((node = i.iterate()) && node->key()->m_data.rank == rank++)
      --node->key()->m_data.rank;
  }
}

MxMDOrderBook::MxMDOrderBook( // single leg
  MxMDShard *shard, 
  MxMDVenue *venue,
  MxID segment, ZuString id,
  MxMDSecurity *security,
  MxMDTickSizeTbl *tickSizeTbl,
  const MxMDLotSizes &lotSizes,
  MxMDSecHandler *handler) :
    MxMDSharded(shard),
    m_venue(venue),
    m_venueShard(venue ? venue->shard_(shard->id()) : (MxMDVenueShard *)0),
    m_key{.id = id, .venue = venue ? venue->id() : MxID(), .segment = segment},
    m_legs(1),
    m_tickSizeTbl(tickSizeTbl),
    m_lotSizes(lotSizes),
    m_bids(new MxMDOBSide(this, MxSide::Buy)),
    m_asks(new MxMDOBSide(this, MxSide::Sell)),
    m_handler(handler)
{
  m_sides[0] = MxEnum();
  m_ratios[0] = MxRatio();
  m_securities[0] = security;
  m_l1Data.pxNDP = security->refData().pxNDP;
  m_l1Data.qtyNDP = security->refData().qtyNDP;
}

MxMDOrderBook::MxMDOrderBook( // multi-leg
  MxMDShard *shard, 
  MxMDVenue *venue,
  MxID segment, ZuString id, MxNDP pxNDP, MxNDP qtyNDP,
  MxUInt legs, const ZmRef<MxMDSecurity> *securities,
  const MxEnum *sides, const MxRatio *ratios,
  MxMDTickSizeTbl *tickSizeTbl,
  const MxMDLotSizes &lotSizes) :
    MxMDSharded(shard),
    m_venue(venue),
    m_key{.id = id, .venue = venue->id(), .segment = segment},
    m_legs(legs),
    m_tickSizeTbl(tickSizeTbl),
    m_lotSizes(lotSizes),
    m_bids(new MxMDOBSide(this, MxSide::Buy)),
    m_asks(new MxMDOBSide(this, MxSide::Sell))
{
  for (unsigned i = 0; i < legs; i++) {
    m_securities[i] = securities[i];
    m_sides[i] = sides[i];
    m_ratios[i] = ratios[i];
  }
  m_l1Data.pxNDP = pxNDP;
  m_l1Data.qtyNDP = qtyNDP;
}

void MxMDOrderBook::subscribe(MxMDSecHandler *handler)
{
  m_handler = handler;
  if (MxMDFeedOB *feedOB = this->feedOB<>())
    feedOB->subscribe(this, handler);
}

void MxMDOrderBook::unsubscribe()
{
  if (MxMDFeedOB *feedOB = this->feedOB<>())
    feedOB->unsubscribe(this, m_handler);
  m_handler = 0;
}

void MxMDOrderBook::l1(MxMDL1Data &l1Data)
{
  if (!*l1Data.pxNDP) {
    l1Data.pxNDP = m_l1Data.pxNDP;
  } else if (ZuUnlikely(m_l1Data.pxNDP != l1Data.pxNDP)) {
#ifdef adjustNDP
#undef adjustNDP
#endif
#define adjustNDP(v) if (*l1Data.v) l1Data.v = \
    MxValNDP{l1Data.v, l1Data.pxNDP}.adjust(m_l1Data.pxNDP)
    adjustNDP(base);
    for (unsigned i = 0; i < MxMDNSessions; i++) {
      adjustNDP(open[i]);
      adjustNDP(close[i]);
    }
    adjustNDP(last);
    adjustNDP(bid);
    adjustNDP(ask);
    adjustNDP(high);
    adjustNDP(low);
    adjustNDP(accVol);
    adjustNDP(match);
#undef adjustNDP
    l1Data.pxNDP = m_l1Data.pxNDP;
  }
  if (!*l1Data.qtyNDP) {
    l1Data.qtyNDP = m_l1Data.qtyNDP;
  } else if (ZuUnlikely(m_l1Data.qtyNDP != l1Data.qtyNDP)) {
#ifdef adjustNDP
#undef adjustNDP
#endif
#define adjustNDP(v) if (*l1Data.v) l1Data.v = \
    MxValNDP{l1Data.v, l1Data.qtyNDP}.adjust(m_l1Data.qtyNDP)
    adjustNDP(lastQty);
    adjustNDP(bidQty);
    adjustNDP(askQty);
    adjustNDP(accVolQty);
    adjustNDP(matchQty);
    adjustNDP(surplusQty);
#undef adjustNDP
    l1Data.qtyNDP = m_l1Data.qtyNDP;
  }
  m_l1Data.stamp = l1Data.stamp;
  m_l1Data.status.update(l1Data.status);
  m_l1Data.base.update(l1Data.base, MxValueReset);
  for (unsigned i = 0; i < MxMDNSessions; i++) {
    m_l1Data.open[i].update(l1Data.open[i], MxValueReset);
    m_l1Data.close[i].update(l1Data.close[i], MxValueReset);
  }
  // permit the feed to reset tickDir/high/low
  m_l1Data.tickDir.update(l1Data.tickDir);
  m_l1Data.high.update(l1Data.high, MxValueReset);
  m_l1Data.low.update(l1Data.low, MxValueReset);
  // update tickDir/high/low based on last
  if (*l1Data.last) {
    if (!*m_l1Data.last)
      m_l1Data.tickDir = l1Data.tickDir = MxEnum();
    else if (l1Data.last == m_l1Data.last) {
      if (m_l1Data.tickDir == MxTickDir::Up)
	m_l1Data.tickDir = l1Data.tickDir = MxTickDir::LevelUp;
      else if (m_l1Data.tickDir == MxTickDir::Down)
	m_l1Data.tickDir = l1Data.tickDir = MxTickDir::LevelDown;
    } else if (l1Data.last > m_l1Data.last)
      m_l1Data.tickDir = l1Data.tickDir = MxTickDir::Up;
    else if (l1Data.last < m_l1Data.last)
      m_l1Data.tickDir = l1Data.tickDir = MxTickDir::Down;
    if (!*m_l1Data.high || m_l1Data.high < l1Data.last)
      m_l1Data.high = l1Data.high = l1Data.last;
    if (!*m_l1Data.low || m_l1Data.low > l1Data.last)
      m_l1Data.low = l1Data.low = l1Data.last;
  }
  m_l1Data.last.update(l1Data.last, MxValueReset);
  m_l1Data.lastQty.update(l1Data.lastQty, MxValueReset);
  m_l1Data.bid.update(l1Data.bid, MxValueReset);
  m_l1Data.bidQty.update(l1Data.bidQty, MxValueReset);
  m_l1Data.ask.update(l1Data.ask, MxValueReset);
  m_l1Data.askQty.update(l1Data.askQty, MxValueReset);
  m_l1Data.accVol.update(l1Data.accVol, MxValueReset);
  m_l1Data.accVolQty.update(l1Data.accVolQty, MxValueReset);
  m_l1Data.match.update(l1Data.match, MxValueReset);
  m_l1Data.matchQty.update(l1Data.matchQty, MxValueReset);
  m_l1Data.surplusQty.update(l1Data.surplusQty, MxValueReset);
  m_l1Data.flags = l1Data.flags;
  
  md()->l1(this, l1Data);
  if (m_handler) m_handler->l1(this, l1Data);
}

bool MxMDOBSide::updateL1Bid(MxMDL1Data &l1Data, MxMDL1Data &delta)
{
  ZmRef<MxMDPxLevel> bid = m_pxLevels.maximum();
  if (bid) {
    if (l1Data.bid != bid->price() ||
	l1Data.bidQty != bid->data().qty) {
      l1Data.bid = delta.bid = bid->price();
      l1Data.bidQty = delta.bidQty = bid->data().qty;
      return true;
    }
  } else {
    if (*l1Data.bid) {
      delta.bid = delta.bidQty = MxValueReset;
      l1Data.bid = l1Data.bidQty = MxValue();
      return true;
    }
  }
  return false;
}

bool MxMDOBSide::updateL1Ask(MxMDL1Data &l1Data, MxMDL1Data &delta)
{
  ZmRef<MxMDPxLevel> ask = m_pxLevels.minimum();
  if (ask) {
    if (l1Data.ask != ask->price() ||
	l1Data.askQty != ask->data().qty) {
      l1Data.ask = delta.ask = ask->price();
      l1Data.askQty = delta.askQty = ask->data().qty;
      return true;
    }
  } else {
    if (*l1Data.ask) {
      delta.ask = delta.askQty = MxValueReset;
      l1Data.ask = l1Data.askQty = MxValue();
      return true;
    }
  }
  return false;
}

void MxMDOrderBook::l2(MxDateTime stamp, bool updateL1)
{
  MxMDL1Data delta{ {}, m_l1Data.pxNDP, m_l1Data.qtyNDP };
  bool l1Updated = false;

  if (updateL1) {
    l1Updated = m_bids->updateL1Bid(m_l1Data, delta);
    l1Updated = m_asks->updateL1Ask(m_l1Data, delta) || l1Updated;
    if (l1Updated) m_l1Data.stamp = delta.stamp = stamp;
    md()->l2(this, stamp, updateL1);
  }

  if (m_handler) {
    m_handler->l2(this, stamp);
    if (l1Updated) m_handler->l1(this, delta);
  }
}

void MxMDOrderBook::pxLevel(
  MxEnum side, MxDateTime transactTime, bool delta,
  MxValue price, MxValue qty, MxUInt nOrders, MxFlags flags)
{
  MxValue d_qty;
  MxUInt d_nOrders;
  pxLevel_(side, transactTime, delta, price, qty, nOrders, flags,
      &d_qty, &d_nOrders);
  if (MxMDOrderBook *out = this->out())
    out->pxLevel_(
	side, transactTime, true, price, d_qty, d_nOrders, flags, 0, 0);
}

void MxMDOBSide::pxLevel_(
  MxDateTime transactTime, bool delta,
  MxValue price, MxValue qty, MxUInt nOrders, MxFlags flags,
  const MxMDSecHandler *handler,
  MxValue &d_qty, MxUInt &d_nOrders,
  const MxMDPxLevelFn *&pxLevelFn,
  ZmRef<MxMDPxLevel> &pxLevel)
{
  if (ZuUnlikely(!*price)) {
    if (!m_mktLevel) {
      if (qty) {
	d_qty = qty, d_nOrders = nOrders;
	pxLevel = m_mktLevel = new MxMDPxLevel(
	    this, transactTime, m_orderBook->pxNDP(), m_orderBook->qtyNDP(),
	    MxValue(), qty, nOrders, flags);
	if (handler) pxLevelFn = &handler->addMktLevel;
      } else {
	pxLevel = 0;
	d_qty = 0, d_nOrders = 0;
      }
    } else {
      pxLevel = m_mktLevel;
      m_mktLevel->update(
	  transactTime, delta, qty, nOrders, flags, d_qty, d_nOrders);
      if (m_mktLevel->data().qty)
	if (handler) pxLevelFn = &handler->updatedMktLevel;
      else {
	m_mktLevel = 0;
	if (handler) pxLevelFn = &handler->deletedMktLevel;
      }
    }
    if (d_qty) m_data.qty += d_qty;
    return;
  }
  pxLevel = m_pxLevels.find(price);
  if (!pxLevel) {
    if (qty) {
      d_qty = qty, d_nOrders = nOrders;
      pxLevel = new MxMDPxLevel(
	  this, transactTime, m_orderBook->pxNDP(), m_orderBook->qtyNDP(),
	  price, qty, nOrders, flags);
      if (handler) pxLevelFn = &handler->addPxLevel;
      m_pxLevels.add(pxLevel);
    } else {
      pxLevel = 0;
      d_qty = 0, d_nOrders = 0;
    }
  } else {
    pxLevel->update(
	transactTime, delta, qty, nOrders, flags, d_qty, d_nOrders);
    if (pxLevel->data().qty)
      if (handler) pxLevelFn = &handler->updatedPxLevel;
    else {
      if (handler) pxLevelFn = &handler->deletedPxLevel;
      m_pxLevels.delVal(price);
    }
  }
  if (d_qty) {
    m_data.qty += d_qty;
    m_data.nv += price * d_qty;
  }
}

void MxMDOrderBook::pxLevel_(
  MxEnum side, MxDateTime transactTime, bool delta,
  MxValue price, MxValue qty, MxUInt nOrders, MxFlags flags,
  MxValue *d_qty_, MxUInt *d_nOrders_)
{
  MxValue d_qty;
  MxUInt d_nOrders;
  const MxMDPxLevelFn *pxLevelFn = 0;
  ZmRef<MxMDPxLevel> pxLevel;

  {
    MxMDOBSide *obSide = side == MxSide::Buy ? m_bids : m_asks;
    obSide->pxLevel_(
	transactTime, delta, price, qty, nOrders, flags, m_handler,
	d_qty, d_nOrders, pxLevelFn, pxLevel);
    if (!m_venue->flags() & (1<<MxMDVenueFlags::Dark))
      md()->pxLevel(
	  this, side, transactTime, delta, price, qty, nOrders, flags);
  }

  if (pxLevelFn) (*pxLevelFn)(pxLevel, transactTime);

  if (d_qty_) *d_qty_ = d_qty;
  if (d_nOrders_) *d_nOrders_ = d_nOrders;
}

#if 0
static void dumporder(const char *op, MxMDOrder *order)
{
  MxMDOrderID3Ref id3 = MxMDOrder::OrderID3Accessor::value(order);
  std::cout << op << ' ' <<
    id3.p1().venue() << ' ' <<
    id3.p1().segment() << ' ' <<
    id3.p1().id() << ' ' <<
    MxSide::name(id3.p2()) << ' ' <<
    id3.p3() << '\n';
}

void MxMDFeed::dumporders(const char *op)
{
  MxMDFeed::Orders3::ReadIterator i(m_orders3);
  while (ZmRef<MxMDOrder> order = i.iterateKey()) {
    dumporder(op, order);
  }
}
#endif

void MxMDOBSide::addOrder_(
    MxMDOrder *order, MxDateTime transactTime,
    const MxMDSecHandler *handler,
    const MxMDPxLevelFn *&pxLevelFn, ZmRef<MxMDPxLevel> &pxLevel)
{
  const MxMDOrderData &orderData = order->data();
  if (!*orderData.price) {
    if (!m_mktLevel) {
      if (handler) pxLevelFn = &handler->addMktLevel;
      pxLevel = m_mktLevel = new MxMDPxLevel(
	  this, transactTime, m_orderBook->pxNDP(), m_orderBook->qtyNDP(),
	  MxValue(), orderData.qty, 1, 0);
    } else {
      if (handler) pxLevelFn = &handler->updatedMktLevel;
      pxLevel = m_mktLevel;
      m_mktLevel->updateDelta(transactTime, orderData.qty, 1, 0);
    }
    order->pxLevel(m_mktLevel);
    m_mktLevel->addOrder(order);
    m_data.qty += orderData.qty;
    return;
  }
  pxLevel = m_pxLevels.find(orderData.price);
  if (!pxLevel) {
    if (handler) pxLevelFn = &handler->addPxLevel;
    pxLevel = new MxMDPxLevel(
	this, transactTime, m_orderBook->pxNDP(), m_orderBook->qtyNDP(),
	orderData.price, orderData.qty, 1, 0);
    m_pxLevels.add(pxLevel);
  } else {
    if (handler) pxLevelFn = &handler->updatedPxLevel;
    pxLevel->updateDelta(transactTime, orderData.qty, 1, 0);
  }
  order->pxLevel(pxLevel);
  pxLevel->addOrder(order);
  m_data.nv += orderData.price * orderData.qty;
  m_data.qty += orderData.qty;
}

void MxMDOrderBook::addOrder_(
  MxMDOrder *order, MxDateTime transactTime,
  const MxMDSecHandler *handler,
  const MxMDPxLevelFn *&pxLevelFn, ZmRef<MxMDPxLevel> &pxLevel)
{
  const MxMDOrderData &orderData = order->data();
  if (orderData.qty) {
    MxMDOBSide *obSide = orderData.side == MxSide::Buy ? m_bids : m_asks;
    obSide->addOrder_(order, transactTime, handler, pxLevelFn, pxLevel);
  }
}

void MxMDOBSide::delOrder_(
    MxMDOrder *order, MxDateTime transactTime,
    const MxMDSecHandler *handler,
    const MxMDPxLevelFn *&pxLevelFn, ZmRef<MxMDPxLevel> &pxLevel)
{
  const MxMDOrderData &orderData = order->data();
  if (!*orderData.price) {
    if (ZuUnlikely(!m_mktLevel)) {
      m_orderBook->md()->raise(ZeEVENT(Error, MxMDNoPxLevel("delOrder")));
      return;
    }
    m_mktLevel->updateDelta(transactTime, -orderData.qty, -1, 0);
    m_mktLevel->delOrder(orderData.rank);
    pxLevel = m_mktLevel;
    if (!m_mktLevel->data().qty) {
      if (handler) pxLevelFn = &handler->deletedMktLevel;
      m_mktLevel = 0;
    } else
      if (handler) pxLevelFn = &handler->updatedMktLevel;
    m_data.qty -= orderData.qty;
    return;
  }
  pxLevel = static_cast<MxMDPxLevel *>(order->pxLevel());
  if (ZuUnlikely(!pxLevel)) {
    m_orderBook->md()->raise(ZeEVENT(Error, MxMDNoPxLevel("delOrder")));
    return;
  }
  pxLevel->updateDelta(transactTime, -orderData.qty, -1, 0);
  pxLevel->delOrder(orderData.rank);
  if (!pxLevel->data().qty) {
    if (handler) pxLevelFn = &handler->deletedPxLevel;
    m_pxLevels.delVal(pxLevel->price());
  } else {
    if (handler) pxLevelFn = &handler->updatedPxLevel;
  }
  order->pxLevel(0);
  m_data.nv -= orderData.price * orderData.qty;
  m_data.qty -= orderData.qty;
}

void MxMDOrderBook::delOrder_(
  MxMDOrder *order, MxDateTime transactTime,
  const MxMDSecHandler *handler,
  const MxMDPxLevelFn *&pxLevelFn, ZmRef<MxMDPxLevel> &pxLevel)
{
  const MxMDOrderData &orderData = order->data();
  if (orderData.qty) {
    MxMDOBSide *obSide = orderData.side == MxSide::Buy ? m_bids : m_asks;
    obSide->delOrder_(order, transactTime, handler, pxLevelFn, pxLevel);
  }
}

ZmRef<MxMDOrder> MxMDOrderBook::addOrder(
  ZuString orderID, MxDateTime transactTime,
  MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags)
{
  if (ZuUnlikely(!m_venueShard)) return (MxMDOrder *)0;

  ZmRef<MxMDOrder> order;
  const MxMDPxLevelFn *pxLevelFn = 0;
  ZmRef<MxMDPxLevel> pxLevel;

  if (ZmRef<MxMDOrder> order = m_venueShard->findOrder(key(), side, orderID))
    return modifyOrder(
	orderID, transactTime, side, rank, price, qty, flags);

  order = new MxMDOrder(this,
    orderID, transactTime, side, rank, price, qty, flags);

  m_venueShard->addOrder(order);

  addOrder_(order, transactTime, m_handler, pxLevelFn, pxLevel);

  md()->addOrder(this, orderID,
      transactTime, side, rank, price, qty, flags);

  if (MxMDOrderBook *out = this->out())
    out->pxLevel_(
      pxLevel->side(), transactTime, true,
      pxLevel->price(), qty, 1, flags, 0, 0);

  if (pxLevelFn) (*pxLevelFn)(pxLevel, transactTime);

  if (m_handler) m_handler->addOrder(order, transactTime);

  return order;
}

ZmRef<MxMDOrder> MxMDOrderBook::modifyOrder(
  ZuString orderID, MxDateTime transactTime,
  MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags)
{
  if (ZuUnlikely(!m_venueShard)) return (MxMDOrder *)0;

  ZmRef<MxMDOrder> order;

  if (ZuUnlikely(!qty))
    order = m_venueShard->delOrder(key(), side, orderID);
  else
    order = m_venueShard->findOrder(key(), side, orderID);
  if (ZuUnlikely(!order)) {
    md()->raise(ZeEVENT(Error, MxMDOrderNotFound("modifyOrder", orderID)));
    return 0;
  }
  modifyOrder_(order, transactTime, side, rank, price, qty, flags);
  return order;
}

void MxMDVenue::modifyOrder(
  ZuString orderID, MxDateTime transactTime,
  MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags,
  ZmFn<MxMDOrder *> fn)
{
  ZmRef<MxMDOrder> order;
  if (ZuUnlikely(!qty))
    order = delOrder(orderID);
  else
    order = findOrder(orderID);
  if (ZuUnlikely(!order)) {
    md()->raise(ZeEVENT(Error, MxMDOrderNotFound("modifyOrder", orderID)));
    return;
  }
  order->orderBook()->shard()->run(
      [=, ob = order->orderBook(), order = ZuMv(order)]() {
	ob->modifyOrder_(order, transactTime, side, rank, price, qty, flags);
	fn(order); });
}

void MxMDOrderBook::modifyOrder_(MxMDOrder *order, MxDateTime transactTime,
  MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags)
{
  const MxMDPxLevelFn *pxLevelFn[2] = { 0 };
  ZmRef<MxMDPxLevel> pxLevel[2];
  delOrder_(order, transactTime, m_handler, pxLevelFn[0], pxLevel[0]);

  MxValue oldQty = order->data().qty;
  order->update_(rank, price, qty, flags);

  if (ZuLikely(qty))
    addOrder_(order, transactTime, m_handler, pxLevelFn[1], pxLevel[1]);
  else
    m_venueShard->delOrder(key(), side, order->id());

  md()->modifyOrder(
      this, order->id(), transactTime, side,
      rank, price, qty, flags);

  if (MxMDOrderBook *out = this->out()) {
    out->pxLevel_(
      pxLevel[0]->side(), transactTime, true,
      pxLevel[0]->price(), -oldQty, -1, 0, 0, 0);
    if (ZuLikely(qty))
      out->pxLevel_(
	pxLevel[1]->side(), transactTime, true,
	pxLevel[1]->price(), qty, 1, 0, 0, 0);
  }

  if (pxLevelFn[0]) (*(pxLevelFn[0]))(pxLevel[0], transactTime);
  if (ZuLikely(qty))
    if (pxLevelFn[1]) (*(pxLevelFn[1]))(pxLevel[1], transactTime);

  if (m_handler) m_handler->modifiedOrder(order, transactTime);
}

ZmRef<MxMDOrder> MxMDOrderBook::reduceOrder(
  ZuString orderID, MxDateTime transactTime,
  MxEnum side, MxValue reduceQty)
{
  if (ZuUnlikely(!m_venueShard)) return (MxMDOrder *)0;

  ZmRef<MxMDOrder> order = m_venueShard->findOrder(key(), side, orderID);
  if (ZuUnlikely(!order)) {
    md()->raise(ZeEVENT(Error, MxMDOrderNotFound("reduceOrder", orderID)));
    return 0;
  }
  if (ZuUnlikely(order->data().qty <= reduceQty))
    m_venueShard->delOrder(key(), side, orderID);
  reduceOrder_(order, transactTime, reduceQty);
  return order;
}

void MxMDVenue::reduceOrder(
  ZuString orderID, MxDateTime transactTime,
  MxValue reduceQty, ZmFn<MxMDOrder *> fn)
{
  ZmRef<MxMDOrder> order = findOrder(orderID);
  if (ZuUnlikely(!order)) {
    md()->raise(ZeEVENT(Error, MxMDOrderNotFound("reduceOrder", orderID)));
    return;
  }
  if (ZuUnlikely(order->data().qty <= reduceQty)) delOrder(orderID);
  order->orderBook()->shard()->run(
      [=, ob = order->orderBook(), order = ZuMv(order)]() {
	ob->reduceOrder_(order, transactTime, reduceQty);
	fn(order); });
}

void MxMDOrderBook::reduceOrder_(MxMDOrder *order,
  MxDateTime transactTime, MxValue reduceQty)
{
  const MxMDPxLevelFn *pxLevelFn[2] = { 0 };
  ZmRef<MxMDPxLevel> pxLevel[2];
  delOrder_(order, transactTime, m_handler, pxLevelFn[0], pxLevel[0]);

  MxValue oldQty = order->data().qty;
  MxValue qty = oldQty - reduceQty;
  if (!qty || (ZuUnlikely(qty < 0 || !*qty))) qty = 0;
  order->updateQty_(qty);

  if (ZuLikely(qty))
    addOrder_(order, transactTime, m_handler, pxLevelFn[1], pxLevel[1]);
  else
    m_venueShard->delOrder(key(), order->data().side, order->id());

  md()->modifyOrder(
      this, order->id(), transactTime, order->data().side,
      MxUInt(), MxValue(), qty, MxFlags());

  if (MxMDOrderBook *out = this->out()) {
    out->pxLevel_(
      pxLevel[0]->side(), transactTime, true,
      pxLevel[0]->price(), -oldQty, -1, 0, 0, 0);
    if (ZuLikely(qty))
      out->pxLevel_(
	pxLevel[1]->side(), transactTime, true,
	pxLevel[1]->price(), qty, 1, 0, 0, 0);
  }

  if (pxLevelFn[0]) (*(pxLevelFn[0]))(pxLevel[0], transactTime);
  if (ZuLikely(qty))
    if (pxLevelFn[1]) (*(pxLevelFn[1]))(pxLevel[1], transactTime);

  if (m_handler) m_handler->modifiedOrder(order, transactTime);
}

ZmRef<MxMDOrder> MxMDOrderBook::cancelOrder(
  ZuString orderID, MxDateTime transactTime,
  MxEnum side)
{
  if (ZuUnlikely(!m_venueShard)) return (MxMDOrder *)0;

  ZmRef<MxMDOrder> order = m_venueShard->delOrder(key(), side, orderID);
  if (ZuUnlikely(!order)) return 0;
  cancelOrder_(order, transactTime);
  return order;
}

void MxMDVenue::cancelOrder(
  ZuString orderID, MxDateTime transactTime,
  ZmFn<MxMDOrder *> fn)
{
  ZmRef<MxMDOrder> order = delOrder(orderID);
  if (ZuUnlikely(!order)) return;
  order->orderBook()->shard()->run(
      [=, ob = order->orderBook(), order = ZuMv(order)]() {
	ob->cancelOrder_(order, transactTime);
	fn(order); });
}

void MxMDOrderBook::cancelOrder_(MxMDOrder *order, MxDateTime transactTime)
{
  const MxMDPxLevelFn *pxLevelFn = 0;
  ZmRef<MxMDPxLevel> pxLevel;
  MxValue qty = order->data().qty;

  delOrder_(order, transactTime, m_handler, pxLevelFn, pxLevel);

  md()->cancelOrder(this,
      order->id(), transactTime, order->data().side);

  if (MxMDOrderBook *out = this->out())
    out->pxLevel_(
      pxLevel->side(), transactTime, true,
      pxLevel->price(), -qty, -1, 0, 0, 0);

  if (pxLevelFn) (*pxLevelFn)(pxLevel, transactTime);
  if (m_handler) m_handler->canceledOrder(order, transactTime);
}

void MxMDOBSide::reset(MxDateTime transactTime, const MxMDSecHandler *handler)
{
  const MxMDOrderFn *canceledOrder =
    handler ? &handler->canceledOrder : (const MxMDOrderFn *)0;
  if (m_mktLevel) {
    m_mktLevel->reset(transactTime, canceledOrder);
    if (handler) handler->deletedPxLevel(m_mktLevel, transactTime);
    m_mktLevel = 0;
  }
  {
    auto i = m_pxLevels.readIterator();
    while (ZmRef<MxMDPxLevel> pxLevel = i.iterate()) {
      MxValue d_qty = -pxLevel->data().qty;
      MxUInt d_nOrders = -pxLevel->data().nOrders;
      pxLevel->reset(transactTime, canceledOrder);
      if (handler) handler->deletedPxLevel(pxLevel, transactTime);
      if (MxMDOrderBook *out = m_orderBook->out())
	out->pxLevel_(
	  m_side, transactTime, true,
	  pxLevel->price(), d_qty, d_nOrders, 0, 0, 0);
    }
  }
  m_pxLevels.clean();
  m_data.nv = m_data.qty = 0;
}

void MxMDOrderBook::reset(MxDateTime transactTime)
{
  MxMDL1Data delta{ {}, m_l1Data.pxNDP, m_l1Data.qtyNDP };
  bool l1Updated = false;

  if (*m_l1Data.bid) {
    delta.bid = delta.bidQty = MxValueReset;
    m_l1Data.bid = m_l1Data.bidQty = MxValue();
    l1Updated = true;
  }
  if (*m_l1Data.ask) {
    delta.ask = delta.askQty = MxValueReset;
    m_l1Data.ask = m_l1Data.askQty = MxValue();
    l1Updated = true;
  }
  if (l1Updated) {
    m_l1Data.stamp = delta.stamp = transactTime;
  }

  m_bids->reset(transactTime, m_handler);
  m_asks->reset(transactTime, m_handler);

  md()->resetOB(this, transactTime);

  if (m_handler) {
    m_handler->l2(this, transactTime);
    if (l1Updated) m_handler->l1(this, delta);
  }
}

void MxMDOrderBook::addTrade(
  ZuString tradeID, MxDateTime transactTime, MxValue price, MxValue qty)
{
  md()->addTrade(this, tradeID, transactTime, price, qty);
  if (m_handler && m_handler->addTrade) {
    ZmRef<MxMDTrade> trade = new MxMDTrade(
	security(0), this, tradeID, transactTime, price, qty);
    m_handler->addTrade(trade, transactTime);
  }
}

void MxMDOrderBook::correctTrade(
  ZuString tradeID, MxDateTime transactTime, MxValue price, MxValue qty)
{
  md()->correctTrade(this, tradeID, transactTime, price, qty);
  if (m_handler && m_handler->correctedTrade) {
    ZmRef<MxMDTrade> trade = new MxMDTrade(
	security(0), this, tradeID, transactTime, price, qty);
    m_handler->correctedTrade(trade, transactTime);
  }
}

void MxMDOrderBook::cancelTrade(
  ZuString tradeID, MxDateTime transactTime, MxValue price, MxValue qty)
{
  md()->cancelTrade(this, tradeID, transactTime, price, qty);
  if (m_handler && m_handler->canceledTrade) {
    ZmRef<MxMDTrade> trade = new MxMDTrade(
	security(0), this, tradeID, transactTime, price, qty);
    m_handler->canceledTrade(trade, transactTime);
  }
}

void MxMDOrderBook::update(
    const MxMDTickSizeTbl *tickSizeTbl,
    const MxMDLotSizes &lotSizes,
    MxDateTime transactTime)
{
  if (tickSizeTbl)
    m_tickSizeTbl = const_cast<MxMDTickSizeTbl *>(tickSizeTbl);
  if (*lotSizes.oddLotSize)
    m_lotSizes.oddLotSize = lotSizes.oddLotSize;
  if (*lotSizes.lotSize)
    m_lotSizes.lotSize = lotSizes.lotSize;
  if (*lotSizes.blockLotSize)
    m_lotSizes.blockLotSize = lotSizes.blockLotSize;

  md()->updateOrderBook(this, tickSizeTbl, lotSizes, transactTime);

  if (m_handler) m_handler->updatedOrderBook(this, transactTime);
}

void MxMDOrderBook::map(unsigned inRank, MxMDOrderBook *outOB)
{
  if (ZuUnlikely(m_out)) {
    MxMDOrderBook *inOB, *prevOB = 0;
    for (inOB = m_out->m_in; inOB; prevOB = inOB, inOB = inOB->m_next)
      if (inOB == this) {
	if (!prevOB)
	  m_out->m_in = m_next;
	else
	  prevOB->m_next = m_next;
	break;
      }
    m_next = 0;
  }
  m_rank = inRank;
  m_out = outOB;
  if (!outOB->m_in) {
    m_next = 0;
    outOB->m_in = this;
  } else {
    MxMDOrderBook *inOB, *prevOB = 0;
    for (inOB = outOB->m_in; inOB; prevOB = inOB, inOB = inOB->m_next)
      if (inRank < inOB->m_rank) break;
    m_next = inOB;
    if (!prevOB)
      outOB->m_in = this;
    else
      prevOB->m_next = this;
  }
}

MxMDSecurity::MxMDSecurity(MxMDShard *shard,
    const MxSecKey &key, const MxMDSecRefData &refData) :
  MxMDSharded(shard), m_key(key), m_refData(refData)
{
}

void MxMDSecurity::subscribe(MxMDSecHandler *handler)
{
  m_handler = handler;
  auto i = m_orderBooks.iterator();
  while (ZmRef<MxMDOrderBook> ob = i.iterateKey())
    if (*(ob->venueID())) ob->subscribe(handler);
}

void MxMDSecurity::unsubscribe()
{
  auto i = m_orderBooks.iterator();
  while (ZmRef<MxMDOrderBook> ob = i.iterateKey())
    if (*(ob->venueID())) ob->unsubscribe();
  m_handler = 0;
}

ZmRef<MxMDOrderBook> MxMDSecurity::findOrderBook_(MxID venue, MxID segment)
{
  return m_orderBooks.findKey(MxMDOrderBook::venueSegment(venue, segment));
}

ZmRef<MxMDOrderBook> MxMDSecurity::addOrderBook(const MxSecKey &key,
    MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes,
    MxDateTime transactTime)
{
  return md()->addOrderBook(this, key, tickSizeTbl, lotSizes, transactTime);
}

void MxMDSecurity::addOrderBook_(MxMDOrderBook *ob)
{
  m_orderBooks.add(ob);
}

void MxMDSecurity::delOrderBook(MxID venue, MxID segment,
    MxDateTime transactTime)
{
  md()->delOrderBook(this, venue, segment, transactTime);
}

ZmRef<MxMDOrderBook> MxMDSecurity::delOrderBook_(MxID venue, MxID segment)
{
  return m_orderBooks.delKey(MxMDOrderBook::venueSegment(venue, segment));
}

void MxMDSecurity::update(
    const MxMDSecRefData &refData, MxDateTime transactTime)
{
  md()->updateSecurity(this, refData, transactTime);
  if (m_handler) m_handler->updatedSecurity(this, transactTime);
}

MxMDVenueShard::MxMDVenueShard(MxMDVenue *venue, MxMDShard *shard) :
    m_venue(venue), m_shard(shard), m_orderIDScope(venue->orderIDScope()),
    m_orders2(ZmHashParams().bits(4).loadFactor(1.0).cBits(4).
      init(ZuStringN<ZmHeapIDSize>() <<
	"MxMDVenueShard." << venue->id() << ".Orders2")),
    m_orders3(ZmHashParams().bits(4).loadFactor(1.0).cBits(4).
      init(ZuStringN<ZmHeapIDSize>() <<
	"MxMDVenueShard." << venue->id() << ".Orders3"))
{
}

MxMDVenue::MxMDVenue(MxMDLib *md, MxMDFeed *feed, MxID id,
    MxEnum orderIDScope, MxFlags flags) :
  m_md(md), m_feed(feed), m_id(id),
  m_orderIDScope(orderIDScope), m_flags(flags),
  m_shards(md->nShards()),
  m_segments(ZmHashParams().bits(2).init(
	ZuStringN<ZmHeapIDSize>() << "MxMDVenue." << id << ".Segments")),
  m_orders(ZmHashParams().bits(4).loadFactor(1.0).cBits(4).
    init(ZuStringN<ZmHeapIDSize>() << "MxMDVenue." << id << ".Orders"))
{
  unsigned n = md->nShards();
  m_shards.length(n);
  for (unsigned i = 0; i < n; i++)
    m_shards[i] = new MxMDVenueShard(this, md->shard_(i));
}

uintptr_t MxMDVenue::allTickSizeTbls(ZmFn<MxMDTickSizeTbl *> fn) const
{
  auto i = m_tickSizeTbls.readIterator();
  while (ZmRef<MxMDTickSizeTbl> tbl = i.iterateKey())
    if (uintptr_t v = fn(tbl)) return v;
  return 0;
}

ZmRef<MxMDTickSizeTbl> MxMDVenue::findTickSizeTbl_(ZuString id)
{
  return m_tickSizeTbls.findKey(id);
}

ZmRef<MxMDTickSizeTbl> MxMDVenue::addTickSizeTbl_(ZuString id, MxNDP pxNDP)
{
  ZmRef<MxMDTickSizeTbl> tbl = new MxMDTickSizeTbl(this, id, pxNDP);
  m_tickSizeTbls.add(tbl);
  return tbl;
}

ZmRef<MxMDTickSizeTbl> MxMDVenue::addTickSizeTbl(ZuString id, MxNDP pxNDP)
{
  return md()->addTickSizeTbl(this, id, pxNDP);
}

uintptr_t MxMDVenue::allSegments(ZmFn<const MxMDSegment &> fn) const
{
  auto i = m_segments.readIterator();
  uintptr_t v;
  while (const MxMDSegment &segment = i.iterateKey())
    if (v = fn(segment)) return v;
  return 0;
}

void MxMDVenue::tradingSession(MxMDSegment segment)
{
  md()->tradingSession(this, segment);
}

ZmRef<MxMDOrderBook> MxMDVenueShard::addCombination(
  MxID segment, ZuString id, MxNDP pxNDP, MxNDP qtyNDP,
  MxUInt legs, const ZmRef<MxMDSecurity> *securities,
  const MxEnum *sides, const MxRatio *ratios,
  MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes,
  MxDateTime transactTime)
{
  return md()->addCombination(
      this, segment, id, pxNDP, qtyNDP,
      legs, securities, sides, ratios, tickSizeTbl, lotSizes,
      transactTime);
}

void MxMDVenueShard::delCombination(MxID segment, ZuString id, MxDateTime transactTime)
{
  md()->delCombination(this, segment, id, transactTime);
}

MxMDFeed::MxMDFeed(MxMDLib *md, MxID id, unsigned level) :
  m_md(md),
  m_id(id),
  m_level(level)
{
}

void MxMDFeed::start() { }
void MxMDFeed::stop() { }

void MxMDFeed::final() { }

void MxMDFeed::addOrderBook(MxMDOrderBook *, MxDateTime) { }
void MxMDFeed::delOrderBook(MxMDOrderBook *, MxDateTime) { }

ZmRef<MxMDSecurity> MxMDShard::addSecurity(
    ZmRef<MxMDSecurity> sec,
    const MxSecKey &key, const MxMDSecRefData &refData,
    MxDateTime transactTime)
{
  return md()->addSecurity(this, ZuMv(sec), key, refData, transactTime);
}

uintptr_t MxMDShard::allSecurities(ZmFn<MxMDSecurity *> fn) const
{
  auto i = m_securities.readIterator();
  while (MxMDSecurity *security = i.iterateKey())
    if (uintptr_t v = fn(security)) return v;
  return 0;
}

uintptr_t MxMDShard::allOrderBooks(ZmFn<MxMDOrderBook *> fn) const
{
  auto i = m_orderBooks.readIterator();
  while (MxMDOrderBook *ob = i.iterateKey())
    if (uintptr_t v = fn(ob)) return v;
  return 0;
}

uintptr_t MxMDLib::allSecurities(ZmFn<MxMDSecurity *> fn) const
{
  thread_local ZmSemaphore sem;
  for (unsigned i = 0, n = m_shards.length(); i < n; i++) {
    uintptr_t v;
    m_shards[i]->invoke(
	[shard = m_shards[i], sem = &sem, &v, fn]() mutable {
	  v = shard->allSecurities(ZuMv(fn)); sem->post(); });
    sem.wait();
    if (v) return v;
  }
  return 0;
}

uintptr_t MxMDLib::allOrderBooks(ZmFn<MxMDOrderBook *> fn) const
{
  thread_local ZmSemaphore sem;
  for (unsigned i = 0, n = m_shards.length(); i < n; i++) {
    uintptr_t v;
    m_shards[i]->invoke(
	[shard = m_shards[i], sem = &sem, &v, fn]() mutable {
	  v = shard->allOrderBooks(ZuMv(fn)); sem->post(); });
    sem.wait();
    if (v) return v;
  }
  return 0;
}

uintptr_t MxMDLib::allFeeds(ZmFn<MxMDFeed *> fn) const
{
  auto i = m_feeds.readIterator();
  while (const ZmRef<MxMDFeed> &feed = i.iterateKey())
    if (uintptr_t v = fn(feed)) return v;
  return 0;
}

uintptr_t MxMDLib::allVenues(ZmFn<MxMDVenue *> fn) const
{
  auto i = m_venues.readIterator();
  while (const ZmRef<MxMDVenue> venue = i.iterateKey())
    if (uintptr_t v = fn(venue)) return v;
  return 0;
}

static void exception(MxMDLib *, ZmRef<ZeEvent> e) { ZeLog::log(ZuMv(e)); }

MxMDLib::MxMDLib(ZmScheduler *scheduler) :
  m_scheduler(scheduler),
  m_allSecurities(ZmHashParams().bits(12).loadFactor(1.0).cBits(4).
    init("MxMDLib.AllSecurities")),
  m_allOrderBooks(ZmHashParams().bits(12).loadFactor(1.0).cBits(4).
    init("MxMDLib.AllOrderBooks")),
  m_securities(ZmHashParams().bits(12).loadFactor(1.0).cBits(4).
    init("MxMDLib.Securities")),
#if 0
  m_feeds(ZmHashParams().bits(4).loadFactor(1.0).cBits(4).
    init("MxMDLib.Feeds")),
  m_venues(ZmHashParams().bits(4).loadFactor(1.0).cBits(4).
    init("MxMDLib.Venues")),
#endif
  m_handler(new MxMDLibHandler())
{
  m_handler->exception = MxMDExceptionFn::Ptr<&exception>::fn();
}

void MxMDLib::init_(void *cf_)
{
  ZvCf *cf = (ZvCf *)cf_;
  ZiMultiplex *mx = static_cast<ZiMultiplex *>(m_scheduler);
  if (ZmRef<ZvCf> shardsCf = cf->subset("shards", false)) {
    ZeLOG(Info, "MxMDLib - configuring shards...");
    m_shards.length(shardsCf->count());
    ZvCf::Iterator i(shardsCf);
    ZuString key;
    while (ZmRef<ZvCf> shardCf = i.subset(key)) {
      ZuBox<unsigned> id = key;
      if (id >= m_shards.length())
	throw ZvCf::RangeInt(0, m_shards.size(), id, "shards");
      unsigned tid = shardCf->getInt("tid", 1, mx->nThreads(), true);
      m_shards[id] = new MxMDShard(this, id, tid);
    }
  } else {
    int tid = mx->workerID(0);
    if (tid < 0)
      throw ZtString("mx misconfigured - no worker threads");
    m_shards.push(new MxMDShard(this, 0, tid));
  }

  // Assumption: DST transitions do not occur while market is open
  {
    ZtDate now(ZtDate::Now);
    ZuString timezone = cf->get("timezone"); // default to system tz
    now.sec() = 0, now.nsec() = 0; // midnight GMT (start of today)
    now += ZmTime((time_t)(now.offset(timezone) + 43200)); // midday local time
    m_tzOffset = now.offset(timezone);
  }
}

void MxMDLib::subscribe(MxMDLibHandler *handler)
{
  SubGuard guard(m_subLock);
  if (!handler->exception)
    handler->exception = MxMDExceptionFn::Ptr<&exception>::fn();
  m_handler = handler;
}

void MxMDLib::unsubscribe()
{
  SubGuard guard(m_subLock);
  m_handler = new MxMDLibHandler();
  m_handler->exception = MxMDExceptionFn::Ptr<&exception>::fn();
}

void MxMDLib::sync()
{
  thread_local ZmSemaphore sem;
  unsigned n = nShards();
  for (unsigned i = 0; i < n; i++)
    shardInvoke(i, ZmFn<>{[](ZmSemaphore *sem) { sem->post(); }, &sem});
  for (unsigned i = 0; i < n; i++) sem.wait();
}

void MxMDLib::raise(ZmRef<ZeEvent> e)
{
  handler()->exception(this, ZuMv(e));
}

void MxMDLib::addFeed(MxMDFeed *feed)
{
  m_feeds.add(feed);
}

void MxMDLib::addVenue(MxMDVenue *venue)
{
  m_venues.add(venue);
}

void MxMDLib::loaded(MxMDVenue *venue)
{
  sync();
  venue->loaded_(1);
  handler()->refDataLoaded(venue);
  {
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::refDataLoaded(core->broadcast(), venue->id());
  }
}

void MxMDLib::addVenueMapping(MxMDVenueMapKey key, MxMDVenueMapping map)
{
  m_venueMap.add(key, map);
}

MxMDVenueMapping MxMDLib::venueMapping(MxMDVenueMapKey key)
{
  return m_venueMap.findVal(key);
}

ZmRef<MxMDTickSizeTbl> MxMDLib::addTickSizeTbl(
    MxMDVenue *venue, ZuString id, MxNDP pxNDP)
{
  ZmRef<MxMDTickSizeTbl> tbl;
  {
    Guard guard(m_refDataLock);
    if (tbl = venue->findTickSizeTbl_(id)) return tbl;
    tbl = venue->addTickSizeTbl_(id, pxNDP);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::addTickSizeTbl(core->broadcast(), venue->id(), id);
  }
  handler()->addTickSizeTbl(tbl);
  return tbl;
}

void MxMDTickSizeTbl::reset_()
{
  m_tickSizes.clean();
}

void MxMDLib::resetTickSizeTbl(MxMDTickSizeTbl *tbl)
{
  {
    Guard guard(m_refDataLock);
    tbl->reset_();
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::resetTickSizeTbl(core->broadcast(),
	    tbl->venue()->id(), tbl->id());
  }
  handler()->resetTickSizeTbl(tbl);
}

void MxMDTickSizeTbl::addTickSize_(
    MxValue minPrice, MxValue maxPrice, MxValue tickSize)
{
  {
    auto i = m_tickSizes.iterator<ZmRBTreeGreaterEqual>(minPrice);
    while (const TickSizes::Node *node = i.iterate())
      if (node->key().minPrice() <= maxPrice)
	i.del();
      else
	break;
  }
  m_tickSizes.add(MxMDTickSize{minPrice, maxPrice, tickSize});
}

void MxMDLib::addTickSize(MxMDTickSizeTbl *tbl,
    MxValue minPrice, MxValue maxPrice, MxValue tickSize)
{
  {
    Guard guard(m_refDataLock);
    tbl->addTickSize_(minPrice, maxPrice, tickSize);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::addTickSize(core->broadcast(),
	    tbl->venue()->id(), tbl->id(), tbl->pxNDP(),
	    minPrice, maxPrice, tickSize);
  }
  handler()->addTickSize(tbl, MxMDTickSize{minPrice, maxPrice, tickSize});
}

ZmRef<MxMDSecurity> MxMDLib::addSecurity(
    MxMDShard *shard, ZmRef<MxMDSecurity> sec,
    const MxSecKey &key, const MxMDSecRefData &refData,
    MxDateTime transactTime)
{
  {
    Guard guard(m_refDataLock);
    if (sec) {
      guard.unlock();
      sec->update(refData, transactTime);
      return sec;
    }
    sec = new MxMDSecurity(shard, key, refData);
    m_allSecurities.add(sec);
    if (key.id == "6502") {
      ZmAssert((key == MxSecKey{"6502", "XTKS", MxID()}));
      ZmAssert(m_allSecurities.findKey(MxSecKey{"6502", "XTKS", MxID()}));
    }
    addSecIndices(sec, refData, transactTime);
    shard->addSecurity(sec);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::addSecurity(core->broadcast(), shard->id(), transactTime,
	  key, refData);
  }
  handler()->addSecurity(sec, transactTime);
  return sec;
}


void MxMDSecurity::update_(
    const MxMDSecRefData &refData, MxDateTime transactTime)
{
  m_refData.tradeable.update(refData.tradeable);
  m_refData.idSrc.update(refData.idSrc);
  m_refData.symbol.update(refData.symbol);
  m_refData.altIDSrc.update(refData.altIDSrc);
  m_refData.altSymbol.update(refData.altSymbol);
  m_refData.underVenue.update(refData.underVenue);
  m_refData.underSegment.update(refData.underSegment);
  m_refData.underlying.update(refData.underlying);
  m_refData.mat.update(refData.mat);
  if ((*refData.pxNDP && refData.pxNDP != m_refData.pxNDP) ||
      (*refData.qtyNDP && refData.qtyNDP != m_refData.qtyNDP)) {
    allOrderBooks([
	oldPxNDP = m_refData.pxNDP, newPxNDP = refData.pxNDP,
	oldQtyNDP = m_refData.qtyNDP, newQtyNDP = refData.qtyNDP,
	transactTime](
	  MxMDOrderBook *ob) -> uintptr_t {
      MxMDL1Data &l1 = ob->m_l1Data;
#ifdef adjustNDP
#undef adjustNDP
#endif
#define adjustNDP(v, n) if (*v) v = \
    MxValNDP{v, old ## n ## NDP}.adjust(new ## n ## NDP)
      adjustNDP(l1.base, Px);
      for (unsigned i = 0; i < MxMDNSessions; i++) adjustNDP(l1.open[i], Px);
      for (unsigned i = 0; i < MxMDNSessions; i++) adjustNDP(l1.close[i], Px);
      adjustNDP(l1.last, Px); adjustNDP(l1.lastQty, Qty);
      adjustNDP(l1.bid, Px); adjustNDP(l1.bidQty, Qty);
      adjustNDP(l1.ask, Px); adjustNDP(l1.askQty, Qty);
      adjustNDP(l1.high, Px);
      adjustNDP(l1.low, Px);
      adjustNDP(l1.accVol, Px); adjustNDP(l1.accVolQty, Qty);
      adjustNDP(l1.match, Px); adjustNDP(l1.matchQty, Qty);
      adjustNDP(l1.surplusQty, Qty);
#undef adjustNDP
      l1.pxNDP = newPxNDP;
      l1.qtyNDP = newQtyNDP;
      ob->reset(transactTime);
      return 0;
    });
#define adjustNDP(v, n) if (*m_refData.v && !*refData.v) m_refData.v = \
    MxValNDP{m_refData.v, m_refData.n ## NDP}.adjust(refData.n ## NDP)
    adjustNDP(strike, px);
    adjustNDP(outstandingShares, qty);
    adjustNDP(adv, px);
    m_refData.pxNDP = refData.pxNDP;
    m_refData.qtyNDP = refData.qtyNDP;
  }
  m_refData.putCall.update(refData.putCall);
  m_refData.strike.update(refData.strike);
  m_refData.outstandingShares.update(
      refData.outstandingShares, MxValueReset);
  m_refData.adv.update(refData.adv, MxValueReset);
}

void MxMDLib::updateSecurity(
  MxMDSecurity *security, const MxMDSecRefData &refData,
  MxDateTime transactTime)
{
  {
    Guard guard(m_refDataLock);

    MxMDSecRefData oldRefData;

    oldRefData = security->refData();
    security->update_(refData, transactTime);

    delSecIndices(security, oldRefData);
    addSecIndices(security, security->refData(), transactTime);

    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::updateSecurity(core->broadcast(), security->shard()->id(),
	  transactTime, security->key(), refData);
  }
  handler()->updatedSecurity(security, transactTime);
}

void MxMDLib::addSecIndices(
    MxMDSecurity *security, const MxMDSecRefData &refData,
    MxDateTime transactTime)
{
  if (*refData.idSrc && refData.symbol)
    m_securities.add(
	MxSymKey{.id = refData.symbol, .src = refData.idSrc}, security);
  if (*refData.altIDSrc && refData.altSymbol)
    m_securities.add(
	MxSymKey{.id = refData.altSymbol, .src = refData.altIDSrc}, security);
  if (*refData.underVenue && refData.underlying && *refData.mat) {
    MxSecKey underKey{
      refData.underVenue, refData.underSegment, refData.underlying};
    ZmRef<MxMDSecurity> underlying = m_allSecurities.findKey(underKey);
    if (!underlying) {
      MxMDShard *shard = security->shard();
      MxMDSecRefData underRefData{0}; // default to non-tradable
      ZmRef<MxMDSecurity> underlying_ =
	new MxMDSecurity(shard, underKey, underRefData);
      m_allSecurities.add(underlying_);
      shard->addSecurity(underlying_);
      MxMDCore *core = static_cast<MxMDCore *>(this);
      if (ZuUnlikely(core->streaming()))
	MxMDStream::addSecurity(core->broadcast(), shard->id(), transactTime,
	      underKey, underRefData);
      underlying = underlying_;
    }
    security->underlying(underlying);
    underlying->addDerivative(security);
  }
}

void MxMDLib::delSecIndices(
    MxMDSecurity *security, const MxMDSecRefData &refData)
{
  if (*refData.idSrc && refData.symbol)
    m_securities.delKey(
	MxSymKey{.id = refData.symbol, .src = refData.idSrc});
  if (*refData.altIDSrc && refData.altSymbol)
    m_securities.delKey(
	MxSymKey{.id = refData.altSymbol, .src = refData.altIDSrc});
  if (*refData.underVenue && refData.underlying && *refData.mat)
    if (MxMDSecurity *underlying = security->underlying()) {
      underlying->delDerivative(security);
      security->underlying(0);
    }
}

void MxMDDerivatives::add(MxMDSecurity *security)
{
  const MxMDSecRefData &refData = security->refData();
  if (*refData.putCall && *refData.strike)
    m_options.add(MxOptKey{.strike = refData.strike,
	.mat = refData.mat, .putCall = refData.putCall}, security);
  else
    m_futures.add(MxFutKey{refData.mat}, security);
}

void MxMDDerivatives::del(MxMDSecurity *security)
{
  const MxMDSecRefData &refData = security->refData();
  if (*refData.putCall && *refData.strike)
    m_options.delVal(MxOptKey{.strike = refData.strike,
	.mat = refData.mat, .putCall = refData.putCall});
  else
    m_futures.delVal(MxFutKey(refData.mat));
}

uintptr_t MxMDDerivatives::allFutures(ZmFn<MxMDSecurity *> fn) const
{
  auto i = m_futures.readIterator();
  while (MxMDSecurity *future = i.iterateVal())
    if (uintptr_t v = fn(future)) return v;
  return 0;
}

uintptr_t MxMDDerivatives::allOptions(ZmFn<MxMDSecurity *> fn) const
{
  auto i = m_options.readIterator();
  while (MxMDSecurity *option = i.iterateVal())
    if (uintptr_t v = fn(option)) return v;
  return 0;
}

ZmRef<MxMDOrderBook> MxMDLib::addOrderBook(
    MxMDSecurity *security, MxSecKey key,
    MxMDTickSizeTbl *tickSizeTbl, MxMDLotSizes lotSizes,
    MxDateTime transactTime)
{
  if (ZuUnlikely(!*key.venue)) {
    raise(ZeEVENT(Error,
      ([id = key.id](const ZeEvent &, ZmStream &s) {
	s << "addOrderBook - null venueID for \"" << id << '"';
      })));
    return 0;
  }
  ZmRef<MxMDOrderBook> newOB, inOB, ob;
  unsigned inRank = 0;
loop:
  {
    Guard guard(m_refDataLock);
    if (ob = security->findOrderBook_(key.venue, key.segment)) {
      guard.unlock();
      if (!newOB) ob->update(tickSizeTbl, lotSizes, transactTime);
      goto added;
    }
    ZmRef<MxMDVenue> venue = m_venues.findKey(key.venue);
    if (ZuUnlikely(!venue)) {
      raise(ZeEVENT(Error,
	([venueID = key.venue, id = key.id](const ZeEvent &, ZmStream &s) {
	  s << "addOrderBook - no such venue for \"" << id << "\" " << venueID;
	})));
      return newOB;
    }
    ob = new MxMDOrderBook(
      security->shard(), venue,
      key.segment, key.id, security, tickSizeTbl, lotSizes,
      security->handler());
    m_allOrderBooks.add(ob);
    ob->shard()->addOrderBook(ob);
    ob->security()->addOrderBook_(ob);
    ob->venue()->feed()->addOrderBook(ob, transactTime);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::addOrderBook(core->broadcast(), ob->shard()->id(),
	  transactTime, key, security->key(), tickSizeTbl->id(),
	  ob->qtyNDP(), ob->lotSizes());
  }
  handler()->addOrderBook(ob, transactTime);
added:
  if (!newOB)
    newOB = ob;
  else {
    if (inOB) inOB->map(inRank, ob);
  }
  if (MxMDVenueMapping mapping =
      venueMapping(MxMDVenueMapKey(key.venue, key.segment))) {
    key.id = security->id();
    key.venue = mapping.venue;
    key.segment = mapping.segment;
    tickSizeTbl = 0;
    lotSizes = MxMDLotSizes();
    inOB = ob;
    inRank = mapping.rank;
    goto loop;
  }
  return newOB;
}

ZmRef<MxMDOrderBook> MxMDLib::addCombination(
    MxMDVenueShard *venueShard, MxID segment, ZuString id,
    MxNDP pxNDP, MxNDP qtyNDP,
    MxUInt legs, const ZmRef<MxMDSecurity> *securities,
    const MxEnum *sides, const MxRatio *ratios,
    MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes,
    MxDateTime transactTime)
{
  MxMDShard *shard = venueShard->shard();
  MxMDVenue *venue = venueShard->venue();
  ZmRef<MxMDOrderBook> ob;
  {
    Guard guard(m_refDataLock);
    if (ob = m_allOrderBooks.findKey(
	  MxSecKey{.id = id, .venue = venue->id(), .segment = segment})) {
      guard.unlock();
      ob->update(tickSizeTbl, lotSizes, transactTime);
      return ob;
    }
    ob = new MxMDOrderBook(shard, venue,
      segment, id, pxNDP, qtyNDP, legs, securities, sides, ratios,
      tickSizeTbl, lotSizes);
    m_allOrderBooks.add(ob);
    shard->addOrderBook(ob);
    venue->feed()->addOrderBook(ob, transactTime);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming())) {
      MxSecKey securityKeys[MxMDNLegs];
      for (unsigned i = 0; i < legs; i++)
	securityKeys[i] = securities[i]->key();
      MxMDStream::addCombination(core->broadcast(), shard->id(),
	  transactTime, ob->key(), pxNDP, qtyNDP,
	  legs, securityKeys, sides, ratios, tickSizeTbl->id(), lotSizes);
    }
  }
  handler()->addOrderBook(ob, transactTime);
  return ob;
}

void MxMDLib::updateOrderBook(
  MxMDOrderBook *ob,
  const MxMDTickSizeTbl *tickSizeTbl,
  const MxMDLotSizes &lotSizes,
  MxDateTime transactTime)
{
  {
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::updateOrderBook(core->broadcast(), ob->shard()->id(),
	  transactTime, ob->key(), tickSizeTbl->id(), lotSizes);
  }
  handler()->updatedOrderBook(ob, transactTime);
}

void MxMDLib::delOrderBook(
    MxMDSecurity *security, MxID venue, MxID segment,
    MxDateTime transactTime)
{
  ZmRef<MxMDOrderBook> ob;
  {
    Guard guard(m_refDataLock);
    ob = security->delOrderBook_(venue, segment);
    if (!ob) return;
    m_allOrderBooks.delKey(
	MxSecKey{.id = ob->id(), .venue = venue, .segment = segment});
    ob->shard()->delOrderBook(ob);
    ob->venue()->feed()->delOrderBook(ob, transactTime);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::delOrderBook(core->broadcast(), ob->shard()->id(),
	  transactTime, ob->key());
  }
  ob->unsubscribe();
  handler()->deletedOrderBook(ob, transactTime);
}

void MxMDLib::delCombination(
    MxMDVenueShard *venueShard, MxID segment, ZuString id,
    MxDateTime transactTime)
{
  MxMDShard *shard = venueShard->shard();
  MxMDVenue *venue = venueShard->venue();
  ZmRef<MxMDOrderBook> ob;
  {
    Guard guard(m_refDataLock);
    ob = m_allOrderBooks.delKey(
	MxSecKey{.id = id, .venue = venue->id(), .segment = segment});
    if (!ob) return;
    shard->delOrderBook(ob);
    venue->feed()->delOrderBook(ob, transactTime);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::delCombination(core->broadcast(), ob->shard()->id(),
	  transactTime, ob->key());
  }
  ob->unsubscribe();
  handler()->deletedOrderBook(ob, transactTime);
}

void MxMDLib::tradingSession(MxMDVenue *venue, MxMDSegment segment)
{
  {
    Guard guard(m_refDataLock);
    venue->tradingSession_(segment);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::tradingSession(core->broadcast(), segment.stamp,
	    venue->id(), segment.id, segment.session);
  }
  handler()->tradingSession(venue, segment);
}

void MxMDLib::l1(const MxMDOrderBook *ob, const MxMDL1Data &l1Data)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::l1(core->broadcast(), ob->shard()->id(), ob->key(), l1Data);
}

void MxMDLib::pxLevel(
  const MxMDOrderBook *ob, MxEnum side, MxDateTime transactTime,
  bool delta, MxValue price, MxValue qty, MxUInt nOrders, MxFlags flags)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::pxLevel(core->broadcast(), ob->shard()->id(),
	transactTime, ob->key(), side, delta, ob->pxNDP(), ob->qtyNDP(),
	price, qty, nOrders, flags);
}

void MxMDLib::l2(const MxMDOrderBook *ob, MxDateTime stamp, bool updateL1)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::l2(core->broadcast(), ob->shard()->id(),
	stamp, ob->key(), updateL1);
}

void MxMDLib::addOrder(const MxMDOrderBook *ob,
    ZuString orderID, MxDateTime transactTime,
    MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::addOrder(core->broadcast(), ob->shard()->id(),
	transactTime, ob->key(), orderID, side, ob->pxNDP(), ob->qtyNDP(),
	rank, price, qty, flags);
}
void MxMDLib::modifyOrder(const MxMDOrderBook *ob,
    ZuString orderID, MxDateTime transactTime,
    MxEnum side, MxUInt rank, MxValue price, MxValue qty, MxFlags flags)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::modifyOrder(core->broadcast(), ob->shard()->id(),
	transactTime, ob->key(), orderID, side, ob->pxNDP(), ob->qtyNDP(),
	rank, price, qty, flags);
}
void MxMDLib::cancelOrder(const MxMDOrderBook *ob,
    ZuString orderID,
    MxDateTime transactTime, MxEnum side)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::cancelOrder(core->broadcast(), ob->shard()->id(),
	transactTime, ob->key(), orderID, side);
}

void MxMDLib::resetOB(const MxMDOrderBook *ob,
    MxDateTime transactTime)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::resetOB(core->broadcast(), ob->shard()->id(),
	transactTime, ob->key());
}

void MxMDLib::addTrade(const MxMDOrderBook *ob,
    ZuString tradeID, MxDateTime transactTime, MxValue price, MxValue qty)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::addTrade(core->broadcast(), transactTime,
	ob->key(), tradeID, ob->pxNDP(), ob->qtyNDP(), price, qty);
}
void MxMDLib::correctTrade(const MxMDOrderBook *ob,
    ZuString tradeID, MxDateTime transactTime, MxValue price, MxValue qty)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::correctTrade(core->broadcast(), transactTime,
	ob->key(), tradeID, ob->pxNDP(), ob->qtyNDP(), price, qty);
}
void MxMDLib::cancelTrade(const MxMDOrderBook *ob,
    ZuString tradeID, MxDateTime transactTime, MxValue price, MxValue qty)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::cancelTrade(core->broadcast(), transactTime,
	ob->key(), tradeID, ob->pxNDP(), ob->qtyNDP(), price, qty);
}

// commands

ZtString MxMDLib::lookupSyntax()
{
  return 
    "S src src { type scalar } "
    "v venue venue { type scalar } "
    "s segment segment { type scalar } "
    "m maturity maturity { type scalar } "
    "p put put { type flag } "
    "c call call { type flag } "
    "x strike strike { type scalar }";
}

ZtString MxMDLib::lookupOptions()
{
  return
    "    -S, --src=SRC\t- symbol ID source is SRC\n"
    "\t(CUSIP|SEDOL|QUIK|ISIN|RIC|EXCH|CTA|BSYM|BBGID|FX|CRYPTO)\n"
    "    -v, --venue=MIC\t - market MIC, e.g. XTKS\n"
    "    -s, --segment=SEGMENT\t- market segment SEGMENT\n"
    "    -m, --maturity=MAT\t- maturity (YYYYMMDD - DD is normally 00)\n"
    "    -p, --put\t\t- put option\n"
    "    -c, --call\t\t- call option\n"
    "    -x, --strike\t- strike price (as integer, per security convention)\n";
}

MxUniKey MxMDLib::parseSecurity(ZvCf *args, unsigned index) const
{
  MxUniKey key;
  key.id = args->get(ZuStringN<16>(ZuBoxed(index)));
  if (ZtString src_ = args->get("src")) {
    key.src = MxSecIDSrc::lookup(src_);
  } else {
    key.venue = args->get("venue", true);
    key.segment = args->get("segment");
  }
  if (ZtString mat = args->get("mat")) {
    const auto &r = ZtStaticRegexUTF8("^\\d{8}$");
    if (!r.m(mat))
      throw ZtString() << "maturity \"" << mat << "\" invalid - "
	"must be YYYYMMDD (DD is usually 00)";
    key.mat = mat;
    bool put = args->getInt("put", 0, 1, false, 0);
    bool call = args->getInt("call", 0, 1, false, 0);
    ZtString strike = args->get("strike");
    if (put && call)
      throw ZtString() << "put and call are mutually exclusive";
    if (put || call) {
      if (!strike)
	throw ZtString() << "strike must be specified for options";
      key.putCall = put ? MxPutCall::PUT : MxPutCall::CALL;
      key.strike = strike;
    }
  }
  return key;
}

bool MxMDLib::lookupSecurity(
    const MxUniKey &key, bool secRequired, ZmFn<MxMDSecurity *> fn) const
{
  bool ok = true;
  thread_local ZmSemaphore sem;
  secInvoke(key, [secRequired, sem = &sem, &ok, fn = ZuMv(fn)](
	MxMDSecurity *sec) {
      if (secRequired && ZuUnlikely(!sec))
	ok = false;
      else
	ok = fn(sec);
      sem->post();
    });
  sem.wait();
  return ok;
}

MxUniKey MxMDLib::parseOrderBook(ZvCf *args, unsigned index) const
{
  MxUniKey key{parseSecurity(args, index)};
  if (*key.src) {
    key.venue = args->get("venue", true);
    key.segment = args->get("segment");
  }
  return key;
}

bool MxMDLib::lookupOrderBook(
    const MxUniKey &key,
    bool secRequired, bool obRequired,
    ZmFn<MxMDSecurity *, MxMDOrderBook *> fn) const
{
  return lookupSecurity(key, secRequired || obRequired,
      [key = key, obRequired, fn = ZuMv(fn)](MxMDSecurity *sec) -> bool {
    ZmRef<MxMDOrderBook> ob = sec->orderBook(key.venue, key.segment);
    if (obRequired && ZuUnlikely(!ob))
      return false;
    else
      return fn(sec, ob);
  });
}
