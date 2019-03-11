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




#include "GraphWidgetDockWindowController.h"
#include "QDebug"
#include "models/wrappers/BasicDockWidget.h"
#include "models/wrappers/GraphDockWidgetModelWrapper.h"


//#include "views/raw/BasicChartView.h"

GraphWidgetDockWindowController::GraphWidgetDockWindowController(DataDistributor& a_dataDistributor):
    DockWindowController(a_dataDistributor, " Chart"),
    m_graphDockWidgetModelWrapper(new GraphDockWidgetModelWrapper(a_dataDistributor))
{
    qDebug() << "    GraphWidgetDockWindowController()";
}


GraphWidgetDockWindowController::~GraphWidgetDockWindowController()
{
    qDebug() << "    ~GraphWidgetDockWindowController() begin";
    delete m_graphDockWidgetModelWrapper;
    qDebug() << "    ~GraphWidgetDockWindowController() end";
}


QAbstractItemModel* GraphWidgetDockWindowController::getModel()
{
    //TODO
    return nullptr;
}


QAbstractItemView*  GraphWidgetDockWindowController::getView()
{
    //TODO
    return nullptr;
}

// DockWindowController interface
void GraphWidgetDockWindowController::handleUserSelection(unsigned int& a_action,
                                                          QDockWidget*& a_dockWidget,
                                                          const QList<QDockWidget *>& a_currentDockList,
                                                          const QString& a_mxTelemetryTypeName,
                                                          const QString& a_mxTelemetryInstanceName) noexcept
{
    // constrct widget name
    // for now, using same name for object and chart header
    const QString l_objectName = a_mxTelemetryTypeName + "::" + a_mxTelemetryInstanceName + m_dockWindowName;

    // is dock widget already exists?
    QDockWidget* l_dock = nullptr;
    bool l_contains = isDockWidgetExists(a_currentDockList, l_objectName, l_dock);

    // handle case that dock widget already exists
    if (l_contains)
    {
        qDebug() << m_dockWindowName << l_objectName << "already exists";
        // out policy for table, only one at a time
        a_action = DockWindowController::ACTIONS::NO_ACTION;
        a_dockWidget = nullptr;
        l_dock->activateWindow();
        return;
    }

    qDebug() << "constrcuting" << m_dockWindowName << l_objectName;

    //handle case that dock widget not exists
    a_action = DockWindowController::ACTIONS::ADD;

    // construct the new dock
    a_dockWidget = new BasicDockWidget(l_objectName,
                                  m_graphDockWidgetModelWrapper,
                                  a_mxTelemetryTypeName,
                                  a_mxTelemetryInstanceName,
                                  nullptr);

    // set dock properties
    a_dockWidget->setAttribute(Qt::WA_DeleteOnClose);       // delete when user close the window
    a_dockWidget->setAllowedAreas(Qt::RightDockWidgetArea); // allocate to the right of the window


    // create the chart view
    QChartView *l_chartView = m_graphDockWidgetModelWrapper->getChartView(a_mxTelemetryTypeName,
                                                                          a_mxTelemetryInstanceName);

    // associate with l_dock;
    l_chartView->setParent(l_dock);

    // associate with chart view
    a_dockWidget->setWidget(l_chartView);

//    //register to datadistributor
}


////    QDockWidget *dock2 = new QDockWidget(tr("Heap::MxTelemetry.Msg Chart"), this);
////    //dock2->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
////    QListWidget *customerList;
////    customerList = new QListWidget(dock2);
////    customerList->addItems(QStringList()
////            << "John Doe, Harmony Enterprises, 12 Lakeside, Ambleton"
////            << "Jane Doe, Memorabilia, 23 Watersedge, Beaton"
////            << "Tammy Shea, Tiblanka, 38 Sea Views, Carlton"
////            << "Tim Sheen, Caraba Gifts, 48 Ocean Way, Deal"
////            << "Sol Harvey, Chicos Coffee, 53 New Springs, Eccleston"
////            << "Sally Hobart, Tiroli Tea, 67 Long River, Fedula");
////    dock2->setWidget(customerList);
////    addDockWidget(Qt::RightDockWidgetArea, dock2);

//    QtCharts::QLineSeries *upper_series = new QtCharts::QLineSeries();
//    QtCharts::QLineSeries *lower_series = new QtCharts::QLineSeries();

//    *upper_series << QPointF(1, 5) << QPointF(3, 7); //<< QPointF(7, 6) << QPointF(9, 7) << QPointF(12, 6)
//             //<< QPointF(16, 7) << QPointF(18, 5);
//    *lower_series << QPointF(1, 0) << QPointF(3, 0); //<< QPointF(7, 0) << QPointF(9, 0) << QPointF(12, 0)
//             //<< QPointF(16, 0) << QPointF(18, 0);

//    QAreaSeries *series = new QAreaSeries(upper_series, lower_series);
//    //series->setName("Batman");
//    QPen pen(0x059605);
//    pen.setWidth(3);
//    series->setPen(pen);

//    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
//    gradient.setColorAt(0.0, 0x3cc63c);
//    gradient.setColorAt(1.0, 0x26f626);
//    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
//    series->setBrush(gradient);

//    QChart *chart = new QChart();
//    chart->addSeries(series);
//    chart->setTitle("Heap::MxTelemetry.Msg");
//    chart->createDefaultAxes();
//    chart->axes(Qt::Horizontal).first()->setRange(0, 200);
//    chart->axes(Qt::Vertical).first()->setRange(0, 100);


//    QDockWidget *dock = new QDockWidget(tr("Heap::MxTelemetry.Msg Graphic Chart"), this);
//    //dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
//    QChartView *chartView = new chartViewSized(chart);
//    chartView->setRenderHint(QPainter::Antialiasing);
//    chartView->sizeHint();
//    dock->setWidget(chartView);
//    addDockWidget(Qt::RightDockWidgetArea, dock);










