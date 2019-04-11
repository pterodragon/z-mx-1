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
#include "src/views/raw/BasicChartView.h"
#include "QCloseEvent"

ChartDockWidget::ChartDockWidget(const int a_mxTelemetryType,
                                 const QString& a_mxTelemetryInstance,
                                 BasicChartView* a_setWidget,
                                 BasicChartController* const a_controller,
                                 const int a_closeActionType,
                                 QWidget *parent):
    QDockWidget (QString(), parent),
    m_mxTelemetryType(a_mxTelemetryType),
    m_mxTelemetryInstance(new QString(a_mxTelemetryInstance)),
    m_className(new QString("ChartDockWidget::"
                            + MxTelemetryGeneralWrapper::fromMxTypeValueToName(m_mxTelemetryType)
                            + "::"
                            + m_mxTelemetryInstance)),
    m_isTitleBarHidden(false),
    m_chartController(a_controller),
    m_closeActionType(a_closeActionType)
{
    hideTitleBar();
    setWidget(a_setWidget);
    setAttribute(Qt::WA_DeleteOnClose);

    // set close functionanility
    connect(a_setWidget, &BasicChartView::closeAction, this, [this](){
        emit closeAction(m_closeActionType);
        delete this;
    });
}


ChartDockWidget::~ChartDockWidget()
{
    qInfo() << QString("~" + *m_className);

    setParent(nullptr);
    auto* l_view = static_cast<BasicChartView*>(this->widget());

    // disassociate with ChartDockWidget
    l_view->setParent(nullptr);

    // update controller
    if (!m_chartController->removeView(l_view))
    {
        qCritical() << QString("~" + *m_className)
                    << "failed to remove view from controller";
    }

    // Important: assign to QT smart pointer for later delete
    // not using this way to delete will cause segmanation fault every time we close the view
    // by deleting this way, we make sure we delete only when then event loop (of qt) has finished
    QSharedPointer<BasicChartView>(l_view,  &QObject::deleteLater);

    setWidget(nullptr);

    delete m_mxTelemetryInstance;
    m_mxTelemetryInstance = nullptr;

    delete m_className;
    m_className = nullptr;
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


void ChartDockWidget::closeEvent(QCloseEvent *event)
{
    qDebug() << "ChartDockWidget::closeEvent";
    event->accept();

    /**
     * I had some wired problem, which may be related to linkage order
     * even when i tell the program to close this DockWidget explicitly
     * it will not call the destrcutor, but only in the end
     */
    delete this;
}




