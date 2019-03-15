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

#include "ZmThreadChartView.h"

ZmThreadChartView::ZmThreadChartView(QChart *a_chart,
                             const std::array<unsigned int, 2>&  a_activeDataList,
                             QWidget *a_parent):
    BasicChartView(a_chart, a_activeDataList, a_parent)

{
    setDefaultUpdateDataFunction();
    initMenuBar();
    createActions();

    getAxes(CHART_AXIS::X).setTickCount(10);
    m_axisArray[CHART_AXIS::X]->setRange(0, 10);

    initSeries();
}


ZmThreadChartView::~ZmThreadChartView()
{

}


bool ZmThreadChartView::isGivenTypeNotUsed(const unsigned int a_activeType) const noexcept
{
    return a_activeType == ChartZmThreadTelemetry::none;
}


unsigned int ZmThreadChartView::getLocalTypeSize() const noexcept
{
    return ChartZmThreadTelemetry::N;
}


qreal ZmThreadChartView::getData(void* const a_mxTelemetryMsg, const unsigned int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    const ZmThreadTelemetry* l_data = static_cast<ZmThreadTelemetry*>(a_mxTelemetryMsg);

    qreal l_result;
    switch (a_index) {
    case ChartZmThreadTelemetry::cpuUsage:
        l_result = (l_data->cpuUsage * 100);
        break;
    case ChartZmThreadTelemetry::cpuset:
        l_result = l_data->cpuset;
        break;
    case ChartZmThreadTelemetry::priority:
        l_result = l_data->priority;
        break;
    case ChartZmThreadTelemetry::stackSize:
        l_result = l_data->stackSize;
        break;
    case ChartZmThreadTelemetry::partition:
        l_result = l_data->partition;
        break;
    case ChartZmThreadTelemetry::main:
        l_result = l_data->main;
        break;
    case ChartZmThreadTelemetry::detached:
        l_result = l_data->detached;
        break;
    case ChartZmThreadTelemetry::none:
        l_result = 0;
        break;
    default:
        l_result = 0;
        break;
    }
    return l_result;
}


std::string  ZmThreadChartView::localTypeValueToString(const unsigned int a_val) const noexcept
{
    std::string l_result = "";
    switch (a_val) {
    case ChartZmThreadTelemetry::cpuUsage:
        l_result = "cpuUsage";
        break;
    case ChartZmThreadTelemetry::cpuset:
        l_result = "cpuset";
        break;
    case ChartZmThreadTelemetry::priority:
        l_result = "priority";
        break;
    case ChartZmThreadTelemetry::stackSize:
        l_result = "stackSize";
        break;
    case ChartZmThreadTelemetry::partition:
        l_result = "partition";
        break;
    case ChartZmThreadTelemetry::main:
        l_result = "main";
        break;
    case ChartZmThreadTelemetry::detached:
        l_result = "detached";
        break;
    case ChartZmThreadTelemetry::none:
        l_result = "none";
        break;
    default:
        l_result = "default";
        break;
    }
    return l_result;
}












