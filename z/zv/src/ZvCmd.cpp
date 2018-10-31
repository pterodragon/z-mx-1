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

#include <ZvCmd.hpp>

#include <ZtRegex.hpp>

namespace {
  static void preprocess(ZuString msg,
      ZtArray<char> &cmd, ZtArray<char> &data) {
    const auto &cmdRedirect = ZtStaticRegexUTF8("<\\s*");
    ZtRegex::Captures c;
    unsigned pos = 0, n = 0;
    if (n = cmdRedirect.m(msg, c, pos)) {
      cmd = c[0];
      data = c[2];
    } else {
      cmd = msg;
    }
    if (data.length() > 0) {
      ZiFile f;
      if (Zi::OK == f.open(data, ZiFile::ReadOnly | ZiFile::GC)) {
	data.length((int)f.size());
	if (f.read(data.data(), data.length()) != (int)data.length())
	  data.null();
	return;
      }
    }
    data.null();
  }
}

void ZvCmdClient::send(ZuString s, uint32_t seqNo)
{
  ZtArray<char> cmd;
  ZtArray<char> data;

  preprocess(s, cmd, data);
  if (!data) data = m_stdinData;

  ZmRef<ZvCmd_Msg> msg = new ZvCmd_Msg(seqNo, cmd, data);
  ZmRef<ZvCmdLine> line;

  {
    Guard guard(m_lock);
    if (!(line = m_line)) return;
    if (m_busy) { m_queue.push(msg); return; }
    m_busy = true;
  }
  line->send(msg);
}

void ZvCmdLine::send(ZvCmd_Msg *msg)
{
  msg->line(this);
  msg->send();
}

void ZvCmdLine::connected(ZiIOContext &io) {
  if (m_connFn) m_connFn(this);
  ZmRef<ZvCmd_Msg> msg = new ZvCmd_Msg(this);
  msg->recv(io);
}

void ZvCmd_Msg::recv(ZiIOContext &io)
{
  io.init(ZiIOFn::Member<&ZvCmd_Msg::recvHdr_>::fn(ZmMkRef(this)),
      header(), headerLen(), 0);
}
void ZvCmd_Msg::recvHdr_(ZiIOContext &io)
{
  if ((io.offset += io.length) < io.size) return;
  int n = cmdLen();
  if (n < 0) {
    io.disconnect();
    ZeLOG(Warning, "received corrupt message");
    return;
  }
  extend();
  if (!n && !dataLen()) { rcvd_(io); return; }
  io.init(ZiIOFn::Member<&ZvCmd_Msg::recvCmd_>::fn(ZmMkRef(this)),
      cmdPtr(), cmdLen(), 0);
}
void ZvCmd_Msg::recvCmd_(ZiIOContext &io)
{
  if ((io.offset += io.length) < io.size) return;
  if (!dataLen()) { rcvd_(io); return; }
  io.init(ZiIOFn::Member<&ZvCmd_Msg::recvData_>::fn(ZmMkRef(this)),
      dataPtr(), dataLen(), 0);
}
void ZvCmd_Msg::recvData_(ZiIOContext &io)
{
  if ((io.offset += io.length) < io.size) return;
  rcvd_(io);
}
void ZvCmd_Msg::rcvd_(ZiIOContext &io)
{
  m_line->mx()->add(
      ZmFn<>::Member<static_cast<void (ZvCmd_Msg::*)()>(
	&ZvCmd_Msg::rcvd_)>::fn(ZmMkRef(this)));
  ZmRef<ZvCmd_Msg> msg = new ZvCmd_Msg(m_line);
  msg->recv(io);
}
void ZvCmd_Msg::rcvd_()
{
  m_line->rcvd(this);
}

void ZvCmd_Msg::send()
{
  m_line->ZiConnection::send(
      ZiIOFn::Member<&ZvCmd_Msg::send_>::fn(ZmMkRef(this)));
}
void ZvCmd_Msg::send_(ZiIOContext &io)
{
  io.init(ZiIOFn::Member<&ZvCmd_Msg::sendHdr_>::fn(ZmMkRef(this)),
      header(), headerLen(), 0);
}
void ZvCmd_Msg::sendHdr_(ZiIOContext &io)
{
  if ((io.offset += io.length) < io.size) return;
  if (!cmdLen()) { io.complete(); sent_(); return; }
  io.init(ZiIOFn::Member<&ZvCmd_Msg::sendCmd_>::fn(ZmMkRef(this)),
      cmdPtr(), cmdLen(), 0);
}
void ZvCmd_Msg::sendCmd_(ZiIOContext &io)
{
  if ((io.offset += io.length) < io.size) return;
  if (!dataLen()) { io.complete(); sent_(); return; }
  io.init(ZiIOFn::Member<&ZvCmd_Msg::sendData_>::fn(ZmMkRef(this)),
      dataPtr(), dataLen(), 0);
}
void ZvCmd_Msg::sendData_(ZiIOContext &io)
{
  if ((io.offset += io.length) < io.size) return;
  io.complete();
  sent_();
}
void ZvCmd_Msg::sent_() { m_line->sent(this); }

void ZvCmdLine::disconnected() { if (m_discFn) m_discFn(this); }

void ZvCmdLine::send(ZuString msg_, uint32_t seqNo)
{
  ZtArray<char> cmd;
  ZtArray<char> data;
  preprocess(msg_, cmd, data);
  ZmRef<ZvCmd_Msg> msg = new ZvCmd_Msg(seqNo, ZuMv(cmd), ZuMv(data));
  msg->line(this);
  msg->send();
}

void ZvCmdLine::send(const ZvAnswer &ans)
{
  ZtArray<char> event;
  event << ans.message();
  ZmRef<ZvCmd_Msg> msg = new ZvCmd_Msg(
      ans.flags(), ans.seqNo(), ZuMv(event), ZuMv(ans.data()));
  msg->line(this);
  msg->send();
}

void ZvCmdLine::rcvd(ZvCmd_Msg *msg)
{
  if (m_incoming) {
    if (m_rcvdFn) {
      ZvInvocation inv(msg->seqNo(), msg->cmd(), msg->data());
      ZvAnswer ans(msg->seqNo());
      m_rcvdFn(this, inv, ans);
      send(ans);
    }
  } else {
    cancelTimeout();
    ZvAnswer ans(msg->flags(), msg->seqNo(), msg->cmd(), msg->data());
    if (m_ackRcvdFn) m_ackRcvdFn(this, ans);
  }
}

void ZvCmdClient::ackd(ZvCmdLine *line, const ZvAnswer &ans)
{
  if (m_ackRcvdFn) m_ackRcvdFn(line, ans);
  ZmRef<ZvCmd_Msg> msg = 0;

  {
    Guard guard(m_lock);
    if (line != m_line) return;
    if (!(msg = m_queue.shift())) { m_busy = false; return; }
  }
  line->send(msg);
}

void ZvCmdLine::sent(ZvCmd_Msg *msg)
{
  if (m_incoming) {
    if (m_ackSentFn) m_ackSentFn(this);
  } else {
    scheduleTimeout();
    if (m_sentFn) {
      ZvInvocation inv(msg->seqNo(), msg->cmd(), msg->data());
      m_sentFn(this, inv);
    }
  }
}

void ZvCmdLine::scheduleTimeout()
{
  if (m_timeoutDisconnect > 0)
    mx()->add(ZmFn<>::Member<&ZvCmdLine::timeout>::fn(this),
	ZmTimeNow(m_timeoutDisconnect), ZmScheduler::Defer, &m_timeout);
}

void ZvCmdLine::cancelTimeout()
{
  if (m_timeoutDisconnect > 0) mx()->del(&m_timeout);
}

void ZvCmdLine::timeout()
{
#ifndef _WIN32
  static ZeError::ErrNo code = ETIMEDOUT;
#else
  static ZeError::ErrNo code = WAIT_TIMEOUT;
#endif
  warn(ZuFnName, Zi::IOError, ZeError(code));
}

void ZvCmdLine::warn(const char *op, int status, ZeError e)
{
  if (status == Zi::IOError) {
    ZeLOG(Warning, ZtSprintf("%s - %s", op, e.message()));
  }
  disconnect();
}

void ZvCmdClient::init(
    ZiMultiplex *mx, ZvCf *cf, ZvCmdConnFn connFn,
    ZvCmdDiscFn discFn, ZvCmdAckRcvdFn ackRcvdFn)
{
  m_mx = mx;
  m_connFn = connFn;
  m_discFn = discFn;
  m_ackRcvdFn = ackRcvdFn;

  m_remotePort = m_localPort = m_reconnectFreq = 0;
  if (cf) {
    m_remoteIP = cf->get("remoteIP", false, "127.0.0.1");
    m_remotePort = cf->getInt("remotePort", 1, (1<<16) - 1, true);
    m_localIP = cf->get("localIP", false, "0.0.0.0");
    m_localPort = cf->getInt("localPort", 1, (1<<16) - 1, false, 0);
    m_reconnectFreq = cf->getInt("reconnectFreq", 0, 3600, false, 0);
  }
}

void ZvCmdClient::final()
{
  m_mx = 0;
  m_connFn = ZvCmdConnFn();
  m_discFn = ZvCmdDiscFn();
  m_ackRcvdFn = ZvCmdAckRcvdFn();
}

void ZvCmdClient::connect(ZiIP ip, int port)
{
  m_mx->connect(
      ZiConnectFn::Member<&ZvCmdClient::connected>::fn(this),
      ZiFailFn::Member<&ZvCmdClient::failed>::fn(this),
      ZiIP(), 0, ip, port);
}

void ZvCmdClient::failed(bool transient)
{
  if (transient && m_reconnectFreq > 0)
    m_mx->add(ZmFn<>::Member<static_cast<void (ZvCmdClient::*)()>(
	  &ZvCmdClient::connect)>::fn(this),
	ZmTimeNow(m_reconnectFreq));
}

void ZvCmdClient::connect()
{
  ZeLOG(Info, ZtString() << "connect(" <<
      m_localIP << ':' << ZuBoxed(m_localPort) << " -> " <<
      m_remoteIP << ':' << ZuBoxed(m_remotePort) << ')');
  m_mx->connect(
      ZiConnectFn::Member<&ZvCmdClient::connected>::fn(this),
      ZiFailFn::Member<&ZvCmdClient::failed>::fn(this),
      m_localIP, m_localPort, m_remoteIP, m_remotePort);
}

void ZvCmdClient::stop()
{
  ZmRef<ZvCmdLine> line;

  {
    Guard guard(m_lock);
    if (!(line = m_line)) return;
  }
  line->disconnect();
}

#if 0
  if (status != Zi::OK) {
    if (status == Zi::IOError) {
      ZeLOG(Warning, ZtSprintf(
	    "connected(%s:%d): %s",
	    ci.remoteIP().string().data(),
	    ci.remotePort(), e.message().data()));
    }
    if (m_discFn) m_discFn((ZvCmdLine *)0);
    reconnect();
    return 0;
  }
#endif

ZiConnection *ZvCmdClient::connected(const ZiConnectionInfo &info)
{
  Guard guard(m_lock);
  m_line = new ZvCmdLine(
      m_mx, info, false, m_timeoutDisconnect, m_connFn,
      ZvCmdDiscFn::Member<&ZvCmdClient::disconnected>::fn(this),
      ZvCmdSentFn::Member<&ZvCmdClient::sent>::fn(this),
      ZvCmdRcvdFn(), ZvCmdAckSentFn(),
      ZvCmdAckRcvdFn::Member<&ZvCmdClient::ackd>::fn(this));
  m_busy = false;
  m_queue.clean();
  return m_line;
}

void ZvCmdClient::disconnected(ZvCmdLine *line)
{
  if (m_discFn) m_discFn(line);
  {
    Guard guard(m_lock);
    if (m_line == line) m_line = 0;
  }
}

void ZvCmdClient::sent(ZvCmdLine *line, const ZvInvocation &inv)
{
}

void ZvCmdServer::init(ZvCf *cf,
    ZiMultiplex *mx, ZvCmdDiscFn discFn, ZvCmdRcvdFn rcvdFn)
{
  m_rebindFreq = cf->getInt("rebindFreq", 0, 3600, false, 0);
  m_ip = cf->get("localIP", false, "127.0.0.1");
  m_port = cf->getInt("localPort", 1, (1<<16) - 1, false, 19400);
  m_nAccepts = cf->getInt("nAccepts", 1, 1024, false, 8);
  m_mx = mx;
  m_discFn = discFn;
  m_rcvdFn = rcvdFn;
}

void ZvCmdServer::final()
{
  m_mx = 0;
  m_discFn = ZvCmdDiscFn();
  m_rcvdFn = ZvCmdRcvdFn();
}

void ZvCmdServer::start()
{
  listen();
  m_started.wait();
}

void ZvCmdServer::stop()
{
  m_mx->stopListening(m_ip, m_port);
  m_listening = false;
}

void ZvCmdServer::listen()
{
  m_mx->listen(
      ZiListenFn::Member<&ZvCmdServer::listening>::fn(this),
      ZiFailFn::Member<&ZvCmdServer::failed>::fn(this),
      ZiConnectFn::Member<&ZvCmdServer::accepted>::fn(this),
      m_ip, m_port, m_nAccepts);
}

void ZvCmdServer::listening(const ZiListenInfo &)
{
  m_listening = true;
  m_started.post();
}

void ZvCmdServer::failed(bool transient)
{
  if (transient && m_rebindFreq > 0)
    m_mx->add(ZmFn<>::Member<&ZvCmdServer::listen>::fn(this),
	ZmTimeNow(m_rebindFreq));
  else {
    m_listening = false;
    m_started.post();
  }
}

ZiConnection *ZvCmdServer::accepted(const ZiConnectionInfo &info)
{
  return new ZvCmdLine(
      m_mx, info, true, 10,
      ZvCmdConnFn::Member<&ZvCmdServer::connected>::fn(this),
      m_discFn, ZvCmdSentFn(), m_rcvdFn,
      ZvCmdAckSentFn::Member<&ZvCmdServer::ackSent>::fn(this),
      ZvCmdAckRcvdFn());
}

void ZvCmdServer::connected(ZvCmdLine *line)
{
}

void ZvCmdServer::ackSent(ZvCmdLine *line)
{
}

ZmRef<ZvCmdObject::CmdOption> ZvCmdObject::HelpOption = 
  new ZvCmdObject::CmdOption("", "help", "", "flag", "display this help");

struct Sort_Helper { 
  char *m_shortName; 
  char *m_longName;
  int  m_idx;
};

int opt_sorter(const void *p1_, const void *p2_)
{
  const Sort_Helper *p1 = (const Sort_Helper *)p1_;
  const Sort_Helper *p2 = (const Sort_Helper *)p2_;
  if (p1->m_shortName && p2->m_shortName)
    return strcmp(p1->m_shortName, p2->m_shortName);
  if (p1->m_shortName && !p2->m_shortName)
    return strcmp(p1->m_shortName, p2->m_longName);
  if (!p1->m_shortName && p2->m_shortName)
    return strcmp(p1->m_longName, p2->m_shortName);
  return strcmp(p1->m_longName, p2->m_longName);
}

void ZvCmdObject::CmdData::sort()
{
  unsigned optGrps = m_optGrps.length();
  for (unsigned x = 0; x < optGrps; x++) {
    unsigned optLen = m_optGrps[x]->m_options.length();
    if (!optLen) continue;
    Sort_Helper *helper = new Sort_Helper[optLen];
    if (!helper) return;
    for (unsigned i = 0; i < optLen; i++) { 
      ZmRef<CmdOption> opt = m_optGrps[x]->m_options[i];
      helper[i].m_shortName = helper[i].m_longName = 0;
      helper[i].m_idx = i;
      if (opt->m_shortName) helper[i].m_shortName = opt->m_shortName.data();
      if (opt->m_longName) helper[i].m_longName = opt->m_longName.data();
    }
    ::qsort(&helper[0], optLen, sizeof(Sort_Helper), opt_sorter);
    ZtArray<ZmRef<ZvCmdObject::CmdOption> > newOpts(0U, optLen);
    for (unsigned i = 0; i < optLen; i++)
      newOpts.push(m_optGrps[x]->m_options[helper[i].m_idx]);
    m_optGrps[x]->m_options = newOpts;
    delete [] helper;
  }
}

#define ZvUsageShort(opt) \
  do { \
    if (opt->m_shortName) { \
      tmp = ZtSprintf("  -%s,", opt->m_shortName.data()); \
    } else { \
      tmp = ZtSprintf("%5.5s", " "); \
    } \
  } while (0)

#define ZvUsageLong(opt) \
  do { \
    if (opt->m_longName) { \
      tmp << ZtSprintf(" --%s", opt->m_longName.data()); \
      if (opt->m_longType) { \
	tmp << ZtSprintf("=%s", opt->m_longType.data()); \
      } \
    } else { \
      tmp << ZtSprintf("%21.21s", " "); \
    } \
  } while (0)

void ZvCmdObject::CmdData::wordWrap(ZtString &dst,
    const char *src, int srcLen, int lineLen, int used, int indent)
{
  int bytesWrote = 0;
  int bytesLeft = srcLen - bytesWrote;
  int wlen = (used ? lineLen - used : lineLen);
  do {
    int wlen_ = wlen;
    if (wlen < bytesLeft) {
      while (wlen_ && src[wlen_ - 1] && src[wlen_ - 1] != ' ') --wlen_;
      if (!wlen_) wlen_ = wlen;
    }
    dst << ZtSprintf("%-*.*s", wlen_, wlen_, src);
    src += wlen_;
    bytesWrote += wlen_;
    bytesLeft -= wlen_;
    wlen = lineLen;
    if (bytesLeft > 0 && indent)
      dst << ZtSprintf("\n%*.*s", indent, indent, " ");
    else if (bytesLeft > 0) dst << "\n";
  } while (bytesLeft > 0);
}

ZtString ZvCmdObject::CmdData::toGNUString(ZuString name)
{
  ZmAssert(name);
  this->sort();

  ZtString usage;
  // generate usage string. 
  // format taken from http://www.gnu.org/software/help2man
  if (m_usages.length()) {
    usage << "Usage: " << name << ' ' << m_usages[0];
    unsigned len = m_usages.length();
    for (unsigned i = 1; i < len; i++)
      usage << "\n  or:  " << name << ' ' << m_usages[i];
  } else {
    usage << "Usage: " << name << " [OPTION]...";
  }

  // add short description
  if (m_brief) usage << "\n" << m_brief;

  unsigned optGrps = m_optGrps.length();
  for (unsigned x = 0; x < optGrps; x++) {
    ZmRef<OptionGroup> group = m_optGrps[x];
    ZtArray<ZmRef<CmdOption> > &options = group->m_options;
    unsigned optLen = options.length();
    if (optLen) {
      usage << "\n";
      if (group->m_header)
	usage << "\n" << group->m_header << ":";
      // add option descriptions
      ZtString tmp;
      for (unsigned i = 0; i < optLen; i++) {
	usage << "\n";
	tmp.init(0U, 256);
	ZvUsageShort(options[i]);
	ZvUsageLong(options[i]);
	if (!options[i]->m_description) {
	  usage << tmp;
	  continue;
	}
	// pad the string with spaces until length is 28
	// but at least two spaces if it is larger
	tmp << "  ";
	while (tmp.length() < 28) tmp << ' ';
	
	wordWrap(tmp, options[i]->m_description.data(),
	    options[i]->m_description.length(),
	    50, 50 - (78 - tmp.length()), 30);
	usage << tmp;
      }
    }
  }

  // always add a break for the help option even if no brief or other
  // options are given
  if (!m_optGrps[0]->m_options.length()) usage << "\n";

  {
    // if more that the default option group add an extra break before
    // the help option to distinguish it from the last group
    if (optGrps > 1) usage << "\n";
    usage << "\n";
    ZtString tmp(0U, 80);
    ZvUsageShort(ZvCmdObject::HelpOption);
    ZvUsageLong(ZvCmdObject::HelpOption);
    tmp << "  ";
    while (tmp.length() < 28) tmp << ' ';
      
    wordWrap(tmp, ZvCmdObject::HelpOption->m_description.data(), 
	     ZvCmdObject::HelpOption->m_description.length(), 
	     50, 50 - (78 - tmp.length()), 30);
    usage << tmp;
  }

  // add long description(s)
  unsigned n = m_manuscripts.length();
  for (unsigned i = 0; i < n; i++) {
    if (m_manuscripts[i]) {
      usage << "\n\n";
      wordWrap(usage, m_manuscripts[i].data(), m_manuscripts[i].length());
    }
  }

  return usage;
}

void ZvCmdObject::CmdData::clean()
{
  m_usages.null();
  m_optGrps.null();
  m_brief.null();
  m_manuscripts.null();
}

#define ZvCheckOpt(s, l, o) \
    do { \
      if (l.length() > 0) { \
	m_syntax << ZtSprintf("%s { type %s }\n", l.data(), o.data());	\
	if (s.length() > 0) { \
	  m_syntax << ZtSprintf("%s %s\n", s.data(), l.data()); \
	} \
      } \
    } while (0)

void ZvCmdObject::compile()
{
  ZmHash<const char *, ZmHashLock<ZmNoLock> > uniqueHash;

  // setup syntax
  for (unsigned i = 0, n = m_data.m_optGrps.length(); i < n; i++) {
    ZmRef<OptionGroup> group = m_data.m_optGrps[i];
    for (unsigned j = 0, m = group->m_options.length(); j < m; j++) {
      if (uniqueHash.findKey(group->m_options[j]->m_shortName.data()) ||
	  uniqueHash.findKey(group->m_options[j]->m_longName.data()))
	continue;
      uniqueHash.add(group->m_options[j]->m_shortName);
      uniqueHash.add(group->m_options[j]->m_longName);
      ZvCheckOpt(group->m_options[j]->m_shortName,
		 group->m_options[j]->m_longName,
		 group->m_options[j]->m_optName);
    }
  }
  m_syntax << "help { type flag }\n}"; // trailing bracket to close namespace
  m_usage = m_data.toGNUString(m_name); // options sorted as side effect
  m_data.clean();
}

#undef ZvUsageShort
#undef ZvUsageLong
#undef ZvCheckOpt
