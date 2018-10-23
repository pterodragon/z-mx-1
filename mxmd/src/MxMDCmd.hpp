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

// MxMD library command extension (interactive + batch scripting)

#ifndef MxMDCmd_HPP
#define MxMDCmd_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <MxMD.hpp>

#include <ZmFn.hpp>
#include <ZmRBTree.hpp>

#include <ZtArray.hpp>
#include <ZtString.hpp>

#include <ZvCmd.hpp>

#include <MxMD.hpp>

class MxMDCore;

class MxMDAPI MxMDCmd : public ZvCmdServer {
public:
  MxMDCmd() { }
  ~MxMDCmd() { }

  void init(MxMDCore *core, ZvCf *cf);
  void final();

  typedef MxMDCmdArgs Args;
  typedef MxMDCmdFn Fn;
  typedef MxMDCmdUsage Usage;

  // add command
  void addCmd(ZuString name, ZuString syntax,
      Fn fn, ZtString brief, ZtString usage);

  // built-in help command
  void help(const Args &args, ZtArray<char> &result);

private:
  void rcvd(ZvCmdLine *, const ZvInvocation &, ZvAnswer &);

  typedef ZmRBTree<ZtString,
	    ZmRBTreeVal<ZuTuple<Fn, ZtString, ZtString>,
	      ZmRBTreeLock<ZmNoLock> > > Cmds;

  ZmRef<ZvCf>	m_syntax;
  Cmds		m_cmds;
};

#endif /* MxMDCmd_HPP */
