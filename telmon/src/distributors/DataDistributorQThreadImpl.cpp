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

#include <MxTelemetry.hpp>
#include "DataDistributorQThreadImpl.h"
#include "QMap"
#include "QVector"
#include "src/subscribers/DataSubscriber.h"
#include "QMutex"

#include "QDebug" // for testing


DataDistributorQThreadImpl::DataDistributorQThreadImpl():
    m_subscribersDB(new QMap<int, QVector<DataSubscriber*>*>),
    m_mutexList(new QList<QMutex*>)
{
    // construct the subscribers according the MxTelemetryTypes
    const int l_mxTelemetryTotalTypes = MxTelemetry::Type::N;
    for (int i = 0; i < l_mxTelemetryTotalTypes; i++)
    {
        m_subscribersDB->insert(i, new QVector<DataSubscriber*>);
    }

    // construct the mutexs according the MxTelemetryTypes
    for (unsigned int i = 0; i < l_mxTelemetryTotalTypes; i++)
    {
        m_mutexList->append(new QMutex());
    }
}


DataDistributorQThreadImpl::~DataDistributorQThreadImpl()
{
    qDebug() << "        ~DataDistributorQThreadImpl() - begin";

    // !! do not clean DataSubscriber inside m_subscribersDB !!
    for (auto i = m_subscribersDB->begin(); i != m_subscribersDB->end() ;i++) { delete i.value(); }
    delete m_subscribersDB;

    //clean all the mutexes
    for (auto i = m_mutexList->begin(); i != m_mutexList->end() ;i++) { delete *i; }
    delete m_mutexList;
    qDebug() << "        ~DataDistributorQThreadImpl() - end";
}


uintptr_t DataDistributorQThreadImpl::subscribeAll(DataSubscriber* a_subscriber)
{
    const int l_mxTelemetryTotalTypes = MxTelemetry::Type::N;
    for (int i = 0; i < l_mxTelemetryTotalTypes; i++)
    {
        subscribe(i, a_subscriber);
    }
    return STATUS::SUCCESS;
}

uintptr_t DataDistributorQThreadImpl::unsubscribeAll(DataSubscriber* a_subscriber)
{
    const int l_mxTelemetryTotalTypes = MxTelemetry::Type::N;
    for (int i = 0; i < l_mxTelemetryTotalTypes; i++)
    {
        unsubscribe(i, a_subscriber);
    }
    return STATUS::SUCCESS;
}


uintptr_t DataDistributorQThreadImpl::subscribe(const int a_mxTelemetryType,
                                                DataSubscriber* a_subscriber)
{
    {
        //get the mutex
        QMutexLocker locker(m_mutexList->at(static_cast<int>(a_mxTelemetryType)));
        //qDebug() << "subscribe acquire mutex:" << static_cast<int>(a_mxTelemetryType);

        //get the vector
        QVector<DataSubscriber*> *l_vector = m_subscribersDB->value(a_mxTelemetryType, nullptr);

        //sanity check
        if (!l_vector)
        {
            qCritical() << "Failed to retreive dats structure for mxTelemetryType"
                        << "is a_mxTelemetryType not in range of MxTelemetry::Type::N ?!"
                        << "MxTelemetry::Type=" <<  a_mxTelemetryType << "subscriber=" << a_subscriber->getName();
            return STATUS::FAILURE;
        }

        //debug
        qDebug() << "subscribed" << a_subscriber->getName() << "to" << fromMxTypeValueToName(a_mxTelemetryType);

        l_vector->append(a_subscriber);
        //qDebug() << "subscribe release mutex:" << static_cast<int>(a_mxTelemetryType);
    }
    return STATUS::SUCCESS;
}

uintptr_t DataDistributorQThreadImpl::unsubscribe(const int a_mxTelemetryType,
                                                  DataSubscriber* a_subscriber)
{
    {
        //get the mutex
        QMutexLocker locker(m_mutexList->at(static_cast<int>(a_mxTelemetryType)));
        //qDebug() << "unsubscribe acquire mutex:" << static_cast<int>(a_mxTelemetryType);

        //get the vector
        QVector<DataSubscriber*> *l_vector = m_subscribersDB->value(a_mxTelemetryType, nullptr);

        //sanity check
        if (!l_vector)
        {
            qCritical() << "Failed to retreive dats structure for mxTelemetryType"
                        << "is a_mxTelemetryType not in range of MxTelemetry::Type::N ?!"
                        << "MxTelemetry::Type=" <<  a_mxTelemetryType << "subscriber=" << a_subscriber->getName();
            return STATUS::FAILURE;
        }

        //get the subscriber
        int i = l_vector->indexOf(a_subscriber);

        // sanity check
        if (i == -1)
        {
             qCritical() << "Trying to unsubscribe unregisterd subscriber!"
                         << "MxTelemetry::Type=" <<  a_mxTelemetryType << "subscriber=" << a_subscriber->getName();
             return STATUS::FAILURE;
        }

        qDebug() << "unsubscribed" << a_subscriber->getName() << "to" <<  fromMxTypeValueToName(a_mxTelemetryType);

        l_vector->remove(i);
        //qDebug() << "unsubscribe release mutex:" << static_cast<int>(a_mxTelemetryType);
    }

    return STATUS::SUCCESS;
}

uintptr_t DataDistributorQThreadImpl::notify(void* a_mxTelemetryMsg)
{
    MxTelemetry::Msg* a_msg = static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg);

    const int l_mxTelemetryType = static_cast<int>(a_msg->hdr().type);

    {
        //get the mutex
        QMutexLocker locker(m_mutexList->at(static_cast<int>(l_mxTelemetryType)));
        //qDebug() << "notify acquire mutex:" << static_cast<int>(l_mxTelemetryType);

        //get the vector
        QVector<DataSubscriber*> *l_vector = m_subscribersDB->value(l_mxTelemetryType, nullptr);

        //sanity check
        if (!l_vector)
        {
            qCritical() << "Failed to retreive dats structure for mxTelemetryType"
                        << "is a_mxTelemetryType not in range of MxTelemetry::Type::N ?!"
                        << "MxTelemetry::Type=" <<  l_mxTelemetryType;
            return STATUS::FAILURE;
        }

        // update subscribers
        for (auto i = l_vector->begin(); i != l_vector->end() ;i++) { (**i).update(a_mxTelemetryMsg); }
        //qDebug() << "notify release mutex:" << static_cast<int>(l_mxTelemetryType);
    }

    return STATUS::SUCCESS;
}





