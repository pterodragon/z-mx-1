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


#include "ZiMultiplex.hpp"
#include "MxTelemetryZiMultiplexerWrapper.h"
#include "QList"
#include "QDebug"




MxTelemetryZiMultiplexerWrapper::MxTelemetryZiMultiplexerWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
}


MxTelemetryZiMultiplexerWrapper::~MxTelemetryZiMultiplexerWrapper()
{
}


void MxTelemetryZiMultiplexerWrapper::initActiveDataSet() noexcept
{
    // 0=nThreads, 1=priority
    m_activeDataSet = {0, 1};
}


void MxTelemetryZiMultiplexerWrapper::initTableList() noexcept
{
    // removed irrelvant for table representation
    m_tableList->reserve(11);
    m_tableList->insert(0, "time");
    m_tableList->insert(1, "state");
    m_tableList->insert(2, "nThreads");
    m_tableList->insert(3, "priority");
    m_tableList->insert(4, "partition");
    m_tableList->insert(5, "isolation");
    m_tableList->insert(6, "rxThread");
    m_tableList->insert(7, "txThread");
    m_tableList->insert(8, "stackSize");
    m_tableList->insert(9, "rxBufSize");
    m_tableList->insert(10, "txBufSize");
}


void MxTelemetryZiMultiplexerWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{

}


void MxTelemetryZiMultiplexerWrapper::initChartList() noexcept
{
    // removed irrelvant for chart representation
    m_chartList->reserve(7);
    m_chartPriorityToHeapIndex->reserve(6); // without none
//    m_chartList->insert(0, "time");
//    m_chartList->insert(1, "state");

    m_chartList->insert(0, "nThreads");
    m_chartPriorityToHeapIndex->insert(0, ZiMxTelemetryStructIndex::e_nThreads);

    m_chartList->insert(1, "priority");
    m_chartPriorityToHeapIndex->insert(1, ZiMxTelemetryStructIndex::e_priority);

    m_chartList->insert(2, "partition");
    m_chartPriorityToHeapIndex->insert(2, ZiMxTelemetryStructIndex::e_partition);

    m_chartList->insert(3, "stackSize");
    m_chartPriorityToHeapIndex->insert(3, ZiMxTelemetryStructIndex::e_stackSize);

    m_chartList->insert(4, "rxBufSize");
    m_chartPriorityToHeapIndex->insert(4, ZiMxTelemetryStructIndex::e_rxBufSize);

    m_chartList->insert(5, "txBufSize");
    m_chartPriorityToHeapIndex->insert(5, ZiMxTelemetryStructIndex::e_txBufSize);
;

    // extra
    m_chartList->insert(6, "none");
}

double MxTelemetryZiMultiplexerWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    double l_result = 0;
    if (a_index >= m_chartPriorityToHeapIndex->count())
    {
        qWarning() << "MxTelemetryHeapWrapper::getDataForChart called with unsupoorted index"
                   << a_index
                   << "returning " << l_result;
        return l_result;
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
    default:
        qDebug() << "Error, unknown conversion a_index=" << a_index
                 << "convet from: " << l_dataPair.second
                 << "returning default value" << l_result;
        break;
    }
    return l_result;
}


QPair<void*, int> MxTelemetryZiMultiplexerWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    ZiMxTelemetry* l_data = static_cast<ZiMxTelemetry*>(a_mxTelemetryMsg);
    QPair<void*, int> l_result;
    switch (a_index) {
    case ZiMxTelemetryStructIndex::e_id:
        l_result.first = &l_data->id;
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case ZiMxTelemetryStructIndex::e_isolation:
        l_result.first = &l_data->isolation;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZiMxTelemetryStructIndex::e_stackSize:
        l_result.first = &l_data->stackSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiMxTelemetryStructIndex::e_rxBufSize:
        l_result.first = &l_data->rxBufSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiMxTelemetryStructIndex::e_txBufSize:
        l_result.first = &l_data->txBufSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiMxTelemetryStructIndex::e_rxThread:
        l_result.first = &l_data->rxThread;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZiMxTelemetryStructIndex::e_txThread:
        l_result.first = &l_data->priority;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZiMxTelemetryStructIndex::e_partition:
        l_result.first = &l_data->partition;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZiMxTelemetryStructIndex::e_state:
        l_result.first = &l_data->state;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case ZiMxTelemetryStructIndex::e_priority:
        l_result.first = &l_data->priority;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case ZiMxTelemetryStructIndex::e_nThreads:
        l_result.first = &l_data->nThreads;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    default:
        qWarning() << "unknown ZmHeapSelemetryStructIndex, a_index" << a_index
                   << "retunring <nullptr CONVERT_FRON::none>";
        l_result.first = nullptr;
        l_result.second = CONVERT_FRON::type_none;
        break;
    }
    return l_result;
}
