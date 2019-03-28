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



#ifndef NETWORKMANAGERQTHREADIMPL_H
#define NETWORKMANAGERQTHREADIMPL_H

#include "NetworkManager.h"


class ConnectionQThread;
class ZmSemaphore;

// In case inherting from this class, remove the "final" if necessary;
class NetworkManagerQThreadImpl : public NetworkManager
{
public:
    NetworkManagerQThreadImpl(DataDistributor* a_dataDistributor);
    virtual ~NetworkManagerQThreadImpl()                 override final;

    virtual uintptr_t connect()                          override final;
    virtual uintptr_t disconnect()                       override final;
    virtual uintptr_t setConfiguration()                 override final;
    virtual uintptr_t getConfiguration() const noexcept  override final;
    virtual uintptr_t getState()         const noexcept  override final;
    virtual void setIP(const QString& a_ip)       noexcept  override final;
    virtual void setPort(const QString& a_port)   noexcept  override final;

protected:
    virtual void setState(const unsigned int a_state)     override final;

private:
    ZmSemaphore* m_disconnectSemaphore;
    ConnectionQThread* m_connectionThread;
    unsigned int m_state;

    QString* m_ip;
    QString* m_port;
};


#endif // NETWORKMANAGERQTHREADIMPL_H
