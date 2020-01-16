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

// command line host program

#ifndef ZCmd_HPP
#define ZCmd_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <zlib/ZrlLib.hpp>
#endif

#include <stdio.h>

#include <zlib/ZuInt.hpp>
#include <zlib/ZuArray.hpp>

#include <zlib/ZmPolymorph.hpp>
#include <zlib/ZmRef.hpp>

#include <zlib/Zfb.hpp>

#include <zlib/ZvCmdHost.hpp>

struct ZCmdPlugin : public ZmPolymorph {
  virtual void final() = 0;
  virtual int processApp(ZuArray<const uint8_t> data) = 0;
  virtual int processTel(const ZvTelemetry::fbs::Telemetry *) = 0;
};

struct ZCmdHost : public ZvCmdHost {
  using AppFn = ZmFn<ZuArray<const uint8_t> >;

  virtual void plugin(ZmRef<ZCmdPlugin>) = 0;
  virtual void sendApp(Zfb::IOBuilder &) = 0;
  virtual int executed(int code, FILE *file, ZuString out) = 0;
};

// loadable module must export void ZCmd_plugin(ZCmdHost *)
extern "C" {
  typedef void (*ZCmdInitFn)(ZCmdHost *host);
}

#endif /* ZCmd_HPP */
