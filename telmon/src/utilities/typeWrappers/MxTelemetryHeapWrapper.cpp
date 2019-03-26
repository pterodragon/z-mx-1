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



#include "MxTelemetryHeapWrapper.h"
#include "QList"
#include "ZmHeap.hpp"
#include <cstdint>
#include "QDebug"
#include "QPair"
#include "QLinkedList"

MxTelemetryHeapWrapper::MxTelemetryHeapWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryHeapWrapper");
}


MxTelemetryHeapWrapper::~MxTelemetryHeapWrapper()
{
}


void MxTelemetryHeapWrapper::initActiveDataSet() noexcept
{
    // 0=cacheAllocs, 1=heapAllocs
    m_activeDataSet = {0, 1};
}


void MxTelemetryHeapWrapper::initTableList() noexcept
{
    // the index for each catagory
    int i = 0;
    m_tableList->insert(i, "time");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::GET_CURRENT_TIME);

    m_tableList->insert(i, "size");
    m_tablePriorityToStructIndex->insert(i++, MxTelemetryHeapWrapper::e_size);

    m_tableList->insert(i, "alignment");
    m_tablePriorityToStructIndex->insert(i++, MxTelemetryHeapWrapper::e_alignment);

    m_tableList->insert(i, "partition");
    m_tablePriorityToStructIndex->insert(i++, MxTelemetryHeapWrapper::e_partition);

    m_tableList->insert(i, "sharded");
    m_tablePriorityToStructIndex->insert(i++, MxTelemetryHeapWrapper::e_sharded);

    m_tableList->insert(i, "cacheSize");
    m_tablePriorityToStructIndex->insert(i++, MxTelemetryHeapWrapper::e_cacheSize);

    m_tableList->insert(i, "cpuset");
    m_tablePriorityToStructIndex->insert(i++, MxTelemetryHeapWrapper::e_cpuset);

    m_tableList->insert(i, "cacheAllocs");
    m_tablePriorityToStructIndex->insert(i++, MxTelemetryHeapWrapper::e_cacheAllocs);

    m_tableList->insert(i, "heapAllocs");
    m_tablePriorityToStructIndex->insert(i++, MxTelemetryHeapWrapper::e_heapAllocs);

    m_tableList->insert(i, "frees");
    m_tablePriorityToStructIndex->insert(i++, MxTelemetryHeapWrapper::e_frees);

    m_tableList->insert(i, "allocated");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::HEAP_MXTYPE_CALCULATE_ALLOCATED);

}


void MxTelemetryHeapWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    uint64_t l_otherResult;
    QPair<void*, int> l_dataPair;

    for (auto i = 0; i < m_tableList->count(); i++)
    {
        switch (const auto l_index = m_tablePriorityToStructIndex->at(i)) {
        case OTHER_ACTIONS::GET_CURRENT_TIME:
            a_result.append(QString::fromStdString(getCurrentTime()));
            break;
        case OTHER_ACTIONS::HEAP_MXTYPE_CALCULATE_ALLOCATED:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHeapTelemetryStructIndex::e_cacheSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHeapTelemetryStructIndex::e_cpuset:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHeapTelemetryStructIndex::e_cacheAllocs:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHeapTelemetryStructIndex::e_heapAllocs:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHeapTelemetryStructIndex::e_frees:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHeapTelemetryStructIndex::e_size:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHeapTelemetryStructIndex::e_partition:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint16_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHeapTelemetryStructIndex::e_sharded:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHeapTelemetryStructIndex::e_alignment:
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


void MxTelemetryHeapWrapper::initChartList() noexcept
{
    int i = 0;
    m_chartList->insert(i, "cacheAllocs");
    m_chartPriorityToStructIndex->insert(i++, ZmHeapTelemetryStructIndex::e_cacheAllocs);

    m_chartList->insert(i, "heapAllocs");
    m_chartPriorityToStructIndex->insert(i++, ZmHeapTelemetryStructIndex::e_heapAllocs);

    m_chartList->insert(i, "frees");
    m_chartPriorityToStructIndex->insert(i++, ZmHeapTelemetryStructIndex::e_frees);

    // extra
    m_chartList->insert(i++, "none");
}


double MxTelemetryHeapWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    uint64_t l_otherResult;

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);

    return typeConvertor<double>(QPair(l_dataPair.first, l_dataPair.second));
}

QPair<void*, int> MxTelemetryHeapWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg,
                                                                 const int a_index,
                                                                 void* a_otherResult) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    ZmHeapTelemetry* l_data = static_cast<ZmHeapTelemetry*>(a_mxTelemetryMsg);
    QPair<void*, int> l_result;
    switch (a_index) {
    case ZmHeapTelemetryStructIndex::e_id:
        l_result.first = &l_data->id;
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case ZmHeapTelemetryStructIndex::e_cacheSize:
        l_result.first = &l_data->cacheSize;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmHeapTelemetryStructIndex::e_cpuset:
        l_result.first = &l_data->cpuset;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmHeapTelemetryStructIndex::e_cacheAllocs:
        l_result.first = &l_data->cacheAllocs;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmHeapTelemetryStructIndex::e_heapAllocs:
        l_result.first = &l_data->heapAllocs;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmHeapTelemetryStructIndex::e_frees:
        l_result.first = &l_data->frees;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmHeapTelemetryStructIndex::e_size:
        l_result.first = &l_data->size;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZmHeapTelemetryStructIndex::e_partition:
        l_result.first = &l_data->partition;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZmHeapTelemetryStructIndex::e_sharded:
        l_result.first = &l_data->sharded;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case ZmHeapTelemetryStructIndex::e_alignment:
        l_result.first = &l_data->alignment;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case OTHER_ACTIONS::HEAP_MXTYPE_CALCULATE_ALLOCATED:
        *(static_cast<uint64_t*>(a_otherResult)) = l_data->cacheAllocs + l_data->heapAllocs - l_data->frees;
        l_result.first = a_otherResult;
        l_result.second = CONVERT_FRON::type_uint64_t;
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


