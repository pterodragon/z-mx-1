#include <stdio.h>
#include <iostream>

// #define MxT_NLegs 4

#include <MxTOrder.hpp>
#include <MxTOrderMgr.hpp>

struct AppTypes : public MxTAppTypes<AppTypes> {
  typedef MxTAppTypes<AppTypes> Base;

  struct AppCxlLeg {
    template <typename S> inline void print(S &) const { }
    template <typename Update> inline void update(const Update &) { }
  };
  struct AppModLeg : public AppCxlLeg { };
  struct AppOrderLeg : public AppModLeg {
    MxIDString	instrument;	// instrument symbol

    template <typename S> inline void print(S &s) const {
      AppModLeg::print(s);
      s << "instrument=" << instrument;
    }
  };

  struct AppMsgID {
    MxIDString	clOrdID;
    MxIDString	orderID;
    MxIDString	execID;

    template <typename S> inline void print(S &s) const {
      s << "clOrdID=" << clOrdID
	<< " orderID=" << orderID
	<< " execID=" << execID;
    }
  };

  struct AppRequest;
  struct AppExec;

  struct AppRequest : public AppMsgID {
    template <typename Update>
    inline typename ZuIs<AppRequest, Update>::T update(const Update &u) {
      clOrdID.update(clOrdID);
      orderID.update(orderID);
      execID.update(execID);
    }
    template <typename Update>
    inline typename ZuIs<AppExec, Update>::T update(const Update &u) {
      orderID.update(orderID);
      execID.update(execID);
    }
    template <typename Update>
    inline typename ZuIfT<
      !ZuConversion<AppRequest, Update>::Is &&
      !ZuConversion<AppExec, Update>::Is>::T update(const Update &) { }
  };

  struct AppExec : public AppMsgID {
    template <typename Update>
    inline typename ZuIs<AppRequest, Update>::T update(const Update &u) {
      clOrdID.update(clOrdID);
    }
    template <typename Update>
    inline typename ZuIs<AppExec, Update>::T update(const Update &u) {
      orderID.update(orderID);
      execID.update(execID);
    }
    template <typename Update>
    inline typename ZuIfT<
      !ZuConversion<AppRequest, Update>::Is &&
      !ZuConversion<AppExec, Update>::Is>::T update(const Update &) { }
  };

  struct AppNewOrder : public AppRequest {
    MxIDString	account;	// account identifier

    template <typename S> inline void print(S &s) const {
      AppRequest::print(s);
      s << " account=" << account;
    }
  };

#ifdef DeclType
#undef DeclType
#endif
#define DeclType(Type, AppType) \
  struct Type : public Base::Type, public App ## AppType { \
    template <typename Update> inline void update(const Update &u) { \
      Base::Type::update(u); \
      App ## AppType::update(u); \
    } \
    template <typename S> inline void print(S &s) const { \
      Base::Type::print(s); s << ' '; App ## AppType::print(s); \
    } \
  }

  DeclType(CancelLeg, CxlLeg);
  DeclType(CanceledLeg, CxlLeg);

  DeclType(ModifyLeg, ModLeg);
  DeclType(ModifiedLeg, ModLeg);

  DeclType(OrderLeg, OrderLeg);

  DeclType(NewOrder, NewOrder);
  DeclType(Ordered, Exec);
  DeclType(Reject, Exec);

  DeclType(Modify, Request);
  DeclType(Modified, Exec);
  DeclType(ModReject, Exec);

  DeclType(Cancel, Request);
  DeclType(Canceled, Exec);
  DeclType(CxlReject, Exec);

  DeclType(Hold, Exec);
  DeclType(Release, Exec);
  DeclType(Deny, Exec);

  DeclType(Fill, Exec);
  DeclType(Closed, Exec);

#undef DeclType
};

struct App : public MxTOrderMgr<App, AppTypes> {
  // log abnormal OSM transition
  template <typename Txn> void abnormal(Order *, const Txn &) { }

  // returns true if async. modify enabled for order
  bool asyncMod(Order *);
  // returns true if async. cancel enabled for order
  bool asyncCxl(Order *);

  // process messages
  void sendNewOrder(Order *) { }
  void sendModify(Order *) { }
  void sendCancel(Order *) { }

  template <typename L> void sendOrdered(Order *, L) { }
  template <typename L> void sendReject(Order *, L) { }

  template <typename L> void sendModified(Order *, L) { }
  template <typename L> void sendModReject(Order *, L) { }

  template <typename L> void sendCanceled(Order *, L) { }
  template <typename L> void sendCxlReject(Order *, L) { }

  template <typename L> void sendSuspend(Order *, L) { }
  template <typename L> void sendRelease(Order *, L) { }

  template <typename L> void sendFill(Order *, L) { }

  template <typename L> void sendClosed(Order *, L) { }

  void main();
};

int main()
{
  App app;
  app.main();
}

void App::main()
{
  Order *order = new Order(); // would be derived and indexed by real app
  Txn<NewOrder> txn;

  // initialize new order
  NewOrder &newOrder = txn.initNewOrder(0);
  // fill out new order data
  newOrder.legs[0].side = MxSide::Buy;
  newOrder.legs[0].pxNDP = 0;
  newOrder.legs[0].qtyNDP = 0;
  // order state management
  newOrderIn(order, txn);
  this->newOrder(order, txn);
  // log newOrder
  // dump order
  std::cout << *order << '\n';

  {
    Txn<OrderSent> sentTxn;
    OrderSent &orderSent = sentTxn.initOrderSent(0);
    if (orderSend(order, sentTxn))
      std::cout << "send OK\n";
    else
      std::cout << "send FAILED\n";
    // log orderSent
  }
  // dump order
  std::cout << *order << '\n';

  Txn<Ordered> ack;
  Ordered &ordered = ack.initOrdered(0);
  this->ordered(order, ack);
  // log ordered
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
    "sizeof(ModFiltered): " << ZuBoxed(sizeof(ModFiltered)) << '\n' <<
    "sizeof(Cancel): " << ZuBoxed(sizeof(Cancel)) << '\n' <<
    "sizeof(CancelLeg): " << ZuBoxed(sizeof(CancelLeg)) << '\n' <<
    "sizeof(CxlFiltered): " << ZuBoxed(sizeof(CxlFiltered)) << '\n' <<
    "sizeof(OrderTxn): " << ZuBoxed(sizeof(OrderTxn)) << '\n' <<
    "sizeof(ModifyTxn): " << ZuBoxed(sizeof(ModifyTxn)) << '\n' <<
    "sizeof(CancelTxn): " << ZuBoxed(sizeof(CancelTxn)) << '\n' <<
    "sizeof(AckTxn): " << ZuBoxed(sizeof(AckTxn)) << '\n' <<
    "sizeof(AnyTxn): " << ZuBoxed(sizeof(AnyTxn)) << '\n' <<
    "sizeof(Order): " << ZuBoxed(sizeof(Order)) << '\n';
}
