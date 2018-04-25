// -*- mode: c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
// vi: noet ts=8 sw=2

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

// User-space <-> Kernel Generic Netlink Interface
// Common source for user-level and kernel-level, so keep it in c-style

#ifndef zi_netlink_H
#define zi_netlink_H

#ifdef __cplusplus
extern "C" {
#endif

#define ZiGenericNetlinkVersion 1

// nlattr.nla_type values
enum ZiGNLAttr {
  ZiGNLAttr_Unspec = 0,
  ZiGNLAttr_Data,		// Data of any length
  ZiGNLAttr_PCI,		// Major/Minor Opcode
  ZiGNLAttr_N
};
  
// genlmsghdr.cmd values
enum ZiGenericNetlinkCmd {
  ZiGenericNetlinkCmd_Unspec = 0,
  ZiGenericNetlinkCmd_Forward,	// forward msg from user-space to wanic board
  ZiGenericNetlinkCmd_Ack,	// user-space acks going to kernel
  ZiGenericNetlinkCmd_N
};

#ifdef __cplusplus
}
#endif

#endif
