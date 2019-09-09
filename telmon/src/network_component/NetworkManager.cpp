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


#include "NetworkManager.h"

NetworkManager::NetworkManager(DataDistributor* a_dataDistributor):
    m_dataDistributor(a_dataDistributor)
{

}


NetworkManager::~NetworkManager()
{
    // Do not delete m_dataDistributor
}


const QString& NetworkManager::stateToString(const unsigned int a_state) noexcept
{
    switch (a_state) {
    case CONNECTED:
        return STATE_CONNECTED;
    case CONNECTING:
        return STATE_CONNECTING;
    case  DISCONNECTING:
        return STATE_DISCONNECTING;
    case DISCONNECTED:
        return STATE_DISCONNECTED;
    default:
        return STATE_UNKNOWN;
    }
}
