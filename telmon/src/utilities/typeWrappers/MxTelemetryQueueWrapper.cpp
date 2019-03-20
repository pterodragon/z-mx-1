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

#include "MxTelemetry.hpp"
#include "MxTelemetryQueueWrapper.h"
#include "QList"
#include "QDebug"


MxTelemetryQueueWrapper::MxTelemetryQueueWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryQueueWrapper");
}


MxTelemetryQueueWrapper::~MxTelemetryQueueWrapper()
{
}


void MxTelemetryQueueWrapper::initActiveDataSet() noexcept
{
    // 0=, 1=
    m_activeDataSet = {0, 1};
}


void MxTelemetryQueueWrapper::initTableList() noexcept
{
    // removed irrelvant for table representation
    m_tableList->reserve(10);
    m_tableList->insert(0, "time");
    m_tableList->insert(1, "type");
    m_tableList->insert(2, "full");
    m_tableList->insert(3, "size");
    m_tableList->insert(4, "count");
    m_tableList->insert(5, "seqNo");
    m_tableList->insert(6, "inCount");
    m_tableList->insert(7, "inBytes");
    m_tableList->insert(8, "outCount");
    m_tableList->insert(9, "outBytes");
}


void MxTelemetryQueueWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{

}


void MxTelemetryQueueWrapper::initChartList() noexcept
{
    // removed irrelvant for chart representation
    m_chartList->reserve(9);
    m_chartPriorityToStructIndex->reserve(8); // without none

    m_chartList->insert(0, "full");
    m_chartPriorityToStructIndex->insert(0, QueueMxTelemetryStructIndex::e_full);

    m_chartList->insert(1, "size");
    m_chartPriorityToStructIndex->insert(1, QueueMxTelemetryStructIndex::e_size);

    m_chartList->insert(2, "count");
    m_chartPriorityToStructIndex->insert(2, QueueMxTelemetryStructIndex::e_count);

    m_chartList->insert(3, "seqNo");
    m_chartPriorityToStructIndex->insert(3, QueueMxTelemetryStructIndex::e_seqNo);

    m_chartList->insert(4, "inCount");
    m_chartPriorityToStructIndex->insert(4, QueueMxTelemetryStructIndex::e_inCount);

    m_chartList->insert(5, "inBytes");
    m_chartPriorityToStructIndex->insert(5, QueueMxTelemetryStructIndex::e_inBytes);

    m_chartList->insert(6, "outCount");
    m_chartPriorityToStructIndex->insert(6, QueueMxTelemetryStructIndex::e_outCount);

    m_chartList->insert(7, "outBytes");
    m_chartPriorityToStructIndex->insert(7, QueueMxTelemetryStructIndex::e_outBytes);

    // extra
    m_chartList->insert(8, "none");
}


double MxTelemetryQueueWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    double l_result = 0;

    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return l_result;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);
    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

    switch (l_dataPair.second) {
    case CONVERT_FRON::type_uint64_t:
        l_result = typeConvertor<double>(QPair(l_dataPair.first, CONVERT_FRON::type_uint64_t));
        break;
    case CONVERT_FRON::type_uint32_t:
        l_result = typeConvertor<double>(QPair(l_dataPair.first, CONVERT_FRON::type_uint32_t));
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


QPair<void*, int> MxTelemetryQueueWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    MxTelemetry::Queue* l_data = static_cast<MxTelemetry::Queue*>(a_mxTelemetryMsg);

    QPair<void*, int> l_result;
    switch (a_index) {
    case QueueMxTelemetryStructIndex::e_id:
        l_result.first = &l_data->id;
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case QueueMxTelemetryStructIndex::e_seqNo:
        l_result.first = &l_data->seqNo;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case QueueMxTelemetryStructIndex::e_count:
        l_result.first = &l_data->count;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case QueueMxTelemetryStructIndex::e_inCount:
        l_result.first = &l_data->inCount;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case QueueMxTelemetryStructIndex::e_inBytes:
        l_result.first = &l_data->inBytes;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case QueueMxTelemetryStructIndex::e_outCount:
        l_result.first = &l_data->outCount;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case QueueMxTelemetryStructIndex::e_outBytes:
        l_result.first = &l_data->outBytes;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case QueueMxTelemetryStructIndex::e_full:
        l_result.first = &l_data->full;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case QueueMxTelemetryStructIndex::e_size:
        l_result.first = &l_data->size;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case QueueMxTelemetryStructIndex::e_type:
        l_result.first = &l_data->type;
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





