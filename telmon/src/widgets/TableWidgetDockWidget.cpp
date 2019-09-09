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

#include "TableWidgetDockWidget.h"
#include "src/models/wrappers/DockWidgetModelWrapper.h"
#include "QCloseEvent"
#include "QDebug"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


TableWidgetDockWidget::TableWidgetDockWidget(DockWidgetModelWrapper* a_dockWidgetModelWrapper,
                                             const int               a_mxTelemetryType,
                                             const QString&          a_mxTelemetryInstanceName,
                                             QWidget*                a_parent):
    BasicDockWidget(a_mxTelemetryType,
                    a_mxTelemetryInstanceName,
                    QString(MxTelemetryGeneralWrapper::fromMxTypeValueToName(a_mxTelemetryType) +
                            "::" +
                            a_mxTelemetryInstanceName +
                            "TableWidgetDockWidget"),
                    a_parent),
    m_dockWidgetModelWrapper(a_dockWidgetModelWrapper),
    m_isTitleBarHidden(false)
{

}


TableWidgetDockWidget::~TableWidgetDockWidget()
{
    if (m_dockWidgetModelWrapper)
    {
        m_dockWidgetModelWrapper->unsubscribe(m_mxTelemetryType, *m_mxTelemetryInstanceName);
    }

    // dont delete the table!
    if (this->widget() != nullptr) {this->widget()->setParent(nullptr);}
    setWidget(nullptr);
}


void TableWidgetDockWidget::closeEvent(QCloseEvent *event)
{
    event->accept();

    /**
     * I had some wired problem, which may be related to linkage order
     * even when i tell the program to close this DockWidget explicitly
     * it will not call the destrcutor, but only in the end
     */
    delete this;
}


void TableWidgetDockWidget::hideTitleBar() noexcept
{
    // set no title for the dock widget
    // see https://stackoverflow.com/questions/18918496/qdockwidget-without-a-title-bar

    // had a lot of issues with this one, eventually this one works
    if (m_isTitleBarHidden == true)
    {
        qWarning() << __PRETTY_FUNCTION__
                   << "title bar already hidden, doing nothing";
        return;
    }

    m_isTitleBarHidden = true;
    setTitleBarWidget(new QWidget(this));
}


