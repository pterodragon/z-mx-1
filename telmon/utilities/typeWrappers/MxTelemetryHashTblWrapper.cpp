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

MxTelemetryHashTblWrapper::MxTelemetryHashTblWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
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
    // removed irrelvant for table representation
    m_tableList->reserve(11);
    m_tableList->insert(0, "time");
//    m_tableList->insert(1, "id");
    m_tableList->insert(1, "linear");
    m_tableList->insert(2, "bits");
    m_tableList->insert(3, "slots");
    m_tableList->insert(4, "cBits");
    m_tableList->insert(5, "cSlots");
    m_tableList->insert(6, "count");
    m_tableList->insert(7, "resized");
    m_tableList->insert(8, "loadFactor");
    m_tableList->insert(9, "effLoadFactor");
    m_tableList->insert(10, "nodeSize");
}


void MxTelemetryHashTblWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    // TODO
    // 1. iterate over priorites
    // 2. for each priority get data

//    l_list.append(QString::fromStdString(a_this->getCurrentTime()));
//    l_list.append(QString::number(ZuBoxed(l_data.linear)));
//    l_list.append(QString::number(ZuBoxed(l_data.bits)));
//    l_list.append(QString::number(ZuBoxed((static_cast<uint64_t>(1))<<l_data.bits)));
//    l_list.append(QString::number(ZuBoxed(l_data.cBits)));

//    l_list.append(QString::number(ZuBoxed((static_cast<uint64_t>(1))<<l_data.cBits)));
//    l_list.append(QString::number(l_data.count));
//    l_list.append(QString::number(l_data.resized));
//    l_list.append(QString::number(static_cast<double>(l_data.loadFactor   / 16.0)));
//    l_list.append(QString::number(static_cast<double>(l_data.effLoadFactor / 16.0)));

//    l_list.append(QString::number(l_data.nodeSize));
}


void MxTelemetryHashTblWrapper::initChartList() noexcept
{
    // removed irrelvant for chart representation
    m_chartList->reserve(11);
    m_chartPriorityToHeapIndex->reserve(10); // without none
//    m_chartList->insert(0, "time");
//    m_chartList->insert(1, "id");

    m_chartList->insert(0, "linear");
    m_chartPriorityToHeapIndex->insert(0, ZmHashTblTelemetryStructIndex::e_linear);

    m_chartList->insert(1, "bits");
    m_chartPriorityToHeapIndex->insert(1, ZmHashTblTelemetryStructIndex::e_bits);

    m_chartList->insert(2, "slots");
    m_chartPriorityToHeapIndex->insert(2, NOT_PRIMITVE);

    m_chartList->insert(3, "cBits");
    m_chartPriorityToHeapIndex->insert(3, ZmHashTblTelemetryStructIndex::e_cBits);

    m_chartList->insert(4, "cSlots");
    m_chartPriorityToHeapIndex->insert(4, NOT_PRIMITVE);

    m_chartList->insert(5, "count");
    m_chartPriorityToHeapIndex->insert(5, ZmHashTblTelemetryStructIndex::e_count);

    m_chartList->insert(6, "resized");
    m_chartPriorityToHeapIndex->insert(6, ZmHashTblTelemetryStructIndex::e_resized);

    m_chartList->insert(7, "loadFactor");
    m_chartPriorityToHeapIndex->insert(7, ZmHashTblTelemetryStructIndex::e_loadFactor);

    m_chartList->insert(8, "effLoadFactor");
    m_chartPriorityToHeapIndex->insert(8, ZmHashTblTelemetryStructIndex::e_effLoadFactor);

    m_chartList->insert(9, "nodeSize");
    m_chartPriorityToHeapIndex->insert(9, ZmHashTblTelemetryStructIndex::e_nodeSize);
    // extra
    m_chartList->insert(10, "none");
}


double MxTelemetryHashTblWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    double l_result = 0;
    if (a_index >= m_chartPriorityToHeapIndex->count())
    {
        qWarning() << "MxTelemetryHashTblWrapper::getDataForChart called with unsupoorted index"
                   << a_index
                   << "returning " << l_result;
        return l_result;
    }

    {
        const ZmHashTelemetry* l_data = static_cast<ZmHashTelemetry*>(a_mxTelemetryMsg);
        uint64_t l_result;
        switch (a_index) {
        case 2: // case slots
            l_result = static_cast<uint64_t>(1) << l_data->bits;
            return typeConvertor(QPair(&l_result, CONVERT_FRON::type_uint64_t));
        case 4:  // case c_slots
            l_result = static_cast<uint64_t>(1) << l_data->cBits;
            return typeConvertor(QPair(&l_result, CONVERT_FRON::type_uint64_t));
        }
    }


    const int l_index = m_chartPriorityToHeapIndex->at(a_index);
    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

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


QPair<void*, int> MxTelemetryHashTblWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg,
                                                                    const int a_index) const noexcept
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
        l_result.first = &l_data->loadFactor;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZmHashTblTelemetryStructIndex::e_count:
        l_result.first = &l_data->count;
        l_result.second = CONVERT_FRON::type_uint32_t;
        qDebug() << "l_data->count" << l_data->count;
        break;
    case ZmHashTblTelemetryStructIndex::e_effLoadFactor:
        l_result.first = &l_data->effLoadFactor;
        l_result.second = CONVERT_FRON::type_uint32_t;
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
    default:
        qWarning() << "unknown ZmHeapTelemetry, a_index" << a_index
                   << "retunring <nullptr CONVERT_FRON::none>";
        l_result.first = nullptr;
        l_result.second = CONVERT_FRON::type_none;
        break;
    }
    return l_result;
}

