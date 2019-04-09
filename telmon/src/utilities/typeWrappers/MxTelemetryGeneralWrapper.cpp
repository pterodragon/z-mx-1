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



#include "MxTelemetry.hpp"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "QPair"
#include "QDebug"


const char* MxTelemetryGeneralWrapper::NAME_DELIMITER = " - ";

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



bool MxTelemetryGeneralWrapper::isChartOptionEnabledInContextMenu() const noexcept
{
    return (m_chartList->size() > 1);
}


QString MxTelemetryGeneralWrapper::fromMxTypeValueToName(const int a_type) noexcept
{
    return QString(MxTelemetry::Type::name(a_type));
}


int MxTelemetryGeneralWrapper::fromMxTypeNameToValue(const QString& a_name) noexcept
{
    return static_cast<int>(MxTelemetry::Type::lookup(a_name.toStdString().c_str()));
}


 int MxTelemetryGeneralWrapper::mxTypeSize() noexcept
{
    return MxTelemetry::Type::N;
}


bool MxTelemetryGeneralWrapper::isChartFunctionalityEnabled() const noexcept
{
    return (m_activeDataSet[0] != 0
            or
            m_activeDataSet[1] != 0);
}



void* MxTelemetryGeneralWrapper::mxTelemetryMsgDataRetriever(void* const a_mxTelemetryMsg) const noexcept
{
    // used for doing the casting for specific type
    // each substype does not care about all the other types
    using namespace MxTelemetry;
    MxTelemetry::Msg* a_msg = static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg);
    const int l_mxTelemetryTypeIndex = static_cast<int>(a_msg->hdr().type);
    void* l_result;

    switch (l_mxTelemetryTypeIndex) {
    case Type::Heap: {
        l_result = &(a_msg->as<MxTelemetry::Heap>());
    } break;
    case Type::HashTbl: {
        l_result = &(a_msg->as<MxTelemetry::HashTbl>());
    } break;
    case Type::Thread: {
        l_result = &(a_msg->as<MxTelemetry::Thread>());
    } break;
    case Type::Multiplexer: {
        l_result = &(a_msg->as<MxTelemetry::Multiplexer>());
    } break;
    case Type::Socket: {
        l_result = &(a_msg->as<MxTelemetry::Socket>());
    } break;
    case Type::Queue: {
        l_result = &(a_msg->as<MxTelemetry::Queue>());
    } break;
    case Type::Engine: {
        l_result = &(a_msg->as<MxTelemetry::Engine>());
    } break;
    case Type::Link: {
        l_result = &(a_msg->as<MxTelemetry::Link>());
    } break;
    case Type::DBEnv: {
        l_result = &(a_msg->as<MxTelemetry::DBEnv>());
    } break;
    case Type::DBHost: {
        l_result = &(a_msg->as<MxTelemetry::DBHost>());
    } break;
    case Type::DB: {
        l_result = &(a_msg->as<MxTelemetry::DB>());
    } break;
    default: {
        qWarning() << "Unkown message header, ignoring:";
        l_result = nullptr;
    } break;
    }
    return l_result;
}


int MxTelemetryGeneralWrapper::fromMxTypeToTelMonType(const int a_type) noexcept
{
    int l_result;
    switch (a_type) {
    case MxTelemetry::Type::Heap: {
        l_result = Type::Heap;
    } break;
    case MxTelemetry::Type::HashTbl: {
        l_result = Type::HashTbl;
    } break;
    case MxTelemetry::Type::Thread: {
        l_result = Type::Thread;
    } break;
    case MxTelemetry::Type::Multiplexer: {
        l_result = Type::Multiplexer;
    } break;
    case MxTelemetry::Type::Socket: {
        l_result = Type::Socket;
    } break;
    case MxTelemetry::Type::Queue: {
        l_result = Type::Queue;
    } break;
    case MxTelemetry::Type::Engine: {
        l_result = Type::Engine;
    } break;
    case MxTelemetry::Type::Link: {
        l_result = Type::Link;
    } break;
    case  MxTelemetry::Type::DBEnv: {
        l_result = Type::DBEnv;
    } break;
    case MxTelemetry::Type::DBHost: {
        l_result = Type::DBHost;
    } break;
    case MxTelemetry::Type::DB: {
        l_result = Type::DB;
    } break;
    default: {
        // error msg should be printed by caller
        l_result = Type::Unknown;
    } break;
    }
    return l_result;
}


void MxTelemetryGeneralWrapper::getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept
{
    _getDataForTable(mxTelemetryMsgDataRetriever(a_mxTelemetryMsg), a_result);
}


const QString MxTelemetryGeneralWrapper::getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept
{
    return _getPrimaryKey(mxTelemetryMsgDataRetriever(a_mxTelemetryMsg));
}


const char* MxTelemetryGeneralWrapper::getMsgHeaderName(void* const a_mxTelemetryMsg) noexcept
{
    MxTelemetry::Msg* a_msg = static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg);
    return  MxTelemetry::Type::name(a_msg->hdr().type);
}


int MxTelemetryGeneralWrapper::getMsgHeaderType(void* const a_mxTelemetryMsg) noexcept
{
    MxTelemetry::Msg* a_msg = static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg);
    return static_cast<int>(a_msg->hdr().type);
}


