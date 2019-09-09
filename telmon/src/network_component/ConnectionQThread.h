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


#ifndef CONNECTIONQTHREAD_H
#define CONNECTIONQTHREAD_H

#include <QThread>
class ZmSemaphore;
class QMutex;
class DataDistributor;
class StatusBarWidget;

class ConnectionQThread: public QThread {
    Q_OBJECT

public:
    ConnectionQThread(ZmSemaphore* a_disconnectSemaphore,
                      DataDistributor* a_dataDistributor,
                      QString& a_ip,
                      QString& a_port,
                      StatusBarWidget& a_statusBar);
    ~ConnectionQThread();

    void run();
    unsigned int getState() const noexcept;

protected:
    unsigned int m_state;
    void setState(unsigned int a_state) noexcept;

private:
    ZmSemaphore* m_disconnectSemaphore;
    QMutex* m_threadStateMutex;
    void waitForDisconnectSignal() noexcept;
    DataDistributor* m_dataDistributor;

    QString m_ip;
    QString m_port;

    StatusBarWidget& m_statusBar;
};

#endif // CONNECTIONQTHREAD_H




