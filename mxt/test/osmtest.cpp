#include <stdio.h>
#include <iostream>

// #define MxT_NLegs 4

#include <MxTMsg.hpp>
#include <MxTOrderMgr.hpp>

struct AppData : public MxTMsgData<AppData> {
  typedef MxTMsgData<AppData> Base;

  struct Request {
    MxIDString	clOrdID;
    MxIDString	orderID;

    template <typename S> inline void print(S &s) const {
      s << "clOrdID=" << clOrdID << " orderID=" << orderID;
    }
  };

  // extend data definitions
  struct NewOrder : public Base::NewOrder, public Request {
    // update
    using Base::NewOrder::update;
    template <typename Update>
    inline typename ZuIs<NewOrder, Update>::T update(const Update &u) {
      Base::NewOrder::update(u);
      clOrdID.update(u.clOrdID);
    }

    // print
    template <typename S> inline void print(S &s) const {
      static_cast<const Base::NewOrder &>(*this).print(s);
      s << ' ';
      static_cast<const Request &>(*this).print(s);
    }
  };
};

struct App : public MxTOrderMgr<App, AppData> {
  // log abnormal OSM transition
  template <typename Msg> void abnormal(Order *, const Msg &) { }

  // returns true if async. modify enabled for order
  bool asyncMod(Order *);
  // returns true if async. cancel enabled for order
  bool asyncCxl(Order *);

  template <typename Msg> void sendNewOrder(Msg &) { }
  template <typename Msg> void sendOrdered(Msg &) { }
  template <typename Msg> void sendReject(Msg &) { }

  template <typename Msg> void sendModify(Msg &) { }
  template <typename Msg> void sendModified(Msg &) { }
  template <typename Msg> void sendModReject(Msg &) { }

  template <typename Msg> void sendCancel(Msg &) { }
  template <typename Msg> void sendCanceled(Msg &) { }
  template <typename Msg> void sendCxlReject(Msg &) { }

  template <typename Msg> void sendSuspend(Msg &) { }
  template <typename Msg> void sendRelease(Msg &) { }

  template <typename Msg> void sendFill(Msg &) { }
  template <typename Msg> void sendCorrect(Msg &) { }
  template <typename Msg> void sendBust(Msg &) { }

  template <typename Msg> void sendClosed(Msg &) { }
  template <typename Msg> void sendMktNotice(Msg &) { }

  void main();
};

int main()
{
  App app;
  app.main();
}

void App::main()
{
  Order *order = new Order();
  Msg<NewOrder> msg;

  // initialize new order
  NewOrder &newOrder = msg.initNewOrder(0);
  // fill out new order data
  newOrder.legs[0].side = MxSide::Buy;
  newOrder.legs[0].pxNDP = 0;
  newOrder.legs[0].qtyNDP = 0;
  // order state management
  newOrderIn(order, msg);
  this->newOrder(order, msg);
  // dump order
  std::cout << *order << '\n';

  sendNewOrder(order);
  Msg<Ordered> ack;
  Ordered &ordered = ack.initOrdered(0);
  this->ordered(order, ack);
  // dump order
  std::cout << *order << '\n';

  delete order;
  std::cout <<
    "sizeof(Reject): " << ZuBoxed(sizeof(Reject)) << '\n' <<
    "sizeof(Hold): " << ZuBoxed(sizeof(Hold)) << '\n' <<
    "sizeof(Release): " << ZuBoxed(sizeof(Release)) << '\n' <<
    "sizeof(NewOrder): " << ZuBoxed(sizeof(NewOrder)) << '\n' <<
    "sizeof(OrderLeg): " << ZuBoxed(sizeof(OrderLeg)) << '\n' <<
    "sizeof(OrderFiltered): " << ZuBoxed(sizeof(OrderFiltered)) << '\n' <<
    "sizeof(Modify): " << ZuBoxed(sizeof(Modify)) << '\n' <<
    "sizeof(ModifyLeg): " << ZuBoxed(sizeof(ModifyLeg)) << '\n' <<
    "sizeof(ModifyFiltered): " << ZuBoxed(sizeof(ModFiltered)) << '\n' <<
    "sizeof(Cancel): " << ZuBoxed(sizeof(Cancel)) << '\n' <<
    "sizeof(CancelLeg): " << ZuBoxed(sizeof(CancelLeg)) << '\n' <<
    "sizeof(CxlFiltered): " << ZuBoxed(sizeof(CxlFiltered)) << '\n' <<
    "sizeof(OrderMsg): " << ZuBoxed(sizeof(OrderMsg)) << '\n' <<
    "sizeof(ModifyMsg): " << ZuBoxed(sizeof(ModifyMsg)) << '\n' <<
    "sizeof(CancelMsg): " << ZuBoxed(sizeof(CancelMsg)) << '\n' <<
    "sizeof(ExecMsg): " << ZuBoxed(sizeof(ExecMsg)) << '\n' <<
    "sizeof(AnyMsg): " << ZuBoxed(sizeof(AnyMsg)) << '\n' <<
    "sizeof(Order): " << ZuBoxed(sizeof(Order)) << '\n';
}
