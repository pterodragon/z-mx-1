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




#include "MxTelemetryGeneralWrapper.h"
#include "QPair"
#include <cstdint>
#include "QDebug"


MxTelemetryGeneralWrapper::MxTelemetryGeneralWrapper()
{

}


MxTelemetryGeneralWrapper::~MxTelemetryGeneralWrapper()
{

}


//template <class T>
// to make template
double MxTelemetryGeneralWrapper::typeConvertor(const QPair< void*, int>& a_param) const noexcept
{
    double l_return;
    void* l_data = a_param.first;
    switch (a_param.second)
    {
    case CONVERT_FRON::type_uint64_t: //uint64_t
        l_return = static_cast<double>(*(static_cast<uint64_t*>(l_data)));
    break;
    case CONVERT_FRON::type_uint32_t: //uint32_t
        l_return = static_cast<double>(*(static_cast<uint32_t*>(l_data)));
    break;
    case CONVERT_FRON::type_uint16_t: //uint16_t
        l_return = static_cast<double>(*(static_cast<uint16_t*>(l_data)));
    break;
    case CONVERT_FRON::type_uint8_t: //uint8_t
        l_return = static_cast<double>(*(static_cast<uint8_t*>(l_data)));
    break;
    default:
        qDebug() << "typeConvertor default, retunring deafult value 0 ";
        l_return = 0;
    break;
    }
    return l_return;
}











