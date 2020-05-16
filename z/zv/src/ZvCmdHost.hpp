//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

#ifndef ZvCmdHost_HPP
#define ZvCmdHost_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZuString.hpp>

#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmRBTree.hpp>
#include <zlib/ZmPLock.hpp>

#include <zlib/ZvCf.hpp>

// command handler (context, command, output)
using ZvCmdFn = ZmFn<void *, ZvCf *, ZtString &>;
// can be thrown by command function
struct ZvCmdUsage { };

class ZvCmdHost {
public:
  void init() {
    m_syntax = new ZvCf();
    addCmd("help", "", ZvCmdFn::Member<&ZvCmdHost::help>::fn(this),
	"list commands", "usage: help [COMMAND]");
  }

  void final() {
    m_syntax = nullptr;
    m_cmds.clean();
  }

  void addCmd(ZuString name,
      ZuString syntax, ZvCmdFn fn, ZtString brief, ZtString usage) {
    {
      ZmRef<ZvCf> cf = m_syntax->subset(name, true);
      cf->fromString(syntax, false);
      cf->set("help:type", "flag");
    }
    if (auto cmd = m_cmds.find(name))
      cmd->val() = CmdData{ZuMv(fn), ZuMv(brief), ZuMv(usage)};
    else
      m_cmds.add(name, CmdData{ZuMv(fn), ZuMv(brief), ZuMv(usage)});
  }

  bool hasCmd(ZuString name) { return m_cmds.find(name); }

  int processCmd(void *context, const ZtArray<ZtString> &args, ZtString &out) {
    if (!args) return 0;
    const ZtString &name = args[0];
    typename Cmds::NodeRef cmd;
    try {
      ZmRef<ZvCf> cf = new ZvCf();
      cmd = m_cmds.find(name);
      if (!cmd) throw ZtString("unknown command");
      cf->fromArgs(m_syntax->subset(name, false), args);
      if (cf->getInt("help", 0, 1, false, 0)) {
	out << cmd->val().usage << '\n';
	return 0;
      }
      uint32_t r = (cmd->val().fn)(context, cf, out);
      r &= ~(((uint32_t)1)<<31); // ensure +ve
      return r;
    } catch (const ZvCmdUsage &) {
      out << cmd->val().usage << '\n';
    } catch (const ZvError &e) {
      out << e << '\n';
    } catch (const ZeError &e) {
      out << '"' << name << "\": " << e << '\n';
    } catch (const ZtString &s) {
      out << '"' << name << "\": " << s << '\n';
    } catch (const ZtArray<char> &s) {
      out << '"' << name << "\": " << s << '\n';
    } catch (...) {
      out << '"' << name << "\": unknown exception\n";
    }
    return -1;
  }

private:
  int help(void *, ZvCf *args, ZtString &out) {
    int argc = ZuBox<int>(args->get("#"));
    if (argc > 2) throw ZvCmdUsage();
    if (ZuUnlikely(argc == 2)) {
      auto cmd = m_cmds.find(args->get("1"));
      if (!cmd) {
	out << args->get("1") << ": unknown command\n";
	return 1;
      }
      out << cmd->val().usage << '\n';
      return 0;
    }
    out.size(m_cmds.count() * 80 + 40);
    out << "commands:\n\n";
    {
      auto i = m_cmds.readIterator();
      while (auto cmd = i.iterate())
	out << cmd->key() << " -- " << cmd->val().brief << '\n';
    }
    return 0;
  }

private:
  struct CmdData {
    ZvCmdFn	fn;
    ZtString	brief;
    ZtString	usage;
  };
  using Cmds =
    ZmRBTree<ZtString,
      ZmRBTreeVal<CmdData,
	ZmRBTreeLock<ZmNoLock> > >;

  ZmRef<ZvCf>		m_syntax;
  Cmds			m_cmds;
};

#endif /* ZvCmdHost_HPP */
