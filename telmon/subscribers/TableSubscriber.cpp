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
#include "TableSubscriber.h"
#include "QLinkedList"
#include "QDebug"

#include <sstream> //for ZmBitmapToQString


TableSubscriber::TableSubscriber(const QString a_name):
    m_name(a_name),
    m_tableName(QString()), // set null QString
    m_stream(new std::stringstream())
{

}


TableSubscriber::~TableSubscriber()
{
    delete m_stream;
};


const QString& TableSubscriber::getName() const noexcept
{
    return m_name;
}


void TableSubscriber::setName(const QString& a_name) noexcept
{
    m_name = a_name;
}


const QString& TableSubscriber::getTableName() const noexcept
{
    return m_tableName;
}


void TableSubscriber::setTableName(QString a_name) noexcept
{
    m_tableName = a_name;
}



void TableSubscriber::update(void* a_mxTelemetryMsg)
{
     m_lambda(this, a_mxTelemetryMsg);
}



void TableSubscriber::setUpdateFunction( std::function<void(TableSubscriber* a_this,
                                                            void* a_mxTelemetryMsg)>     a_lambda )
{
    m_lambda = a_lambda;
}



std::string TableSubscriber::getCurrentTime() const noexcept
{
    ZtDate::CSVFmt nowFmt;
    ZtDate now(ZtDate::Now);
    return (ZuStringN<512>() << now.csv(nowFmt)).data();
}


QString TableSubscriber::ZmBitmapToQString(const ZmBitmap& a_zmBitMap) const noexcept
{
    a_zmBitMap.print(*m_stream);
    const QString l_result = QString::fromStdString(m_stream->str());
    m_stream->str(std::string());
    m_stream->clear();
    return l_result;
}



QString TableSubscriber::ZiIPTypeToQString(const ZiIP& a_ZiIP) const noexcept
{
    a_ZiIP.print(*m_stream);
    const QString l_result = QString::fromStdString(m_stream->str());
    m_stream->str(std::string());
    m_stream->clear();
    return l_result;
}


QString TableSubscriber::ZiCxnFlagsTypeToQString(const uint32_t a_ZiCxnFlags) const noexcept
{
    *m_stream << ZiCxnFlags::Flags::print(a_ZiCxnFlags);
    const QString l_result = QString::fromStdString(m_stream->str());
    m_stream->str(std::string());
    m_stream->clear();
    return l_result;
}

bool TableSubscriber::isAssociatedWithTable() const noexcept
{
    if (m_tableName.isNull() || m_tableName.isEmpty())
    {
        qWarning() << m_name
                   << "is not registered to any table, please setTableName or unsubscribe, returning..."
                   << "table name is:"
                   << getTableName();
        return false;
    }
    return true;
}


bool TableSubscriber::isTelemtryInstanceNameMatchsTableName(const QString& a_instanceName) const noexcept
{
    return  (m_tableName ==  a_instanceName);
}



