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


#ifndef DATASUBSCRIBER_H
#define DATASUBSCRIBER_H

#include <cstdint> // for uintptr_t

class QString;
#include <string>

/**
 * @brief The DataSubscriber class
 * General interface for data subscriber to implement
 */
class DataSubscriber
{
public:
    DataSubscriber();
    virtual ~DataSubscriber();
    // Stop the compiler generating methods of copy the object
    DataSubscriber(DataSubscriber const& copy);            // Not Implemented
    DataSubscriber& operator=(DataSubscriber const& copy); // Not Implemented

    virtual const QString& getName() const noexcept = 0;
    virtual void update(void* a_mxTelemetryMsg) = 0;

    /**
     * @brief used to create Queue name
     * @param
     * @return
     */
    const std::string constructQueueName(const char* a_id, const char* a_type) const noexcept;

private:
    static const char* QUEUE_NAME_DELIMITER;
};

#endif // DATASUBSCRIBER_H
