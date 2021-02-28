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

// ZvCmd locally hosted commands

#ifndef ZvCmdHost_HPP
#define ZvCmdHost_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZuString.hpp>

#include <zlib/ZmPolymorph.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmRBTree.hpp>

#include <zlib/Zfb.hpp>

#include <zlib/ZvCf.hpp>

class ZvCmdHost;

struct ZvCmdContext {
  ZvCmdHost		*app_ = nullptr;	
  void			*link_ = nullptr;	// opaque to plugin
  void			*user_ = nullptr;	// ''
  ZmRef<ZvCf>		args;
  FILE			*file = nullptr;	// file output
  ZtString		out;			// string output
  int			code = 0;		// result code
  bool			interactive = false;	// true unless scripted

  template <typename T = ZvCmdHost>
  auto app() { return static_cast<T *>(app_); }
  template <typename T> auto link() { return static_cast<T *>(link_); }
  template <typename T> auto user() { return static_cast<T *>(user_); }
};

// command handler (context)
using ZvCmdFn = ZmFn<ZvCmdContext *>;
// can be thrown by command function
struct ZvCmdUsage { };

struct ZvCmdPlugin : public ZmPolymorph {
  virtual void final() = 0;
  virtual int processApp(ZuArray<const uint8_t> data) = 0;
};

class ZvAPI ZvCmdHost {
public:
  void init();
  void final();

  void addCmd(ZuString name,
      ZuString syntax, ZvCmdFn fn, ZtString brief, ZtString usage);

  bool hasCmd(ZuString name);

  int processCmd(ZvCmdContext *, ZuArray<const ZtString> args);

  using AppFn = ZmFn<ZuArray<const uint8_t> >;

  int loadMod(ZuString name, ZtString &out);

  virtual void plugin(ZmRef<ZvCmdPlugin>) { } // register plugin

  virtual int executed(ZvCmdContext *ctx) { return 0; }
  virtual void sendApp(void *link, Zfb::IOBuilder &) { }

private:
  int help(ZvCmdContext *);

private:
  struct CmdData {
    ZvCmdFn	fn;
    ZtString	brief;
    ZtString	usage;
  };
  using Cmds =
    ZmRBTree<ZtString,
      ZmRBTreeVal<CmdData,
	ZmRBTreeUnique<true,
	  ZmRBTreeLock<ZmNoLock> > > >;

  ZmRef<ZvCf>		m_syntax;
  Cmds			m_cmds;
};

// loadable module must export void ZCmd_plugin(ZCmdHost *)
extern "C" {
  typedef void (*ZvCmdInitFn)(ZvCmdHost *host);
}

#endif /* ZvCmdHost_HPP */
