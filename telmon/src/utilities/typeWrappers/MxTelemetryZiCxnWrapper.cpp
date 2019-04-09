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
#include "MxTelemetryZiCxnWrapper.h"
#include "QList"
#include "QDebug"
#include "QLinkedList"

MxTelemetryZiCxnWrapper::MxTelemetryZiCxnWrapper()
{
    initTableList();
    initChartList();
    initActiveDataSet();
    setClassName("MxTelemetryZiCxnWrapper");
}


MxTelemetryZiCxnWrapper::~MxTelemetryZiCxnWrapper()
{
}


void MxTelemetryZiCxnWrapper::initActiveDataSet() noexcept
{
    // 2=rxBufLen, 4=txBufLen
    m_activeDataSet = {2, 4};
}


void MxTelemetryZiCxnWrapper::initTableList() noexcept
{
    // the index for each catagory
    int i = 0;
    m_tableList->insert(i, "time");
    m_tablePriorityToStructIndex->insert(i++, OTHER_ACTIONS::GET_CURRENT_TIME);

    m_tableList->insert(i, "type");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_type);

    m_tableList->insert(i, "remoteIP");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_remoteIP);

    m_tableList->insert(i, "remotePort");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_remotePort);

    m_tableList->insert(i, "localIP");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_localIP);

    m_tableList->insert(i, "localPort");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_localPort);

    m_tableList->insert(i, "fd");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_socket);

    m_tableList->insert(i, "flags");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_flags);

    m_tableList->insert(i, "mreqAddr");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_mreqAddr);

    m_tableList->insert(i, "mreqIf");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_mreqIf);

    m_tableList->insert(i, "mif");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_mif);

    m_tableList->insert(i, "ttl");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_ttl);

    m_tableList->insert(i, "rxBufSize");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_rxBufSize);

    m_tableList->insert(i, "rxBufLen");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_rxBufLen);

    m_tableList->insert(i, "txBufSize");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_txBufSize);

    m_tableList->insert(i, "txBufLen");
    m_tablePriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_txBufLen);
}


void MxTelemetryZiCxnWrapper::_getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    QPair<void*, int> l_dataPair;
    ZiIP l_otherResult;

    for (auto i = 0; i < m_tableList->count(); i++)
    {
        switch (const auto l_index = m_tablePriorityToStructIndex->at(i)) {
        case OTHER_ACTIONS::GET_CURRENT_TIME:
            a_result.append(QString::fromStdString(getCurrentTime()));
            break;
        case ZiCxnMxTelemetryStructIndex::e_type:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(ZiCxnType::name(
                                typeConvertor<uint8_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_remoteIP:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(streamToQString(
                                *(static_cast<ZiIP*>(l_dataPair.first))
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_remotePort:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint16_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_localIP:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(streamToQString(
                                *(static_cast<ZiIP*>(l_dataPair.first))
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_localPort:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint16_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_socket:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint64_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_flags:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            // Explanation of the following lambda:
            // 1. return the convertion from flags to QString
            // 2. it used only here, so no need to implement function for it
            a_result.append([this](const uint32_t a_ZiCxnFlags) -> const QString
            {
                this->getStream() << ZiCxnFlags::Flags::print(a_ZiCxnFlags);
                const QString l_result = QString::fromStdString(m_stream->str());
                this->getStream().str(std::string());
                this->getStream().clear();
                return l_result;
            }( typeConvertor<uint32_t> (
                   QPair(l_dataPair.first, l_dataPair.second)
                   )
               )
            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_mreqAddr:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(streamToQString(
                                *(static_cast<ZiIP*>(l_dataPair.first))
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_mreqIf:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(streamToQString(
                                *(static_cast<ZiIP*>(l_dataPair.first))
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_mif:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(streamToQString(
                                *(static_cast<ZiIP*>(l_dataPair.first))
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_ttl:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_rxBufSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_rxBufLen:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_txBufSize:
            l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);
            a_result.append(QString::number(
                                typeConvertor<uint32_t>(
                                    QPair(l_dataPair.first, l_dataPair.second)
                                    )
                                )
                            );
            break;
        case ZiCxnMxTelemetryStructIndex::e_txBufLen:
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


void MxTelemetryZiCxnWrapper::initChartList() noexcept
{
    int i = 0;
    m_chartList->insert(i, "fd");
    m_chartPriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_socket);

    m_chartList->insert(i, "rxBufSize");
    m_chartPriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_rxBufSize);

    m_chartList->insert(i, "rxBufLen");
    m_chartPriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_rxBufLen);

    m_chartList->insert(i, "txBufSize");
    m_chartPriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_txBufSize);

    m_chartList->insert(i, "txBufLen");
    m_chartPriorityToStructIndex->insert(i++, ZiCxnMxTelemetryStructIndex::e_txBufLen);


    // extra
    m_chartList->insert(i++, "none");
}


int MxTelemetryZiCxnWrapper::_getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    // sanity check
    if ( ! (isIndexInChartPriorityToHeapIndexContainer(a_index)) ) {return 0;}

    const int l_index = m_chartPriorityToStructIndex->at(a_index);

    ZiIP l_otherResult;

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, l_index, &l_otherResult);

    return typeConvertor<int>(QPair(l_dataPair.first, l_dataPair.second));
}


QPair<void*, int> MxTelemetryZiCxnWrapper::getMxTelemetryDataType(void* const a_mxTelemetryMsg,
                                                                  const int a_index,
                                                                  void* a_otherResult) const noexcept
{
    // Notice: we defiently know a_mxTelemetryMsg type !
    ZiCxnTelemetry* l_data = static_cast<ZiCxnTelemetry*>(a_mxTelemetryMsg);
    QPair<void*, int> l_result;
    switch (a_index) {
    case ZiCxnMxTelemetryStructIndex::e_mxID:
        l_result.first = &l_data->mxID;
        l_result.second = CONVERT_FRON::type_c_char;
        break;
    case ZiCxnMxTelemetryStructIndex::e_socket:
        l_result.first = &l_data->socket;
        l_result.second = CONVERT_FRON::type_uint64_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_rxBufSize:
        l_result.first = &l_data->rxBufSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_rxBufLen:
        l_result.first = &l_data->rxBufLen;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_txBufSize:
        l_result.first = &l_data->txBufSize;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_txBufLen:
        l_result.first = &l_data->txBufLen;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_flags:
        l_result.first = &l_data->flags;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_mreqAddr:
        *(static_cast<ZiIP*>(a_otherResult)) = l_data->mreqAddr;
        l_result.first = a_otherResult;
        l_result.second = CONVERT_FRON::type_ZiIP;
        break;
    case ZiCxnMxTelemetryStructIndex::e_mreqIf:
        *(static_cast<ZiIP*>(a_otherResult)) = l_data->mreqIf;
        l_result.first = a_otherResult;
        l_result.second = CONVERT_FRON::type_ZiIP;
        break;
    case ZiCxnMxTelemetryStructIndex::e_mif:
        *(static_cast<ZiIP*>(a_otherResult)) = l_data->mif;
        l_result.first = a_otherResult;
        l_result.second = CONVERT_FRON::type_ZiIP;
        break;
    case ZiCxnMxTelemetryStructIndex::e_ttl:
        l_result.first = &l_data->ttl;
        l_result.second = CONVERT_FRON::type_uint32_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_localIP:
        *(static_cast<ZiIP*>(a_otherResult)) = l_data->localIP;
        l_result.first = a_otherResult;
        l_result.second = CONVERT_FRON::type_ZiIP;
        break;
    case ZiCxnMxTelemetryStructIndex::e_remoteIP:
        *(static_cast<ZiIP*>(a_otherResult)) = l_data->remoteIP;
        l_result.first = a_otherResult;
        l_result.second = CONVERT_FRON::type_ZiIP;
        break;
    case ZiCxnMxTelemetryStructIndex::e_localPort:
        l_result.first = &l_data->localPort;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_remotePort:
        l_result.first = &l_data->remotePort;
        l_result.second = CONVERT_FRON::type_uint16_t;
        break;
    case ZiCxnMxTelemetryStructIndex::e_type:
        l_result.first = &l_data->type;
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


const std::string MxTelemetryZiCxnWrapper::__getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept
{
    ZiIP l_otherResult;
    const std::string l_localIP = streamToStdString(*(static_cast<ZiIP*>(
                                                          getMxTelemetryDataType(a_mxTelemetryMsg,
                                                                                 ZiCxnMxTelemetryStructIndex::e_localIP,
                                                                                 &l_otherResult).first)
                                                      )
                                                    );


    const std::string l_localPort = std::to_string(typeConvertor<uint16_t>(
                                                  getMxTelemetryDataType(a_mxTelemetryMsg,
                                                                         ZiCxnMxTelemetryStructIndex::e_localPort,
                                                                         &l_otherResult)));


    const std::string l_remoteIP = streamToStdString(*(static_cast<ZiIP*>(
                                                          getMxTelemetryDataType(a_mxTelemetryMsg,
                                                                                 ZiCxnMxTelemetryStructIndex::e_remoteIP,
                                                                                 &l_otherResult).first)
                                                      )
                                                    );

    const std::string l_remotePort = std::to_string(typeConvertor<uint16_t>(
                                                  getMxTelemetryDataType(a_mxTelemetryMsg,
                                                                         ZiCxnMxTelemetryStructIndex::e_remotePort,
                                                                         &l_otherResult)));

    const QPair<void*, int> l_dataPair = getMxTelemetryDataType(a_mxTelemetryMsg, ZiCxnMxTelemetryStructIndex::e_type, &l_otherResult);
    uint8_t l_type = typeConvertor<uint8_t>(QPair(l_dataPair.first, l_dataPair.second));

    switch (l_type)
    {
    case ZiCxnType::TCPIn:   // primary key is localIP:localPort<remoteIP:remotePort
        return std::string().append(l_localIP).append(":").append(l_localPort).append("<").append(l_remoteIP).append(":").append(l_remotePort);
        break;
    case ZiCxnType::UDP:     // primary key is remoteIP:remotePort<localIP:localPort
    case ZiCxnType::TCPOut:
        return std::string().append(l_remoteIP).append(":").append(l_remotePort).append("<").append(l_localIP).append(":").append(l_localPort);
        break;
    default:
        qCritical() << *m_className << __PRETTY_FUNCTION__ << "unknown ZiCxnType";
        return std::string();
    }
}


const QString MxTelemetryZiCxnWrapper::_getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept
{
     const auto l_result = __getPrimaryKey(a_mxTelemetryMsg);
     if (l_result.length() == 0)
     {
         return QString();
     } else
     {
        return QString::fromStdString(l_result);
     }
}


