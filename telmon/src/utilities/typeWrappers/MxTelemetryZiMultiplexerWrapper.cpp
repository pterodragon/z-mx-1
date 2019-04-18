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


#include "ZiMultiplex.hpp"
#include "MxTelemetryZiMultiplexerWrapper.h"
#include "QList"
#include "QDebug"
#include "QLinkedList"



MxTelemetryZiMultiplexerWrapper::MxTelemetryZiMultiplexerWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryZiMultiplexerWrapper");
}


MxTelemetryZiMultiplexerWrapper::~MxTelemetryZiMultiplexerWrapper()
{
}


void MxTelemetryZiMultiplexerWrapper::initActiveDataSet() noexcept
{
    // 0=none, 1=none
    m_activeDataSet = {0, 0};
}


void MxTelemetryZiMultiplexerWrapper::initTableList() noexcept
{
    // the index for each catagory
    int i = 0;
    m_tableList->insert(i, "time");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::GET_CURRENT_TIME);

    m_tableList->insert(i, "state");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_state);

    m_tableList->insert(i, "nThreads");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_nThreads);

    m_tableList->insert(i, "rxThread");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_rxThread);

    m_tableList->insert(i, "txThread");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_txThread);

    m_tableList->insert(i, "priority");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_priority);

    m_tableList->insert(i, "stackSize");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_stackSize);

    m_tableList->insert(i, "partition");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_partition);

    m_tableList->insert(i, "rxBufSize");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_rxBufSize);

    m_tableList->insert(i, "txBufSize");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_txBufSize);

    m_tableList->insert(i, "queueSize");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_queueSize);

    m_tableList->insert(i, "ll");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_ll);

    m_tableList->insert(i, "spin");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_spin);

    m_tableList->insert(i, "timeout");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_timeout);
}


void MxTelemetryZiMultiplexerWrapper::_getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    QPair<void*, int> l_dataPair;

    for (auto i = 0; i < m_tableList->count(); i++)
    {
        switch (const auto l_index = m_tablePriorityToStructIndex->at(i)) {
        case OTHER_ACTIONS::GET_CURRENT_TIME:
            a_result.append(QString::fromStdString(getCurrentTime()));
            break;
        case ZiMxTelemetryStructIndex::e_state:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(ZmScheduler::stateName(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiMxTelemetryStructIndex::e_nThreads:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiMxTelemetryStructIndex::e_rxThread:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiMxTelemetryStructIndex::e_txThread:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiMxTelemetryStructIndex::e_priority:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiMxTelemetryStructIndex::e_stackSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiMxTelemetryStructIndex::e_partition:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint16_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiMxTelemetryStructIndex::e_rxBufSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiMxTelemetryStructIndex::e_txBufSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;

        case ZiMxTelemetryStructIndex::e_queueSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiMxTelemetryStructIndex::e_ll:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiMxTelemetryStructIndex::e_spin:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiMxTelemetryStructIndex::e_timeout:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
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


void MxTelemetryZiMultiplexerWrapper::initChartList() noexcept
{
    int i = 0;

    // extra
    m_chartList->insert(i++, "none");
}


int MxTelemetryZiMultiplexerWrapper::_getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

    return typeConvertor<int>(QPair(l_dataPair.first, l_dataPair.second));
}


QPair<void*, int> MxTelemetryZiMultiplexerWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    ZiMxTelemetry* l_data = static_cast<ZiMxTelemetry*>(a_mxTelemetryMsg);
    QPair<void*, int> l_result;
    switch (a_index) {
    case ZiMxTelemetryStructIndex::e_id:
        l_result.first = &l_data->id;
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case ZiMxTelemetryStructIndex::e_stackSize:
        l_result.first = &l_data->stackSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiMxTelemetryStructIndex::e_queueSize:
        l_result.first = &l_data->queueSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiMxTelemetryStructIndex::e_spin:
        l_result.first = &l_data->spin;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiMxTelemetryStructIndex::e_timeout:
        l_result.first = &l_data->timeout;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiMxTelemetryStructIndex::e_rxBufSize:
        l_result.first = &l_data->rxBufSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiMxTelemetryStructIndex::e_txBufSize:
        l_result.first = &l_data->txBufSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiMxTelemetryStructIndex::e_rxThread:
        l_result.first = &l_data->rxThread;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZiMxTelemetryStructIndex::e_txThread:
        l_result.first = &l_data->txThread;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZiMxTelemetryStructIndex::e_partition:
        l_result.first = &l_data->partition;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZiMxTelemetryStructIndex::e_state:
        l_result.first = &l_data->state;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case ZiMxTelemetryStructIndex::e_ll:
        l_result.first = &l_data->ll;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case ZiMxTelemetryStructIndex::e_priority:
        l_result.first = &l_data->priority;
        l_result.second = CONVERT_FRON::type_uint8_t;
        break;
    case ZiMxTelemetryStructIndex::e_nThreads:
        l_result.first = &l_data->nThreads;
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


const QString MxTelemetryZiMultiplexerWrapper::_getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept
{
     ZiMxTelemetry* l_data = static_cast<ZiMxTelemetry*>(a_mxTelemetryMsg);
     if (l_data)
     {
         return QString(l_data->id.data());
     } else
     {
        return QString();
     }
}


const QString MxTelemetryZiMultiplexerWrapper::_getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept
{
    const auto* const l_data = static_cast<const ZiMxTelemetry* const>(a_mxTelemetryMsg);

    const auto l_result = _title     +  _getPrimaryKey(a_mxTelemetryMsg) + _Bold_end
            + _time      + getCurrentTimeQTImpl(TIME_FORMAT__hh_mm_ss)

            + getColor(l_data->state)
            + _state     + ZmScheduler::stateName(l_data->state)
            + _color_off

            + _nThreads  +  QString::number(l_data->nThreads)
            + _rxThread  +  QString::number(l_data->rxThread)
            + _txThread  +  QString::number(l_data->txThread)
            + _priority  +  QString::number(l_data->priority)
            + _stackSize +  QString::number(l_data->stackSize)
            + _partition +  QString::number(l_data->partition)
            + _rxBufSize +  QString::number(l_data->rxBufSize)
            + _txBufSize +  QString::number(l_data->txBufSize)
            + _queueSize +  QString::number(l_data->queueSize)
            + _ll        +  QString::number(l_data->ll)
            + _spin      +  QString::number(l_data->spin)
            + _timeout   +  QString::number(l_data->timeout)

    ;
    return l_result;
}

const QString& MxTelemetryZiMultiplexerWrapper::getColor(const int a_state) const noexcept{
    switch (a_state) {
    case ZmScheduler::Running:
        return _color_green;
        break;
    case ZmScheduler::Stopped:
        return _color_red;
        break;
    default:
        return _color_amber;
        break;
    }
}


