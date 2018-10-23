#include <MxMDCore.hpp>

#include <MxMDCmd.hpp>

void MxMDCmd::init(MxMDCore *core, ZvCf *cf)
{
  m_syntax = new ZvCf();

  ZvCmdServer::init(cf, core->mx(), ZvCmdDiscFn(),
      ZvCmdRcvdFn::Member<&MxMDCmd::rcvd>::fn(this));

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

void MxMDCmd::help(const Args &args, ZtArray<char> &help)
{
  int argc = ZuBox<int>(args.get("#"));
  if (argc > 2) throw Usage();
  if (ZuUnlikely(argc == 2)) {
    Cmds::NodeRef cmd = m_cmds.find(args.get("1"));
    if (!cmd) throw Usage();
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
      Args args;
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
  } catch (const Usage &) {
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
