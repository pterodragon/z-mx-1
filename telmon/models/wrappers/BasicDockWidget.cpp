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
#include "QDebug"
#include "QTableWidget"
#include "QLayout"
#include "models/wrappers/TableModelWrapper.h"
#include "QCloseEvent"

BasicDockWidget::BasicDockWidget(const QString &title,
                               TableModelWrapper* a_tempModelWrapper,
                               const QString& a_mxTelemetryTypeName,
                               const QString& a_mxTelemetryInstanceNameQWidget,
                               QWidget *parent):
    QDockWidget (title, parent),
    m_tempModelWrapper(a_tempModelWrapper),
    m_mxTelemetryTypeName(a_mxTelemetryTypeName),
    m_mxTelemetryInstanceName(a_mxTelemetryInstanceNameQWidget)
{

}


BasicDockWidget::~BasicDockWidget()
{
    qDebug() << "    ~BasicDockWidget() -BEGIN";
    //unsubscribe
    if (m_tempModelWrapper)
    {
        m_tempModelWrapper->unsubscribe(m_mxTelemetryTypeName, m_mxTelemetryInstanceName);
        //m_tempModelWrapper = nullptr;
    }

    // dont delete the table!
    if (this->widget() != nullptr) {this->widget()->setParent(nullptr);}
    setWidget(nullptr);
    qDebug() << "    ~BasicDockWidget() -END";

}


const QString& BasicDockWidget::getMxTelemetryTypeName() const noexcept
{
    return m_mxTelemetryTypeName;
}


const QString& BasicDockWidget::getMxTelemetryInstanceName() const noexcept
{
    return m_mxTelemetryInstanceName;
}


void BasicDockWidget::closeEvent(QCloseEvent *event)
{

    event->accept();

    /**
     * I had some wired problem, which may be related to linkage order
     * even when i tell the program to close this DockWidget explicitly
     * it will not call the destrcutor, but only in the end
     */
    delete this;
}


