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
#include "QLinkedList"

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
    // 0=none, 0=none
    m_activeDataSet = {0, 0};
}


void MxTelemetryDBEnvWrapper::initTableList() noexcept
{
    // the index for each catagory
    int i = 0;
    m_tableList->insert(i, "time");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::GET_CURRENT_TIME);

    m_tableList->insert(i, "self");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_self);

    m_tableList->insert(i, "master");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_master);

    m_tableList->insert(i, "prev");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_prev);

    m_tableList->insert(i, "next");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_next);

    m_tableList->insert(i, "state");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_state);

    m_tableList->insert(i, "active");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_active);

    m_tableList->insert(i, "recovering");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_recovering);

    m_tableList->insert(i, "replicating");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_replicating);

    m_tableList->insert(i, "nDBs");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_nDBs);

    m_tableList->insert(i, "nHosts");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_nHosts);

    m_tableList->insert(i, "nPeers");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_nPeers);

    m_tableList->insert(i, "nCxns");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_nCxns);

    m_tableList->insert(i, "heartbeatFreq");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_heartbeatFreq);

    m_tableList->insert(i, "heartbeatTimeout");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_heartbeatTimeout);

    m_tableList->insert(i, "reconnectFreq");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_reconnectFreq);

    m_tableList->insert(i, "electionTimeout");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_electionTimeout);

    m_tableList->insert(i, "writeThread");
    m_tablePriorityToStructIndex->insert(i++, DBEnvMxTelemetryStructIndex::e_writeThread);
}


void MxTelemetryDBEnvWrapper::_getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    QPair<void*, int> l_dataPair;

    for (auto i = 0; i < m_tableList->count(); i++)
    {
        switch (const auto l_index = m_tablePriorityToStructIndex->at(i)) {
        case OTHER_ACTIONS::GET_CURRENT_TIME:
            a_result.append(QString::fromStdString(getCurrentTime()));
            break;
        case DBEnvMxTelemetryStructIndex::e_self:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_master:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_prev:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_next:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_state:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(ZdbHost::stateName(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_active:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_recovering:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_replicating:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_nDBs:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_nHosts:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_nPeers:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_nCxns:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_heartbeatFreq:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_heartbeatTimeout:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_reconnectFreq:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_electionTimeout:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBEnvMxTelemetryStructIndex::e_writeThread:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint16_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        default:
            qCritical() << *m_className
                        << __func__
                        << "unsupported index"
                        << l_index;
            break;
        }
    }
}


void MxTelemetryDBEnvWrapper::initChartList() noexcept
{
    int i = 0;
    // extra
    m_chartList->insert(i++, "none");
}


int MxTelemetryDBEnvWrapper::_getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

    return typeConvertor<int>(QPair(l_dataPair.first, l_dataPair.second));
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
                    << __PRETTY_FUNCTION__
                    << "unsupported struct index"
                    << a_index;
        l_result.first = nullptr;
        l_result.second = CONVERT_FRON::type_none;
        break;
    }
    return l_result;
}


const QString MxTelemetryDBEnvWrapper::_getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept
{
    ZdbEnv::Telemetry* l_data = static_cast<ZdbEnv::Telemetry*>(a_mxTelemetryMsg);
    if (l_data)
    {
        return QString::number(l_data->self);
    } else {
        return QString();
    }
}


const QString MxTelemetryDBEnvWrapper::_getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept
{
    const auto* const l_data = static_cast<const ZdbEnv::Telemetry* const>(a_mxTelemetryMsg);

    const auto l_result = _title     +  _getPrimaryKey(a_mxTelemetryMsg) + _Bold_end
            + _time             +  getCurrentTimeQTImpl(TIME_FORMAT__hh_mm_ss)
            + _self             +  QString::number(l_data->self)
            + _master           +  QString::number(l_data->master)
            + _prev             +  QString::number(l_data->prev)
            + _next             +  QString::number(l_data->next)

            + getColor(l_data->state)
            + _state            +  ZdbHost::stateName(l_data->state)
            + _color_off

            + _active           +  QString::number(l_data->active)
            + _recovering       +  QString::number(l_data->recovering)
            + _replicating      +  QString::number(l_data->replicating)
            + _nDBs             +  QString::number(l_data->nDBs)

            + _nHosts           +  QString::number(l_data->nHosts)
            + _nPeers           +  QString::number(l_data->nPeers)
            + _nCxns            +  QString::number(l_data->nCxns)
            + _heartbeatFreq    +  QString::number(l_data->heartbeatFreq)
            + _heartbeatTimeout +  QString::number(l_data->heartbeatTimeout)
            + _reconnectFreq    +  QString::number(l_data->reconnectFreq)
            + _electionTimeout  +  QString::number(l_data->electionTimeout)
            + _writeThread      +  QString::number(l_data->writeThread)
            ;

    return l_result;
}

//ZdbHost::
//    Instantiated = 0,	// Red - instantiated, init() not yet called
//    Initialized,	    // Amber - init() called
//    Stopped,		    // Amber - open() called
//    Electing,		    // Amber - start() called, determining active/inactive
//    Activating,		// Amber - activating application
//    Active,		    // Green - active (master)
//    Deactivating,	    // Amber - deactivating application
//    Inactive,		    // Amber - inactive (client)
//    Stopping		    // Amber - stop() called - stopping
const QString& MxTelemetryDBEnvWrapper::getColor(const int a_state) const noexcept{
    switch (a_state) {
    case ZdbHost::Active:
        return _color_green;
        break;
    case ZdbHost::Instantiated:
        return _color_red;
        break;
    default:
        return _color_amber;
        break;
    }
}







