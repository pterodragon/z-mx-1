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

    m_tableList->insert(i, "priority");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_priority);

    m_tableList->insert(i, "partition");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_partition);

    m_tableList->insert(i, "isolation");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_isolation);

    m_tableList->insert(i, "rxThread");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_rxThread);

    m_tableList->insert(i, "txThread");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_txThread);

    m_tableList->insert(i, "stackSize");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_stackSize);

    m_tableList->insert(i, "rxBufSize");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_rxBufSize);

    m_tableList->insert(i, "txBufSize");
    m_tablePriorityToStructIndex->insert(i++, ZiMxTelemetryStructIndex::e_txBufSize);
}


void MxTelemetryZiMultiplexerWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
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
        case ZiMxTelemetryStructIndex::e_priority:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint8_t>(
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
        case ZiMxTelemetryStructIndex::e_isolation:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(streamToQString(ZmBitmap(
                                                typeConvertor<uint64_t>(
                                                    QPair(l_dataPair.first, l_dataPair.second)
                                                    )
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
        case ZiMxTelemetryStructIndex::e_stackSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
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


double MxTelemetryZiMultiplexerWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index);

    return typeConvertor<double>(QPair(l_dataPair.first, l_dataPair.second));
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
    case ZiMxTelemetryStructIndex::e_isolation:
        l_result.first = &l_data->isolation;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZiMxTelemetryStructIndex::e_stackSize:
        l_result.first = &l_data->stackSize;
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
