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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <cstdint> // for uintptr_t
#include "QString"
// based on
// https://stackoverflow.com/questions/318064/how-do-you-declare-an-interface-in-c

class DataDistributor;
class StatusBarWidget;

class NetworkManager
{
public:
    NetworkManager(DataDistributor* a_dataDistributor);
    virtual ~ NetworkManager();
    // Stop the compiler generating methods of copy the object
    NetworkManager(NetworkManager const& copy);            // Not Implemented
    NetworkManager& operator=(NetworkManager const& copy); // Not Implemented

    // interface
    enum STATE {CONNECTED, CONNECTING, DISCONNECTING, DISCONNECTED};
    inline static const QString STATE_CONNECTED     = "Connected";
    inline static const QString STATE_CONNECTING    = "Conntecting";
    inline static const QString STATE_DISCONNECTING = "Disconnecting";
    inline static const QString STATE_DISCONNECTED  = "Disconnected";
    inline static const QString STATE_UNKNOWN       = "Unknown";

    virtual uintptr_t connect(StatusBarWidget& a_statusBar) = 0;
    virtual uintptr_t disconnect()                          = 0;
    virtual void setIP(const QString& a_ip)    noexcept     = 0;
    virtual const QString& getIP()       const noexcept     = 0;
    virtual void setPort(const QString& a_ip)  noexcept     = 0;
    virtual const QString& getPort()     const noexcept     = 0;

    static const QString& stateToString(const unsigned int a_state) noexcept;

protected:
    DataDistributor* m_dataDistributor;
};

#endif // NETWORKMANAGER_H
