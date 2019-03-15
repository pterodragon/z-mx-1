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

#ifndef ZMTHREADCHARTVIEW_H
#define ZMTHREADCHARTVIEW_H


#include "BasicChartView.h"

class ZmThreadChartView : public BasicChartView
{
public:
    ZmThreadChartView(QChart *chart, const std::array<unsigned int, 2>&  a_activeDataList, QWidget *a_parent = nullptr);
    virtual ~ZmThreadChartView() override;

    // should match ZmThreadTelemetry, ordered by priority, added none option for presentation
    enum ChartZmThreadTelemetry {cpuUsage, cpuset, priority,
                                 stackSize, partition, main, detached, none,
                                 N};

    qreal virtual getData(void* const a_mxTelemetryMsg, const unsigned int a_index) const noexcept override final;
    bool virtual isGivenTypeNotUsed(const unsigned int a_activeType) const noexcept override final;
    unsigned int virtual getLocalTypeSize() const noexcept override final;
    std::string virtual localTypeValueToString(const unsigned int a_val) const noexcept override final;

protected:

};


#endif // ZMTHREADCHARTVIEW_H
