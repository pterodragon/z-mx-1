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

struct MxMDAPI MxMDCmd {
  typedef ZmRBTree<ZtString,
	    ZmRBTreeVal<const ZtArray<ZtString> *,
	      ZmRBTreeLock<ZmNoLock> > > CmdArgs_;
  struct CmdArgs : public CmdArgs_ {
    inline ZuString get(ZuString arg) const {
      const ZtArray<ZtString> *values = findVal(arg);
      if (!values) return ZtString();
      return (*values)[0];
    }
    inline const ZtArray<ZtString> *getMultiple(ZuString arg) const {
      return findVal(arg);
    }
  };

  typedef ZmFn<const CmdArgs &, ZtArray<char> &> CmdFn;
  struct CmdUsage { };

  static MxMDCmd *instance(MxMDLib *md);

  // add command
  void addCmd(ZuString cmd, ZuString syntax, CmdFn fn,
      ZuString brief, ZuString usage);

  // single security / order book lookup
  static ZtString lookupSyntax();
  static ZtString lookupOptions();
  static void lookupSecurity(
      MxMDLib *md, const MxMDCmd::CmdArgs &args, unsigned index,
      bool secRequired, ZmFn<MxMDSecurity *> fn);
  static void lookupOrderBook(
      MxMDLib *md, const MxMDCmd::CmdArgs &args, unsigned index,
      bool secRequired, bool obRequired,
      ZmFn<MxMDSecurity *, MxMDOrderBook *> fn);
};

#endif /* MxMDCmd_HPP */
