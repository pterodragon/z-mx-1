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
#include "QObjectDataSubscriber.h"
#include "QDebug"

QObjectDataSubscriber::QObjectDataSubscriber(const QString& a_subscriberName):
    m_subscriberName(a_subscriberName),
    m_associateObjectName(QString()) // set null QString
{

}


QObjectDataSubscriber::~QObjectDataSubscriber()
{

};


const QString& QObjectDataSubscriber::getName() const noexcept
{
    return m_subscriberName;
}


void QObjectDataSubscriber::setName(const QString& a_subscriberName) noexcept
{
    m_subscriberName = a_subscriberName;
}


const QString& QObjectDataSubscriber::getAssociatedObjectName() const noexcept
{
    return m_associateObjectName;
}


void QObjectDataSubscriber::setAssociatedObjesctName(QString a_associateObjectName) noexcept
{
    m_associateObjectName = a_associateObjectName;
}


std::string QObjectDataSubscriber::getCurrentTime() const noexcept
{
    ZtDate::CSVFmt nowFmt;
    ZtDate now(ZtDate::Now);
    return (ZuStringN<512>() << now.csv(nowFmt)).data();
}


bool QObjectDataSubscriber::isAssociatedWithObject() const noexcept
{
    if (m_associateObjectName.isNull() || m_associateObjectName.isEmpty())
    {
        qWarning() << m_subscriberName
                   << "is not registered to any object, please setTableName or unsubscribe, returning..."
                   << "current object name is:"
                   << m_associateObjectName;
        return false;
    }
    return true;
}


bool QObjectDataSubscriber::isTelemtryInstanceNameMatchsObjectName(const QString& a_instanceName) const noexcept
{
    return  (m_associateObjectName ==  a_instanceName);
}
