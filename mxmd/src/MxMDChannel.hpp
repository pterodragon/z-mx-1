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

// MxMD library internal API

#ifndef MxMDChannel_HPP
#define MxMDChannel_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <ZuInt.hpp>
#include <ZuBox.hpp>

#include <ZiIP.hpp>

#include <MxBase.hpp>

#include <MxMDCSV.hpp>

struct MxMDChannel {
  MxID			id;
  MxBool		enabled;
  ZuBox_1(int)		shardID;
  ZiIP			tcpIP, tcpIP2;
  ZuBox0(uint16_t)	tcpPort, tcpPort2;
  MxIDString		tcpUsername, tcpPassword;
  ZiIP			udpIP, udpIP2;
  ZuBox0(uint16_t)	udpPort, udpPort2;
  ZiIP			resendIP, resendIP2;
  ZuBox0(uint16_t)	resendPort, resendPort2;
};

class MxMDChannelCSV : public ZvCSV, public MxCSV<MxMDChannelCSV> {
public:
  typedef MxMDChannel Data;
  typedef ZuPOD<Data> POD;

  template <typename App = MxCSVApp>
  MxMDChannelCSV(App *app = 0) {
    new ((m_pod = new POD())->ptr()) Data{};
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new MxIDCol("id", Offset(id)));
    add(new MxBoolCol("enabled", Offset(enabled), -1, 1));
    add(new MxIntCol("shardID", Offset(shardID)));
    add(new MxIPCol("tcpIP", Offset(tcpIP)));
    add(new MxPortCol("tcpPort", Offset(tcpPort)));
    add(new MxIPCol("tcpIP2", Offset(tcpIP2)));
    add(new MxPortCol("tcpPort2", Offset(tcpPort2)));
    add(new MxIDStrCol("tcpUsername", Offset(tcpUsername)));
    add(new MxIDStrCol("tcpPassword", Offset(tcpPassword)));
    add(new MxIPCol("udpIP", Offset(udpIP)));
    add(new MxPortCol("udpPort", Offset(udpPort)));
    add(new MxIPCol("udpIP2", Offset(udpIP2)));
    add(new MxPortCol("udpPort2", Offset(udpPort2)));
    add(new MxIPCol("resendIP", Offset(resendIP)));
    add(new MxPortCol("resendPort", Offset(resendPort)));
    add(new MxIPCol("resendIP2", Offset(resendIP2)));
    add(new MxPortCol("resendPort2", Offset(resendPort2)));
#undef Offset
  }

  void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

  template <typename File>
  void read(File &&file, ZvCSVReadFn fn) {
    ZvCSV::readFile(ZuFwd<File>(file),
	ZvCSVAllocFn::Member<&MxMDChannelCSV::alloc>::fn(this), fn);
  }

  ZuInline POD *pod() { return m_pod.ptr(); }
  ZuInline Data *ptr() { return m_pod->ptr(); }

private:
  ZuRef<POD>	m_pod;
};

#endif /* MxMDChannel_HPP */
