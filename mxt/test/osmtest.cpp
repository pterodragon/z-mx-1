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
      // AppModLeg::print(s);
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

  // pre-process messages
  template <typename Txn> void sendNewOrder(Txn &) { }
  template <typename Txn> void sendOrdered(Txn &) { }
  template <typename Txn> void sendReject(Txn &) { }

  template <typename Txn> void sendModify(Txn &) { }
  template <typename Txn> void sendModified(Txn &) { }
  template <typename Txn> void sendModReject(Txn &) { }

  template <typename Txn> void sendCancel(Txn &) { }
  template <typename Txn> void sendCanceled(Txn &) { }
  template <typename Txn> void sendCxlReject(Txn &) { }

  template <typename Txn> void sendSuspend(Txn &) { }
  template <typename Txn> void sendRelease(Txn &) { }

  template <typename Txn> void sendFill(Txn &) { }
  template <typename Txn> void sendCorrect(Txn &) { }
  template <typename Txn> void sendBust(Txn &) { }

  template <typename Txn> void sendClosed(Txn &) { }
  template <typename Txn> void sendMktNotice(Txn &) { }

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
  // dump order
  std::cout << *order << '\n';

  sendNewOrder(order);
  Txn<Ordered> ack;
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
    "sizeof(OrderTxn): " << ZuBoxed(sizeof(OrderTxn)) << '\n' <<
    "sizeof(ModifyTxn): " << ZuBoxed(sizeof(ModifyTxn)) << '\n' <<
    "sizeof(CancelTxn): " << ZuBoxed(sizeof(CancelTxn)) << '\n' <<
    "sizeof(ExecTxn): " << ZuBoxed(sizeof(ExecTxn)) << '\n' <<
    "sizeof(AnyTxn): " << ZuBoxed(sizeof(AnyTxn)) << '\n' <<
    "sizeof(Order): " << ZuBoxed(sizeof(Order)) << '\n';
}
