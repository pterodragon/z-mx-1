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

int ZvCmdMsg::redirectIn(ZmRef<ZeEvent> *e)
{
  const auto &redirect = ZtStaticRegexUTF8("\\s*<\\s*");
  ZtRegex::Captures c;
  unsigned pos = 0, n = 0;
  ZtString cmd = ZuMv(m_cmd);
  if (n = redirect.m(cmd, c, pos)) {
    m_cmd = c[0];
    ZiFile f;
    ZeError e_;
    int r = f.open(c[2], ZiFile::ReadOnly | ZiFile::GC, 0777, &e_);
    if (r == Zi::OK) {
      m_data.length(f.size());
      if (f.read(m_data.data(), m_data.length()) != (int)m_data.length())
	m_data.null();
    } else {
      if (e) *e = ZeEVENT(Error, ([file = ZtString(c[2]), e = e_]
	  (const ZeEvent &, ZmStream &s) { s << file << ": " << e; }));
    }
    return r;
  } else {
    m_cmd = ZuMv(cmd);
    return Zi::OK;
  }
}

int ZvCmdMsg::redirectOut(ZiFile &f, ZmRef<ZeEvent> *e)
{
  const auto &redirect = ZtStaticRegexUTF8("\\s*>\\s*");
  ZtRegex::Captures c;
  unsigned pos = 0, n = 0;
  ZtString cmd = ZuMv(m_cmd);
  if (n = redirect.m(cmd, c, pos)) {
    m_cmd = c[0];
    ZeError e_;
    int r = f.open(c[2],
	ZiFile::Create | ZiFile::WriteOnly | ZiFile::GC, 0777, &e_);
    if (r != Zi::OK) {
      if (e) *e = ZeEVENT(Error, ([file = ZtString(c[2]), e = e_]
	  (const ZeEvent &, ZmStream &s) { s << file << ": " << e; }));
    }
    return r;
  } else {
    m_cmd = ZuMv(cmd);
    return Zi::OK;
  }
}

void ZvCmdMsg::send(ZiConnection *cxn)
{
  m_hdr.cmdLen = m_cmd.length();
  m_hdr.dataLen = m_data.length();
  cxn->send(ZiIOFn{[](ZvCmdMsg *msg, ZiIOContext &io) {
    io.init(ZiIOFn{[](ZvCmdMsg *msg, ZiIOContext &io) {
      if (ZuUnlikely((io.offset += io.length) < io.size)) return;
      if (msg->m_cmd)
	msg->sendCmd(io);
      else if (msg->m_data)
	msg->sendData(io);
    }, io.fn.mvObject<ZvCmdMsg>()}, &msg->m_hdr, sizeof(Hdr), 0); 
  }, ZmMkRef(this)});
}

void ZvCmdMsg::sendCmd(ZiIOContext &io)
{
  io.init(ZiIOFn{[](ZvCmdMsg *msg, ZiIOContext &io) {
    if (ZuUnlikely((io.offset += io.length) < io.size)) return;
    if (msg->m_data) msg->sendData(io);
  }, io.fn.mvObject<ZvCmdMsg>()},
  m_cmd.data(), m_cmd.length(), 0);
}

void ZvCmdMsg::sendData(ZiIOContext &io)
{
  io.init(ZiIOFn{[](ZvCmdMsg *msg, ZiIOContext &io) {
    if (ZuUnlikely((io.offset += io.length) < io.size)) return;
  }, io.fn.mvObject<ZvCmdMsg>()},
  m_data.data(), m_data.length(), 0);
}

// client

void ZvCmdClientCxn::connected_()
{
  mgr()->connected(this);
}
void ZvCmdClientCxn::disconnected_()
{
  cancelTimeout();
  mgr()->disconnected(this);
}
void ZvCmdClientCxn::rcvd(ZmRef<ZvCmdMsg> msg)
{
  cancelTimeout();
  mgr()->rcvd(ZuMv(msg));
}
void ZvCmdClientCxn::sent()
{
  scheduleTimeout();
}

void ZvCmdClient::init(ZiMultiplex *mx, unsigned timeout)
{
  m_mx = mx;
  m_timeout = timeout;
}

void ZvCmdClient::init(ZiMultiplex *mx, ZvCf *cf)
{
  m_mx = mx;

  m_remoteIP = cf->get("remoteIP", false, "127.0.0.1");
  m_remotePort = cf->getInt("remotePort", 1, (1<<16) - 1, true);
  m_localIP = cf->get("localIP", false, "0.0.0.0");
  m_localPort = cf->getInt("localPort", 1, (1<<16) - 1, false, 0);
  m_reconnFreq = cf->getInt("reconnFreq", 0, 3600, false, 0);
  m_timeout = cf->getInt("timeout", 0, 3600, false, 0);
}

void ZvCmdClient::final()
{
}

void ZvCmdClient::connect(ZiIP ip, int port)
{
  m_mx->connect(
      ZiConnectFn{[](ZvCmdClient *client, const ZiCxnInfo &info) -> uintptr_t {
	  return (uintptr_t)(new ZvCmdClientCxn(client, info));
	}, this},
      ZiFailFn{[](ZvCmdClient *client, bool transient) {
	  client->failed(transient);
	}, this},
      ZiIP(), 0, ip, port);
}

void ZvCmdClient::connect()
{
  if (!m_remoteIP || !m_remotePort) {
    error(ZeEVENT(Error, ([](const ZeEvent &, ZmStream &s) {
      s << "no remote address configured"; })));
    return;
  }
  m_mx->connect(
      ZiConnectFn{[](ZvCmdClient *client, const ZiCxnInfo &info) -> uintptr_t {
	  return (uintptr_t)(new ZvCmdClientCxn(client, info));
	}, this},
      ZiFailFn{[](ZvCmdClient *client, bool transient) {
	  client->failed(transient);
	}, this},
      m_localIP, m_localPort, m_remoteIP, m_remotePort);
}

void ZvCmdClient::failed(bool transient)
{
  if (transient && m_reconnFreq > 0)
    m_mx->add(ZmFn<>{[](ZvCmdClient *client) { client->connect(); }, this},
	ZmTimeNow(m_reconnFreq), &m_reconnTimer);
  else
    error(ZeEVENT(Error, ([](const ZeEvent &, ZmStream &s) {
      s << "could not connect"; })));
}

void ZvCmdClient::disconnect()
{
  m_mx->del(&m_reconnTimer);

  ZmRef<ZvCmdClientCxn> cxn;

  {
    Guard guard(m_lock);
    cxn = ZuMv(m_cxn);
    m_cxn = nullptr;
    m_busy = false;
  }

  if (cxn) cxn->disconnect();
}

void ZvCmdClient::connected(ZvCmdClientCxn *cxn)
{
  {
    Guard guard(m_lock);
    m_cxn = cxn;
    if (m_queue.count()) {
      m_busy = true;
      ZmRef<ZvCmdMsg> msg = m_queue.shift();
      msg->send(m_cxn);
    } else
      m_busy = false;
  }
  connected();
}

void ZvCmdClient::disconnected(ZvCmdClientCxn *cxn)
{
  {
    Guard guard(m_lock);
    if (m_cxn == cxn) m_cxn = nullptr;
    m_busy = false;
  }
  disconnected();
}

void ZvCmdClient::send(ZmRef<ZvCmdMsg> msg)
{
  Guard guard(m_lock);
  msg->seqNo(m_seqNo++);
  if (m_cxn && !m_busy) {
    m_busy = true;
    msg->send(m_cxn);
  } else
    m_queue.push(msg);
}

void ZvCmdClient::rcvd(ZmRef<ZvCmdMsg> msg)
{
  process(msg);
  Guard guard(m_lock);
  if (m_cxn && m_queue.count()) {
    ZmRef<ZvCmdMsg> msg = m_queue.shift();
    msg->send(m_cxn);
  } else
    m_busy = false;
}

// server

void ZvCmdServerCxn::connected_()
{
  scheduleTimeout();
}
void ZvCmdServerCxn::disconnected_()
{
  cancelTimeout();
}
void ZvCmdServerCxn::rcvd(ZmRef<ZvCmdMsg> msg)
{
  scheduleTimeout();
  mgr()->rcvd(this, ZuMv(msg));
}
void ZvCmdServerCxn::sent()
{
  scheduleTimeout();
}

void ZvCmdServer::init(ZiMultiplex *mx, ZvCf *cf)
{
  m_mx = mx;

  m_rebindFreq = cf->getInt("rebindFreq", 0, 3600, false, 0);
  m_ip = cf->get("localIP", false, "127.0.0.1");
  m_port = cf->getInt("localPort", 1, (1<<16) - 1, false, 19400);
  m_nAccepts = cf->getInt("nAccepts", 1, 1024, false, 8);
  m_timeout = cf->getInt("timeout", 0, 3600, false, 0);

  m_listening = false;

  m_syntax = new ZvCf();

  addCmd("help", "", ZvCmdFn::Member<&ZvCmdServer::help>::fn(this),
      "list commands", "usage: help [COMMAND]");
}

void ZvCmdServer::final()
{
  m_syntax = nullptr;
  m_cmds.clean();
}

void ZvCmdServer::start()
{
  listen();
  m_started.wait();
}

void ZvCmdServer::stop()
{
  m_mx->del(&m_rebindTimer);
  m_mx->stopListening(m_ip, m_port);
  m_listening = false;
}

void ZvCmdServer::listen()
{
  m_mx->listen(
      ZiListenFn{[](ZvCmdServer *server, const ZiListenInfo &) {
	  server->listening();
	}, this},
      ZiFailFn{[](ZvCmdServer *server, bool transient) {
	  server->failed(transient);
	}, this},
      ZiConnectFn{[](ZvCmdServer *server, const ZiCxnInfo &info) -> uintptr_t {
	  return (uintptr_t)(new ZvCmdServerCxn(server, info));
	}, this},
      m_ip, m_port, m_nAccepts);
}

void ZvCmdServer::listening()
{
  m_listening = true;
  m_started.post();
}

void ZvCmdServer::failed(bool transient)
{
  if (transient && m_rebindFreq > 0)
    m_mx->add(ZmFn<>{[](ZvCmdServer *server) { server->listen(); }, this},
	ZmTimeNow(m_rebindFreq), &m_rebindTimer);
  else {
    m_listening = false;
    error(ZeEVENT(Error, ([](const ZeEvent &, ZmStream &s) {
      s << "ZvCmdServer::listen() - could not listen"; })));
    m_started.post();
  }
}

void ZvCmdServer::rcvd(ZvCmdServerCxn *cxn, ZmRef<ZvCmdMsg> in)
{
  ZmRef<ZvCf> cf = new ZvCf();
  ZtString name;
  Cmds::NodeRef cmd;
  uint32_t seqNo = in->seqNo();
  try {
    cf->fromCLI(m_syntax, in->cmd());
    name = cf->get("0");
    cmd = m_cmds.find(name);
    if (!cmd) throw ZtString("unknown command");
    if (cf->getInt("help", 0, 1, false, 0)) {
      ZmRef<ZvCmdMsg> out = new ZvCmdMsg(in->seqNo());
      out->cmd() << cmd->val().usage << '\n';
      out->send(cxn);
    } else {
      ZmRef<ZvCmdMsg> out;
      (cmd->val().fn)(cxn, cf, ZuMv(in), out);
      if (!out) out = new ZvCmdMsg(seqNo);
      out->send(cxn);
    }
  } catch (const ZvCmdUsage &) {
    ZmRef<ZvCmdMsg> out = new ZvCmdMsg(seqNo, -1);
    out->cmd() << cmd->val().usage << '\n';
    out->send(cxn);
  } catch (const ZvError &e) {
    ZmRef<ZvCmdMsg> out = new ZvCmdMsg(seqNo, -1);
    out->cmd() << e << '\n';
    out->send(cxn);
  } catch (const ZeError &e) {
    ZmRef<ZvCmdMsg> out = new ZvCmdMsg(seqNo, -1);
    out->cmd() << '"' << name << "\": " << e << '\n';
    out->send(cxn);
  } catch (const ZtString &s) {
    ZmRef<ZvCmdMsg> out = new ZvCmdMsg(seqNo, -1);
    out->cmd() << '"' << name << "\": " << s << '\n';
    out->send(cxn);
  } catch (const ZtArray<char> &s) {
    ZmRef<ZvCmdMsg> out = new ZvCmdMsg(seqNo, -1);
    out->cmd() << '"' << name << "\": " << s << '\n';
    out->send(cxn);
  } catch (...) {
    ZmRef<ZvCmdMsg> out = new ZvCmdMsg(seqNo, -1);
    out->cmd() << '"' << name << "\": unknown exception\n";
    out->send(cxn);
  }
}

void ZvCmdServer::addCmd(ZuString name, ZuString syntax,
    ZvCmdFn fn, ZtString brief, ZtString usage)
{
  {
    ZmRef<ZvCf> cf = m_syntax->subset(name, true);
    cf->fromString(syntax, false);
    cf->set("help:type", "flag");
  }
  if (Cmds::NodeRef cmd = m_cmds.find(name))
    cmd->val() = CmdData{ZuMv(fn), ZuMv(brief), ZuMv(usage)};
  else
    m_cmds.add(name, CmdData{ZuMv(fn), ZuMv(brief), ZuMv(usage)});
}

void ZvCmdServer::help(
    ZvCmdServerCxn *, ZvCf *args, ZmRef<ZvCmdMsg> in, ZmRef<ZvCmdMsg> &out)
{
  int argc = ZuBox<int>(args->get("#"));
  if (argc > 2) throw ZvCmdUsage();
  out = new ZvCmdMsg(in->seqNo());
  if (ZuUnlikely(argc == 2)) {
    Cmds::NodeRef cmd = m_cmds.find(args->get("1"));
    if (!cmd) throw ZvCmdUsage();
    out->cmd() << cmd->val().usage << '\n';
    return;
  }
  auto &help = out->cmd();
  help.size(m_cmds.count() * 80 + 40);
  help << "commands:\n\n";
  {
    auto i = m_cmds.readIterator();
    while (Cmds::NodeRef cmd = i.iterate())
      help << cmd->key() << " -- " << cmd->val().brief << '\n';
  }
}
