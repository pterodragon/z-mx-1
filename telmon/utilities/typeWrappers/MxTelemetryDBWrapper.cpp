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
#include "MxTelemetryDBWrapper.h"
#include "QList"
#include "QDebug"

MxTelemetryDBWrapper::MxTelemetryDBWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
}


MxTelemetryDBWrapper::~MxTelemetryDBWrapper()
{
}


void MxTelemetryDBWrapper::initActiveDataSet() noexcept
{
    // 0=, 1=
    m_activeDataSet = {0, 1};
}


void MxTelemetryDBWrapper::initTableList() noexcept
{
    // removed irrelvant for table representation
    m_tableList->reserve(19);
    m_tableList->insert(0, "time");
    m_tableList->insert(1, "name");
    m_tableList->insert(2, "id");
    m_tableList->insert(3, "recSize");
    m_tableList->insert(4, "compress");

    m_tableList->insert(5, "cacheMode");
    m_tableList->insert(6, "cacheSize");
    m_tableList->insert(7, "path");
    m_tableList->insert(8, "fileSize");
    m_tableList->insert(9, "fileRecs");

    m_tableList->insert(10, "filesMax");
    m_tableList->insert(11, "preAlloc");
    m_tableList->insert(12, "minRN");
    m_tableList->insert(13, "allocRN");
    m_tableList->insert(14, "fileRN");

    m_tableList->insert(15, "cacheLoads");
    m_tableList->insert(16, "cacheMisses");
    m_tableList->insert(17, "fileLoads");
    m_tableList->insert(18, "fileMisses");
}


void MxTelemetryDBWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{

}


void MxTelemetryDBWrapper::initChartList() noexcept
{
    // removed irrelvant for chart representation
    m_chartList->reserve(19);
    m_chartPriorityToHeapIndex->reserve(18); // without none

//    m_chartList->insert(0, "time");
    m_chartList->insert(0, "name");
    m_chartPriorityToHeapIndex->insert(0, DBMxTelemetryStructIndex::e_name);
    m_chartList->insert(1, "id");
    m_chartPriorityToHeapIndex->insert(1, DBMxTelemetryStructIndex::e_id);
    m_chartList->insert(2, "recSize");
    m_chartPriorityToHeapIndex->insert(2, DBMxTelemetryStructIndex::e_recSize);
    m_chartList->insert(3, "compress");
    m_chartPriorityToHeapIndex->insert(3, DBMxTelemetryStructIndex::e_compress);

    m_chartList->insert(4, "cacheMode");
    m_chartPriorityToHeapIndex->insert(4, DBMxTelemetryStructIndex::e_cacheMode);
    m_chartList->insert(5, "cacheSize");
    m_chartPriorityToHeapIndex->insert(5, DBMxTelemetryStructIndex::e_cacheSize);
    m_chartList->insert(6, "path");
    m_chartPriorityToHeapIndex->insert(6, DBMxTelemetryStructIndex::e_path);
    m_chartList->insert(7, "fileSize");
    m_chartPriorityToHeapIndex->insert(7, DBMxTelemetryStructIndex::e_fileSize);
    m_chartList->insert(8, "fileRecs");
    m_chartPriorityToHeapIndex->insert(8, DBMxTelemetryStructIndex::e_fileRecs);

    m_chartList->insert(9, "filesMax");
    m_chartPriorityToHeapIndex->insert(9, DBMxTelemetryStructIndex::e_filesMax);
    m_chartList->insert(10, "preAlloc");
    m_chartPriorityToHeapIndex->insert(10, DBMxTelemetryStructIndex::e_preAlloc);
    m_chartList->insert(11, "minRN");
    m_chartPriorityToHeapIndex->insert(11, DBMxTelemetryStructIndex::e_minRN);
    m_chartList->insert(12, "allocRN");
    m_chartPriorityToHeapIndex->insert(12, DBMxTelemetryStructIndex::e_allocRN);
    m_chartList->insert(13, "fileRN");
    m_chartPriorityToHeapIndex->insert(13, DBMxTelemetryStructIndex::e_fileRN);

    m_chartList->insert(14, "cacheLoads");
    m_chartPriorityToHeapIndex->insert(14, DBMxTelemetryStructIndex::e_cacheLoads);
    m_chartList->insert(15, "cacheMisses");
    m_chartPriorityToHeapIndex->insert(15, DBMxTelemetryStructIndex::e_cacheMisses);
    m_chartList->insert(16, "fileLoads");
    m_chartPriorityToHeapIndex->insert(16, DBMxTelemetryStructIndex::e_fileLoads);
    m_chartList->insert(17, "fileMisses");
    m_chartPriorityToHeapIndex->insert(17, DBMxTelemetryStructIndex::e_fileMisses);

    // extra
    m_chartList->insert(18, "none");
}


double MxTelemetryDBWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
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
        qDebug() << "Error, unknown conversion a_index=" << a_index
                 << "convet from: " << l_dataPair.second
                 << "returning default value" << l_result;
        break;
    }
    return l_result;
}



//    struct Telemetry {
//      typedef ZuStringN<124> Path;
//      typedef ZuStringN<28> Name;

//      Path	path;
//      Name	name;
//      uint64_t	fileSize;

//      uint64_t	minRN;
//      uint64_t	allocRN;
//      uint64_t	fileRN;

//      uint64_t	cacheLoads;
//      uint64_t	cacheMisses;
//      uint64_t	fileLoads;

//      uint64_t	fileMisses;
//      uint32_t	id;
//      uint32_t	preAlloc;

//      uint32_t	recSize;
//      uint32_t	fileRecs;
//      uint32_t	cacheSize;

//      uint32_t	filesMax;
//      uint8_t	compress;
//      int8_t	cacheMode;
//    };


QPair<void*, int> MxTelemetryDBWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    ZdbAny::Telemetry* l_data = static_cast<ZdbAny::Telemetry*>(a_mxTelemetryMsg);

    QPair<void*, int> l_result;
    switch (a_index) {
    case DBMxTelemetryStructIndex::e_path:
        l_result.first = l_data->path.data();
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case DBMxTelemetryStructIndex::e_name:
        l_result.first = l_data->name.data();
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case DBMxTelemetryStructIndex::e_fileSize:              // 3
        l_result.first = &l_data->fileSize;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case DBMxTelemetryStructIndex::e_minRN:
        l_result.first = &l_data->minRN;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case DBMxTelemetryStructIndex::e_allocRN:
        l_result.first = &l_data->allocRN;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case DBMxTelemetryStructIndex::e_fileRN:                // 6
        l_result.first = &l_data->fileRN;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case DBMxTelemetryStructIndex::e_cacheLoads:
        l_result.first = &l_data->cacheLoads;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case DBMxTelemetryStructIndex::e_cacheMisses:
        l_result.first = &l_data->cacheMisses;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case DBMxTelemetryStructIndex::e_fileLoads:             // 9
        l_result.first = &l_data->fileLoads;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case DBMxTelemetryStructIndex::e_fileMisses:
        l_result.first = &l_data->fileMisses;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case DBMxTelemetryStructIndex::e_id:
        l_result.first = &l_data->id;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBMxTelemetryStructIndex::e_preAlloc:              // 12
        l_result.first = &l_data->preAlloc;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBMxTelemetryStructIndex::e_recSize:
        l_result.first = &l_data->recSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBMxTelemetryStructIndex::e_fileRecs:
        l_result.first = &l_data->fileRecs;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBMxTelemetryStructIndex::e_cacheSize:             // 15
        l_result.first = &l_data->cacheSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBMxTelemetryStructIndex::e_filesMax:
        l_result.first = &l_data->filesMax;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case DBMxTelemetryStructIndex::e_compress:
        l_result.first = &l_data->compress;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case DBMxTelemetryStructIndex::e_cacheMode:             // 18
        l_result.first = &l_data->cacheMode;
        l_result.second = CONVERT_FRON::type_int8_t;
        break;
    default:
        qWarning() << "unknown DBMxTelemetryStructIndex, a_index" << a_index
                   << "retunring <nullptr CONVERT_FRON::none>";
        l_result.first = nullptr;
        l_result.second = CONVERT_FRON::type_none;
        break;
    }
    return l_result;
}















