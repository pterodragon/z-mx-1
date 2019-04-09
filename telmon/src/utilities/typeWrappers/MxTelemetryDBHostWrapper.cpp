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
#include "QLinkedList"


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
    // 0=none, 0=none
    m_activeDataSet = {0, 0};
}


void MxTelemetryDBHostWrapper::initTableList() noexcept
{
    // the index for each catagory
    int i = 0;
    m_tableList->insert(i, "time");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::GET_CURRENT_TIME);

    m_tableList->insert(i, "priority");
    m_tablePriorityToStructIndex->insert(i++, DBHostMxTelemetryStructIndex::e_priority);

    m_tableList->insert(i, "state");
    m_tablePriorityToStructIndex->insert(i++, DBHostMxTelemetryStructIndex::e_state);

    m_tableList->insert(i, "voted");
    m_tablePriorityToStructIndex->insert(i++, DBHostMxTelemetryStructIndex::e_voted);

    m_tableList->insert(i, "ip");
    m_tablePriorityToStructIndex->insert(i++, DBHostMxTelemetryStructIndex::e_ip);

    m_tableList->insert(i, "port");
    m_tablePriorityToStructIndex->insert(i++, DBHostMxTelemetryStructIndex::e_port);
}


void MxTelemetryDBHostWrapper::_getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    QPair<void*, int> l_dataPair;

    for (auto i = 0; i < m_tableList->count(); i++)
    {
        switch (const auto l_index = m_tablePriorityToStructIndex->at(i)) {
        case OTHER_ACTIONS::GET_CURRENT_TIME:
            a_result.append(QString::fromStdString(getCurrentTime()));
            break;
        case DBHostMxTelemetryStructIndex::e_priority:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBHostMxTelemetryStructIndex::e_state:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(ZdbHost::stateName(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBHostMxTelemetryStructIndex::e_voted:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBHostMxTelemetryStructIndex::e_ip:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(streamToQString(
                                *(static_cast<ZiIP*>(l_dataPair.first))
                                )
                            );
            break;
            break;
        case DBHostMxTelemetryStructIndex::e_port:
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


void MxTelemetryDBHostWrapper::initChartList() noexcept
{
    int i = 0;

    // extra
    m_chartList->insert(i++, "none");
}


int MxTelemetryDBHostWrapper::_getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

    return typeConvertor<int>(QPair(l_dataPair.first, l_dataPair.second));
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
                    << __PRETTY_FUNCTION__
                    << "unsupported struct index"
                    << a_index;
        l_result.first = nullptr;
        l_result.second = CONVERT_FRON::type_none;
        break;
    }
    return l_result;
}


const QString MxTelemetryDBHostWrapper::_getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept
{
    ZdbHost::Telemetry* l_data = static_cast<ZdbHost::Telemetry*>(a_mxTelemetryMsg);
    if (l_data)
    {
        return QString(l_data->id);
    } else {
        return QString();
    }
}






