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

#include "MxEngine.hpp"
#include "MxTelemetryLinkWrapper.h"
#include "QList"
#include "QDebug"

MxTelemetryLinkWrapper::MxTelemetryLinkWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryLinkWrapper");
}



MxTelemetryLinkWrapper::~MxTelemetryLinkWrapper()
{
}


void MxTelemetryLinkWrapper::initActiveDataSet() noexcept
{
    // 0=, 1=
    m_activeDataSet = {0, 1};
}


void MxTelemetryLinkWrapper::initTableList() noexcept
{
    // removed irrelvant for table representation
    m_tableList->reserve(5);
    m_tableList->insert(0, "time");
    m_tableList->insert(1, "state");
    m_tableList->insert(2, "reconnects");
    m_tableList->insert(3, "rxSeqNo");
    m_tableList->insert(4, "txSeqNo");
}


void MxTelemetryLinkWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{

}


void MxTelemetryLinkWrapper::initChartList() noexcept
{
    // removed irrelvant for chart representation
    m_chartList->reserve(4);
    m_chartPriorityToHeapIndex->reserve(3); // without none

    m_chartList->insert(0, "reconnects");
    m_chartPriorityToHeapIndex->insert(0, LinkMxTelemetryStructIndex::e_reconnects);

    m_chartList->insert(1, "rxSeqNo");
    m_chartPriorityToHeapIndex->insert(1, LinkMxTelemetryStructIndex::e_rxSeqNo);

    m_chartList->insert(2, "txSeqNo");
    m_chartPriorityToHeapIndex->insert(2, LinkMxTelemetryStructIndex::e_txSeqNo);


    // extra
    m_chartList->insert(3, "none");
}


double MxTelemetryLinkWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
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


QPair<void*, int> MxTelemetryLinkWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    MxAnyLink::Telemetry* l_data = static_cast<MxAnyLink::Telemetry*>(a_mxTelemetryMsg);

    QPair<void*, int> l_result;
    switch (a_index) {
    case LinkMxTelemetryStructIndex::e_id:
        l_result.first = l_data->id.data();  //not tested
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case LinkMxTelemetryStructIndex::e_rxSeqNo:
        l_result.first = &l_data->rxSeqNo;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case LinkMxTelemetryStructIndex::e_txSeqNo:
        l_result.first = &l_data->txSeqNo;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case LinkMxTelemetryStructIndex::e_reconnects:
        l_result.first = &l_data->reconnects;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case LinkMxTelemetryStructIndex::e_state:
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

