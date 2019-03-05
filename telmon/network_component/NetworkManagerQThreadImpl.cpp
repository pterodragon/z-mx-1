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



#include "NetworkManagerQThreadImpl.h"
#include "ConnectionQThread.h"
#include "QDebug"
#include "ZmSemaphore.hpp"

NetworkManagerQThreadImpl::NetworkManagerQThreadImpl(DataDistributor* a_dataDistributor):
    NetworkManager(a_dataDistributor),
    m_disconnectSemaphore(new ZmSemaphore),
    m_connectionThread(nullptr),
    m_state(STATE::DISCONNECTED)
{

}


NetworkManagerQThreadImpl::~NetworkManagerQThreadImpl()
{
    qDebug() << "        ~NetworkManagerQThreadImpl() - begin";
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
    qDebug() << "        ~NetworkManagerQThreadImpl() - end";
}


uintptr_t NetworkManagerQThreadImpl::connect()
{
    if (!m_connectionThread)
    {
        // creating the thread, the thread is not running until call to start()
        m_connectionThread = new ConnectionQThread(m_disconnectSemaphore, m_dataDistributor);
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
        qWarning() << "disconnect called while thread is not running, returning..";
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

uintptr_t NetworkManagerQThreadImpl::setConfiguration()
{
    //todo
    return 0;
}


uintptr_t NetworkManagerQThreadImpl::getConfiguration() const noexcept
{
    //todo
    return 0;
}


uintptr_t NetworkManagerQThreadImpl::getState() const noexcept
{
    return m_state;
}


void NetworkManagerQThreadImpl::setState(const unsigned int a_state)
{
    // todo -- add check for a_state verification... must be in ENUM range
    m_state = a_state;
}


