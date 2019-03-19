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


MxTelemetryGeneralWrapper::MxTelemetryGeneralWrapper():
    m_tableList(new QList<QString>),
    m_chartList(new QVector<QString>),
    m_chartPriorityToHeapIndex(new QVector<int>),
    m_tablePriorityToHeapIndex(new QVector<int>)
{

}


MxTelemetryGeneralWrapper::~MxTelemetryGeneralWrapper()
{
    delete m_tableList;
    delete m_chartList;
    delete m_chartPriorityToHeapIndex;
    delete m_tablePriorityToHeapIndex;
}


const QList<QString>& MxTelemetryGeneralWrapper::getTableList() const noexcept
{
    return *m_tableList;
}


const QVector<QString>& MxTelemetryGeneralWrapper::getChartList() const noexcept
{
    return *m_chartList;
}


const std::array<int, 2>&  MxTelemetryGeneralWrapper::getActiveDataSet() const noexcept
{
    return m_activeDataSet;
}


bool MxTelemetryGeneralWrapper::isDataTypeNotUsed(const int a_index) const noexcept
{
    // for now,
    // we denote the last element in the conatiner as none
    // to do: maybe implement as map
    return a_index == (m_chartList->size() - 1);
}


//QString MxTelemetryGeneralWrapper::typeConvertor(const QPair< void*, int>& a_param) const noexcept
//{
//    double l_return;
//    void* l_data = a_param.first;
//    switch (a_param.second)
//    {
//    case CONVERT_FRON::type_uint64_t: //uint64_t
//        l_return = static_cast<double>(*(static_cast<uint64_t*>(l_data)));
//    break;
//    case CONVERT_FRON::type_uint32_t: //uint32_t
//        l_return = static_cast<double>(*(static_cast<uint32_t*>(l_data)));
//    break;
//    case CONVERT_FRON::type_uint16_t: //uint16_t
//        l_return = static_cast<double>(*(static_cast<uint16_t*>(l_data)));
//    break;
//    case CONVERT_FRON::type_uint8_t: //uint8_t
//        l_return = static_cast<double>(*(static_cast<uint8_t*>(l_data)));
//    break;
//    default:
//        qDebug() << "typeConvertor default, retunring deafult value 0 ";
//        l_return = 0;
//    break;
//    }
//    return l_return;
//}








