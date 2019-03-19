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
#include "MxTelemetryEngineWrapper.h"
#include "QList"
#include "QDebug"

MxTelemetryEngineWrapper::MxTelemetryEngineWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryEngineWrapper");
}


MxTelemetryEngineWrapper::~MxTelemetryEngineWrapper()
{
}


void MxTelemetryEngineWrapper::initActiveDataSet() noexcept
{
    // 0=, 1=
    m_activeDataSet = {0, 1};
}

void MxTelemetryEngineWrapper::initTableList() noexcept
{
    // removed irrelvant for table representation
    m_tableList->reserve(12);
    m_tableList->insert(0, "time");
    m_tableList->insert(1, "state");
    m_tableList->insert(2, "nLinks");
    m_tableList->insert(3, "up");
    m_tableList->insert(4, "down");
    m_tableList->insert(5, "disabled");
    m_tableList->insert(6, "transient");
    m_tableList->insert(7, "reconn");
    m_tableList->insert(8, "failed");
    m_tableList->insert(9, "mxID");
    m_tableList->insert(10, "rxThread");
    m_tableList->insert(11, "txThread");
}


void MxTelemetryEngineWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{

}


void MxTelemetryEngineWrapper::initChartList() noexcept
{
    // removed irrelvant for chart representation
    m_chartList->reserve(8);
    m_chartPriorityToHeapIndex->reserve(7); // without none

    m_chartList->insert(0, "nLinks");
    m_chartPriorityToHeapIndex->insert(0, EngineMxTelemetryStructIndex::e_nLinks);

    m_chartList->insert(1, "up");
    m_chartPriorityToHeapIndex->insert(1, EngineMxTelemetryStructIndex::e_up);

    m_chartList->insert(2, "down");
    m_chartPriorityToHeapIndex->insert(2, EngineMxTelemetryStructIndex::e_down);

    m_chartList->insert(3, "disable");
    m_chartPriorityToHeapIndex->insert(3, EngineMxTelemetryStructIndex::e_disabled);

    m_chartList->insert(4, "transient");
    m_chartPriorityToHeapIndex->insert(4, EngineMxTelemetryStructIndex::e_transient);

    m_chartList->insert(5, "reconn");
    m_chartPriorityToHeapIndex->insert(5, EngineMxTelemetryStructIndex::e_reconn);

    m_chartList->insert(6, "failed");
    m_chartPriorityToHeapIndex->insert(6, EngineMxTelemetryStructIndex::e_failed);


    // extra
    m_chartList->insert(7, "none");
}


double MxTelemetryEngineWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
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



QPair<void*, int> MxTelemetryEngineWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    MxEngine::Telemetry* l_data = static_cast<MxEngine::Telemetry*>(a_mxTelemetryMsg);

    QPair<void*, int> l_result;
    switch (a_index) {
    case EngineMxTelemetryStructIndex::e_id:
        l_result.first = l_data->id.data();  //not tested
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case EngineMxTelemetryStructIndex::e_mxID:
        l_result.first = l_data->mxID.data(); //not tested
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case EngineMxTelemetryStructIndex::e_down:
        l_result.first = &l_data->down;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_disabled:
        l_result.first = &l_data->disabled;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_transient:
        l_result.first = &l_data->transient;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_up:
        l_result.first = &l_data->up;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_reconn:
        l_result.first = &l_data->reconn;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_failed:
        l_result.first = &l_data->failed;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_nLinks:
        l_result.first = &l_data->nLinks;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case EngineMxTelemetryStructIndex::e_rxThread:
        l_result.first = &l_data->rxThread;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case EngineMxTelemetryStructIndex::e_txThread:
        l_result.first = &l_data->txThread;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case EngineMxTelemetryStructIndex::e_state:
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














