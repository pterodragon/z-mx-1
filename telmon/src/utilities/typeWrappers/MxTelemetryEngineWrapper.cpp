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
#include "MxTelemetryEngineWrapper.h"
#include "QList"
#include "QDebug"
#include "QLinkedList"

MxTelemetryEngineWrapper::MxTelemetryEngineWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryEngineWrapper");
}


MxTelemetryEngineWrapper::~MxTelemetryEngineWrapper()
{
}


void MxTelemetryEngineWrapper::initActiveDataSet() noexcept
{
    // 0=, 1=
    m_activeDataSet = {0, 0};
}

void MxTelemetryEngineWrapper::initTableList() noexcept
{
    // the index for each catagory
    int i = 0;
    m_tableList->insert(i, "time");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::GET_CURRENT_TIME);

    m_tableList->insert(i, "state");
    m_tablePriorityToStructIndex->insert(i++, EngineMxTelemetryStructIndex::e_state);

    m_tableList->insert(i, "nLinks");
    m_tablePriorityToStructIndex->insert(i++, EngineMxTelemetryStructIndex::e_nLinks);

    m_tableList->insert(i, "up");
    m_tablePriorityToStructIndex->insert(i++, EngineMxTelemetryStructIndex::e_up);

    m_tableList->insert(i, "down");
    m_tablePriorityToStructIndex->insert(i++, EngineMxTelemetryStructIndex::e_down);

    m_tableList->insert(i, "disabled");
    m_tablePriorityToStructIndex->insert(i++, EngineMxTelemetryStructIndex::e_disabled);

    m_tableList->insert(i, "transient");
    m_tablePriorityToStructIndex->insert(i++, EngineMxTelemetryStructIndex::e_transient);

    m_tableList->insert(i, "reconn");
    m_tablePriorityToStructIndex->insert(i++, EngineMxTelemetryStructIndex::e_reconn);

    m_tableList->insert(i, "failed");
    m_tablePriorityToStructIndex->insert(i++, EngineMxTelemetryStructIndex::e_failed);

    m_tableList->insert(i, "mxID");
    m_tablePriorityToStructIndex->insert(i++, EngineMxTelemetryStructIndex::e_mxID);

    m_tableList->insert(i, "rxThread");
    m_tablePriorityToStructIndex->insert(i++, EngineMxTelemetryStructIndex::e_rxThread);

    m_tableList->insert(i, "txThread");
    m_tablePriorityToStructIndex->insert(i++, EngineMxTelemetryStructIndex::e_txThread);
}


void MxTelemetryEngineWrapper::_getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    QPair<void*, int> l_dataPair;

    for (auto i = 0; i < m_tableList->count(); i++)
    {
        switch (const auto l_index = m_tablePriorityToStructIndex->at(i)) {
        case OTHER_ACTIONS::GET_CURRENT_TIME:
            a_result.append(QString::fromStdString(getCurrentTime()));
            break;
        case EngineMxTelemetryStructIndex::e_state:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(MxEngineState::name(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case EngineMxTelemetryStructIndex::e_nLinks:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint16_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case EngineMxTelemetryStructIndex::e_up:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case EngineMxTelemetryStructIndex::e_down:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case EngineMxTelemetryStructIndex::e_disabled:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case EngineMxTelemetryStructIndex::e_transient:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case EngineMxTelemetryStructIndex::e_reconn:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case EngineMxTelemetryStructIndex::e_failed:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case EngineMxTelemetryStructIndex::e_mxID:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(static_cast<char*>(l_dataPair.first));
            break;
        case EngineMxTelemetryStructIndex::e_rxThread:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case EngineMxTelemetryStructIndex::e_txThread:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
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


void MxTelemetryEngineWrapper::initChartList() noexcept
{
    int i = 0;

    // extrae_mxID
    m_chartList->insert(i++, "none");
}


int MxTelemetryEngineWrapper::_getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

    return typeConvertor<int>(QPair(l_dataPair.first, l_dataPair.second));
}



QPair<void*, int> MxTelemetryEngineWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    MxEngine::Telemetry* l_data = static_cast<MxEngine::Telemetry*>(a_mxTelemetryMsg);

    QPair<void*, int> l_result;
    switch (a_index) {
    case EngineMxTelemetryStructIndex::e_id:
        l_result.first = l_data->id.data();
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case EngineMxTelemetryStructIndex::e_mxID:
        l_result.first = l_data->mxID.data();

        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case EngineMxTelemetryStructIndex::e_down:
        l_result.first = &l_data->down;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_disabled:
        l_result.first = &l_data->disabled;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_transient:
        l_result.first = &l_data->transient;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_up:
        l_result.first = &l_data->up;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_reconn:
        l_result.first = &l_data->reconn;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_failed:
        l_result.first = &l_data->failed;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case EngineMxTelemetryStructIndex::e_nLinks:
        l_result.first = &l_data->nLinks;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case EngineMxTelemetryStructIndex::e_rxThread:
        l_result.first = &l_data->rxThread;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case EngineMxTelemetryStructIndex::e_txThread:
        l_result.first = &l_data->txThread;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case EngineMxTelemetryStructIndex::e_state:
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



const QString MxTelemetryEngineWrapper::_getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept
{
     MxEngine::Telemetry* l_data = static_cast<MxEngine::Telemetry*>(a_mxTelemetryMsg);

     if (l_data)
     {
         return QString(l_data->id.data());
     } else
     {
        return QString();
     }
}


const QString MxTelemetryEngineWrapper::_getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept
{
    const auto* const l_data = static_cast<MxEngine::Telemetry*>(a_mxTelemetryMsg);

    const auto l_result = _title     +  _getPrimaryKey(a_mxTelemetryMsg) + _Bold_end
            + _time     +  getCurrentTimeQTImpl(TIME_FORMAT__hh_mm_ss)

            + getColor(l_data->state)
            + _state     + MxEngineState::name(l_data->state)
            + _color_off

            + _nLinks    + QString::number(l_data->nLinks)
            + _up        + QString::number(l_data->up)
            + _down      + QString::number(l_data->down)

            + _disabled  + QString::number(l_data->disabled)
            + _transient + QString::number(l_data->transient)
            + _reconn    + QString::number(l_data->reconn)
            + _failed    + QString::number(l_data->failed)
            + _mxID      + l_data->mxID.data()

            + _rxThread  + QString::number(l_data->rxThread)
            + _txThread  + QString::number(l_data->txThread)
            ;

    return l_result;
}


const QString& MxTelemetryEngineWrapper::getColor(const int a_state) const noexcept{
    switch (a_state) {
    case MxEngineState::Running:
        return _color_green;
        break;
    case MxEngineState::Stopped:
        return _color_red;
        break;
    default:
        return _color_amber;
        break;
    }
}





