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


#include "MxTelemetryZmThreadWrapper.h"
#include "QList"
#include "ZmThread.hpp"
#include "QDebug"

MxTelemetryZmThreadWrapper::MxTelemetryZmThreadWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryZmThreadWrapper");
}


MxTelemetryZmThreadWrapper::~MxTelemetryZmThreadWrapper()
{
}


void MxTelemetryZmThreadWrapper::initActiveDataSet() noexcept
{
    // 0=cpuUsage, 1=cpuset
    m_activeDataSet = {0, 1};
}


void MxTelemetryZmThreadWrapper::initTableList() noexcept
{
    // removed irrelvant for table representation
    m_tableList->reserve(10);
    m_tableList->insert(0, "time");
    m_tableList->insert(1, "id");
    m_tableList->insert(2, "tid");
    m_tableList->insert(3, "cpuUsage");
    m_tableList->insert(4, "cpuset");
    m_tableList->insert(5, "priority");
    m_tableList->insert(6, "stackSize");
    m_tableList->insert(7, "partition");
    m_tableList->insert(8, "main");
    m_tableList->insert(9, "detached");
}


void MxTelemetryZmThreadWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{

}


void MxTelemetryZmThreadWrapper::initChartList() noexcept
{
    // removed irrelvant for chart representation
    m_chartList->reserve(7);
    m_chartPriorityToHeapIndex->reserve(6); // without none
//    m_chartList->insert(0, "time");
//    m_chartList->insert(1, "id");

    m_chartList->insert(0, "cpuUsage");
    m_chartPriorityToHeapIndex->insert(0, ZmThreadTelemetryStructIndex::e_cpuUsage);

    m_chartList->insert(1, "cpuset");
    m_chartPriorityToHeapIndex->insert(1, ZmThreadTelemetryStructIndex::e_cpuset);

    m_chartList->insert(2, "priority");
    m_chartPriorityToHeapIndex->insert(2, ZmThreadTelemetryStructIndex::e_priority);

    m_chartList->insert(3, "stackSize");
    m_chartPriorityToHeapIndex->insert(3, ZmThreadTelemetryStructIndex::e_stackSize);

    m_chartList->insert(4, "partition");
    m_chartPriorityToHeapIndex->insert(4, ZmThreadTelemetryStructIndex::e_partition);

    m_chartList->insert(5, "main");
    m_chartPriorityToHeapIndex->insert(5, ZmThreadTelemetryStructIndex::e_main);

    m_chartList->insert(6, "detached");
    m_chartPriorityToHeapIndex->insert(6, ZmThreadTelemetryStructIndex::e_detached);

    // extra
    m_chartList->insert(7, "none");
}


double MxTelemetryZmThreadWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    double l_result = 0;

    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return l_result;}

    if (a_index == 0) // case: cpuUsage
    {
        ZmThreadTelemetry* l_data = static_cast<ZmThreadTelemetry*>(a_mxTelemetryMsg);
        double l_cpuUsage = typeConvertor<double>(QPair(&l_data->cpuUsage, CONVERT_FRON::type_double));
        return (l_cpuUsage * 100);
    }


    const int l_index = m_chartPriorityToHeapIndex->at(a_index);
    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

    switch (l_dataPair.second) {
    case CONVERT_FRON::type_uint64_t:
        l_result = typeConvertor<double>(QPair(l_dataPair.first, CONVERT_FRON::type_uint64_t));
        break;
    case CONVERT_FRON::type_uint32_t:
        l_result = typeConvertor<double>(QPair(l_dataPair.first, CONVERT_FRON::type_uint32_t));
        break;
    case CONVERT_FRON::type_uint16_t:
        l_result = typeConvertor<double>(QPair(l_dataPair.first, CONVERT_FRON::type_uint16_t));
        break;
    case CONVERT_FRON::type_uint8_t:
        l_result = typeConvertor<double>(QPair(l_dataPair.first, CONVERT_FRON::type_uint8_t));
        break;
    case CONVERT_FRON::type_int32_t:
        l_result = typeConvertor<double>(QPair(l_dataPair.first, CONVERT_FRON::type_int32_t));
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


QPair<void*, int> MxTelemetryZmThreadWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    ZmThreadTelemetry* l_data = static_cast<ZmThreadTelemetry*>(a_mxTelemetryMsg);
    QPair<void*, int> l_result;
    switch (a_index) {
    case ZmThreadTelemetryStructIndex::e_name:
        l_result.first = &l_data->name;
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case ZmThreadTelemetryStructIndex::e_tid:
        l_result.first = &l_data->tid;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmThreadTelemetryStructIndex::e_stackSize:
        l_result.first = &l_data->stackSize;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmThreadTelemetryStructIndex::e_cpuset:
        l_result.first = &l_data->cpuset;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmThreadTelemetryStructIndex::e_cpuUsage:
        l_result.first = &l_data->cpuUsage;
        l_result.second = CONVERT_FRON::type_double;
        break;
    case ZmThreadTelemetryStructIndex::e_id:
        l_result.first = &l_data->id;
        l_result.second = CONVERT_FRON::type_int32_t;
        break;
    case ZmThreadTelemetryStructIndex::e_priority:
        l_result.first = &l_data->priority;
        l_result.second = CONVERT_FRON::type_int32_t;
        break;
    case ZmThreadTelemetryStructIndex::e_partition:
        l_result.first = &l_data->partition;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZmThreadTelemetryStructIndex::e_main:
        l_result.first = &l_data->main;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case ZmThreadTelemetryStructIndex::e_detached:
        l_result.first = &l_data->detached;
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
