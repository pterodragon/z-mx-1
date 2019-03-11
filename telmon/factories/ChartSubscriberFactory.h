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



#ifndef CHARTSUBSCRIBERFACTORY_H
#define CHARTSUBSCRIBERFACTORY_H


class TableSubscriber;
class DataDistributor;

class ChartSubscriberFactory
{
private:
    // Private Constructor
    ChartSubscriberFactory();
    // Stop the compiler generating methods of copy the object
    ChartSubscriberFactory(ChartSubscriberFactory const& copy);            // Not Implemented
    ChartSubscriberFactory& operator=(ChartSubscriberFactory const& copy); // Not Implemented

public:
    static ChartSubscriberFactory& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static ChartSubscriberFactory m_instance;
        return m_instance;
    }

    // Not responsible for deallocating
    TableSubscriber* getSubscriber(const int a_type) const noexcept;
};

#endif // CHARTSUBSCRIBERFACTORY_H
