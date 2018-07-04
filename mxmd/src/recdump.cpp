//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stdio.h>
#include <stddef.h>

#include <ZmTime.hpp>
#include <ZmTrap.hpp>

#include <ZeLog.hpp>

#include <ZiFile.hpp>

#include <MxBase.hpp>
#include <MxCSV.hpp>

#include <MxMDStream.hpp>
#include <MxMD.hpp>

#include <version.h>

typedef MxBoolCol BoolCol;
typedef MxIntCol IntCol;
typedef MxUIntCol UIntCol;
typedef MxFloatCol FloatCol;
template <typename Map> using EnumCol = MxEnumCol<Map>;
template <typename Map> using FlagsCol_ = MxFlagsCol<Map>;
typedef MxSymStringCol SymCol;
typedef MxIDStringCol IDStrCol;

class IDCol : public ZvCSVColumn<ZvCSVColType::Func, MxID>  {
  typedef ZvCSVColumn<ZvCSVColType::Func, MxID> Base;
  typedef Base::ParseFn ParseFn;
  typedef Base::PlaceFn PlaceFn;
public:
  template <typename ID>
  inline IDCol(const ID &id, int offset) :
      Base(id, offset,
	   ParseFn::Ptr<&IDCol::parse>::fn(),
	   PlaceFn::Ptr<&IDCol::place>::fn()) { }
  static void parse(MxID *i, ZuString b) { *i = b; }
  static void place(ZtArray<char> &b, const MxID *i) { b << *i; }
};

class HHMMSSCol : public ZvCSVColumn<ZvCSVColType::Func, MxDateTime>  {
  typedef ZvCSVColumn<ZvCSVColType::Func, MxDateTime> Base;
  typedef Base::ParseFn ParseFn;
  typedef Base::PlaceFn PlaceFn;
public:
  template <typename ID>
  inline HHMMSSCol(const ID &id, int offset, unsigned yyyymmdd, int tzOffset) :
    Base(id, offset,
	ParseFn::Member<&HHMMSSCol::parse>::fn(this),
	PlaceFn::Member<&HHMMSSCol::place>::fn(this)),
      m_yyyymmdd(yyyymmdd), m_tzOffset(tzOffset) { }
  virtual ~HHMMSSCol() { }

  void parse(MxDateTime *t, ZuString b) {
    new (t) MxDateTime(
	MxDateTime::YYYYMMDD, m_yyyymmdd, MxDateTime::HHMMSS, MxUInt(b));
    *t -= ZmTime(m_tzOffset);
  }
  void place(ZtArray<char> &b, const MxDateTime *t) {
    MxDateTime l = *t + m_tzOffset;
    b << MxUInt(l.hhmmss()).fmt(ZuFmt::Right<6>());
  }

  ZtString	m_tz;
  unsigned	m_yyyymmdd;
  int		m_tzOffset;
};
class NSecCol : public ZvCSVColumn<ZvCSVColType::Func, MxDateTime> {
  typedef ZvCSVColumn<ZvCSVColType::Func, MxDateTime> Base;
  typedef Base::ParseFn ParseFn;
  typedef Base::PlaceFn PlaceFn;
public:
  template <typename ID>
  inline NSecCol(const ID &id, int offset) :
    Base(id, offset,
	ParseFn::Ptr<&NSecCol::parse>::fn(),
	PlaceFn::Ptr<&NSecCol::place>::fn()) { }
  virtual ~NSecCol() { }
  static void parse(MxDateTime *t, ZuString b) {
    t->nsec() = MxUInt(b);
  }
  static void place(ZtArray<char> &b, const MxDateTime *t) {
    b << MxUInt(t->nsec()).fmt(ZuFmt::Right<9>());
  }
};

template <typename Flags>
class FlagsCol : public ZvCSVColumn<ZvCSVColType::Func, MxFlags> {
  typedef ZvCSVColumn<ZvCSVColType::Func, MxFlags> Base;
  typedef Base::ParseFn ParseFn;
  typedef Base::PlaceFn PlaceFn;
public:
  template <typename ID>
  inline FlagsCol(const ID &id, int offset, int venueOffset) :
    Base(id, offset,
	ParseFn::Member<&FlagsCol::parse>::fn(this),
	PlaceFn::Member<&FlagsCol::place>::fn(this)),
    m_venueOffset(venueOffset - offset) { }
  virtual ~FlagsCol() { }
  void parse(MxFlags *f, ZuString b) {
    if (b.length() < 5 || b[4] != ':') { *f = 0; return; }
    MxID venue = ZuString(&b[0], 4);
    *(MxID *)((char *)(void *)f + m_venueOffset) = venue;
    MxMDFlagsStr in = ZuString(&b[5], b.length() - 5);
    Flags::scan(in, venue, *f);
  }
  void place(ZtArray<char> &b, const MxFlags *f) {
    if (!*f) return;
    MxID venue =
      *(const MxID *)((const char *)(const void *)f + m_venueOffset);
    MxMDFlagsStr out;
    Flags::print(out, venue, *f);
    if (!out) return;
    { unsigned n = b.length() + 8; if (b.size() < n) b.size(n); }
    b << venue << ':' << out;
  }

  int	m_venueOffset;
};

template <class Writer>
class TickSizeCSV : public ZmObject, public ZvCSV {
public:
  struct Data {
    MxID		venue;
    MxIDString		id;
    MxEnum		event;
    MxFloat		minPrice;
    MxFloat		maxPrice;
    MxFloat		tickSize;
  };
  typedef ZuPOD<Data> POD;

  template <typename App>
  TickSizeCSV(App *app) {
    new ((m_pod = new POD())->ptr()) Data();
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new IDCol("venue", Offset(venue)));
    add(new IDStrCol("id", Offset(id)));
    add(new EnumCol<MxMDStream::Type::CSVMap>("event", Offset(event)));
    add(new FloatCol("minPrice", Offset(minPrice), app->ndp()));
    add(new FloatCol("maxPrice", Offset(maxPrice), app->ndp()));
    add(new FloatCol("tickSize", Offset(tickSize), app->ndp()));
#undef Offset
  }

  ZuAnyPOD *row(const MxMDStream::Msg *msg) {
    Data *data = m_pod->ptr();

    {
      using namespace MxMDStream;
      const Frame *frame = msg->data().frame();
      switch ((int)frame->type) {
	case Type::AddTickSizeTbl:
	case Type::ResetTickSizeTbl:
	  {
	    const AddTickSizeTbl &obj = frame->as<AddTickSizeTbl>();
	    new (data) Data{obj.venue, obj.id, frame->type};
	  }
	  break;
	case Type::AddTickSize:
	  {
	    const AddTickSize &obj = frame->as<AddTickSize>();
	    new (data) Data{obj.venue, obj.id, frame->type,
	      obj.minPrice, obj.maxPrice, obj.tickSize};
	  }
	  break;
	default:
	  return 0;
      }
    }

    return m_pod.ptr();
  }

private:
  ZuRef<POD>	m_pod;
};

template <class Writer>
class SecurityCSV : public ZmObject, public ZvCSV {
public:
  struct Data {
    MxID		venue;
    MxID		segment;
    MxIDString		id;
    MxEnum		event;
    MxUInt		shard;
    MxMDSecRefData	refData;
  };
  typedef ZuPOD<Data> POD;

  template <typename App>
  SecurityCSV(App *app) {
    new ((m_pod = new POD())->ptr()) Data();
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new IDCol("venue", Offset(venue)));
    add(new IDCol("segment", Offset(segment)));
    add(new IDStrCol("id", Offset(id)));
    add(new EnumCol<MxMDStream::Type::CSVMap>("event", Offset(event)));
    add(new UIntCol("shard", Offset(shard)));
#undef Offset
#define Offset(x) offsetof(Data, refData) + offsetof(MxMDSecRefData, x)
    add(new BoolCol("tradeable", Offset(tradeable)));
    add(new EnumCol<MxSecIDSrc::CSVMap>("idSrc", Offset(idSrc)));
    add(new SymCol("symbol", Offset(symbol)));
    add(new EnumCol<MxSecIDSrc::CSVMap>("altIDSrc", Offset(altIDSrc)));
    add(new SymCol("altSymbol", Offset(altSymbol)));
    add(new IDCol("underVenue", Offset(underVenue)));
    add(new IDCol("underSegment", Offset(underSegment)));
    add(new IDStrCol("underlying", Offset(underlying)));
    add(new UIntCol("mat", Offset(mat)));
    add(new EnumCol<MxPutCall::CSVMap>("putCall", Offset(putCall)));
    add(new IntCol("strike", Offset(strike)));
    add(new FloatCol("strikeMultiplier",
	Offset(strikeMultiplier), app->ndp()));
    add(new FloatCol("outstandingShares",
	Offset(outstandingShares), app->ndp()));
    add(new FloatCol("adv", Offset(adv), app->ndp()));
#undef Offset
  }

  ZuAnyPOD *row(const MxMDStream::Msg *msg) {
    Data *data = m_pod->ptr();

    {
      using namespace MxMDStream;
      const Frame *frame = msg->data().frame();
      switch ((int)frame->type) {
	case Type::AddSecurity:
	  {
	    const AddSecurity &obj = frame->as<AddSecurity>();
	    new (data) Data{
	      obj.key.venue(), obj.key.segment(), obj.key.id(),
	      frame->type, obj.shard, obj.refData};
	  }
	  break;
	case Type::UpdateSecurity:
	  {
	    const UpdateSecurity &obj = frame->as<UpdateSecurity>();
	    new (data) Data{
	      obj.key.venue(), obj.key.segment(), obj.key.id(),
	      frame->type, {}, obj.refData};
	  }
	  break;
	default:
	  return 0;
      }
    }

    return m_pod.ptr();
  }

private:
  ZuRef<POD>	m_pod;
};

template <class Writer>
class OrderBookCSV : public ZmObject, public ZvCSV {
public:
  struct Data {
    MxID		venue;
    MxID		segment;
    MxIDString		id;
    MxEnum		event;
    MxUInt		legs;
    MxID		secVenues[MxMDNLegs];
    MxID		secSegments[MxMDNLegs];
    MxIDString		securities[MxMDNLegs];
    MxEnum		sides[MxMDNLegs];
    MxFloat		ratios[MxMDNLegs];
    MxIDString		tickSizeTbl;
    MxMDLotSizes	lotSizes;
  };
  typedef ZuPOD<Data> POD;

  template <typename App>
  OrderBookCSV(App *app) {
    new ((m_pod = new POD())->ptr()) Data();
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new IDCol("venue", Offset(venue)));
    add(new IDCol("segment", Offset(segment)));
    add(new IDStrCol("id", Offset(id)));
    add(new EnumCol<MxMDStream::Type::CSVMap>("event", Offset(event)));
    add(new UIntCol("legs", Offset(legs)));
    for (unsigned i = 0; i < MxMDNLegs; i++) {
      ZuStringN<16> secVenue = "secVenue"; secVenue << ZuBoxed(i);
      ZuStringN<16> secSegment = "secSegment"; secSegment << ZuBoxed(i);
      ZuStringN<16> security = "security"; security << ZuBoxed(i);
      ZuStringN<8> side = "side"; side << ZuBoxed(i);
      ZuStringN<8> ratio = "ratio"; ratio << ZuBoxed(i);
      add(new IDCol(secVenue, Offset(secVenues) + (i * sizeof(MxID))));
      add(new IDCol(secSegment, Offset(secSegments) + (i * sizeof(MxID))));
      add(new IDStrCol(security,
	    Offset(securities) + (i * sizeof(MxSymString))));
      add(new EnumCol<MxSide::CSVMap>(side,
	    Offset(sides) + (i * sizeof(MxEnum))));
      add(new FloatCol(ratio,
	    Offset(ratios) + (i * sizeof(MxFloat)), app->ndp()));
    }
    add(new IDStrCol("tickSizeTbl", Offset(tickSizeTbl)));
#undef Offset
#define Offset(x) offsetof(Data, lotSizes) + offsetof(MxMDLotSizes, x)
    add(new FloatCol("oddLotSize", Offset(oddLotSize), app->ndp()));
    add(new FloatCol("lotSize", Offset(lotSize), app->ndp()));
    add(new FloatCol("blockLotSize", Offset(blockLotSize), app->ndp()));
#undef Offset
  }

  ZuAnyPOD *row(const MxMDStream::Msg *msg) {
    Data *data = m_pod->ptr();

    {
      using namespace MxMDStream;
      const Frame *frame = msg->data().frame();
      switch ((int)frame->type) {
	case Type::AddOrderBook:
	  {
	    const AddOrderBook &obj = frame->as<AddOrderBook>();
	    new (data) Data{
	      obj.key.venue(), obj.key.segment(), obj.key.id(),
	      frame->type, 1,
	      { obj.security.venue() },
	      { obj.security.segment() },
	      { obj.security.id() },
	      {}, {},
	      obj.tickSizeTbl, obj.lotSizes};
	  }
	  break;
	case Type::DelOrderBook:
	  {
	    const DelOrderBook &obj = frame->as<DelOrderBook>();
	    new (data) Data{
	      obj.key.venue(), obj.key.segment(), obj.key.id(), frame->type};
	  }
	  break;
	case Type::AddCombination:
	  {
	    const AddCombination &obj = frame->as<AddCombination>();
	    new (data) Data{
	      obj.key.venue(), obj.key.segment(), obj.key.id(), frame->type,
	      obj.legs, {}, {}, {}, {}, {},
	      obj.tickSizeTbl, obj.lotSizes};
	    for (unsigned i = 0, n = obj.legs; i < n; i++) {
	      data->secVenues[i] = obj.securities[i].venue();
	      data->secSegments[i] = obj.securities[i].segment();
	      data->securities[i] = obj.securities[i].id();
	      data->sides[i] = obj.sides[i];
	      data->ratios[i] = obj.ratios[i];
	    }
	  }
	  break;
	case Type::DelCombination:
	  {
	    const DelCombination &obj = frame->as<DelCombination>();
	    new (data) Data{
	      obj.key.venue(), obj.key.segment(), obj.key.id(), frame->type};
	  }
	  break;
	case Type::UpdateOrderBook:
	  {
	    const UpdateOrderBook &obj = frame->as<UpdateOrderBook>();
	    new (data) Data{
	      obj.key.venue(), obj.key.segment(), obj.key.id(), frame->type,
	      MxUInt(), {}, {}, {}, {}, {},
	      obj.tickSizeTbl, obj.lotSizes};
	  }
	  break;
	default:
	  return 0;
      }
    }

    return m_pod.ptr();
  }

private:
  ZuRef<POD>	m_pod;
};

template <class Writer>
class RealTimeCSV : public ZmObject, public ZvCSV {
public:
  struct L2Data {
    MxIDString		orderID;
    MxEnum		side;
    MxInt		rank;
    uint8_t		delta;
    MxFloat		price;
    MxFloat		qty;
    MxFloat		nOrders;
    MxFlags		flags;
    MxFlags		orderFlags;
  };
  struct Data {
    MxID		venue;
    MxID		segment;
    MxSymString		id;
    MxEnum		event;
    MxEnum		session;
    MxMDL1Data		l1Data;
    L2Data		l2Data;
    MxBool		updateL1;
  };
  typedef ZuPOD<Data> POD;

  template <typename App>
  RealTimeCSV(App *app) {
    new ((m_pod = new POD())->ptr()) Data();
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new IDCol("venue", Offset(venue)));
    add(new IDCol("segment", Offset(segment)));
    add(new IDStrCol("id", Offset(id)));
    add(new EnumCol<MxMDStream::Type::CSVMap>("event", Offset(event)));
    add(new EnumCol<MxTradingSession::CSVMap>("session", Offset(session)));
#undef Offset
#define Offset(x) offsetof(Data, l1Data) + offsetof(MxMDL1Data, x)
    if (app->hhmmss())
      add(new HHMMSSCol("stamp", Offset(stamp),
	    app->yyyymmdd(), app->tzOffset()));
    else
      add(new TimeCol("stamp", Offset(stamp), app->tzOffset()));
    add(new EnumCol<MxTradingStatus::CSVMap>("status", Offset(status)));
    add(new FloatCol("base", Offset(base), app->ndp()));
    for (unsigned i = 0; i < MxMDNSessions; i++) {
      ZuStringN<8> open = "open"; open << ZuBoxed(i);
      ZuStringN<8> close = "close"; close << ZuBoxed(i);
      add(new FloatCol(open,
	    Offset(open) + (i * sizeof(MxFloat)), app->ndp()));
      add(new FloatCol(close,
	    Offset(close) + (i * sizeof(MxFloat)), app->ndp()));
    }
    add(new FloatCol("last", Offset(last), app->ndp()));
    add(new FloatCol("lastQty", Offset(lastQty), app->ndp()));
    add(new FloatCol("bid", Offset(bid), app->ndp()));
    add(new FloatCol("bidQty", Offset(bidQty), app->ndp()));
    add(new FloatCol("ask", Offset(ask), app->ndp()));
    add(new FloatCol("askQty", Offset(askQty), app->ndp()));
    add(new EnumCol<MxTickDir::CSVMap>("tickDir", Offset(tickDir)));
    add(new FloatCol("high", Offset(high), app->ndp()));
    add(new FloatCol("low", Offset(low), app->ndp()));
    add(new FloatCol("accVol", Offset(accVol), app->ndp()));
    add(new FloatCol("accVolQty", Offset(accVolQty), app->ndp()));
    add(new FloatCol("match", Offset(match), app->ndp()));
    add(new FloatCol("matchQty", Offset(matchQty), app->ndp()));
    add(new FloatCol("surplusQty", Offset(surplusQty), app->ndp()));
    add(new FlagsCol<MxMDL1Flags>("l1Flags",
	  Offset(flags), offsetof(Data, venue)));
#undef Offset
#define Offset(x) offsetof(Data, l2Data) + offsetof(L2Data, x)
    add(new IDStrCol("orderID", Offset(orderID)));
    add(new EnumCol<MxSide::CSVMap>("side", Offset(side)));
    add(new IntCol("rank", Offset(rank)));
    add(new BoolCol("delta", Offset(delta)));
    add(new FloatCol("price", Offset(price), app->ndp()));
    add(new FloatCol("qty", Offset(qty), app->ndp()));
    add(new FloatCol("nOrders", Offset(nOrders), app->ndp()));
    add(new FlagsCol<MxMDL2Flags>("l2Flags",
	  Offset(flags), offsetof(Data, venue)));
    add(new FlagsCol<MxMDOrderFlags>("orderFlags",
	  Offset(orderFlags), offsetof(Data, venue)));
#undef Offset
    add(new BoolCol("updateL1", offsetof(Data, updateL1)));
  }

  ZuAnyPOD *row(const MxMDStream::Msg *msg) {
    Data *data = m_pod->ptr();

    {
      using namespace MxMDStream;
      const Frame *frame = msg->data().frame();
      switch ((int)frame->type) {
	case Type::TradingSession:
	  {
	    const TradingSession &obj = frame->as<TradingSession>();
	    new (data) Data{
	      obj.venue, obj.segment, {}, frame->type,
	      obj.session, { obj.stamp } };
	  }
	  break;
	case Type::L1:
	  {
	    const L1 &obj = frame->as<L1>();
	    new (data) Data{
	      obj.key.venue(), obj.key.segment(), obj.key.id(), frame->type,
	      {}, obj.data};
	  }
	  break;
	case Type::PxLevel:
	  {
	    const PxLevel &obj = frame->as<PxLevel>();
	    new (data) Data{
	      obj.key.venue(), obj.key.segment(), obj.key.id(), frame->type,
	      {}, { obj.transactTime },
	      { {}, {}, obj.side, {}, obj.delta, obj.price, obj.qty,
		obj.nOrders, obj.flags } };
	  }
	  break;
	case Type::AddOrder:
	case Type::ModifyOrder:
	  {
	    const AddOrder &obj = frame->as<AddOrder>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = frame->type;
	    data->l1Data.stamp = obj.transactTime;
	    data->l2Data.orderID = obj.orderID;
	    data->l2Data.side = obj.side;
	    data->l2Data.rank = obj.rank;
	    data->l2Data.price = obj.price;
	    data->l2Data.qty = obj.qty;
	    data->l2Data.orderFlags = obj.flags;
	  }
	  break;
	case Type::CancelOrder:
	  {
	    const CancelOrder &obj = frame->as<CancelOrder>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = frame->type;
	    data->l1Data.stamp = obj.transactTime;
	    data->l2Data.orderID = obj.orderID;
	    data->l2Data.side = obj.side;
	  }
	  break;
	case Type::L2:
	  {
	    const L2 &obj = frame->as<L2>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = frame->type;
	    data->l1Data.stamp = obj.stamp;
	    data->updateL1 = obj.updateL1;
	  }
	  break;
	case Type::ResetOB:
	  {
	    const ResetOB &obj = frame->as<ResetOB>();
	    data->venue = obj.key.venue();
	    data->segment = obj.key.segment();
	    data->id = obj.key.id();
	    data->event = frame->type;
	    data->l1Data.stamp = obj.transactTime;
	  }
	  break;
	case Type::RefDataLoaded:
	  {
	    const RefDataLoaded &obj = frame->as<RefDataLoaded>();
	    data->venue = obj.venue;
	    data->event = frame->type;
	  }
	  break;
	default:
	  return 0;
      }
    }
    return m_pod.ptr();
  }

private:
  ZuRef<POD>	m_pod;
};

typedef ZmList<ZmRef<MxMDStream::Msg>, ZmListNodeIsItem<true> > MsgQueue;

template <template <class> class CSV>
class CSVWriter : public CSV<CSVWriter<CSV> > {
public:
  typedef CSV<CSVWriter<CSV> > Base;
  typedef MxMDStream::Msg Msg;

  template <typename Path, typename App>
  CSVWriter(const Path &path, App *app) : Base(app), m_path(path) { }

  void start() { m_fn = this->writeFile(m_path); }

  void stop() { m_fn((ZuAnyPOD *)0); }

  inline void enqueue(Msg *msg) {
    ZuAnyPOD *pod = Base::row(msg);
    if (ZuLikely(pod)) m_fn(pod);
  }

private:
  ZtString		m_path;
  ZvCSVWriteFn		m_fn;
};

class App {
  typedef ZmLHash<MxSecKey> SecIDHash;

public:
  App() :
    m_refData(0), m_l1(0), m_l2(0), m_hhmmss(0), m_verbose(0),
    m_yyyymmdd(ZtDateNow().yyyymmdd()), m_tzOffset(0),
    m_ndp(-5) { }

  // dump options

  inline void verbose(bool b) { m_verbose = b; }

  // CSV formatting

  inline bool hhmmss() const { return m_hhmmss; }
  inline const unsigned &yyyymmdd() const { return m_yyyymmdd; }
  inline const int &tzOffset() const { return m_tzOffset; }
  inline const int &ndp() const { return m_ndp; }

  inline void hhmmss(bool b) { m_hhmmss = b; }
  inline void yyyymmdd(unsigned n) { m_yyyymmdd = n; }
  inline void tz(const char *tz) {
    ZtDate now(ZtDate::YYYYMMDD, m_yyyymmdd, ZtDate::HHMMSS, 120000, tz);
    m_tzOffset = now.offset(tz);
    m_isoFmt.offset(m_tzOffset);
  }
  inline void ndp(int n) { m_ndp = n; }

  // filters

  inline void refData(bool b) { m_refData = b; }
  inline void l1(bool b) { m_l1 = b; }
  inline void l2(bool b) { m_l2 = b; }

  inline void secID(const MxSecKey &key) { m_secIDs.add(key); }
  inline bool filterID(MxSecKey key) {
    if (!m_secIDs.count()) return false;
    if (m_secIDs.exists(key)) return false;
    if (*key.segment()) {
      key.segment() = MxID();
      if (m_secIDs.exists(key)) return false;
    }
    if (*key.venue()) {
      key.venue() = MxID();
      if (m_secIDs.exists(key)) return false;
    }
    return true;
  }

  // outputs

  inline const ZtString &path() const { return m_path; }
  inline const ZtString &outPath() const { return m_outPath; }

  template <typename Path>
  inline void path(const Path &path) { m_path = path; }
  template <typename Path>
  inline void outPath(const Path &path) { m_outPath = path; }

  template <typename Path>
  inline void tickSizeCSV(const Path &path) {
    m_tickSizeCSV = new CSVWriter<TickSizeCSV>(path, this);
  }
  template <typename Path>
  inline void securityCSV(const Path &path) {
    m_securityCSV = new CSVWriter<SecurityCSV>(path, this);
  }
  template <typename Path>
  inline void orderBookCSV(const Path &path) {
    m_orderBookCSV = new CSVWriter<OrderBookCSV>(path, this);
  }
  template <typename Path>
  inline void realTimeCSV(const Path &path) {
    m_realTimeCSV = new CSVWriter<RealTimeCSV>(path, this);
  }

  // application control

  void start() {
    ZeError e;
    if (m_file.open(m_path, ZiFile::ReadOnly, 0, &e) < 0) {
      ZeLOG(Error, ZtString() << '"' << m_path << "\": " << e);
      exit(1);
    }
    if (m_outPath) {
      ZeError e;
      if (m_outFile.open(m_outPath,
	    ZiFile::WriteOnly | ZiFile::Append | ZiFile::Create,
	    0666, &e) != Zi::OK) {
	ZeLOG(Error, ZtString() << '"' << m_outPath << "\": " << e);
	exit(1);
      }
      MxMDStream::FileHdr hdr("RMD",
	  MXMD_VMAJOR(MXMD_VERSION), 
	  MXMD_VMINOR(MXMD_VERSION));
      if (m_outFile.write(&hdr, sizeof(MxMDStream::FileHdr), &e) != Zi::OK) {
	m_outFile.close();
	ZeLOG(Error, ZtString() << '"' << m_outPath << "\": " << e);
	exit(1);
      }
    }
    if (m_tickSizeCSV) m_tickSizeCSV->start();
    if (m_securityCSV) m_securityCSV->start();
    if (m_orderBookCSV) m_orderBookCSV->start();
    if (m_realTimeCSV) m_realTimeCSV->start();
  }

  void stop() {
    m_file.close();
    if (m_outFile) m_outFile.close();
    if (m_tickSizeCSV) m_tickSizeCSV->stop();
    if (m_securityCSV) m_securityCSV->stop();
    if (m_orderBookCSV) m_orderBookCSV->stop();
    if (m_realTimeCSV) m_realTimeCSV->stop();
  }

  void read();

private:
  unsigned			m_refData:1,
				m_l1:1,
				m_l2:1,
				m_hhmmss:1,
				m_verbose:1;
  unsigned			m_yyyymmdd;
  int				m_tzOffset;
  int				m_ndp;

  ZtDateFmt::ISO		m_isoFmt;

  SecIDHash			m_secIDs;

  ZtString			m_path;
  ZiFile			m_file;

  ZtString			m_outPath;
  ZiFile			m_outFile;

  ZmRef<CSVWriter<TickSizeCSV> >	m_tickSizeCSV;
  ZmRef<CSVWriter<SecurityCSV> >	m_securityCSV;
  ZmRef<CSVWriter<OrderBookCSV> >	m_orderBookCSV;
  ZmRef<CSVWriter<RealTimeCSV> >	m_realTimeCSV;
};

void App::read()
{
  using namespace MxMDStream;

  if (!m_file) return;

  ZeError e;
  ZuPair<ZuBox0(uint16_t), ZuBox0(uint16_t)> v;
  int n;
  off_t o;

  try {
    FileHdr hdr(m_file, &e);
    v = ZuMkPair(hdr.vmajor, hdr.vminor);
    std::cout << "version: " <<
      ZuBoxed(v.p1()) << '.' << ZuBoxed(v.p2()) << '\n';
  } catch (const FileHdr::IOError &) {
    goto error;
  } catch (const FileHdr::InvalidFmt &) {
    ZeLOG(Error, ZtString() << '"' << m_path << "\": invalid format");
    return;
  }

  for (;;) {
    ZmRef<Msg> msg = new Msg();
    Frame *frame = msg->data().frame();
    o = m_file.offset();
    n = m_file.read(frame, sizeof(Frame), &e);
    if (n == Zi::IOError) goto error;
    if (n == Zi::EndOfFile || (unsigned)n < sizeof(Frame)) return;
    if (frame->len > sizeof(Buf)) goto lenerror;
    n = m_file.read(frame->ptr(), frame->len, &e);
    if (n == Zi::IOError) goto error;
    if (n == Zi::EndOfFile || (unsigned)n < frame->len) return;

    if (m_verbose) {
      if (frame->sec) {
	ZtDate stamp((time_t)frame->sec, (int32_t)frame->nsec);
	std::cout << "len: " << ZuBoxed(frame->len).fmt(ZuFmt::Right<6>()) <<
	  "  type: " << ZuBoxed(frame->type).fmt(ZuFmt::Right<6>()) <<
	  "  stamp: " << stamp.iso(m_isoFmt) << '\n';
      } else {
	std::cout << "len: " << ZuBoxed(frame->len).fmt(ZuFmt::Right<6>()) <<
	  "  type: " << ZuBoxed(frame->type).fmt(ZuFmt::Right<6>()) <<
	  "  stamp: (null)\n";
      }
    }
      
    {
      switch ((int)frame->type) {
	case Type::AddTickSizeTbl:
	case Type::ResetTickSizeTbl:
	  if (n != (int)sizeof(AddTickSizeTbl)) goto dataerror;
	  if (m_tickSizeCSV) m_tickSizeCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::AddTickSize:
	  if (n != (int)sizeof(AddTickSize)) goto dataerror;
	  if (m_tickSizeCSV) m_tickSizeCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;

	case Type::AddSecurity:
	  if (n != (int)sizeof(AddSecurity)) goto dataerror;
	  if (m_securityCSV) m_securityCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::UpdateSecurity:
	  if (n != (int)sizeof(UpdateSecurity)) goto dataerror;
	  if (m_securityCSV) m_securityCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;

	case Type::AddOrderBook:
	  if (n != (int)sizeof(AddOrderBook)) goto dataerror;
	  if (m_orderBookCSV) m_orderBookCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::DelOrderBook:
	  if (n != (int)sizeof(DelOrderBook)) goto dataerror;
	  if (m_orderBookCSV) m_orderBookCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::AddCombination:
	  if (n != (int)sizeof(AddCombination)) goto dataerror;
	  if (m_orderBookCSV) m_orderBookCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::DelCombination:
	  if (n != (int)sizeof(DelCombination)) goto dataerror;
	  if (m_orderBookCSV) m_orderBookCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;
	case Type::UpdateOrderBook:
	  if (n != (int)sizeof(UpdateOrderBook)) goto dataerror;
	  if (m_orderBookCSV) m_orderBookCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;

	case Type::RefDataLoaded:
	  if (n != (int)sizeof(RefDataLoaded)) goto dataerror;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  if (!m_refData) continue;
	  break;

	case Type::TradingSession:
	  if (n != (int)sizeof(TradingSession)) goto dataerror;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::L1:
	  if (n != (int)sizeof(L1)) goto dataerror;
	  if (!m_l1) continue;
	  if (filterID(frame->as<L1>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::PxLevel:
	  if (n != (int)sizeof(PxLevel)) goto dataerror;
	  if (!m_l2) continue;
	  if (filterID(frame->as<PxLevel>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::AddOrder:
	case Type::ModifyOrder:
	  if (n != (int)sizeof(AddOrder)) goto dataerror;
	  if (!m_l2) continue;
	  if (filterID(frame->as<AddOrder>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::CancelOrder:
	  if (n != (int)sizeof(CancelOrder)) goto dataerror;
	  if (!m_l2) continue;
	  if (filterID(frame->as<CancelOrder>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::L2:
	  if (n != (int)sizeof(L2)) goto dataerror;
	  if (!m_l2) continue;
	  if (filterID(frame->as<L2>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	case Type::ResetOB:
	  if (n != (int)sizeof(ResetOB)) goto dataerror;
	  if (!m_l2) continue;
	  if (filterID(frame->as<ResetOB>().key)) continue;
	  if (m_realTimeCSV) m_realTimeCSV->enqueue(msg);
	  break;
	default:
	  continue;
      }
    }

    if (m_outFile) {
      ZiVec vec[2];
      ZiVec_ptr(vec[0]) = (void *)frame;
      ZiVec_len(vec[0]) = sizeof(Frame);
      ZiVec_ptr(vec[1]) = (void *)frame->ptr();
      ZiVec_len(vec[1]) = frame->len;
      ZeError e;
      if (m_outFile.writev(vec, 2, &e) != Zi::OK) {
	ZeLOG(Error, ZtString() << '"' << m_outPath << "\": " << e);
	return;
      }
    }
  }

dataerror:
  ZeLOG(Error, ZtString() << '"' << m_path << "\": corrupt data error " <<
      ZuBoxed(n) << " at offset " << ZuBoxed(o));
  return;

lenerror:
  ZeLOG(Error, ZtString() << '"' << m_path << "\": message length >" <<
      ZuBoxed(sizeof(MxMDStream::Buf)) << " at offset " << ZuBoxed(o));
  return;

error:
  ZeLOG(Error, ZtString() << '"' << m_path << "\": " << e);
}

void usage()
{
  std::cerr <<
    "usage: recdump [OPTION]... RECFILE\n"
    "\tRECFILE\t- market data recording file\n\n"
    "Options:\n"
    "  -r\t\t- include reference data in output\n"
    "  -1\t\t- include Level 1 data in output\n"
    "  -2\t\t- include Level 2 data in output\n"
    "  -n\t\t- CSV time stamps as HHMMSS instead of Excel format\n"
    "  -V\t\t- dump message frames to standard output\n"
    "  -d YYYYMMDD\t- CSV time stamps use date YYYYMMDD\n"
    "  -z ZONE\t- CSV time stamps in local time ZONE (defaults to GMT)\n"
    "  -p NDP\t- CSV values with NDP precision (-ve to suppress trailing 0s)\n"
    "  -v MIC\t- select venue MIC for following securities\n"
    "\t\t\t(may be specified multiple times)\n"
    "  -s SEGMENT\t- select SEGMENT for following securities\n"
    "\t\t\t(may be specified multiple times)\n"
    "  -i ID\t\t- filter for security ID\n"
    "\t\t\t(may be specified multiple times)\n"
    "  -o OUT\t- record filtered output in file OUT\n"
    "  -T CSV\t- dump tick size messages to CSV\n"
    "  -S CSV\t- dump security messages to CSV\n"
    "  -O CSV\t- dump order book messages to CSV\n"
    "  -R CSV\t- dump real-time messages to CSV\n"
    << std::flush;
  exit(1);
}

int main(int argc, const char *argv[])
{
  App app;
  MxID venue;
  MxID segment;
  bool tickSizeCSV = false,
       securityCSV = false,
       orderBookCSV = false,
       realTimeCSV = false;

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      if (app.path()) usage();
      app.path(argv[i]);
      continue;
    }
    switch (argv[i][1]) {
      case 'r':
	app.refData(true);
	break;
      case '1':
	app.l1(true);
	break;
      case '2':
	app.l2(true);
	break;
      case 'n':
	app.hhmmss(true);
	break;
      case 'd':
	if (++i >= argc) usage();
	app.yyyymmdd(atoi(argv[i]));
	break;
      case 'z':
	if (++i >= argc) usage();
	app.tz(argv[i]);
	break;
      case 'p':
	if (++i >= argc) usage();
	app.ndp(atoi(argv[i]));
	break;
      case 'v':
	if (++i >= argc) usage();
	venue = argv[i];
	break;
      case 's':
	if (++i >= argc) usage();
	segment = argv[i];
	break;
      case 'i':
	if (++i >= argc) usage();
	app.secID(MxSecKey(venue, segment, argv[i]));
	break;
      case 'o':
	if (app.outPath()) usage();
	if (++i >= argc) usage();
	app.outPath(argv[i]);
	break;
      case 'T':
	if (tickSizeCSV || ++i >= argc) usage();
	tickSizeCSV = true;
	app.tickSizeCSV(argv[i]);
	break;
      case 'S':
	if (securityCSV || ++i >= argc) usage();
	securityCSV = true;
	app.securityCSV(argv[i]);
	break;
      case 'O':
	if (orderBookCSV || ++i >= argc) usage();
	orderBookCSV = true;
	app.orderBookCSV(argv[i]);
	break;
      case 'R':
	if (realTimeCSV || ++i >= argc) usage();
	realTimeCSV = true;
	app.realTimeCSV(argv[i]);
	break;
      case 'V':
	app.verbose(true);
	break;
      default:
	usage();
	break;
    }
  }
  if (!app.path()) usage();

  ZeLog::init("recdump");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink("&2"));
  ZeLog::start();

  app.start();
  app.read();
  app.stop();
  
  ZeLog::stop();
  return 0;
}
