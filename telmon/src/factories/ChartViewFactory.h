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


#ifndef CHARTVIEWFACTORY_H
#define CHARTVIEWFACTORY_H

#include "QtCharts"

class QString;

class ChartViewFactory
{
private:
    // Private Constructor
    ChartViewFactory();
    // Stop the compiler generating methods of copy the object
    ChartViewFactory(ChartViewFactory const& copy);            // Not Implemented
    ChartViewFactory& operator=(ChartViewFactory const& copy); // Not Implemented

public:
    static ChartViewFactory& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static ChartViewFactory m_instance;
        return m_instance;
    }

    // Not responsible for deallocating
    QChartView* getChartView(const int a_tableType,
                                   const QString& a_mxTelemetryInstanceName) const noexcept;
};

#endif // CHARTVIEWFACTORY_H
