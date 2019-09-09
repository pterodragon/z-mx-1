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

#ifndef MAINWINDOWMODEL_H
#define MAINWINDOWMODEL_H

#include <cstdint> // for uintptr_t

class NetworkManager;
class DataDistributor;
class QString;
class StatusBarWidget;

class MainWindowModel {

public:
    MainWindowModel();
    virtual ~MainWindowModel();
    uintptr_t connect(StatusBarWidget& a_statusBar);
    uintptr_t disconnect();

    DataDistributor* getDataDistributor() const noexcept;

    void setIP(const QString& a_ip)               noexcept;
    const QString& getIP()                  const noexcept;

    void setPort(const QString& a_port)           noexcept;
    const QString& getPort()                const noexcept;

    const QString& getInitialStatusBarMsg() const noexcept;

private:
    DataDistributor* m_dataDistributor;
    NetworkManager* m_networkManager;
};

#endif // MAINWINDOWMODEL_H
