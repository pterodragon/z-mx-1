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
#include "QLinkedList"

MxTelemetryDBWrapper::MxTelemetryDBWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryDBWrapper");
}


MxTelemetryDBWrapper::~MxTelemetryDBWrapper()
{
}


void MxTelemetryDBWrapper::initActiveDataSet() noexcept
{
    // 2=cacheLoads, 3=cacheMisses
    m_activeDataSet = {2, 3};
}


void MxTelemetryDBWrapper::initTableList() noexcept
{
    // the index for each catagory
    int i = 0;
    m_tableList->insert(i, "time");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::GET_CURRENT_TIME);

    m_tableList->insert(i, "name");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_name);

    m_tableList->insert(i, "id");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_id);

    m_tableList->insert(i, "recSize");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_recSize);

    m_tableList->insert(i, "compress");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_compress);

    m_tableList->insert(i, "cacheMode");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_cacheMode);

    m_tableList->insert(i, "cacheSize");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_cacheSize);

    m_tableList->insert(i, "path");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_path);

    m_tableList->insert(i, "fileSize");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_fileSize);

    m_tableList->insert(i, "fileRecs");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_fileRecs);

    m_tableList->insert(i, "filesMax");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_filesMax);

    m_tableList->insert(i, "preAlloc");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_preAlloc);

    m_tableList->insert(i, "minRN");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_minRN);

    m_tableList->insert(i, "nextRN");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_nextRN);

    m_tableList->insert(i, "fileRN");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_fileRN);

    m_tableList->insert(i, "cacheLoads");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_cacheLoads);

    m_tableList->insert(i, "cacheMisses");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_cacheMisses);

    m_tableList->insert(i, "fileLoads");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_fileLoads);

    m_tableList->insert(i, "fileMisses");
    m_tablePriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_fileMisses);
}


void MxTelemetryDBWrapper::_getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    QPair<void*, int> l_dataPair;

    for (auto i = 0; i < m_tableList->count(); i++)
    {
        switch (const auto l_index = m_tablePriorityToStructIndex->at(i)) {
        case OTHER_ACTIONS::GET_CURRENT_TIME:
            a_result.append(QString::fromStdString(getCurrentTime()));
            break;
        case DBMxTelemetryStructIndex::e_name:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(static_cast<char*>(l_dataPair.first));
            break;
        case DBMxTelemetryStructIndex::e_id:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_recSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_compress:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_cacheMode:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(ZdbCacheMode::name(
                                typeConvertor<int8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_cacheSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_path:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(static_cast<char*>(l_dataPair.first));
            break;
        case DBMxTelemetryStructIndex::e_fileSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_fileRecs:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_filesMax:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_preAlloc:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_minRN:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_nextRN:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_fileRN:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_cacheLoads:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_cacheMisses:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_fileLoads:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case DBMxTelemetryStructIndex::e_fileMisses:
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


void MxTelemetryDBWrapper::initChartList() noexcept
{
    int i = 0;
    m_chartList->insert(i, "nextRN");
    m_chartPriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_nextRN);

    m_chartList->insert(i, "fileRN");
    m_chartPriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_fileRN);

    m_chartList->insert(i, "cacheLoads");
    m_chartPriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_cacheLoads);

    m_chartList->insert(i, "cacheMisses");
    m_chartPriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_cacheMisses);

    m_chartList->insert(i, "fileLoads");
    m_chartPriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_fileLoads);

    m_chartList->insert(i, "fileMisses");
    m_chartPriorityToStructIndex->insert(i++, DBMxTelemetryStructIndex::e_fileMisses);


    // extra
    m_chartList->insert(i++, "none");
}


int MxTelemetryDBWrapper::_getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

    return typeConvertor<int>(QPair(l_dataPair.first, l_dataPair.second));
}


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
    case DBMxTelemetryStructIndex::e_nextRN:
        l_result.first = &l_data->nextRN;
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


const QString MxTelemetryDBWrapper::_getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept
{
     ZdbAny::Telemetry* l_data = static_cast<ZdbAny::Telemetry*>(a_mxTelemetryMsg);
     if (l_data)
     {
         return QString(l_data->name);
     } else
     {
        return QString();
     }
}


const QString MxTelemetryDBWrapper::_getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept
{
    const auto* const l_data = static_cast<ZdbAny::Telemetry*>(a_mxTelemetryMsg);

    const auto l_result = _title     +  _getPrimaryKey(a_mxTelemetryMsg) + _Bold_end
            + _time        +  getCurrentTimeQTImpl(TIME_FORMAT__hh_mm_ss)
            + _id          +  QString::number(l_data->id)
            + _recSize     +  QString::number(l_data->recSize)
            + _compress    +  QString::number(l_data->compress)
            + _cacheMode   +  (ZdbCacheMode::name(l_data->cacheMode))
            + _cacheSize   +  QString::number(l_data->cacheSize)
            + _path        +  (l_data->path)
            + _fileSize    +  QString::number(l_data->fileSize)
            + _fileRecs    +  QString::number(l_data->fileRecs)
            + _filesMax    +  QString::number(l_data->filesMax)

            + _preAlloc    +  QString::number(l_data->preAlloc)
            + _minRN       +  QString::number(l_data->minRN)
            + _nextRN      +  QString::number(l_data->nextRN)
            + _fileRN      +  QString::number(l_data->fileRN)
            + _cacheLoads  +  QString::number(l_data->cacheLoads)
            + _cacheMisses +  QString::number(l_data->cacheMisses)
            + _fileLoads   +  QString::number(l_data->fileLoads)
            + _fileMisses  +  QString::number(l_data->fileMisses)
            ;

    return l_result;
}











