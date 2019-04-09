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



//#ifndef MXTELEMETRYGENERALWRAPPERGENERICPART_H
//#define MXTELEMETRYGENERALWRAPPERGENERICPART_H

#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "QDebug"
#include <sstream> //for ZmBitmapToQString

template <class T>
T MxTelemetryGeneralWrapper::typeConvertor(const QPair<void*, int>& a_param) const noexcept
{
    double l_return;
    void* l_data = a_param.first;
    switch (a_param.second)
    {
    case CONVERT_FRON::type_uint64_t:
        l_return = static_cast<T>(*(static_cast<uint64_t*>(l_data)));
    break;
    case CONVERT_FRON::type_uint32_t:
        l_return = static_cast<T>(*(static_cast<uint32_t*>(l_data)));
    break;
    case CONVERT_FRON::type_uint16_t:
        l_return = static_cast<T>(*(static_cast<uint16_t*>(l_data)));
    break;
    case CONVERT_FRON::type_uint8_t:
        l_return = static_cast<T>(*(static_cast<uint8_t*>(l_data)));
    break;
    case CONVERT_FRON::type_int32_t:
        l_return = static_cast<T>(*(static_cast<int32_t*>(l_data)));
    break;
    case CONVERT_FRON::type_double:
        l_return = static_cast<T>((*(static_cast<double*>(l_data))));
    break;
    case CONVERT_FRON::type_int8_t:
        l_return = static_cast<T>((*(static_cast<int8_t*>(l_data))));
    break;
    default:
        qCritical() << *m_className
                    << __PRETTY_FUNCTION__
                    << "Unknown Converstion, a_param.second="
                    << a_param.second;
        l_return = 0;
    break;
    }
    return l_return;
}


template <class T>
QString MxTelemetryGeneralWrapper::streamToQString(const T& a_toStream) const noexcept
{
    a_toStream.print(*m_stream);
    const QString l_result = QString::fromStdString(m_stream->str());
    m_stream->str(std::string());
    m_stream->clear();
    return l_result;
}


template <class T>
const std::string MxTelemetryGeneralWrapper::streamToStdString(const T& a_toStream) const noexcept
{
    a_toStream.print(*m_stream);
    const std::string l_result = m_stream->str();
    m_stream->str(std::string());
    m_stream->clear();
    return l_result;
}


template <class T>
const T MxTelemetryGeneralWrapper::getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept
{
    return static_cast<T>(_getDataForChart(mxTelemetryMsgDataRetriever(a_mxTelemetryMsg), a_index));
}


//#endif // MXTELEMETRYGENERALWRAPPERGENERICPART_H
