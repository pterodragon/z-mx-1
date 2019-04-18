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

#include "MxEngine.hpp"
#include "MxTelemetryLinkWrapper.h"
#include "QList"
#include "QDebug"
#include "QLinkedList"

MxTelemetryLinkWrapper::MxTelemetryLinkWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryLinkWrapper");
}



MxTelemetryLinkWrapper::~MxTelemetryLinkWrapper()
{
}


void MxTelemetryLinkWrapper::initActiveDataSet() noexcept
{
    // 0=none, 0=none
    m_activeDataSet = {0, 0};
}


void MxTelemetryLinkWrapper::initTableList() noexcept
{
    // the index for each catagory
    int i = 0;
    m_tableList->insert(i, "time");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::GET_CURRENT_TIME);

    m_tableList->insert(i, "state");
    m_tablePriorityToStructIndex->insert(i++, LinkMxTelemetryStructIndex::e_state);

    m_tableList->insert(i, "reconnects");
    m_tablePriorityToStructIndex->insert(i++, LinkMxTelemetryStructIndex::e_reconnects);

    m_tableList->insert(i, "rxSeqNo");
    m_tablePriorityToStructIndex->insert(i++, LinkMxTelemetryStructIndex::e_rxSeqNo);

    m_tableList->insert(i, "txSeqNo");
    m_tablePriorityToStructIndex->insert(i++, LinkMxTelemetryStructIndex::e_txSeqNo);

}


void MxTelemetryLinkWrapper::_getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    QPair<void*, int> l_dataPair;

    for (auto i = 0; i < m_tableList->count(); i++)
    {
        switch (const auto l_index = m_tablePriorityToStructIndex->at(i)) {
        case OTHER_ACTIONS::GET_CURRENT_TIME:
            a_result.append(QString::fromStdString(getCurrentTime()));
            break;
        case LinkMxTelemetryStructIndex::e_state:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(MxLinkState::name(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case LinkMxTelemetryStructIndex::e_reconnects:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case LinkMxTelemetryStructIndex::e_rxSeqNo:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case LinkMxTelemetryStructIndex::e_txSeqNo:
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


void MxTelemetryLinkWrapper::initChartList() noexcept
{
    int i = 0;

    // extra
    m_chartList->insert(i++, "none");
}


int MxTelemetryLinkWrapper::_getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

    return  typeConvertor<int>(QPair(l_dataPair.first, l_dataPair.second));

}


QPair<void*, int> MxTelemetryLinkWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    MxAnyLink::Telemetry* l_data = static_cast<MxAnyLink::Telemetry*>(a_mxTelemetryMsg);

    QPair<void*, int> l_result;
    switch (a_index) {
    case LinkMxTelemetryStructIndex::e_id:
        l_result.first = l_data->id.data();
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case LinkMxTelemetryStructIndex::e_rxSeqNo:
        l_result.first = &l_data->rxSeqNo;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case LinkMxTelemetryStructIndex::e_txSeqNo:
        l_result.first = &l_data->txSeqNo;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case LinkMxTelemetryStructIndex::e_reconnects:
        l_result.first = &l_data->reconnects;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case LinkMxTelemetryStructIndex::e_state:
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

const QString MxTelemetryLinkWrapper::_getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept
{
     MxAnyLink::Telemetry* l_data = static_cast<MxAnyLink::Telemetry*>(a_mxTelemetryMsg);

     if (l_data)
     {
         return QString(l_data->id.data());
     } else
     {
        return QString();
     }
}


const QString MxTelemetryLinkWrapper::_getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept
{
    const auto* const l_data = static_cast<const MxAnyLink::Telemetry* const>(a_mxTelemetryMsg);

    const auto l_result = _title     +  _getPrimaryKey(a_mxTelemetryMsg) + _Bold_end
            + _time       + getCurrentTimeQTImpl(TIME_FORMAT__hh_mm_ss)

            + getColor(l_data->state)
            + _state      + MxLinkState::name(l_data->state)
            + _color_off

            + _reconnects + QString::number(l_data->reconnects)
            + _rxSeqNo    + QString::number(l_data->rxSeqNo)
            + _txSeqNo    + QString::number(l_data->txSeqNo)
    ;

    return l_result;
}


const QString& MxTelemetryLinkWrapper::getColor(const int a_state) const noexcept{
    switch (a_state) {
    case MxRAG::Green:
        return _color_green;
        break;
    case MxRAG::Amber:
        return _color_amber;
        break;
    case MxRAG::Red:
        return _color_red;
        break;
    case MxRAG::Off:
        break;
        return _color_off;
    default:
        return _color_off;
        break;
    }
}







