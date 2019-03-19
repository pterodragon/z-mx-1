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
#include "MxTelemetryZiCxnWrapper.h"
#include "QList"
#include "QDebug"

MxTelemetryZiCxnWrapper::MxTelemetryZiCxnWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryZiCxnWrapper");
}


MxTelemetryZiCxnWrapper::~MxTelemetryZiCxnWrapper()
{
}


void MxTelemetryZiCxnWrapper::initActiveDataSet() noexcept
{
    // 0=fd, 1=flagsrxBufSize
    m_activeDataSet = {0, 1};
}


void MxTelemetryZiCxnWrapper::initTableList() noexcept
{
    // removed irrelvant for table representation
    m_tableList->reserve(16);
    m_tableList->insert(0, "time");
    m_tableList->insert(1, "type");
    m_tableList->insert(2, "remoteIP");
    m_tableList->insert(3, "remotePort");
    m_tableList->insert(4, "localIP");
    m_tableList->insert(5, "localPort");
    m_tableList->insert(6, "fd");
    m_tableList->insert(7, "flags");
    m_tableList->insert(8, "mreqAddr");
    m_tableList->insert(9, "mreqIf");
    m_tableList->insert(10, "mif");
    m_tableList->insert(11, "ttl");
    m_tableList->insert(12, "rxBufSize");
    m_tableList->insert(13, "rxBufLen");
    m_tableList->insert(14, "txBufSize");
    m_tableList->insert(15, "txBufLen");
}


void MxTelemetryZiCxnWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{

}


void MxTelemetryZiCxnWrapper::initChartList() noexcept
{
    // removed irrelvant for chart representation
    m_chartList->reserve(6);
    m_chartPriorityToHeapIndex->reserve(5); // without none

    m_chartList->insert(0, "fd");
    m_chartPriorityToHeapIndex->insert(0, ZiCxnMxTelemetryStructIndex::e_socket);

    m_chartList->insert(1, "rxBufSize");
    m_chartPriorityToHeapIndex->insert(1, ZiCxnMxTelemetryStructIndex::e_rxBufSize);

    m_chartList->insert(2, "rxBufLen");
    m_chartPriorityToHeapIndex->insert(2, ZiCxnMxTelemetryStructIndex::e_rxBufLen);

    m_chartList->insert(3, "txBufSize");
    m_chartPriorityToHeapIndex->insert(3, ZiCxnMxTelemetryStructIndex::e_txBufSize);

    m_chartList->insert(4, "txBufLen");
    m_chartPriorityToHeapIndex->insert(4, ZiCxnMxTelemetryStructIndex::e_txBufLen);


    // extra
    m_chartList->insert(5, "none");
}


double MxTelemetryZiCxnWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    double l_result = 0;

    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return l_result;}

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
        qCritical() << *m_className
                    << __func__
                    << "Unknown Converstion (a_index, l_dataPair.second)"
                    << a_index
                    << l_dataPair.second;
        break;
    }
    return l_result;
}


QPair<void*, int> MxTelemetryZiCxnWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    ZiCxnTelemetry* l_data = static_cast<ZiCxnTelemetry*>(a_mxTelemetryMsg);
    QPair<void*, int> l_result;
    switch (a_index) {
    case ZiCxnMxTelemetryStructIndex::e_mxID:
        l_result.first = &l_data->mxID;
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case ZiCxnMxTelemetryStructIndex::e_socket:
        l_result.first = &l_data->socket;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_rxBufSize:
        l_result.first = &l_data->rxBufSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_rxBufLen:
        l_result.first = &l_data->rxBufLen;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_txBufSize:
        l_result.first = &l_data->txBufSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_txBufLen:
        l_result.first = &l_data->txBufLen;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_flags:
        l_result.first = &l_data->flags;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_mreqAddr:
        l_result.first = &l_data->mreqAddr;
        l_result.second = CONVERT_FRON::type_ZiIP;
        break;
    case ZiCxnMxTelemetryStructIndex::e_mreqIf:
        l_result.first = &l_data->mreqIf;
        l_result.second = CONVERT_FRON::type_ZiIP;
        break;
    case ZiCxnMxTelemetryStructIndex::e_mif:
        l_result.first = &l_data->mif;
        l_result.second = CONVERT_FRON::type_ZiIP;
        break;
    case ZiCxnMxTelemetryStructIndex::e_ttl:
        l_result.first = &l_data->ttl;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_localIP:
        l_result.first = &l_data->localIP;
        l_result.second = CONVERT_FRON::type_ZiIP;
        break;
    case ZiCxnMxTelemetryStructIndex::e_remoteIP:
        l_result.first = &l_data->remoteIP;
        l_result.second = CONVERT_FRON::type_ZiIP;
        break;
    case ZiCxnMxTelemetryStructIndex::e_localPort:
        l_result.first = &l_data->localPort;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_remotePort:
        l_result.first = &l_data->remotePort;
        l_result.second = CONVERT_FRON::type_uint16_t;
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

