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

class MxMDCore;

class MxMDAPI MxMDCmd : public ZvCmdServer {
public:
  MxMDCmd() { }
  ~MxMDCmd() { }

  ZuInline MxMDCore *core() const { return m_core; }

  void init(MxMDCore *core, ZvCf *cf);
  void final();

  void start();
  void stop();

  typedef ZmRBTree<ZtString,
	    ZmRBTreeVal<const ZtArray<ZtString> *,
	      ZmRBTreeLock<ZmNoLock> > > Args_;
  struct Args : public Args_ {
    inline ZuString get(ZuString arg) const {
      const ZtArray<ZtString> *values = findVal(arg);
      if (!values) return ZtString();
      return (*values)[0];
    }
    inline const ZtArray<ZtString> *getMultiple(ZuString arg) const {
      return findVal(arg);
    }
  };

  typedef ZmFn<const Args &, ZtArray<char> &> Fn;

  struct Usage { };

  // add command
  void addCmd(ZuString cmd, ZuString syntax, Fn fn,
      ZtString brief, ZtString usage);

  // built-in help command
  void help(const Args &args, ZtArray<char> &result);

  // CLI time format (using local timezone)
  typedef ZuBoxFmt<ZuBox<unsigned>, ZuFmt::Right<6> > TimeFmt;
  inline TimeFmt timeFmt(MxDateTime t) {
    return ZuBox<unsigned>((t + m_tzOffset).hhmmss()).fmt(ZuFmt::Right<6>());
  }

private:
  void rcvd(ZvCmdLine *, const ZvInvocation &, ZvAnswer &);

  typedef ZmRBTree<ZtString,
	    ZmRBTreeVal<ZuTuple<Fn, ZtString, ZtString>,
	      ZmRBTreeLock<ZmNoLock> > > Cmds;

  MxMDCore	*m_core = 0;
  ZmRef<ZvCf>	m_syntax;
  Cmds		m_cmds;
  int		m_tzOffset = 0;
};

#endif /* MxMDCmd_HPP */
