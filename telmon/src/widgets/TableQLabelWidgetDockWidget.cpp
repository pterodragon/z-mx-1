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

#include "TableQLabelWidgetDockWidget.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "src/widgets/TableQLabelWidget.h"
#include "src/subscribers/TabelQLabelSubscriber.h"
#include "src/distributors/DataDistributor.h"
#include "QCloseEvent"

TableQLabelWidgetDockWidget::TableQLabelWidgetDockWidget(const int       a_mxTelemetryType,
                                                         const QString&  a_mxTelemetryInstanceName,
                                                         DataDistributor& a_dataDistributor,
                                                         QWidget*        a_parent,
                                                         Qt::WindowFlags a_flags):
    BasicDockWidget (a_mxTelemetryType,
                     a_mxTelemetryInstanceName,
                     QString(MxTelemetryGeneralWrapper::fromMxTypeValueToName(a_mxTelemetryType) +
                             "::" +
                             a_mxTelemetryInstanceName +
                             "TableQLabelWidgetDockWidget"),
                     a_parent,
                     a_flags),
    m_dataDistributor(&a_dataDistributor),
    m_subscriber(new TabelQLabelSubscriber(a_mxTelemetryType, a_mxTelemetryInstanceName))
{
    setAttribute(Qt::WA_DeleteOnClose);

    // hide title
    setTitleBarWidget(new QWidget(this));

    auto* l_label = new TableQLabelWidget(this);

    connect(m_subscriber, &TabelQLabelSubscriber::updateDone,
            l_label,      &TableQLabelWidget::setText);

    setWidget(l_label);

    // connect to close functioanility
    connect(l_label, &TableQLabelWidget::closeAction, this, [this]() {
        delete this; // emitting QObject::destroyed as part of the delete process
    });

    // subscribe
    m_dataDistributor->subscribe(m_mxTelemetryType, m_subscriber);
}


TableQLabelWidgetDockWidget::~TableQLabelWidgetDockWidget()
{
    // Important: assign to QT smart pointer for later delete
    // otherwise, delete causes segmanation fault if user closes view
    // This way makes sure delete event will happen only after event loop (of qt) has finished
    auto* l_label = static_cast<TableQLabelWidget*>(widget());
    l_label->setParent(nullptr);
    QSharedPointer<TableQLabelWidget>(l_label,  &QObject::deleteLater);

    m_dataDistributor->unsubscribe(m_mxTelemetryType, m_subscriber);

    delete m_subscriber;
    m_subscriber = nullptr;
}


void TableQLabelWidgetDockWidget::closeEvent(QCloseEvent *event)
{
    event->accept();

    /**
     * I had some wired problem, which may be related to linkage order
     * even when i tell the program to close this DockWidget explicitly
     * it will not call the destrcutor, but only in the end
     */
    delete this;
}

