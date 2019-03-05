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


#include "MainWindowModel.h"
#include "factories/NetworkManagerFactory.h"
#include "network_component/NetworkManager.h"
#include "distributors/DataDistributor.h"
#include "factories/DataDistributorFactory.h"
#include "QDebug"

MainWindowModel::MainWindowModel():
    m_dataDistributor(nullptr),
    m_networkManager(nullptr)
{
    // not initilization list only for readability
    const unsigned int l_dataDistributorType = DataDistributorFactory::DATA_DISTRIBUTOR_TYPE::Q_THREAD_IMPL;
    m_dataDistributor = DataDistributorFactory::getInstance().getController(l_dataDistributorType);

    const unsigned int l_networkManageType = NetworkManagerFactory::NETWORK_MANAGER_TYPE::Q_THREAD_IMPL;
    m_networkManager = NetworkManagerFactory::getInstance().getNetworkManager(l_networkManageType, m_dataDistributor);
}


MainWindowModel::~MainWindowModel()
{
    qDebug() << "    ~MainWindowModel - Begin";
    delete m_networkManager;
    m_networkManager = nullptr;

    delete m_dataDistributor;
    m_dataDistributor = nullptr;
    qDebug() << "    ~MainWindowModel - End";
}


uintptr_t MainWindowModel::connect()
{
    return m_networkManager->connect();
}

uintptr_t MainWindowModel::disconnect()
{
    return m_networkManager->disconnect();
}

DataDistributor* MainWindowModel::getDataDistributor() const noexcept
{
    return m_dataDistributor;
}


