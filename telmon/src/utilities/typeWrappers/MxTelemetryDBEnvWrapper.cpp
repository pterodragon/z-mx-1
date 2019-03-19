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
#include "MxTelemetryDBEnvWrapper.h"
#include "QList"
#include "QDebug"

MxTelemetryDBEnvWrapper::MxTelemetryDBEnvWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryDBEnvWrapper");
}


MxTelemetryDBEnvWrapper::~MxTelemetryDBEnvWrapper()
{
}


void MxTelemetryDBEnvWrapper::initActiveDataSet() noexcept
{
    // 0=, 1=
    m_activeDataSet = {0, 1};
}


void MxTelemetryDBEnvWrapper::initTableList() noexcept
{
    // removed irrelvant for table representation
    m_tableList->reserve(18);
    m_tableList->insert(0, "time");
    m_tableList->insert(1, "self");
    m_tableList->insert(2, "master");
    m_tableList->insert(3, "prev");
    m_tableList->insert(4, "next");

    m_tableList->insert(5, "state");
    m_tableList->insert(6, "active");
    m_tableList->insert(7, "recovering");
    m_tableList->insert(8, "replicating");
    m_tableList->insert(9, "nDBs");

    m_tableList->insert(10, "nHosts");
    m_tableList->insert(11, "nPeers");
    m_tableList->insert(12, "nCxns");
    m_tableList->insert(13, "heartbeatFreq");
    m_tableList->insert(14, "heartbeatTimeout");

    m_tableList->insert(15, "reconnectFreq");
    m_tableList->insert(16, "electionTimeout");
    m_tableList->insert(17, "writeThread");
}


void MxTelemetryDBEnvWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{

}


void MxTelemetryDBEnvWrapper::initChartList() noexcept
{
    // removed irrelvant for chart representation
    m_chartList->reserve(18);
    m_chartPriorityToHeapIndex->reserve(17); // without none

//    m_chartList->insert(0, "time");
    m_chartList->insert(0, "self");
    m_chartPriorityToHeapIndex->insert(0, DBEnvMxTelemetryStructIndex::e_self);
    m_chartList->insert(1, "master");
    m_chartPriorityToHeapIndex->insert(1, DBEnvMxTelemetryStructIndex::e_master);
    m_chartList->insert(2, "prev");
    m_chartPriorityToHeapIndex->insert(2, DBEnvMxTelemetryStructIndex::e_prev);
    m_chartList->insert(3, "next");
    m_chartPriorityToHeapIndex->insert(3, DBEnvMxTelemetryStructIndex::e_next);

    m_chartList->insert(4, "state");
    m_chartPriorityToHeapIndex->insert(4, DBEnvMxTelemetryStructIndex::e_state);
    m_chartList->insert(5, "active");
    m_chartPriorityToHeapIndex->insert(5, DBEnvMxTelemetryStructIndex::e_active);
    m_chartList->insert(6, "recovering");
    m_chartPriorityToHeapIndex->insert(6, DBEnvMxTelemetryStructIndex::e_recovering);
    m_chartList->insert(7, "replicating");
    m_chartPriorityToHeapIndex->insert(7, DBEnvMxTelemetryStructIndex::e_replicating);
    m_chartList->insert(8, "nDBs");
    m_chartPriorityToHeapIndex->insert(8, DBEnvMxTelemetryStructIndex::e_nDBs);

    m_chartList->insert(9, "nHosts");
    m_chartPriorityToHeapIndex->insert(9, DBEnvMxTelemetryStructIndex::e_nHosts);
    m_chartList->insert(10, "nPeers");
    m_chartPriorityToHeapIndex->insert(10, DBEnvMxTelemetryStructIndex::e_nPeers);
    m_chartList->insert(11, "nCxns");
    m_chartPriorityToHeapIndex->insert(11, DBEnvMxTelemetryStructIndex::e_nCxns);
    m_chartList->insert(12, "heartbeatFreq");
    m_chartPriorityToHeapIndex->insert(12, DBEnvMxTelemetryStructIndex::e_heartbeatFreq);
    m_chartList->insert(13, "heartbeatTimeout");
    m_chartPriorityToHeapIndex->insert(13, DBEnvMxTelemetryStructIndex::e_heartbeatTimeout);

    m_chartList->insert(14, "reconnectFreq");
     m_chartPriorityToHeapIndex->insert(14, DBEnvMxTelemetryStructIndex::e_reconnectFreq);
    m_chartList->insert(15, "electionTimeout");
     m_chartPriorityToHeapIndex->insert(15, DBEnvMxTelemetryStructIndex::e_electionTimeout);
    m_chartList->insert(16, "writeThread");
     m_chartPriorityToHeapIndex->insert(16, DBEnvMxTelemetryStructIndex::e_writeThread);

    // extra
    m_chartList->insert(17, "none");
}


double MxTelemetryDBEnvWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
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


QPair<void*, int> MxTelemetryDBEnvWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    ZdbEnv::Telemetry* l_data = static_cast<ZdbEnv::Telemetry*>(a_mxTelemetryMsg);

    QPair<void*, int> l_result;
    switch (a_index) {
    case DBEnvMxTelemetryStructIndex::e_nCxns:
        l_result.first = &l_data->nCxns;  //not tested
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_heartbeatFreq:
        l_result.first = &l_data->heartbeatFreq;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_heartbeatTimeout:
        l_result.first = &l_data->heartbeatTimeout;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_reconnectFreq:
        l_result.first = &l_data->reconnectFreq;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_electionTimeout:
        l_result.first = &l_data->electionTimeout;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_self:
        l_result.first = &l_data->self;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_master:
        l_result.first = &l_data->master;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_prev:
        l_result.first = &l_data->prev;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_next:
        l_result.first = &l_data->next;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_writeThread:
        l_result.first = &l_data->writeThread;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_nHosts:
        l_result.first = &l_data->nHosts;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_nPeers:
        l_result.first = &l_data->nPeers;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_nDBs:
        l_result.first = &l_data->nDBs;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_state:
        l_result.first = &l_data->state;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_active:
        l_result.first = &l_data->active;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_recovering:
        l_result.first = &l_data->recovering;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case DBEnvMxTelemetryStructIndex::e_replicating:
        l_result.first = &l_data->replicating;
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












