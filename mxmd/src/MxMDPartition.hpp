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

#ifndef MxMDPartition_HPP
#define MxMDPartition_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

struct MxMDPartition {
  MxID		id;
  ZiIP		tcpIP, tcpIP2;
  uint16_t	tcpPort, tcpPort2;
  MxIDString	tcpUsername, tcpPassword;
  ZiIP		udpIP, udpIP2;
  uint16_t	udpPort, udpPort2;
  ZiIP		resendIP, resendIP2;
  uint16_t	resendPort, resendPort2;
};

class MxMDPartitionCSV : public ZvCSV, public MxMDCSV<MxMDPartitionCSV> {
public:
  typedef MxMDPartition Data;
  typedef ZuPOD<Data> POD;

  template <typename App = MxMDCSVApp>
  MxMDPartitionCSV(App *app = 0) {
    new ((m_pod = new POD())->ptr()) Data{};
#ifdef Offset
#undef Offset
#endif
#define Offset(x) offsetof(Data, x)
    add(new MxIDCol("id", Offset(id)));
    add(new MxIPCol("tcpIP", offsetof(Partition, tcpIP)));
    add(new MxPortCol("tcpPort", offsetof(Partition, tcpPort)));
    add(new MxIPCol("tcpIP2", offsetof(Partition, tcpIP2)));
    add(new MxPortCol("tcpPort2", offsetof(Partition, tcpPort2)));
    add(new MxIDStrCol("tcpUsername", Offset(tcpUsername)));
    add(new MxIDStrCol("tcpPassword", Offset(tcpPassword)));
    add(new MxIPCol("udpIP", offsetof(Partition, udpIP)));
    add(new MxPortCol("udpPort", offsetof(Partition, udpPort)));
    add(new MxIPCol("udpIP2", offsetof(Partition, udpIP2)));
    add(new MxPortCol("udpPort2", offsetof(Partition, udpPort2)));
    add(new MxIPCol("resendIP", offsetof(Partition, resendIP)));
    add(new MxPortCol("resendPort", offsetof(Partition, resendPort)));
    add(new MxIPCol("resendIP2", offsetof(Partition, resendIP2)));
    add(new MxPortCol("resendPort2", offsetof(Partition, resendPort2)));
#undef Offset
  }

  void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

  template <typename File>
  void read(File &&file, ZvCSVReadFn fn) {
    ZvCSV::readFile(ZuFwd<File>(file),
	ZvCSVAllocFn::Member<&MxMDPartitionCSV::alloc>::fn(this), fn);
  }

  ZuInline POD *pod() { return m_pod.ptr(); }
  ZuInline Data *ptr() { return m_pod->ptr(); }

private:
  ZuRef<POD>	m_pod;
};

#endif /* MxMDPartition_HPP */
