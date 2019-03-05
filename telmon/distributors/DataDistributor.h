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

// based on
// https://stackoverflow.com/questions/318064/how-do-you-declare-an-interface-in-c


#ifndef DATADISTRIBUTOR_H
#define DATADISTRIBUTOR_H

#include <cstdint> // for uintptr_t
#include <QString>

class DataSubscriber;

//template <typename T> //use instead of void ?
class DataDistributor
{
public:
    enum STATUS {SUCCESS, FAILURE};
    DataDistributor();
    virtual ~DataDistributor();
    // Stop the compiler generating methods of copy the object
    DataDistributor(DataDistributor const& copy);            // Not Implemented
    DataDistributor& operator=(DataDistributor const& copy); // Not Implemented

    virtual uintptr_t subscribeAll( DataSubscriber*  a_subscriber) = 0;
    virtual uintptr_t unsubscribeAll( DataSubscriber*  a_subscriber) = 0;
    virtual uintptr_t subscribe(const unsigned int a_mxTelemetryType, DataSubscriber*  a_subscriber) = 0;
    virtual uintptr_t unsubscribe(const unsigned int a_mxTelemetryType, DataSubscriber* a_subscriber) = 0;
    virtual uintptr_t notify(void* a_mxTelemetryMsg) = 0;

    QString fromMxTypeValueToName(const unsigned int a_type) const noexcept;
    unsigned int fromMxTypeNameToValue(const QString& a_name) const noexcept;
    unsigned int mxTypeSize() const noexcept;

};

#endif // DATADISTRIBUTOR_H
