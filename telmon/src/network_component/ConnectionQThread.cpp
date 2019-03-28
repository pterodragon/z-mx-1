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

#include "MxTelMonClient.h"
#include "ConnectionQThread.h"
#include "NetworkManager.h"

#include "QDebug"
#include "QMutex"
#include <QMutexLocker>

ConnectionQThread::ConnectionQThread(ZmSemaphore* a_disconnectSemaphore,
                                     DataDistributor* a_dataDistributor,
                                     QString& a_ip,
                                     QString& a_port):
    m_state(NetworkManager::STATE::DISCONNECTED),
    m_disconnectSemaphore(a_disconnectSemaphore),
    m_threadStateMutex(new QMutex()),
    m_dataDistributor(a_dataDistributor),
    m_ip(a_ip),
    m_port(a_port)
{

}

ConnectionQThread::~ConnectionQThread()
{
    delete m_threadStateMutex;
}

void ConnectionQThread::setState(unsigned int a_state) noexcept
{
    QMutexLocker locker(m_threadStateMutex);
    m_state = a_state;
}


unsigned int ConnectionQThread::getState() const noexcept
{
    QMutexLocker locker(m_threadStateMutex);
    return m_state;
}

void ConnectionQThread::waitForDisconnectSignal() noexcept
{
    m_disconnectSemaphore->wait();
}


void ConnectionQThread::run()
{
    qInfo() << "ConnectionThread::run(), Begin: ";

    setState(NetworkManager::STATE::CONNECTING);

    ZmRef<ZvCf> cf = new ZvCf();
//    cf->fromString(
//                "telemetry {\n"
//                "  ip 127.0.0.1\n"
//                "  port 19300\n"
//                "}\n",
//                false);

    QString l_config =
            "telemetry {\n"
            "  ip " +
            m_ip +
            "\n"
            "  port " +
            m_port +
            "\n"
            "}\n";

    cf->fromString(
                l_config.toLocal8Bit().data(),
                false);

    qDebug() << "Qthread using the following config\n" << l_config;

    ZmRef<MxMultiplex> m_mx = new MxMultiplex("mx", cf->subset("mx", true));

    MxTelMonClient app;
    app.init(m_mx, cf->subset("telemetry", false));
    app.initDataDistributor(m_dataDistributor);
    m_mx->start();
    app.start();

    setState(NetworkManager::STATE::CONNECTED);

    waitForDisconnectSignal();

    setState(NetworkManager::STATE::DISCONNECTING);

    app.stop();
    m_mx->stop(true);
    app.final();

    setState(NetworkManager::STATE::DISCONNECTED);

    qInfo() << "ConnectionThread::run(), End: ";;
}



