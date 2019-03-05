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

class ConnectionQThread: public QThread {
    Q_OBJECT

public:
    ConnectionQThread(ZmSemaphore* a_disconnectSemaphore, DataDistributor* a_dataDistributor);
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

//    ZmSemaphore *m_semaphore;
//    MainWindow *m_mainWindow;
//    ZvCf *m_configurationFile;
//    MxMultiplex *m_mx;
//    ClientApp *m_app;
//    QWaitCondition* m_connectSequenceSwitch;
//    QMutex *m_connectSequenceMutex;
};

#endif // CONNECTIONQTHREAD_H




