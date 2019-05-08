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



#include "src/network_component/NetworkManagerQThreadImpl.h"
#include "src/network_component/ConnectionQThread.h"
#include "QDebug"
#include "ZmSemaphore.hpp"

NetworkManagerQThreadImpl::NetworkManagerQThreadImpl(DataDistributor* a_dataDistributor):
    NetworkManager(a_dataDistributor),
    m_disconnectSemaphore(new ZmSemaphore),
    m_connectionThread(nullptr),
    m_state(STATE::DISCONNECTED),
    m_ip(new QString("127.0.0.1")),
    m_port(new QString("19300"))
{

}


NetworkManagerQThreadImpl::~NetworkManagerQThreadImpl()
{
    qInfo() << "        ~NetworkManagerQThreadImpl() - begin";
    // If user close application while connection thread is still running,
    // terminate thread first
    if (m_connectionThread && m_connectionThread->isRunning())
    {
        qWarning() << "destrcutor called while thread still running";
        disconnect();
        delete m_connectionThread;
        m_connectionThread = nullptr;
    }

    // make sure to delete only after thread was deleted
    delete m_disconnectSemaphore;
    m_disconnectSemaphore = nullptr;

    delete m_ip;
    m_ip = nullptr;

    delete m_port;
    m_port = nullptr;
    qInfo() << "        ~NetworkManagerQThreadImpl() - end";
}


uintptr_t NetworkManagerQThreadImpl::connect(StatusBarWidget& a_statusBar)
{
    if (!m_connectionThread)
    {
        // creating the thread, the thread is not running until call to start()
        m_connectionThread = new ConnectionQThread(m_disconnectSemaphore,
                                                   m_dataDistributor,
                                                   *m_ip,
                                                   *m_port,
                                                   a_statusBar);
    }

    m_connectionThread->start();

    return 1;
}

uintptr_t NetworkManagerQThreadImpl::disconnect()
{
    if (!m_connectionThread) {
        qWarning() << "disconnect called while thread was not constructed! returning..";
        return 0;
    }

    if (m_connectionThread->getState() != STATE::CONNECTED) {
        qWarning() << "disconnect called while thread is connected, returning..";
        return 0;
    }

    m_disconnectSemaphore->post();

    while (m_connectionThread->isRunning())
    {
        qInfo() << "m_connectionThread has not finished disconnecting sequence, sleeping 0.1 second, thread state is: " << m_connectionThread->getState();

        usleep(100000); // sleep for 0.1 seconds
    }

    delete m_connectionThread;
    m_connectionThread = nullptr;

    return 1;
}


void NetworkManagerQThreadImpl::setIP(const QString& a_ip)     noexcept
{
    *m_ip = a_ip;
}


void NetworkManagerQThreadImpl::setPort(const QString& a_port)   noexcept
{
    *m_port = a_port;
}

const QString& NetworkManagerQThreadImpl::getIP() const noexcept
{
    return *m_ip;
}

const QString& NetworkManagerQThreadImpl::getPort() const noexcept
{
    return *m_port;
}




