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

#include "ZmHeap.hpp"
#include "HeapChartView.h"

HeapChartView::HeapChartView(QChart *a_chart,
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


HeapChartView::~HeapChartView()
{

}


bool HeapChartView::isGivenTypeNotUsed(const unsigned int a_activeType) const noexcept
{
    return a_activeType == ChartHeapTelemetry::none;
}


unsigned int HeapChartView::getLocalTypeSize() const noexcept
{
    return ChartHeapTelemetry::N;
}


qreal HeapChartView::getData(void* const a_mxTelemetryMsg, const unsigned int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    const ZmHeapTelemetry* l_data = static_cast<ZmHeapTelemetry*>(a_mxTelemetryMsg);

    qreal l_result;
    switch (a_index) {
    case ChartHeapTelemetry::size:
        l_result = l_data->size;
        break;
    case ChartHeapTelemetry::alignment:
        l_result = l_data->alignment;
        break;
    case ChartHeapTelemetry::partition:
        l_result = l_data->partition;
        break;
    case ChartHeapTelemetry::sharded:
        l_result = l_data->sharded;
        break;
    case ChartHeapTelemetry::cacheSize:
        l_result = l_data->cacheSize;
        break;
    case ChartHeapTelemetry::cpuset:
        l_result = l_data->cpuset;
        break;
    case ChartHeapTelemetry::cacheAllocs:
        l_result = l_data->cacheAllocs;
        break;
    case ChartHeapTelemetry::heapAllocs:
        l_result = l_data->heapAllocs;
        break;
    case ChartHeapTelemetry::frees:
        l_result = l_data->frees;
        break;
    case ChartHeapTelemetry::allocated:
        l_result = (l_data->cacheAllocs + l_data->heapAllocs - l_data->frees);
        break;
    case ChartHeapTelemetry::none:
        l_result = 0;
        break;
    default:
        l_result = 0;
        break;
    }
    return l_result;
}


std::string  HeapChartView::localTypeValueToString(const unsigned int a_val) const noexcept
{
    std::string l_result = "";
    switch (a_val) {
    case ChartHeapTelemetry::size:
        l_result = "size";
        break;
    case ChartHeapTelemetry::alignment:
        l_result = "alignment";
        break;
    case ChartHeapTelemetry::partition:
        l_result = "partition";
        break;
    case ChartHeapTelemetry::sharded:
        l_result = "sharded";
        break;
    case ChartHeapTelemetry::cacheSize:
        l_result = "cache size";
        break;
    case ChartHeapTelemetry::cpuset:
        l_result = "cpuset";
        break;
    case ChartHeapTelemetry::cacheAllocs:
        l_result = "cache allocs";
        break;
    case ChartHeapTelemetry::heapAllocs:
        l_result = "heap allocs";
        break;
    case ChartHeapTelemetry::frees:
        l_result = "frees";
        break;
    case ChartHeapTelemetry::allocated:
        l_result = "allocated";
        break;
    case ChartHeapTelemetry::none:
        l_result = "none";
        break;
    default:
        l_result = "default";
        break;
    }
    return l_result;
}


