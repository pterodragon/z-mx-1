#include <MxMDCore.hpp>

#include <MxMDCmd.hpp>

void MxMDCmd::init(MxMDCore *core, ZvCf *cf)
{
  m_core = core;

  m_syntax = new ZvCf();

  ZvCmdServer::init(cf, core->mx(), ZvCmdDiscFn(),
      ZvCmdRcvdFn::Member<&MxMDCmd::rcvd>::fn(this));

  // Assumption: DST transitions do not occur while market is open
  {
    ZtDate now(ZtDate::Now);
    ZuString timezone = cf->get("timezone"); // default to system tz
    now.sec() = 0, now.nsec() = 0; // midnight GMT (start of today)
    now += ZmTime((time_t)(now.offset(timezone) + 43200)); // midday local time
    m_tzOffset = now.offset(timezone);
  }

  addCmd("help", "", Fn::Member<&MxMDCmd::help>::fn(this),
      "list commands", "usage: help [COMMAND]");
}

void MxMDCmd::final()
{
  m_syntax = 0;
  m_cmds.clean();
  ZvCmdServer::final();
}

void MxMDCmd::addCmd(ZuString name, ZuString syntax, Fn fn,
    ZtString brief, ZtString usage)
{
  {
    ZmRef<ZvCf> cf = m_syntax->subset(name, true);
    cf->fromString(syntax, false);
    cf->set("help:type", "flag");
  }
  if (Cmds::NodeRef cmd = m_cmds.find(name))
    cmd->val() = ZuMkTuple(ZuMv(fn), ZuMv(brief), ZuMv(usage));
  else
    m_cmds.add(name, ZuMkTuple(ZuMv(fn), ZuMv(brief), ZuMv(usage)));
}

ZtString MxMDCmd::lookupSyntax()
{
  return 
    "S src src { type scalar } "
    "m market market { type scalar } "
    "s segment segment { type scalar }";
}

ZtString MxMDCmd::lookupOptions()
{
  return
    "    -S --src=SRC\t- symbol ID source is SRC\n"
    "\t(CUSIP|SEDOL|QUIK|ISIN|RIC|EXCH|CTA|BSYM|BBGID|FX|CRYPTO)\n"
    "    -m --market=MIC\t - market MIC, e.g. XTKS\n"
    "    -s --segment=SEGMENT\t- market segment SEGMENT\n";
}

void MxMDCmd::lookupSecurity(
    MxMDLib *md, const MxMDCmd::CmdArgs &args, unsigned index,
    bool secRequired, ZmFn<MxMDSecurity *> fn)
{
  ZuString symbol = args.get(ZuStringN<16>(ZuBoxed(index)));
  MxID venue = args.get("market");
  MxID segment = args.get("segment");
  bool notFound = 0;
  thread_local ZmSemaphore sem;
  if (ZuString src_ = args.get("src")) {
    MxEnum src = MxSecIDSrc::lookup(src_);
    MxSecSymKey key{src, symbol};
    m_core->secInvoke(key,
	[secRequired, sem = &sem, &notFound, fn = ZuMv(fn)](MxMDSecurity *sec) {
      if (secRequired && ZuUnlikely(!sec))
	notFound = 1;
      else
	fn(sec);
      sem->post();
    });
    sem.wait();
    if (ZuUnlikely(notFound))
      throw ZtString() << "security " << key << " not found";
  } else {
    if (!*venue) throw MxMDCmd::CmdUsage();
    MxSecKey key{venue, segment, symbol};
    m_core->secInvoke(key,
	[secRequired, sem = &sem, &notFound, fn = ZuMv(fn)](MxMDSecurity *sec) {
      if (secRequired && ZuUnlikely(!sec))
	notFound = 1;
      else
	fn(sec);
      sem->post();
    });
    sem.wait();
    if (ZuUnlikely(notFound))
      throw ZtString() << "security " << key << " not found";
  }
}

void MxMDCmd::lookupOrderBook(
    MxMDLib *md, const MxMDCmd::CmdArgs &args, unsigned index,
    bool secRequired, bool obRequired,
    ZmFn<MxMDSecurity *, MxMDOrderBook *> fn)
{
  MxID venue = args.get("market");
  MxID segment = args.get("segment");
  bool notFound = 0;
  thread_local ZmSemaphore sem;
  lookupSecurity(md, args, index, secRequired || obRequired,
      [obRequired, &notFound, sem = &sem, venue, segment, fn = ZuMv(fn)](
	MxMDSecurity *sec) {
    ZmRef<MxMDOrderBook> ob = sec->orderBook(venue, segment);
    if (obRequired && ZuUnlikely(!ob))
      notFound = 1;
    else
      fn(sec, ob);
    sem->post();
  });
  sem.wait();
  if (ZuUnlikely(notFound))
    throw ZtString() << "order book " <<
	MxSecKey{venue, segment, args.get(ZuStringN<16>(ZuBoxed(index)))} <<
	" not found";
}

void MxMDCmd::rcvd(ZvCmdLine *line, const ZvInvocation &inv, ZvAnswer &ans)
{
  ZmRef<ZvCf> cf = new ZvCf();
  ZuString name;
  Cmds::NodeRef cmd;
  try {
    cf->fromCLI(m_syntax, inv.cmd());
    name = cf->get("0");
    cmd = m_cmds.find(name);
    if (!cmd) throw ZtString("unknown command");
    if (cf->getInt("help", 0, 1, false, 0))
      ans.make(ZvCmd::Success, Ze::Info,
	       ZvAnswerArgs, cmd->val().p3());
    else {
      CmdArgs args;
      ZvCf::Iterator i(cf);
      {
	ZuString arg;
	const ZtArray<ZtString> *values;
	while (values = i.getMultiple(arg, 0, INT_MAX)) args.add(arg, values);
      }
      ZtArray<char> out;
      (cmd->val().p1())(args, out);
      ans.make(ZvCmd::Success, out);
    }
  } catch (const CmdUsage &) {
    ans.make(ZvCmd::Fail, Ze::Warning, ZvAnswerArgs, cmd->val().p3().data());
  } catch (const ZvError &e) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtString() << '"' << name << "\": " << e);
  } catch (const ZeError &e) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtString() << '"' << name << "\": " << e);
  } catch (const ZtString &s) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtString() << '"' << name << "\": " << s);
  } catch (const ZtArray<char> &s) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtString() << '"' << name << "\": " << s);
  } catch (...) {
    ans.make(ZvCmd::Fail | ZvCmd::Log, Ze::Error, ZvAnswerArgs,
	ZtString() << '"' << name << "\": unknown exception");
  }
}

void MxMDCmd::help(const CmdArgs &args, ZtArray<char> &help)
{
  int argc = ZuBox<int>(args.get("#"));
  if (argc > 2) throw CmdUsage();
  if (ZuUnlikely(argc == 2)) {
    Cmds::NodeRef cmd = m_cmds.find(args.get("1"));
    if (!cmd) throw CmdUsage();
    help = cmd->val().p3();
    return;
  }
  help.size(m_cmds.count() * 80 + 40);
  help += "List of commands:\n\n";
  {
    auto i = m_cmds.readIterator();
    while (Cmds::NodeRef cmd = i.iterate()) {
      help += cmd->key();
      help += " -- ";
      help += cmd->val().p2();
      help += "\n";
    }
  }
}
