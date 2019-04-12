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

#ifndef DATADISTRIBUTORQTHREADIMPL_H
#define DATADISTRIBUTORQTHREADIMPL_H

#include "DataDistributor.h"

template<class T, class H>
class QMap;

template<class T>
class QList;
class QMutex;

template<class T>
class QVector;

class DataDistributorQThreadImpl : public DataDistributor
{
public:
    DataDistributorQThreadImpl();
    virtual ~DataDistributorQThreadImpl() override;

    virtual uintptr_t subscribeAll( DataSubscriber*  a_subscriber) override final;
    virtual uintptr_t unsubscribeAll( DataSubscriber*  a_subscriber) override final;
    virtual uintptr_t subscribe(const int a_mxTelemetryType, DataSubscriber* a_subscriber) override final;
    virtual uintptr_t unsubscribe(const int a_mxTelemetryType, DataSubscriber* a_subscriber) override final;
    virtual uintptr_t notify(void* a_mxTelemetryMsg, const int a_mxType) override final;

private:
    // Data Struture based on: https://stackoverflow.com/questions/471432/in-which-scenario-do-i-use-a-particular-stl-container
    QMap<int, QVector<DataSubscriber*>*>*  m_subscribersDB; // do not clean DataSubscriber!!

    // mutexs array
    QList<QMutex*>* m_mutexList;

private:
    const int m_mxTelemetryTotalTypes;
};

#endif // DATADISTRIBUTORQTHREADIMPL_H
