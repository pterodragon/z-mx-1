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


#include "ChartDockWidget.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "src/controllers/BasicChartController.h"
#include "QCloseEvent"

#include "QSplitter"

ChartDockWidget::ChartDockWidget(const int a_mxTelemetryType,
                                 const QString& a_mxTelemetryInstance,
                                 QWidget *parent):
    QDockWidget (QString(), parent),
    m_mxTelemetryType(a_mxTelemetryType),
    m_mxTelemetryInstance(new QString(a_mxTelemetryInstance)),
    m_className(new QString("ChartDockWidget::"
                            + MxTelemetryGeneralWrapper::fromMxTypeValueToName(m_mxTelemetryType)
                            + "::"
                            + m_mxTelemetryInstance)),
    m_isTitleBarHidden(false),
    m_chartController(nullptr)
{
    hideTitleBar();
    setAttribute(Qt::WA_DeleteOnClose);
}


ChartDockWidget::~ChartDockWidget()
{
    qDebug() << QString("~" + *m_className + " B");
    // deleting chart view through the controller
    if (widget())
    {
        if (m_chartController)
        {
            m_chartController->finalizeView(widget());
        } else {
            qCritical() << *m_className
                        << __PRETTY_FUNCTION__
                        << "Closing chartDockWidget without associated m_chartController";
        }
    }
    setWidget(nullptr);
    delete m_mxTelemetryInstance;
    m_mxTelemetryInstance = nullptr;

    delete m_className;
    m_className = nullptr;

    static_cast<QSplitter*>(this->parent())->findChild<ChartDockWidget*>()->hide();
//    qDebug() << "static_cast<QSplitter*>(this->parent())->findChild<ChartDockWidget*>();"
//             << static_cast<QSplitter*>(this->parent())->findChild<ChartDockWidget*>()->hide();
    qDebug() << "this->parent()" << this->parent();
}


void ChartDockWidget::hideTitleBar() noexcept
{
    // set no title for the dock widget
    // see https://stackoverflow.com/questions/18918496/qdockwidget-without-a-title-bar

    // had a lot of issues with this one, eventually this one works
    if (m_isTitleBarHidden == true)
    {
        qWarning() << "BasicDockWidget::hideTitleBar() title bar already hidden, doing nothing";
        return;
    }

    m_isTitleBarHidden = true;
    setTitleBarWidget(new QWidget(this));

}


void ChartDockWidget::setController(BasicChartController* a_chartController) noexcept
{
    m_chartController = a_chartController;
}


void ChartDockWidget::closeEvent(QCloseEvent *event)
{
    event->accept();

    /**
     * I had some wired problem, which may be related to linkage order
     * even when i tell the program to close this DockWidget explicitly
     * it will not call the destrcutor, but only in the end
     */
    delete this;
}




