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




#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "QPair"
#include <cstdint>
#include "QDebug"
#include "ZtDate.hpp"


MxTelemetryGeneralWrapper::MxTelemetryGeneralWrapper():
    m_tableList(new QList<QString>),
    m_chartList(new QVector<QString>),
    m_chartPriorityToStructIndex(new QVector<int>),
    m_tablePriorityToStructIndex(new QVector<int>),
    m_className(new QString("MxTelemetryGeneralWrapper")),
    m_stream(new std::stringstream())
{

}


MxTelemetryGeneralWrapper::~MxTelemetryGeneralWrapper()
{
    delete m_tableList;
    delete m_chartList;
    delete m_chartPriorityToStructIndex;
    delete m_tablePriorityToStructIndex;
    delete m_className;
    delete m_stream;
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


void MxTelemetryGeneralWrapper::setClassName(const std::string& a_className) noexcept
{
    *m_className = QString::fromStdString(a_className);
}


bool MxTelemetryGeneralWrapper::isIndexInChartPriorityToHeapIndexContainer(const int a_index) const noexcept
{
    if (a_index >= m_chartPriorityToStructIndex->count())
    {
        qCritical() << *m_className << __PRETTY_FUNCTION__ << "unsupported index:" << a_index;
        return false;
    }
    return true;
}


std::string MxTelemetryGeneralWrapper::getCurrentTime() const noexcept
{
    ZtDate::CSVFmt nowFmt;
    ZtDate now(ZtDate::Now);
    return (ZuStringN<512>() << now.csv(nowFmt)).data();
}


// Some defaule implementation

// QPair(void*, int); void* = the type, int = to convert
QPair<void*, int> MxTelemetryGeneralWrapper::getMxTelemetryDataType(void* const /* unused */,
                                                                     const int  /* unused */) const noexcept
{
    qCritical() << *m_className << __PRETTY_FUNCTION__ << "called from MxTelemetryGeneralWrapper";
    return QPair(nullptr, 0);
};

// used in case of retrieving data which is callculated from some values in the struct
QPair<void*, int> MxTelemetryGeneralWrapper::getMxTelemetryDataType(void* const /* unused */,
                                                                    const int   /* unused */,
                                                                    void*       /* unused */) const noexcept
{
    qCritical() << *m_className << __PRETTY_FUNCTION__ << "called from MxTelemetryGeneralWrapper";
    return QPair(nullptr, 0);
};













