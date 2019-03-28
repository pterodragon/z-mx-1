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
#include "QLinkedList"


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
    // 2=inCount, 4=outCount
    m_activeDataSet = {2, 4};
}


void MxTelemetryQueueWrapper::initTableList() noexcept
{
    // the index for each catagory
    int i = 0;
    m_tableList->insert(i, "time");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::GET_CURRENT_TIME);

    m_tableList->insert(i, "type");
    m_tablePriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_type);

    m_tableList->insert(i, "full");
    m_tablePriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_full);

    m_tableList->insert(i, "size");
    m_tablePriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_size);

    m_tableList->insert(i, "count");
    m_tablePriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_count);

    m_tableList->insert(i, "seqNo");
    m_tablePriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_seqNo);

    m_tableList->insert(i, "inCount");
    m_tablePriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_inCount);

    m_tableList->insert(i, "inBytes");
    m_tablePriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_inBytes);

    m_tableList->insert(i, "outCount");
    m_tablePriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_outCount);

    m_tableList->insert(i, "outBytes");
    m_tablePriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_outBytes);
}


void MxTelemetryQueueWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    QPair<void*, int> l_dataPair;

    for (auto i = 0; i < m_tableList->count(); i++)
    {
        switch (const auto l_index = m_tablePriorityToStructIndex->at(i)) {
        case OTHER_ACTIONS::GET_CURRENT_TIME:
            a_result.append(QString::fromStdString(getCurrentTime()));
            break;
        case QueueMxTelemetryStructIndex::e_type:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(MxTelemetry::QueueType::name(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case QueueMxTelemetryStructIndex::e_full:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case QueueMxTelemetryStructIndex::e_size:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case QueueMxTelemetryStructIndex::e_count:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case QueueMxTelemetryStructIndex::e_seqNo:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case QueueMxTelemetryStructIndex::e_inCount:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case QueueMxTelemetryStructIndex::e_inBytes:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case QueueMxTelemetryStructIndex::e_outCount:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case QueueMxTelemetryStructIndex::e_outBytes:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
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


void MxTelemetryQueueWrapper::initChartList() noexcept
{
    int i = 0;

    m_chartList->insert(i, "seqNo");
    m_chartPriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_seqNo);

    m_chartList->insert(i, "count");
    m_chartPriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_count);

    m_chartList->insert(i, "inCount");
    m_chartPriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_inCount);

    m_chartList->insert(i, "inBytes");
    m_chartPriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_inBytes);

    m_chartList->insert(i, "outCount");
    m_chartPriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_outCount);

    m_chartList->insert(i, "outBytes");
    m_chartPriorityToStructIndex->insert(i++, QueueMxTelemetryStructIndex::e_outBytes);

    // extra
    m_chartList->insert(i++, "none");
}


double MxTelemetryQueueWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

    return typeConvertor<double>(QPair(l_dataPair.first, l_dataPair.second));
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
                    << __PRETTY_FUNCTION__
                    << "unsupported struct index"
                    << a_index;
        l_result.first = nullptr;
        l_result.second = CONVERT_FRON::type_none;
        break;
    }
    return l_result;
}


const std::string MxTelemetryQueueWrapper::getPrimaryKey(const std::initializer_list<std::string>& a_list) const noexcept
{
    return std::string(a_list.begin()[0]).append(NAME_DELIMITER).append(a_list.begin()[1]);
}


