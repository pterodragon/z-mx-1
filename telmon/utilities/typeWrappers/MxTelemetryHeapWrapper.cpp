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

MxTelemetryHeapWrapper::MxTelemetryHeapWrapper():
    m_tableList(new QList<QString>),
    m_chartList(new QVector<QString>),
    m_chartPriorityToHeapIndex(new QVector<int>)
{
    initTableList();
    initChartList();
    initActiveDataSet();
}


MxTelemetryHeapWrapper::~MxTelemetryHeapWrapper()
{
    delete m_tableList;
    delete m_chartList;
    delete m_chartPriorityToHeapIndex;
}


void MxTelemetryHeapWrapper::initActiveDataSet() noexcept
{
    // 0=size, 1=alignment
    m_activeDataSet = {0, 1};
}


void MxTelemetryHeapWrapper::initTableList() noexcept
{
    // removed irrelvant for table representation
    m_tableList->reserve(11);
    m_tableList->insert(0, "time");
//    m_tableList->insert(1, "id");
    m_tableList->insert(1, "size");
    m_tableList->insert(2, "alignment");
    m_tableList->insert(3, "partition");
    m_tableList->insert(4, "sharded");
    m_tableList->insert(5, "cacheSize");
    m_tableList->insert(6, "cpuset");
    m_tableList->insert(7, "cacheAllocs");
    m_tableList->insert(8, "heapAllocs");
    m_tableList->insert(9, "frees");
    m_tableList->insert(10, "allocated");
}


void MxTelemetryHeapWrapper::initChartList() noexcept
{
    // removed irrelvant for chart representation
    m_chartList->reserve(11);
    m_chartPriorityToHeapIndex->reserve(10); // without none
//    m_chartList->insert(0, "time");
//    m_chartList->insert(1, "id");


    m_chartList->insert(0, "size");
    m_chartPriorityToHeapIndex->insert(0, ZmHeapSelemetryStructIndex::e_size);

    m_chartList->insert(1, "alignment");
    m_chartPriorityToHeapIndex->insert(1, ZmHeapSelemetryStructIndex::e_alignment);

    m_chartList->insert(2, "partition");
    m_chartPriorityToHeapIndex->insert(2, ZmHeapSelemetryStructIndex::e_partition);

    m_chartList->insert(3, "sharded");
    m_chartPriorityToHeapIndex->insert(3, ZmHeapSelemetryStructIndex::e_sharded);

    m_chartList->insert(4, "cacheSize");
    m_chartPriorityToHeapIndex->insert(4, ZmHeapSelemetryStructIndex::e_cacheSize);

    m_chartList->insert(5, "cpuset");
    m_chartPriorityToHeapIndex->insert(5, ZmHeapSelemetryStructIndex::e_cpuset);

    m_chartList->insert(6, "cacheAllocs");
    m_chartPriorityToHeapIndex->insert(6, ZmHeapSelemetryStructIndex::e_cacheAllocs);

    m_chartList->insert(7, "heapAllocs");
    m_chartPriorityToHeapIndex->insert(7, ZmHeapSelemetryStructIndex::e_heapAllocs);

    m_chartList->insert(8, "frees");
    m_chartPriorityToHeapIndex->insert(8, ZmHeapSelemetryStructIndex::e_frees);

    m_chartList->insert(9, "allocated");
    m_chartPriorityToHeapIndex->insert(9, 100); // 100 representing not supported
    // extra
    m_chartList->insert(10, "none");
}


const QList<QString>& MxTelemetryHeapWrapper::getTableList() const noexcept
{
    return *m_tableList;
}


const QVector<QString>& MxTelemetryHeapWrapper::getChartList() const noexcept
{
    return *m_chartList;
}


const std::array<int, 2>&  MxTelemetryHeapWrapper::getActiveDataSet() const noexcept
{
    return m_activeDataSet;
}


bool MxTelemetryHeapWrapper::isDataTypeNotUsed(const int a_index) const noexcept
{
    // for now,
    // we denote the last element in the conatiner as none
    // to do: maybe implement as map
    return a_index == (m_chartList->size() - 1);
}


double MxTelemetryHeapWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
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

    if (a_index == 9) // case: allocated
    {
        const ZmHeapTelemetry* l_data = static_cast<ZmHeapTelemetry*>(a_mxTelemetryMsg);
        uint64_t l_allocated = l_data->cacheAllocs + l_data->heapAllocs - l_data->frees;
        return typeConvertor(QPair(&l_allocated, CONVERT_FRON::type_uint64_t));
    }

    const int l_index = m_chartPriorityToHeapIndex->at(a_index);
    const QPair<void*, int> l_dataPair = getHeapDataType(a_mxTelemetryMsg, l_index);

    switch (l_dataPair.second) {
    case CONVERT_FRON::type_uint64_t:
        l_result = typeConvertor(QPair(l_dataPair.first, CONVERT_FRON::type_uint64_t));
        break;
    case CONVERT_FRON::type_uint32_t:
        l_result = typeConvertor(QPair(l_dataPair.first, CONVERT_FRON::type_uint32_t));
        break;
    case CONVERT_FRON::type_uint16_t:
        l_result = typeConvertor(QPair(l_dataPair.first, CONVERT_FRON::type_uint16_t));
        break;
    case CONVERT_FRON::type_uint8_t:
        l_result = typeConvertor(QPair(l_dataPair.first, CONVERT_FRON::type_uint8_t));
        break;
    default:
        qDebug() << "Error, unknown conversion a_index=" << a_index
                 << "convet from: " << l_dataPair.second
                 << "returning default value" << l_result;
        break;
    }

    return l_result;
}


QPair<void*, int> MxTelemetryHeapWrapper::getHeapDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    ZmHeapTelemetry* l_data = static_cast<ZmHeapTelemetry*>(a_mxTelemetryMsg);
    QPair<void*, int> l_result;
    switch (a_index) {
    case ZmHeapSelemetryStructIndex::e_id:
        l_result.first = &l_data->id;
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case ZmHeapSelemetryStructIndex::e_cacheSize:
        l_result.first = &l_data->cacheSize;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmHeapSelemetryStructIndex::e_cpuset:
        l_result.first = &l_data->cpuset;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmHeapSelemetryStructIndex::e_cacheAllocs:
        l_result.first = &l_data->cacheAllocs;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmHeapSelemetryStructIndex::e_heapAllocs:
        l_result.first = &l_data->heapAllocs;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmHeapSelemetryStructIndex::e_frees:
        l_result.first = &l_data->frees;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZmHeapSelemetryStructIndex::e_size:
        l_result.first = &l_data->size;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZmHeapSelemetryStructIndex::e_partition:
        l_result.first = &l_data->partition;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZmHeapSelemetryStructIndex::e_sharded:
        l_result.first = &l_data->sharded;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case ZmHeapSelemetryStructIndex::e_alignment:
        l_result.first = &l_data->alignment;
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


