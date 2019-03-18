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

#include "ZmHashMgr.hpp"
#include "HashTblChartView.h"


HashTblChartView::HashTblChartView(QChart *a_chart,
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


HashTblChartView::~HashTblChartView()
{

}


bool HashTblChartView::isGivenTypeNotUsed(const unsigned int a_activeType) const noexcept
{
    return a_activeType == ChartHashTblTelemetry::none;
}


unsigned int HashTblChartView::getLocalTypeSize() const noexcept
{
    return ChartHashTblTelemetry::N;
}


qreal HashTblChartView::getData(void* const a_mxTelemetryMsg, const unsigned int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    const ZmHashTelemetry* l_data = static_cast<ZmHashTelemetry*>(a_mxTelemetryMsg);

    qreal l_result;
    switch (a_index) {
    case ChartHashTblTelemetry::linear:
        l_result = ZuBoxed(l_data->linear);
        break;
    case ChartHashTblTelemetry::bits:
        l_result = ZuBoxed(l_data->bits);
        break;
    case ChartHashTblTelemetry::l_slots:
        l_result = ZuBoxed((static_cast<uint64_t>(1))<<l_data->bits);
        break;
    case ChartHashTblTelemetry::cBits:
        l_result = ZuBoxed(l_data->cBits);
        break;
    case ChartHashTblTelemetry::cSlots:
        l_result = ZuBoxed((static_cast<uint64_t>(1))<<l_data->cBits);
        break;
    case ChartHashTblTelemetry::count:
        l_result = l_data->count;
        break;
    case ChartHashTblTelemetry::resized:
        l_result = l_data->resized;
        break;
    case ChartHashTblTelemetry::loadFactor:
        l_result = static_cast<double>(l_data->loadFactor   / 16.0);
        break;
    case ChartHashTblTelemetry::effLoadFactor:
        l_result = static_cast<double>(l_data->effLoadFactor / 16.0);
        break;
    case ChartHashTblTelemetry::nodeSize:
        l_result = (l_data->nodeSize);
        break;
    case ChartHashTblTelemetry::none:
        l_result = 0;
        break;
    default:
        l_result = 0;
        break;
    }
    return l_result;
}


std::string HashTblChartView::localTypeValueToString(const unsigned int a_val) const noexcept
{
    std::string l_result = "";
    switch (a_val) {
    case ChartHashTblTelemetry::linear:
        l_result = "linear";
        break;
    case ChartHashTblTelemetry::bits:
        l_result = "bits";
        break;
    case ChartHashTblTelemetry::l_slots:
        l_result = "slots";
        break;
    case ChartHashTblTelemetry::cBits:
        l_result = "cBits";
        break;
    case ChartHashTblTelemetry::cSlots:
        l_result = "cSlots";
        break;
    case ChartHashTblTelemetry::count:
        l_result = "count";
        break;
    case ChartHashTblTelemetry::resized:
        l_result = "resized";
        break;
    case ChartHashTblTelemetry::loadFactor:
        l_result = "loadFactor";
        break;
    case ChartHashTblTelemetry::effLoadFactor:
        l_result = "effLoadFactor";
        break;
    case ChartHashTblTelemetry::nodeSize:
        l_result = "nodeSize";
        break;
    case ChartHashTblTelemetry::none:
        l_result = "none";
        break;
    default:
        l_result = "default";
        break;
    }
    return l_result;
}
