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


#include "Zdb.hpp"
#include "MxTelemetryDBHostWrapper.h"
#include "QList"
#include "QDebug"


MxTelemetryDBHostWrapper::MxTelemetryDBHostWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryDBHostWrapper");
}


MxTelemetryDBHostWrapper::~MxTelemetryDBHostWrapper()
{
}


void MxTelemetryDBHostWrapper::initActiveDataSet() noexcept
{
    // 0=, 1=
    m_activeDataSet = {0, 1};
}


void MxTelemetryDBHostWrapper::initTableList() noexcept
{
    // removed irrelvant for table representation
    m_tableList->reserve(6);
    m_tableList->insert(0, "time");
    m_tableList->insert(1, "priority");
    m_tableList->insert(2, "state");
    m_tableList->insert(3, "voted");
    m_tableList->insert(4, "ip");

    m_tableList->insert(5, "port");
}


void MxTelemetryDBHostWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{

}


void MxTelemetryDBHostWrapper::initChartList() noexcept
{
    // removed irrelvant for chart representation
    m_chartList->reserve(4);
    m_chartPriorityToHeapIndex->reserve(3); // without none

//    m_chartList->insert(0, "time");
    m_chartList->insert(0, "priority");
    m_chartPriorityToHeapIndex->insert(0, DBHostMxTelemetryStructIndex::e_priority);
    m_chartList->insert(1, "state");
    m_chartPriorityToHeapIndex->insert(1, DBHostMxTelemetryStructIndex::e_state);
    m_chartList->insert(2, "voted");
    m_chartPriorityToHeapIndex->insert(2, DBHostMxTelemetryStructIndex::e_voted);


    // extra
    m_chartList->insert(3, "none");
}


double MxTelemetryDBHostWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    double l_result = 0;

    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return l_result;}

    const int l_index = m_chartPriorityToHeapIndex->at(a_index);
    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

    switch (l_dataPair.second) {
    case CONVERT_FRON::type_uint32_t:
        l_result = typeConvertor<double>(QPair(l_dataPair.first, CONVERT_FRON::type_uint32_t));
        break;
    case CONVERT_FRON::type_uint16_t:
        l_result = typeConvertor<double>(QPair(l_dataPair.first, CONVERT_FRON::type_uint16_t));
        break;
    case CONVERT_FRON::type_uint8_t:
        l_result = typeConvertor<double>(QPair(l_dataPair.first, CONVERT_FRON::type_uint8_t));
        break;
    default:
        qCritical() << *m_className
                    << __func__
                    << "Unknown Converstion (a_index, l_dataPair.second)"
                    << a_index
                    << l_dataPair.second;
        break;
    }
    return l_result;
}


QPair<void*, int> MxTelemetryDBHostWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    ZdbHost::Telemetry* l_data = static_cast<ZdbHost::Telemetry*>(a_mxTelemetryMsg);

    QPair<void*, int> l_result;
    switch (a_index) {
    case DBHostMxTelemetryStructIndex::e_ip:
        l_result.first = &l_data->ip;  //not tested
        l_result.second = CONVERT_FRON::type_ZiIP;
        break;
    case DBHostMxTelemetryStructIndex::e_id:
        l_result.first = &l_data->id;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBHostMxTelemetryStructIndex::e_priority:
        l_result.first = &l_data->priority;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBHostMxTelemetryStructIndex::e_port:
        l_result.first = &l_data->port;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case DBHostMxTelemetryStructIndex::e_voted:
        l_result.first = &l_data->voted;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case DBHostMxTelemetryStructIndex::e_state:
        l_result.first = &l_data->state;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    default:
        qCritical() << *m_className
                    << __func__
                    << "unsupported struct index"
                    << a_index;
        l_result.first = nullptr;
        l_result.second = CONVERT_FRON::type_none;
        break;
    }
    return l_result;
}












