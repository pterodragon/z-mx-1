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
#include "QLinkedList"

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
    // 0=cpuUsage, 1=none
    m_activeDataSet = {0, 1};
}


void MxTelemetryZmThreadWrapper::initTableList() noexcept
{
    // the index for each catagory
    int i = 0;
    m_tableList->insert(i, "time");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::GET_CURRENT_TIME);

    m_tableList->insert(i, "id");
    m_tablePriorityToStructIndex->insert(i++, ZmThreadTelemetryStructIndex::e_id);

    m_tableList->insert(i, "tid");
    m_tablePriorityToStructIndex->insert(i++, ZmThreadTelemetryStructIndex::e_tid);

    m_tableList->insert(i, "cpuUsage");
    m_tablePriorityToStructIndex->insert(i++, ZmThreadTelemetryStructIndex::e_cpuUsage);

    m_tableList->insert(i, "cpuset");
    m_tablePriorityToStructIndex->insert(i++, ZmThreadTelemetryStructIndex::e_cpuset);

    m_tableList->insert(i, "priority");
    m_tablePriorityToStructIndex->insert(i++, ZmThreadTelemetryStructIndex::e_priority);

    m_tableList->insert(i, "stackSize");
    m_tablePriorityToStructIndex->insert(i++, ZmThreadTelemetryStructIndex::e_stackSize);

    m_tableList->insert(i, "partition");
    m_tablePriorityToStructIndex->insert(i++, ZmThreadTelemetryStructIndex::e_partition);

    m_tableList->insert(i, "main");
    m_tablePriorityToStructIndex->insert(i++, ZmThreadTelemetryStructIndex::e_main);

    m_tableList->insert(i, "detached");
    m_tablePriorityToStructIndex->insert(i++, ZmThreadTelemetryStructIndex::e_detached);

}


void MxTelemetryZmThreadWrapper::_getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    double l_otherResult;
    QPair<void*, int> l_dataPair;

    for (auto i = 0; i < m_tableList->count(); i++)
    {
        switch (const auto l_index = m_tablePriorityToStructIndex->at(i)) {
        case OTHER_ACTIONS::GET_CURRENT_TIME:
            a_result.append(QString::fromStdString(getCurrentTime()));
            break;
        case ZmThreadTelemetryStructIndex::e_id:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<int32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmThreadTelemetryStructIndex::e_tid:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmThreadTelemetryStructIndex::e_cpuUsage:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<double>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    ),
                                'f', 2) //take 2 digits precise
                            );
            break;
        case ZmThreadTelemetryStructIndex::e_cpuset:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(streamToQString(ZmBitmap(
                                                typeConvertor<uint64_t>(
                                                    QPair(l_dataPair.first, l_dataPair.second)
                                                    )
                                                )
                                            )
                            );
            break;
        case ZmThreadTelemetryStructIndex::e_priority:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<int32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmThreadTelemetryStructIndex::e_stackSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmThreadTelemetryStructIndex::e_partition:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint16_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmThreadTelemetryStructIndex::e_main:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmThreadTelemetryStructIndex::e_detached:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
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


void MxTelemetryZmThreadWrapper::initChartList() noexcept
{
    int i = 0;
    m_chartList->insert(i, "cpuUsage");
    m_chartPriorityToStructIndex->insert(i++, ZmThreadTelemetryStructIndex::e_cpuUsage);

    // extra
    m_chartList->insert(i, "none");
}


int MxTelemetryZmThreadWrapper::_getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    double l_otherResult; // for cpuUsage

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);

    return typeConvertor<int>(QPair(l_dataPair.first, l_dataPair.second));

}


QPair<void*, int> MxTelemetryZmThreadWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg,
                                                                     const int a_index,
                                                                     void* a_otherResult) const noexcept
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
        *(static_cast<double*>(a_otherResult)) = static_cast<double>(l_data->cpuUsage  * 100.0);
        l_result.first = a_otherResult;
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
                    << __PRETTY_FUNCTION__
                    << "unsupported struct index"
                    << a_index;
        l_result.first = nullptr;
        l_result.second = CONVERT_FRON::type_none;
        break;
    }
    return l_result;
}


const QString MxTelemetryZmThreadWrapper::_getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept
{
     ZmThreadTelemetry* l_data = static_cast<ZmThreadTelemetry*>(a_mxTelemetryMsg);
     if (l_data)
     {
         return  QString(l_data->name).append(NAME_DELIMITER).append(QString::number(l_data->tid));;
     } else
     {
        return QString();
     }
}














