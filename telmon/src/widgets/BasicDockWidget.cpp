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


#include "BasicDockWidget.h"


BasicDockWidget::BasicDockWidget(const int       a_mxTelemetryTypeName,
                                 const QString&  a_mxTelemetryInstanceName,
                                 const QString&  a_className,
                                 QWidget*        a_parent,
                                 Qt::WindowFlags a_flags):
    QDockWidget(QString(), a_parent, a_flags),
    m_mxTelemetryType(a_mxTelemetryTypeName),
    m_mxTelemetryInstanceName(new QString(a_mxTelemetryInstanceName)),
    m_className(new QString(a_className))
{

}


BasicDockWidget::~BasicDockWidget()
{
    delete m_className;
    m_className = nullptr;

    delete m_mxTelemetryInstanceName;
    m_mxTelemetryInstanceName = nullptr;
}


const QString& BasicDockWidget::getClassName() const noexcept
{
    return *m_className;
}


int BasicDockWidget::getMxTelemetryType() const noexcept
{
    return m_mxTelemetryType;
}


const QString& BasicDockWidget::getMxTelemetryInstanceName() const noexcept
{
    return *m_mxTelemetryInstanceName;
}




