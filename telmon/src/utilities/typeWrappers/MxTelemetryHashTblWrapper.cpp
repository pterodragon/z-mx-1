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


#include "MxTelemetryHashTblWrapper.h"
#include "QList"
#include "QDebug"
#include "ZmHashMgr.hpp"
#include "QLinkedList"

MxTelemetryHashTblWrapper::MxTelemetryHashTblWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryHashTblWrapper");
}


MxTelemetryHashTblWrapper::~MxTelemetryHashTblWrapper()
{

}


void MxTelemetryHashTblWrapper::initActiveDataSet() noexcept
{
    // 0=linear, 1=bits
    m_activeDataSet = {0, 1};
}


void MxTelemetryHashTblWrapper::initTableList() noexcept
{
    // the index for each catagory
    int i = 0;
    m_tableList->insert(i, "time");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::GET_CURRENT_TIME);

    m_tableList->insert(i, "linear");
    m_tablePriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_linear);

    m_tableList->insert(i, "bits");
    m_tablePriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_bits);

    m_tableList->insert(i, "slots");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::HASH_TBL_MXTYPE_CALCULATE_SLOTS);

    m_tableList->insert(i, "cBits");
    m_tablePriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_cBits);

    m_tableList->insert(i, "cSlots");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::HASH_TBL_MXTYPE_CALCULATE_C_SLOTS);

    m_tableList->insert(i, "count");
    m_tablePriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_count);

    m_tableList->insert(i, "resized");
    m_tablePriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_resized);

    m_tableList->insert(i, "loadFactor");
    m_tablePriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_loadFactor);

    m_tableList->insert(i, "effLoadFactor");
    m_tablePriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_effLoadFactor);

    m_tableList->insert(i, "nodeSize");
    m_tablePriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_nodeSize);
}


void MxTelemetryHashTblWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    double l_otherResult;
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
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHashTblTelemetryStructIndex::e_linear:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHashTblTelemetryStructIndex::e_bits:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case OTHER_ACTIONS::HASH_TBL_MXTYPE_CALCULATE_SLOTS:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHashTblTelemetryStructIndex::e_cBits:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case OTHER_ACTIONS::HASH_TBL_MXTYPE_CALCULATE_C_SLOTS:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHashTblTelemetryStructIndex::e_count:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHashTblTelemetryStructIndex::e_resized:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHashTblTelemetryStructIndex::e_loadFactor:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHashTblTelemetryStructIndex::e_effLoadFactor:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZmHashTblTelemetryStructIndex::e_nodeSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
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



void MxTelemetryHashTblWrapper::initChartList() noexcept
{
    int i = 0;
    m_chartList->insert(i, "linear");
    m_chartPriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_linear);

    m_chartList->insert(i, "bits");
    m_chartPriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_bits);

    m_chartList->insert(i, "slots");
    m_chartPriorityToStructIndex->insert(i++, OTHER_ACTIONS::HASH_TBL_MXTYPE_CALCULATE_SLOTS);

    m_chartList->insert(i, "cBits");
    m_chartPriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_cBits);

    m_chartList->insert(i, "cSlots");
    m_chartPriorityToStructIndex->insert(i++, OTHER_ACTIONS::HASH_TBL_MXTYPE_CALCULATE_C_SLOTS);

    m_chartList->insert(i, "count");
    m_chartPriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_count);

    m_chartList->insert(i, "resized");
    m_chartPriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_resized);

    m_chartList->insert(i, "loadFactor");
    m_chartPriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_loadFactor);

    m_chartList->insert(i, "effLoadFactor");
    m_chartPriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_effLoadFactor);

    m_chartList->insert(i, "nodeSize");
    m_chartPriorityToStructIndex->insert(i++, ZmHashTblTelemetryStructIndex::e_nodeSize);

    // extra
    m_chartList->insert(i++, "none");
}


double MxTelemetryHashTblWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    double l_otherResult;

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);

    return typeConvertor<double>(QPair(l_dataPair.first, l_dataPair.second));
}


QPair<void*, int> MxTelemetryHashTblWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg,
                                                                    const int a_index,
                                                                    void* a_otherResult) const noexcept
{

    // Notice: we defiently know a_mxTelemetryMsg type !
    ZmHashTelemetry* l_data = static_cast<ZmHashTelemetry*>(a_mxTelemetryMsg);
    QPair<void*, int> l_result;
    switch (a_index) {
    case ZmHashTblTelemetryStructIndex::e_id:
        l_result.first = &l_data->id;
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case ZmHashTblTelemetryStructIndex::e_nodeSize:
        l_result.first = &l_data->nodeSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZmHashTblTelemetryStructIndex::e_loadFactor:
        *(static_cast<double*>(a_otherResult)) = static_cast<double>(l_data->loadFactor  / 16.0);
        l_result.first = a_otherResult;
        l_result.second = CONVERT_FRON::type_double;
        break;
    case ZmHashTblTelemetryStructIndex::e_count:
        l_result.first = &l_data->count;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZmHashTblTelemetryStructIndex::e_effLoadFactor:
        *(static_cast<double*>(a_otherResult)) = static_cast<double>(l_data->effLoadFactor  / 16.0);
        l_result.first = a_otherResult;
        l_result.second = CONVERT_FRON::type_double;
        break;
    case ZmHashTblTelemetryStructIndex::e_resized:
        l_result.first = &l_data->resized;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZmHashTblTelemetryStructIndex::e_bits:
        l_result.first = &l_data->bits;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case ZmHashTblTelemetryStructIndex::e_cBits:
        l_result.first = &l_data->cBits;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case ZmHashTblTelemetryStructIndex::e_linear:
        l_result.first = &l_data->linear;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case OTHER_ACTIONS::HASH_TBL_MXTYPE_CALCULATE_SLOTS:
        // uint_64 can hold  2^64 - 1
        // double  can hold  2^53 and -(2^53)
        // so we lose precision if (a_otherResult > 2^53)
        *(static_cast<uint64_t*>(a_otherResult)) = static_cast<uint64_t>(1) << l_data->bits;
        l_result.first = a_otherResult;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case OTHER_ACTIONS::HASH_TBL_MXTYPE_CALCULATE_C_SLOTS:
        // uint_64 can hold  2^64 - 1
        // double  can hold  2^53 and -(2^53)
        // so we lose precision if (a_otherResult > 2^53)
        *(static_cast<uint64_t*>(a_otherResult)) = static_cast<uint64_t>(1) << l_data->cBits;
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

