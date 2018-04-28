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
  MxFloat minPrice, MxFloat maxPrice, MxFloat tickSize)
{
  m_venue->md()->addTickSize(this, minPrice, maxPrice, tickSize);
}

MxMDOrderBook::MxMDOrderBook( // single leg
  MxMDShard *shard, 
  MxMDVenue *venue,
  MxMDOrderBook *consolidated,
  MxID segment, ZuString id,
  MxMDSecurity *security,
  MxMDTickSizeTbl *tickSizeTbl,
  const MxMDLotSizes &lotSizes,
  MxMDSecHandler *handler) :
    MxMDSharded(shard),
    m_venue(venue),
    m_venueShard(venue ? venue->shard(shard->id()) : (MxMDVenueShard *)0),
    m_consolidated(consolidated),
    m_key(venue ? venue->id() : MxID(), segment, id),
    m_legs(1),
    m_tickSizeTbl(tickSizeTbl),
    m_lotSizes(lotSizes),
    m_bids(new MxMDOBSide(this, MxSide::Buy)),
    m_asks(new MxMDOBSide(this, MxSide::Sell)),
    m_handler(handler)
{
  m_sides[0] = MxEnum();
  m_ratios[0] = MxFloat();
  m_securities[0] = security;
}

MxMDOrderBook::MxMDOrderBook( // multi-leg
  MxMDShard *shard, 
  MxMDVenue *venue,
  MxID segment, ZuString id,
  MxUInt legs, const ZmRef<MxMDSecurity> *securities,
  const MxEnum *sides, const MxFloat *ratios,
  MxMDTickSizeTbl *tickSizeTbl,
  const MxMDLotSizes &lotSizes) :
    MxMDSharded(shard),
    m_venue(venue),
    m_consolidated(0),
    m_key(venue->id(), segment, id),
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
  m_l1Data.stamp = l1Data.stamp;
  m_l1Data.status.update(l1Data.status);
  m_l1Data.base.update(l1Data.base, -MxFloat::inf());
  for (int i = 0; i < MxMDNSessions; i++) {
    m_l1Data.open[i].update(l1Data.open[i], -MxFloat::inf());
    m_l1Data.close[i].update(l1Data.close[i], -MxFloat::inf());
  }
  // permit the feed to reset tickDir/high/low
  m_l1Data.tickDir.update(l1Data.tickDir);
  m_l1Data.high.update(l1Data.high, -MxFloat::inf());
  m_l1Data.low.update(l1Data.low, -MxFloat::inf());
  // update tickDir/high/low based on last
  if (*l1Data.last) {
    if (!*m_l1Data.last)
      m_l1Data.tickDir = l1Data.tickDir = MxEnum();
    else if (l1Data.last.feq(m_l1Data.last)) {
      if (m_l1Data.tickDir == MxTickDir::Up)
	m_l1Data.tickDir = l1Data.tickDir = MxTickDir::LevelUp;
      else if (m_l1Data.tickDir == MxTickDir::Down)
	m_l1Data.tickDir = l1Data.tickDir = MxTickDir::LevelDown;
    } else if (l1Data.last.fgt(m_l1Data.last))
      m_l1Data.tickDir = l1Data.tickDir = MxTickDir::Up;
    else if (l1Data.last.flt(m_l1Data.last))
      m_l1Data.tickDir = l1Data.tickDir = MxTickDir::Down;
    if (!*m_l1Data.high || m_l1Data.high.flt(l1Data.last))
      m_l1Data.high = l1Data.high = l1Data.last;
    if (!*m_l1Data.low || m_l1Data.low.fgt(l1Data.last))
      m_l1Data.low = l1Data.low = l1Data.last;
  }
  m_l1Data.last.update(l1Data.last, -MxFloat::inf());
  m_l1Data.lastQty.update(l1Data.lastQty, -MxFloat::inf());
  m_l1Data.bid.update(l1Data.bid, -MxFloat::inf());
  m_l1Data.bidQty.update(l1Data.bidQty, -MxFloat::inf());
  m_l1Data.ask.update(l1Data.ask, -MxFloat::inf());
  m_l1Data.askQty.update(l1Data.askQty, -MxFloat::inf());
  m_l1Data.accVol.update(l1Data.accVol, -MxFloat::inf());
  m_l1Data.accVolQty.update(l1Data.accVolQty, -MxFloat::inf());
  m_l1Data.match.update(l1Data.match, -MxFloat::inf());
  m_l1Data.matchQty.update(l1Data.matchQty, -MxFloat::inf());
  m_l1Data.surplusQty.update(l1Data.surplusQty, -MxFloat::inf());
  m_l1Data.flags = l1Data.flags;

  md()->l1(this, l1Data);
  if (m_handler) m_handler->l1(this, l1Data);
}

bool MxMDOBSide::updateL1Bid(MxMDL1Data &l1Data, MxMDL1Data &delta)
{
  ZuRef<MxMDPxLevel> bid = m_pxLevels.maximumKey();
  if (bid) {
    if (l1Data.bid.fne(bid->price()) ||
	l1Data.bidQty.fne(bid->data().qty)) {
      l1Data.bid = delta.bid = bid->price();
      l1Data.bidQty = delta.bidQty = bid->data().qty;
      return true;
    }
  } else {
    if (*l1Data.bid) {
      delta.bid = delta.bidQty = -MxFloat::inf();
      l1Data.bid = l1Data.bidQty = MxFloat();
      return true;
    }
  }
  return false;
}

bool MxMDOBSide::updateL1Ask(MxMDL1Data &l1Data, MxMDL1Data &delta)
{
  ZuRef<MxMDPxLevel> ask = m_pxLevels.minimumKey();
  if (ask) {
    if (l1Data.ask.fne(ask->price()) ||
	l1Data.askQty.fne(ask->data().qty)) {
      l1Data.ask = delta.ask = ask->price();
      l1Data.askQty = delta.askQty = ask->data().qty;
      return true;
    }
  } else {
    if (*l1Data.ask) {
      delta.ask = delta.askQty = -MxFloat::inf();
      l1Data.ask = l1Data.askQty = MxFloat();
      return true;
    }
  }
  return false;
}

void MxMDOrderBook::l2(MxDateTime stamp, bool updateL1)
{
  MxMDL1Data delta;
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

#undef MxOrderBook_update

void MxMDOrderBook::pxLevel(
  MxEnum side, MxDateTime transactTime, bool delta,
  MxFloat price, MxFloat qty, MxFloat nOrders, MxFlags flags)
{
  MxFloat d_qty, d_nOrders;
  pxLevel_(side, transactTime, delta, price, qty, nOrders, flags, false,
      &d_qty, &d_nOrders);
  if (MxMDOrderBook *consolidated = this->consolidated())
    consolidated->pxLevel_(
	side, transactTime, true, price, d_qty, d_nOrders, flags, true, 0, 0);
}

void MxMDOBSide::pxLevel_(
  MxDateTime transactTime, bool delta,
  MxFloat price, MxFloat qty, MxFloat nOrders, MxFlags flags,
  const MxMDSecHandler *handler,
  MxFloat &d_qty, MxFloat &d_nOrders,
  const MxMDPxLevelFn *&pxLevelFn,
  ZuRef<MxMDPxLevel> &pxLevel)
{
  pxLevelFn = 0;
  if (ZuUnlikely(!*price)) {
    if (!m_mktLevel) {
      if (qty.fne(0.0)) {
	d_qty = qty, d_nOrders = nOrders;
	pxLevel = m_mktLevel = new MxMDPxLevel(
	    this, m_side, transactTime, MxFloat(), qty, nOrders, flags);
	pxLevelFn = &handler->addMktLevel;
      } else {
	pxLevel = 0;
	pxLevelFn = 0;
	d_qty = 0, d_nOrders = 0;
      }
    } else {
      pxLevel = m_mktLevel;
      m_mktLevel->update(
	  transactTime, delta, qty, nOrders, flags, d_qty, d_nOrders);
      if (m_mktLevel->data().qty.fne(0.0))
	pxLevelFn = &handler->updatedMktLevel;
      else {
	m_mktLevel = 0;
	pxLevelFn = &handler->deletedMktLevel;
      }
    }
    if (d_qty.fne(0.0)) m_data.qty += d_qty;
    return;
  }
  pxLevel = m_pxLevels.findKey(price);
  if (!pxLevel) {
    if (qty.fne(0.0)) {
      d_qty = qty, d_nOrders = nOrders;
      pxLevel = new MxMDPxLevel(
	  this, m_side, transactTime, price, qty, nOrders, flags);
      pxLevelFn = &handler->addPxLevel;
      m_pxLevels.add(pxLevel);
    } else {
      pxLevel = 0;
      pxLevelFn = 0;
      d_qty = 0, d_nOrders = 0;
    }
  } else {
    pxLevel->update(
	transactTime, delta, qty, nOrders, flags, d_qty, d_nOrders);
    if (pxLevel->data().qty.fne(0.0))
      pxLevelFn = &handler->updatedPxLevel;
    else {
      pxLevelFn = &handler->deletedPxLevel;
      m_pxLevels.delVal(price);
    }
  }
  if (d_qty.fne(0.0)) {
    m_data.qty += d_qty;
    m_data.nv += price * d_qty;
  }
}

void MxMDOrderBook::pxLevel_(
  MxEnum side, MxDateTime transactTime, bool delta,
  MxFloat price, MxFloat qty, MxFloat nOrders, MxFlags flags,
  bool consolidated, MxFloat *d_qty_, MxFloat *d_nOrders_)
{
  MxFloat d_qty, d_nOrders;
  const MxMDPxLevelFn *pxLevelFn;
  ZuRef<MxMDPxLevel> pxLevel;

  {
    MxMDOBSide *obSide = side == MxSide::Buy ? m_bids : m_asks;
    obSide->pxLevel_(
	transactTime, delta, price, qty, nOrders, flags, m_handler,
	d_qty, d_nOrders, pxLevelFn, pxLevel);
    if (!consolidated)
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
    const MxMDPxLevelFn *&pxLevelFn, ZuRef<MxMDPxLevel> &pxLevel)
{
  const MxMDOrderData &orderData = order->data();
  if (!*orderData.price) {
    if (!m_mktLevel) {
      pxLevelFn = &handler->addMktLevel;
      pxLevel = m_mktLevel = new MxMDPxLevel(this,
	  orderData.side, transactTime,
	  MxFloat(), orderData.qty, 1, 0);
    } else {
      pxLevelFn = &handler->updatedMktLevel;
      pxLevel = m_mktLevel;
      m_mktLevel->updateDelta(transactTime, orderData.qty, 1, 0);
    }
    order->pxLevel(m_mktLevel);
    m_mktLevel->addOrder(order);
    m_data.qty += orderData.qty;
    return;
  }
  pxLevel = m_pxLevels.findKey(orderData.price);
  if (!pxLevel) {
    pxLevelFn = &handler->addPxLevel;
    pxLevel = new MxMDPxLevel(this,
      orderData.side, transactTime,
      orderData.price, orderData.qty, 1, 0);
    m_pxLevels.add(pxLevel);
  } else {
    pxLevelFn = &handler->updatedPxLevel;
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
  const MxMDPxLevelFn *&pxLevelFn, ZuRef<MxMDPxLevel> &pxLevel)
{
  const MxMDOrderData &orderData = order->data();
  if (orderData.qty.fgt(0.0)) {
    MxMDOBSide *obSide = orderData.side == MxSide::Buy ? m_bids : m_asks;
    obSide->addOrder_(order, transactTime, handler, pxLevelFn, pxLevel);
  }
}

void MxMDOBSide::delOrder_(
    MxMDOrder *order, MxDateTime transactTime,
    const MxMDSecHandler *handler,
    const MxMDPxLevelFn *&pxLevelFn, ZuRef<MxMDPxLevel> &pxLevel)
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
    if (m_mktLevel->data().qty.feq(0.0)) {
      pxLevelFn = &handler->deletedMktLevel;
      m_mktLevel = 0;
    } else
      pxLevelFn = &handler->updatedMktLevel;
    m_data.qty -= orderData.qty;
    return;
  }
  pxLevel = order->pxLevel();
  if (ZuUnlikely(!pxLevel)) {
    m_orderBook->md()->raise(ZeEVENT(Error, MxMDNoPxLevel("delOrder")));
    return;
  }
  pxLevel->updateDelta(transactTime, -orderData.qty, -1, 0);
  pxLevel->delOrder(orderData.rank);
  if (pxLevel->data().qty.feq(0.0)) {
    pxLevelFn = &handler->deletedPxLevel;
    m_pxLevels.delVal(pxLevel->price());
  } else {
    pxLevelFn = &handler->updatedPxLevel;
  }
  order->pxLevel(0);
  m_data.nv -= orderData.price * orderData.qty;
  m_data.qty -= orderData.qty;
}

void MxMDOrderBook::delOrder_(
  MxMDOrder *order, MxDateTime transactTime,
  const MxMDSecHandler *handler,
  const MxMDPxLevelFn *&pxLevelFn, ZuRef<MxMDPxLevel> &pxLevel)
{
  const MxMDOrderData &orderData = order->data();
  if (orderData.qty.fgt(0.0)) {
    MxMDOBSide *obSide = orderData.side == MxSide::Buy ? m_bids : m_asks;
    obSide->delOrder_(order, transactTime, handler, pxLevelFn, pxLevel);
  }
}

ZmRef<MxMDOrder> MxMDOrderBook::addOrder(
  ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
  MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags)
{
  if (ZuUnlikely(!m_venueShard)) return (MxMDOrder *)0;

  ZmRef<MxMDOrder> order;
  const MxMDPxLevelFn *pxLevelFn = 0;
  ZuRef<MxMDPxLevel> pxLevel;

  if (ZmRef<MxMDOrder> order = m_venueShard->findOrder(
	key(), side, orderID, orderIDScope))
    return modifyOrder(
	orderID, orderIDScope, transactTime, side, rank, price, qty, flags);

  order = new MxMDOrder(this,
    orderID, orderIDScope, transactTime, side, rank, price, qty, flags);

  m_venueShard->addOrder(order);

  addOrder_(order, transactTime, m_handler, pxLevelFn, pxLevel);

  md()->addOrder(this, orderID, orderIDScope,
      transactTime, side, rank, price, qty, flags);

  if (MxMDOrderBook *consolidated = this->consolidated())
    consolidated->pxLevel_(
      pxLevel->side(), transactTime, true,
      pxLevel->price(), qty, 1, flags, true, 0, 0);

  if (pxLevelFn) (*pxLevelFn)(pxLevel, transactTime);

  if (m_handler) m_handler->addOrder(order, transactTime);

  return order;
}

ZmRef<MxMDOrder> MxMDOrderBook::modifyOrder(
  ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
  MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags)
{
  if (ZuUnlikely(!m_venueShard)) return (MxMDOrder *)0;

  ZmRef<MxMDOrder> order;
  if (ZuUnlikely(qty.feq(0)))
    order = m_venueShard->delOrder(key(), side, orderID, orderIDScope);
  else
    order = m_venueShard->findOrder(key(), side, orderID, orderIDScope);
  if (ZuUnlikely(!order)) {
    md()->raise(ZeEVENT(Error, MxMDOrderNotFound("modifyOrder", orderID)));
    return 0;
  }
  modifyOrder_(order, transactTime, side, rank, price, qty, flags);
  return order;
}

void MxMDVenue::modifyOrder(
  ZuString orderID, MxDateTime transactTime,
  MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags,
  ZmFn<MxMDOrder *> fn)
{
  ZmRef<MxMDOrder> order;
  if (ZuUnlikely(qty.feq(0)))
    order = delOrder(orderID);
  else
    order = findOrder(orderID);
  if (ZuUnlikely(!order)) {
    md()->raise(ZeEVENT(Error, MxMDOrderNotFound("modifyOrder", orderID)));
    return;
  }
  order->orderBook()->run(
      [=, ob = order->orderBook(), order = ZuMv(order)]() {
	ob->modifyOrder_(order, transactTime, side, rank, price, qty, flags);
	fn(order); });
}

void MxMDOrderBook::modifyOrder_(MxMDOrder *order, MxDateTime transactTime,
  MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags)
{
  const MxMDPxLevelFn *pxLevelFn[2] = { 0 };
  ZuRef<MxMDPxLevel> pxLevel[2];
  delOrder_(order, transactTime, m_handler, pxLevelFn[0], pxLevel[0]);

  MxFloat oldQty = order->data().qty;
  order->update_(rank, price, qty, flags);

  if (ZuLikely(qty.fne(0)))
    addOrder_(order, transactTime, m_handler, pxLevelFn[1], pxLevel[1]);
  else
    m_venueShard->delOrder(key(), side, order->id(), order->idScope());

  md()->modifyOrder(
      this, order->id(), order->idScope(), transactTime, side,
      rank, price, qty, flags);

  if (MxMDOrderBook *consolidated = this->consolidated()) {
    consolidated->pxLevel_(
      pxLevel[0]->side(), transactTime, true,
      pxLevel[0]->price(), -oldQty, -1, 0, true, 0, 0);
    if (ZuLikely(qty.fne(0)))
      consolidated->pxLevel_(
	pxLevel[1]->side(), transactTime, true,
	pxLevel[1]->price(), qty, 1, 0, true, 0, 0);
  }

  if (pxLevelFn[0]) (*(pxLevelFn[0]))(pxLevel[0], transactTime);
  if (ZuLikely(qty.fne(0)))
    if (pxLevelFn[1]) (*(pxLevelFn[1]))(pxLevel[1], transactTime);

  if (m_handler) m_handler->modifiedOrder(order, transactTime);
}

ZmRef<MxMDOrder> MxMDOrderBook::reduceOrder(
  ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
  MxEnum side, MxFloat reduceQty)
{
  if (ZuUnlikely(!m_venueShard)) return (MxMDOrder *)0;

  ZmRef<MxMDOrder> order =
    m_venueShard->findOrder(key(), side, orderID, orderIDScope);
  if (ZuUnlikely(!order)) {
    md()->raise(ZeEVENT(Error, MxMDOrderNotFound("reduceOrder", orderID)));
    return 0;
  }
  if (ZuUnlikely(order->data().qty <= ~reduceQty))
    m_venueShard->delOrder(key(), side, orderID, orderIDScope);
  reduceOrder_(order, transactTime, reduceQty);
  return order;
}

void MxMDVenue::reduceOrder(
  ZuString orderID, MxDateTime transactTime,
  MxFloat reduceQty, ZmFn<MxMDOrder *> fn)
{
  ZmRef<MxMDOrder> order = findOrder(orderID);
  if (ZuUnlikely(!order)) {
    md()->raise(ZeEVENT(Error, MxMDOrderNotFound("reduceOrder", orderID)));
    return;
  }
  if (ZuUnlikely(order->data().qty <= ~reduceQty)) delOrder(orderID);
  order->orderBook()->run(
      [=, ob = order->orderBook(), order = ZuMv(order)]() {
	ob->reduceOrder_(order, transactTime, reduceQty);
	fn(order); });
}

void MxMDOrderBook::reduceOrder_(MxMDOrder *order,
  MxDateTime transactTime, MxFloat reduceQty)
{
  const MxMDPxLevelFn *pxLevelFn[2] = { 0 };
  ZuRef<MxMDPxLevel> pxLevel[2];
  delOrder_(order, transactTime, m_handler, pxLevelFn[0], pxLevel[0]);

  MxFloat oldQty = order->data().qty;
  MxFloat qty = oldQty - reduceQty;
  if (qty.feq(0) || (ZuUnlikely(qty < 0 || !*qty))) qty = 0;
  order->updateQty_(qty);

  if (ZuLikely(qty.fne(0)))
    addOrder_(order, transactTime, m_handler, pxLevelFn[1], pxLevel[1]);
  else
    m_venueShard->delOrder(
	key(), order->data().side, order->id(), order->idScope());

  md()->modifyOrder(
      this, order->id(), order->idScope(), transactTime, order->data().side,
      MxUInt(), MxFloat(), qty, MxFlags());

  if (MxMDOrderBook *consolidated = this->consolidated()) {
    consolidated->pxLevel_(
      pxLevel[0]->side(), transactTime, true,
      pxLevel[0]->price(), -oldQty, -1, 0, true, 0, 0);
    if (ZuLikely(qty.fne(0)))
      consolidated->pxLevel_(
	pxLevel[1]->side(), transactTime, true,
	pxLevel[1]->price(), qty, 1, 0, true, 0, 0);
  }

  if (pxLevelFn[0]) (*(pxLevelFn[0]))(pxLevel[0], transactTime);
  if (ZuLikely(qty.fne(0)))
    if (pxLevelFn[1]) (*(pxLevelFn[1]))(pxLevel[1], transactTime);

  if (m_handler) m_handler->modifiedOrder(order, transactTime);
}

ZmRef<MxMDOrder> MxMDOrderBook::cancelOrder(
  ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
  MxEnum side)
{
  if (ZuUnlikely(!m_venueShard)) return (MxMDOrder *)0;
  ZmRef<MxMDOrder> order =
    m_venueShard->delOrder(key(), side, orderID, orderIDScope);
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
  order->orderBook()->run(
      [=, ob = order->orderBook(), order = ZuMv(order)]() {
	ob->cancelOrder_(order, transactTime);
	fn(order); });
}

void MxMDOrderBook::cancelOrder_(MxMDOrder *order, MxDateTime transactTime)
{
  const MxMDPxLevelFn *pxLevelFn = 0;
  ZuRef<MxMDPxLevel> pxLevel;
  MxFloat qty = order->data().qty;

  delOrder_(order, transactTime, m_handler, pxLevelFn, pxLevel);

  md()->cancelOrder(this,
      order->id(), order->idScope(), transactTime, order->data().side);

  if (MxMDOrderBook *consolidated = this->consolidated())
    consolidated->pxLevel_(
      pxLevel->side(), transactTime, true,
      pxLevel->price(), -qty, -1, 0, true, 0, 0);

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
    PxLevels::ReadIterator i(m_pxLevels);
    while (ZuRef<MxMDPxLevel> pxLevel = i.iterateKey()) {
      MxFloat d_qty = -pxLevel->data().qty;
      MxFloat d_nOrders = -pxLevel->data().nOrders;
      pxLevel->reset(transactTime, canceledOrder);
      if (handler) handler->deletedPxLevel(pxLevel, transactTime);
      if (MxMDOrderBook *consolidated = m_orderBook->consolidated())
	consolidated->pxLevel_(
	  m_side, transactTime, true,
	  pxLevel->price(), d_qty, d_nOrders, 0, true, 0, 0);
    }
  }
  m_pxLevels.clean();
  m_data.nv = m_data.qty = 0;
}

void MxMDOrderBook::reset(MxDateTime transactTime)
{
  MxMDL1Data delta;
  bool l1Updated = false;

  if (*m_l1Data.bid) {
    delta.bid = delta.bidQty = -MxFloat::inf();
    m_l1Data.bid = m_l1Data.bidQty = MxFloat();
    l1Updated = true;
  }
  if (*m_l1Data.ask) {
    delta.ask = delta.askQty = -MxFloat::inf();
    m_l1Data.ask = m_l1Data.askQty = MxFloat();
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
  ZuString tradeID, MxDateTime transactTime, MxFloat price, MxFloat qty)
{
  md()->addTrade(this, tradeID, transactTime, price, qty);
  if (m_handler && m_handler->addTrade) {
    ZmRef<MxMDTrade> trade = new MxMDTrade(
	security(0), this, tradeID, transactTime, price, qty);
    m_handler->addTrade(trade, transactTime);
  }
}

void MxMDOrderBook::correctTrade(
  ZuString tradeID, MxDateTime transactTime, MxFloat price, MxFloat qty)
{
  md()->correctTrade(this, tradeID, transactTime, price, qty);
  if (m_handler && m_handler->correctedTrade) {
    ZmRef<MxMDTrade> trade = new MxMDTrade(
	security(0), this, tradeID, transactTime, price, qty);
    m_handler->correctedTrade(trade, transactTime);
  }
}

void MxMDOrderBook::cancelTrade(
  ZuString tradeID, MxDateTime transactTime, MxFloat price, MxFloat qty)
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
    const MxMDLotSizes &lotSizes)
{
  if (tickSizeTbl)
    m_tickSizeTbl = const_cast<MxMDTickSizeTbl *>(tickSizeTbl);
  if (*lotSizes.oddLotSize)
    m_lotSizes.oddLotSize = lotSizes.oddLotSize;
  if (*lotSizes.lotSize)
    m_lotSizes.lotSize = lotSizes.lotSize;
  if (*lotSizes.blockLotSize)
    m_lotSizes.blockLotSize = lotSizes.blockLotSize;

  md()->updateOrderBook(this, tickSizeTbl, lotSizes);

  if (m_handler) m_handler->updatedOrderBook(this);
}

MxMDSecurity::MxMDSecurity(MxMDShard *shard,
  MxID primaryVenue, MxID primarySegment, ZuString id,
  const MxMDSecRefData &refData) :
    MxMDSharded(shard),
    m_key(primaryVenue, primarySegment, id),
    m_refData(refData)
{
}

void MxMDSecurity::subscribe(MxMDSecHandler *handler)
{
  m_handler = handler;
  OrderBooks::Iterator i(m_orderBooks);
  while (ZmRef<MxMDOrderBook> ob = i.iterateKey())
    if (*(ob->venueID())) ob->subscribe(handler);
}

void MxMDSecurity::unsubscribe()
{
  OrderBooks::Iterator i(m_orderBooks);
  while (ZmRef<MxMDOrderBook> ob = i.iterateKey())
    if (*(ob->venueID())) ob->unsubscribe();
  m_handler = 0;
}

ZmRef<MxMDOrderBook> MxMDSecurity::findOrderBook_(MxID venue, MxID segment)
{
  return m_orderBooks.findKey(MxMDOrderBook::venueSegment(venue, segment));
}

ZmRef<MxMDOrderBook> MxMDSecurity::addOrderBook(
  MxID venue, MxID segment, ZuString id,
  MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes)
{
  return md()->addOrderBook(
      this, venue, segment, id, tickSizeTbl, lotSizes);
}

void MxMDSecurity::addOrderBook_(MxMDOrderBook *ob)
{
  switch (m_orderBooks.count()) {
    case 0:
      break;
    case 1:
      {
	ZmRef<MxMDOrderBook> ob2 = m_orderBooks.minimumKey();
	ZmRef<MxMDOrderBook> consolidated = new MxMDOrderBook(
	  shard(), 0, 0,
	  MxID(), m_refData.symbol, this, 0, MxMDLotSizes(), 0);
	ob->consolidated(consolidated);
	ob2->consolidated(consolidated);
	m_orderBooks.add(consolidated);
      }
      break;
    default:
      ob->consolidated(m_orderBooks.findKey(
	    MxMDOrderBook::venueSegment(MxID(), MxID())));
      break;
  }
  m_orderBooks.add(ob);
}

void MxMDSecurity::delOrderBook(MxID venue, MxID segment)
{
  md()->delOrderBook(this, venue, segment);
}

ZmRef<MxMDOrderBook> MxMDSecurity::delOrderBook_(MxID venue, MxID segment)
{
  ZmRef<MxMDOrderBook> ob;
  switch (m_orderBooks.count()) {
    case 0:
      break;
    case 3:
      ob = m_orderBooks.delKey(MxMDOrderBook::venueSegment(venue, segment));
      if (ZuUnlikely(!ob)) break;
      m_orderBooks.delKey(MxMDOrderBook::venueSegment(MxID(), MxID()));
      m_orderBooks.minimumKey()->consolidated(0);
      ob->consolidated(0);
      break;
    default:
      ob = m_orderBooks.delKey(MxMDOrderBook::venueSegment(venue, segment));
      break;
  }
  return ob;
}

void MxMDSecurity::update(const MxMDSecRefData &refData)
{
  md()->updateSecurity(this, refData);
  if (m_handler) m_handler->updatedSecurity(this);
}

MxMDVenueShard::MxMDVenueShard(MxMDVenue *venue, MxMDShard *shard) :
    m_venue(venue), m_shard(shard),
    m_orders2(ZmHashParams().bits(4).loadFactor(1.0).cBits(4).
      init(ZuStringN<ZmHeapIDSize>() <<
	"MxMDVenueShard." << venue->id() << ".Orders2")),
    m_orders3(ZmHashParams().bits(4).loadFactor(1.0).cBits(4).
      init(ZuStringN<ZmHeapIDSize>() <<
	"MxMDVenueShard." << venue->id() << ".Orders3"))
{
}

MxMDVenue::MxMDVenue(MxMDLib *md, MxMDFeed *feed, MxID id) :
    m_md(md), m_feed(feed), m_id(id),
    m_shards(md->nShards()),
    m_segments(ZmHashParams().bits(2).init(
	  ZuStringN<ZmHeapIDSize>() << "MxMDVenue." << id << ".Segments")),
    m_orders(ZmHashParams().bits(4).loadFactor(1.0).cBits(4).
      init(ZuStringN<ZmHeapIDSize>() << "MxMDVenue." << id << ".Orders"))
{
  unsigned n = md->nShards();
  m_shards.length(n);
  for (unsigned i = 0; i < n; i++)
    m_shards[i] = new MxMDVenueShard(this, md->shard(i));
}

uintptr_t MxMDVenue::allTickSizeTbls(ZmFn<MxMDTickSizeTbl *> fn) const
{
  TickSizeTbls::ReadIterator i(m_tickSizeTbls);
  while (ZmRef<MxMDTickSizeTbl> tbl = i.iterateKey())
    if (uintptr_t v = fn(tbl)) return v;
  return 0;
}

ZmRef<MxMDTickSizeTbl> MxMDVenue::findTickSizeTbl_(ZuString id)
{
  return m_tickSizeTbls.findKey(id);
}

ZmRef<MxMDTickSizeTbl> MxMDVenue::addTickSizeTbl_(ZuString id)
{
  ZmRef<MxMDTickSizeTbl> tbl = new MxMDTickSizeTbl(this, id);
  m_tickSizeTbls.add(tbl);
  return tbl;
}

ZmRef<MxMDTickSizeTbl> MxMDVenue::addTickSizeTbl(ZuString id)
{
  return md()->addTickSizeTbl(this, id);
}

uintptr_t MxMDVenue::allSegments(ZmFn<MxID, MxEnum, MxDateTime> fn) const
{
  Segments::ReadIterator i(m_segments);
  uintptr_t v = 0;
  do {
    ZuPair<MxID, Session> kv = i.iterate();
    if (!kv.p1()) break;
    v = fn(kv.p1(), kv.p2().p1(), kv.p2().p2());
  } while (!v);
  return v;
}

void MxMDVenue::tradingSession(MxID segment, MxEnum session, MxDateTime stamp)
{
  md()->tradingSession(this, segment, session, stamp);
}

ZmRef<MxMDOrderBook> MxMDVenue::addCombination(
  MxID segment, ZuString id,
  MxUInt legs, const ZmRef<MxMDSecurity> *securities,
  const MxEnum *sides, const MxFloat *ratios,
  MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes)
{
  return md()->addCombination(
      this, segment, id,
      legs, securities, sides, ratios, tickSizeTbl, lotSizes);
}

void MxMDVenue::delCombination(MxID segment, ZuString id)
{
  md()->delCombination(this, segment, id);
}

MxMDFeed::MxMDFeed(MxMDLib *md, MxID id) :
  m_md(md),
  m_id(id)
{
}

void MxMDFeed::start() { }
void MxMDFeed::stop() { }

void MxMDFeed::final() { }

void MxMDFeed::addOrderBook(MxMDOrderBook *) { }
void MxMDFeed::delOrderBook(MxMDOrderBook *) { }

uintptr_t MxMDShard::allSecurities_(ZmFn<MxMDSecurity *> fn) const
{
  Securities::ReadIterator i(m_securities);
  while (MxMDSecurity *security = i.iterateKey())
    if (uintptr_t v = fn(security)) return v;
  return 0;
}

uintptr_t MxMDShard::allOrderBooks_(ZmFn<MxMDOrderBook *> fn) const
{
  OrderBooks::ReadIterator i(m_orderBooks);
  while (MxMDOrderBook *ob = i.iterateKey())
    if (uintptr_t v = fn(ob)) return v;
  return 0;
}

uintptr_t MxMDShard::allSecurities(ZmFn<MxMDSecurity *> fn) const
{
  uintptr_t v;
  thread_local ZmSemaphore sem;
  this->run([this, &v, sem = &sem, fn = ZuMv(fn)]() {
      v = allSecurities_(ZuMv(fn)); sem->post(); });
  sem.wait();
  return v;
}

uintptr_t MxMDShard::allOrderBooks(ZmFn<MxMDOrderBook *> fn) const
{
  uintptr_t v;
  thread_local ZmSemaphore sem;
  this->run([this, &v, sem = &sem, fn = ZuMv(fn)]() {
      v = allOrderBooks_(ZuMv(fn)); sem->post(); });
  sem.wait();
  return v;
}

uintptr_t MxMDLib::allSecurities(ZmFn<MxMDSecurity *> fn) const
{
  for (unsigned i = 0, n = m_shards.length(); i < n; i++)
    if (uintptr_t v = m_shards[i]->allSecurities(fn))
      return v;
  return 0;
}

uintptr_t MxMDLib::allOrderBooks(ZmFn<MxMDOrderBook *> fn) const
{
  for (unsigned i = 0, n = m_shards.length(); i < n; i++)
    if (uintptr_t v = m_shards[i]->allOrderBooks(fn))
      return v;
  return 0;
}

uintptr_t MxMDLib::allFeeds(ZmFn<MxMDFeed *> fn) const
{
  Feeds::ReadIterator i(m_feeds);
  while (const Feeds::Node *node = i.iterate())
    if (uintptr_t v = fn(node->key().ptr())) return v;
  return 0;
}

uintptr_t MxMDLib::allVenues(ZmFn<MxMDVenue *> fn) const
{
  Venues::ReadIterator i(m_venues);
  while (const Venues::Node *node = i.iterate())
    if (uintptr_t v = fn(node->key().ptr())) return v;
  return 0;
}

MxMDLib::MxMDLib(ZmScheduler *scheduler) :
  m_scheduler(scheduler),
  m_allSecurities(ZmHashParams().bits(12).loadFactor(1.0).cBits(4).
    init("MxMDLib.AllSecurities")),
  m_allOrderBooks(ZmHashParams().bits(12).loadFactor(1.0).cBits(4).
    init("MxMDLib.AllOrderBooks")),
  m_securities(ZmHashParams().bits(12).loadFactor(1.0).cBits(4).
    init("MxMDLib.Securities")),
  m_feeds(ZmHashParams().bits(4).loadFactor(1.0).cBits(4).
    init("MxMDLib.Feeds")),
  m_venues(ZmHashParams().bits(4).loadFactor(1.0).cBits(4).
    init("MxMDLib.Venues")),
  m_handler(new MxMDLibHandler())
{
}

static void exception(MxMDLib *, ZeEvent *e) { ZeLog::log(e); }

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

void MxMDLib::raise(ZeEvent *e)
{
  handler()->exception(this, e);
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
  venue->loaded_(1);
  handler()->refDataLoaded(venue);
  {
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::refDataLoaded(core->broadcast(), venue->id());
  }
}

ZmRef<MxMDTickSizeTbl> MxMDLib::addTickSizeTbl(
    MxMDVenue *venue, ZuString id)
{
  ZmRef<MxMDTickSizeTbl> tbl;
  {
    Guard guard(m_refDataLock);
    if (tbl = venue->findTickSizeTbl_(id)) return tbl;
    tbl = venue->addTickSizeTbl_(id);
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
    MxFloat minPrice, MxFloat maxPrice, MxFloat tickSize)
{
  {
    TickSizes::Iterator i(
	m_tickSizes, minPrice, MxMDTickSizeTbl::TickSizes::GreaterEqual);
    while (TickSizes::Node *node = i.iterate())
      if (node->key().minPrice() <= ~maxPrice)
	i.del();
      else
	break;
  }
  m_tickSizes.add(MxMDTickSize{minPrice, maxPrice, tickSize});
}

void MxMDLib::addTickSize(MxMDTickSizeTbl *tbl,
    MxFloat minPrice, MxFloat maxPrice, MxFloat tickSize)
{
  {
    Guard guard(m_refDataLock);
    tbl->addTickSize_(minPrice, maxPrice, tickSize);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::addTickSize(core->broadcast(),
	    tbl->venue()->id(), tbl->id(), minPrice, maxPrice, tickSize);
  }
  handler()->addTickSize(tbl, MxMDTickSize{minPrice, maxPrice, tickSize});
}

ZmRef<MxMDSecurity> MxMDLib::addSecurity(
  MxID venue, MxID segment, ZuString id,
  unsigned shardID, const MxMDSecRefData &refData)
{
  ZmRef<MxMDSecurity> security;
  shardID %= m_shards.length();
  MxMDShard *shard = m_shards[shardID].ptr();
  {
    Guard guard(m_refDataLock);
    if (security = m_allSecurities.findKey(MxSecKey(venue, segment, id))) {
      guard.unlock();
      security->update(refData);
      return security;
    }
    security = new MxMDSecurity(shard, venue, segment, id, refData);
    m_allSecurities.add(security);
    addSecIndices(security, refData);
    shard->addSecurity(security);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::addSecurity(core->broadcast(),
	    security->key(), shardID, refData);
  }
  handler()->addSecurity(security);
  return security;
}

void MxMDSecurity::update_(const MxMDSecRefData &refData)
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
  m_refData.putCall.update(refData.putCall);
  m_refData.strike.update(refData.strike);
  m_refData.strikeMultiplier.update(
      refData.strikeMultiplier, -MxFloat::inf());
  m_refData.outstandingShares.update(
      refData.outstandingShares, -MxFloat::inf());
  m_refData.adv.update(refData.adv, -MxFloat::inf());
}

void MxMDLib::updateSecurity(
  MxMDSecurity *security, const MxMDSecRefData &refData)
{
  {
    Guard guard(m_refDataLock);

    MxMDSecRefData oldRefData;

    oldRefData = security->refData();
    security->update_(refData);

    delSecIndices(security, oldRefData);
    addSecIndices(security, security->refData());

    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::updateSecurity(
	  core->broadcast(), security->key(), refData);
  }
  handler()->updatedSecurity(security);
}

void MxMDLib::addSecIndices(
    MxMDSecurity *security, const MxMDSecRefData &refData)
{
  if (*refData.idSrc && refData.symbol)
    m_securities.add(MxSecSymKey(refData.idSrc, refData.symbol), security);
  if (*refData.altIDSrc && refData.altSymbol)
    m_securities.add(
	MxSecSymKey(refData.altIDSrc, refData.altSymbol), security);
  if (*refData.underVenue && refData.underlying && *refData.mat) {
    MxMDSecurity *underlying = m_allSecurities.findKey(
	MxSecKey(refData.underVenue, refData.underSegment, refData.underlying));
    if (!underlying) {
      MxMDShard *shard = security->shard();
      MxMDSecRefData underRefData{0}; // default to non-tradeable
      ZmRef<MxMDSecurity> underlying_ = new MxMDSecurity(shard,
	  refData.underVenue, refData.underSegment, refData.underlying,
	  underRefData);
      m_allSecurities.add(underlying_);
      shard->addSecurity(underlying_);
      MxMDCore *core = static_cast<MxMDCore *>(this);
      if (ZuUnlikely(core->streaming()))
	MxMDStream::addSecurity(core->broadcast(),
	      underlying_->key(), shard->id(), underRefData);
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
    m_securities.delKey(MxSecSymKey(refData.idSrc, refData.symbol));
  if (*refData.altIDSrc && refData.altSymbol)
    m_securities.delKey(MxSecSymKey(refData.altIDSrc, refData.altSymbol));
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
    m_options.add(MxOptKey(refData.mat, refData.putCall, refData.strike),
	security);
  else
    m_futures.add(MxFutKey(refData.mat), security);
}

void MxMDDerivatives::del(MxMDSecurity *security)
{
  const MxMDSecRefData &refData = security->refData();
  if (*refData.putCall && *refData.strike)
    m_options.delVal(MxOptKey(refData.mat, refData.putCall, refData.strike));
  else
    m_futures.delVal(MxFutKey(refData.mat));
}

uintptr_t MxMDDerivatives::allFutures(ZmFn<MxMDSecurity *> fn) const
{
  Futures::ReadIterator i(m_futures);
  while (MxMDSecurity *future = i.iterateVal())
    if (uintptr_t v = fn(future)) return v;
  return 0;
}

uintptr_t MxMDDerivatives::allOptions(ZmFn<MxMDSecurity *> fn) const
{
  Options::ReadIterator i(m_options);
  while (MxMDSecurity *option = i.iterateVal())
    if (uintptr_t v = fn(option)) return v;
  return 0;
}

ZmRef<MxMDOrderBook> MxMDLib::addOrderBook(MxMDSecurity *security,
    MxID venueID, MxID segment, ZuString id,
    MxMDTickSizeTbl *tickSizeTbl,
    const MxMDLotSizes &lotSizes)
{
  if (ZuUnlikely(!*venueID)) {
    raise(ZeEVENT(Error,
      ([id = MxSymString(id)](const ZeEvent &, ZmStream &s) {
	s << "addOrderBook - null venueID for \"" << id << '"';
      })));
    return 0;
  }
  ZmRef<MxMDOrderBook> ob;
  ZmRef<MxMDFeed> feed;
  {
    Guard guard(m_refDataLock);
    if (ob = security->findOrderBook_(venueID, segment)) {
      guard.unlock();
      ob->update(tickSizeTbl, lotSizes);
      return ob;
    }
    ZmRef<MxMDVenue> venue = m_venues.findKey(venueID);
    if (ZuUnlikely(!venue)) {
      raise(ZeEVENT(Error,
	([venueID, id = MxSymString(id)](const ZeEvent &, ZmStream &s) {
	  s << "addOrderBook - no such venue for \"" << id << "\" " << venueID;
	})));
      return 0;
    }
    ob = new MxMDOrderBook(
      security->shard(), venue, 0,
      segment, id, security, tickSizeTbl, lotSizes,
      security->handler());
    security->addOrderBook_(ob);
    m_allOrderBooks.add(ob);
    ob->shard()->addOrderBook(ob);
    venue->feed()->addOrderBook(ob);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::addOrderBook(core->broadcast(),
	    ob->key(), security->key(), tickSizeTbl->id(), ob->lotSizes());
  }
  handler()->addOrderBook(ob);
  return ob;
}

ZmRef<MxMDOrderBook> MxMDLib::addCombination(
  MxMDVenue *venue, MxID segment, ZuString id,
  MxUInt legs, const ZmRef<MxMDSecurity> *securities,
  const MxEnum *sides, const MxFloat *ratios,
  MxMDTickSizeTbl *tickSizeTbl, const MxMDLotSizes &lotSizes)
{
  ZmRef<MxMDOrderBook> ob;
  {
    Guard guard(m_refDataLock);
    if (ob = m_allOrderBooks.findKey(MxSecKey(venue->id(), segment, id))) {
      guard.unlock();
      ob->update(tickSizeTbl, lotSizes);
      return ob;
    }
    ob = new MxMDOrderBook(
      securities[0]->shard(), venue,
      segment, id, legs, securities, sides, ratios,
      tickSizeTbl, lotSizes);
    m_allOrderBooks.add(ob);
    ob->shard()->addOrderBook(ob);
    venue->feed()->addOrderBook(ob);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming())) {
      MxSecKey securityKeys[MxMDNLegs];
      for (unsigned i = 0; i < legs; i++)
	securityKeys[i] = securities[i]->key();
      MxMDStream::addCombination(core->broadcast(),
	    ob->key(), legs, securityKeys, sides, ratios,
	    tickSizeTbl->id(), lotSizes);
    }
  }
  handler()->addOrderBook(ob);
  return ob;
}

void MxMDLib::updateOrderBook(
  MxMDOrderBook *ob,
  const MxMDTickSizeTbl *tickSizeTbl,
  const MxMDLotSizes &lotSizes)
{
  {
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::updateOrderBook(core->broadcast(),
	    ob->key(), tickSizeTbl->id(), lotSizes);
  }
  handler()->updatedOrderBook(ob);
}

void MxMDLib::delOrderBook(MxMDSecurity *security, MxID venue, MxID segment)
{
  ZmRef<MxMDOrderBook> ob;
  {
    Guard guard(m_refDataLock);
    ob = security->delOrderBook_(venue, segment);
    if (!ob) return;
    m_allOrderBooks.delKey(MxSecKey(venue, segment, ob->id()));
    ob->shard()->delOrderBook(ob);
    ob->venue()->feed()->delOrderBook(ob);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::delOrderBook(core->broadcast(), ob->key());
  }
  ob->unsubscribe();
  handler()->deletedOrderBook(ob);
}

void MxMDLib::delCombination(MxMDVenue *venue, MxID segment, ZuString id)
{
  ZmRef<MxMDOrderBook> ob;
  {
    Guard guard(m_refDataLock);
    ob = m_allOrderBooks.delKey(MxSecKey(venue->id(), segment, id));
    if (!ob) return;
    ob->shard()->delOrderBook(ob);
    ob->venue()->feed()->delOrderBook(ob);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::delCombination(core->broadcast(), ob->key());
  }
  ob->unsubscribe();
  handler()->deletedOrderBook(ob);
}

void MxMDVenue::tradingSession_(
    MxID segment, MxEnum session, MxDateTime stamp)
{
  m_segments.delKey(segment);
  m_segments.add(segment, ZuMkPair(session, stamp));
}

void MxMDLib::tradingSession(MxMDVenue *venue,
    MxID segment, MxEnum session, MxDateTime stamp)
{
  {
    Guard guard(m_refDataLock);
    venue->tradingSession_(segment, session, stamp);
    MxMDCore *core = static_cast<MxMDCore *>(this);
    if (ZuUnlikely(core->streaming()))
      MxMDStream::tradingSession(core->broadcast(),
	    venue->id(), segment, session, stamp);
  }
  handler()->tradingSession(venue, segment, session, stamp);
}

void MxMDLib::l1(const MxMDOrderBook *ob, const MxMDL1Data &l1Data)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::l1(core->broadcast(), ob->key(), l1Data);
}

void MxMDLib::pxLevel(
  const MxMDOrderBook *ob, MxEnum side, MxDateTime transactTime,
  bool delta, MxFloat price, MxFloat qty, MxFloat nOrders, MxFlags flags)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::pxLevel(core->broadcast(),
	  ob->key(), side, transactTime, delta,
	  price, qty, nOrders, flags);
}

void MxMDLib::l2(const MxMDOrderBook *ob, MxDateTime stamp, bool updateL1)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::l2(core->broadcast(), ob->key(), stamp, updateL1);
}

void MxMDLib::addOrder(const MxMDOrderBook *ob,
    ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
    MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::addOrder(core->broadcast(),
	ob->key(), orderID, orderIDScope,
	transactTime, side, rank, price, qty, flags);
}
void MxMDLib::modifyOrder(const MxMDOrderBook *ob,
    ZuString orderID, MxEnum orderIDScope, MxDateTime transactTime,
    MxEnum side, MxUInt rank, MxFloat price, MxFloat qty, MxFlags flags)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::modifyOrder(core->broadcast(),
	ob->key(), orderID, orderIDScope,
	transactTime, side, rank, price, qty, flags);
}
void MxMDLib::cancelOrder(const MxMDOrderBook *ob,
    ZuString orderID, MxEnum orderIDScope,
    MxDateTime transactTime, MxEnum side)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::cancelOrder(core->broadcast(),
	ob->key(), orderID, orderIDScope, transactTime, side);
}

void MxMDLib::resetOB(const MxMDOrderBook *ob,
    MxDateTime transactTime)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::resetOB(core->broadcast(),
	  ob->key(), transactTime);
}

void MxMDLib::addTrade(const MxMDOrderBook *ob,
    ZuString tradeID, MxDateTime transactTime, MxFloat price, MxFloat qty)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::addTrade(core->broadcast(),
	ob->key(), tradeID, transactTime, price, qty);
}
void MxMDLib::correctTrade(const MxMDOrderBook *ob,
    ZuString tradeID, MxDateTime transactTime, MxFloat price, MxFloat qty)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::correctTrade(core->broadcast(),
	ob->key(), tradeID, transactTime, price, qty);
}
void MxMDLib::cancelTrade(const MxMDOrderBook *ob,
    ZuString tradeID, MxDateTime transactTime, MxFloat price, MxFloat qty)
{
  MxMDCore *core = static_cast<MxMDCore *>(this);
  if (ZuUnlikely(core->streaming()))
    MxMDStream::cancelTrade(core->broadcast(),
	ob->key(), tradeID, transactTime, price, qty);
}
