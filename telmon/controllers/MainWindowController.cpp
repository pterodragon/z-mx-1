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



#include "MainWindowController.h"
#include "models/raw/MainWindowModel.h"
#include "views/raw/MainWindowView.h"
#include "BasicController.h"
#include "DockWindowController.h"
#include "factories/ControllerFactory.h"
#include "QAbstractItemView"
#include "QAction"

#include "QDebug" // perhaps remove after testing

#include "QDockWidget" // For Dock testing
#include "QListWidget" // For Dock testing
#include "QtCharts"


MainWindowController::MainWindowController(QWidget *parent) :
    QMainWindow(parent),
    m_mainWindowModel(new MainWindowModel()),
    m_mainWindowView(new MainWindowView(this)),
    m_controllersDB(new QMap<unsigned int, BasicController*>()) // key, value
{
    m_mainWindowView->initMenuBar();
    m_mainWindowView->setGeometry();
    m_mainWindowView->setWindowTitle();

    //init Tree Menu Widget
    const unsigned int l_key = ControllerFactory::CONTROLLER_TYPE::TREE_MENU_WIDGET_CONTROLLER;

    // create
    initController(l_key);
    // set as central
    setCentralWidget(m_controllersDB->value(l_key)->getView());

    // init Dock Windows controller
    initController(ControllerFactory::CONTROLLER_TYPE::TABLE_DOCK_WINDOW_CONTROLLER);

    createActions();
    //createDockWindows();
}

MainWindowController::~MainWindowController()
{
    qDebug() << "~MainWindowController() - Begin";

    // destroy dock windows first
    foreach (auto child, findChildren<QDockWidget *>())
    {
        qDebug() << "closing" << child << "status:" << child->close();
    }

    for (auto i = m_controllersDB->begin(); i != m_controllersDB->end() ;i++) { delete i.value(); }
    //foreach (auto controller, m_controllersDB) { delete controller} //totest and use //qt  foreach keyword
    delete m_controllersDB;
    delete m_mainWindowModel;
    delete m_mainWindowView;
    qDebug() << "~MainWindowController() - End";
}


void MainWindowController::initController(const unsigned int a_key) noexcept
{
    // construct controller and insert the DB of controllers
    m_controllersDB->insert( a_key, ControllerFactory::getInstance().getController(a_key, *m_mainWindowModel->getDataDistributor(), *this));
}


void MainWindowController::terminateController(const unsigned int a_key) noexcept
{
    // sanity check
    if (!m_controllersDB->contains(a_key))
    {
        qCritical() << "terminateController called with invalid key:" << a_key << "returning...";
        return;
    }

    delete m_controllersDB->value(a_key);
    if (m_controllersDB->remove(a_key) != 1)
    {
        //should never happen! (because we already checked the DB contains the key
        qCritical() << "Failed to remove key:" << a_key << "m_controllersDB, although already deleted it, something went wrong!";
    }
}


void MainWindowController::createActions() noexcept
{
    // todo - improve lambda performance by using the example from "ZmFn.hp"
    // File->Connect functionality
    QObject::connect(m_mainWindowView->m_connectSubMenu, &QAction::triggered, this, [this](){
        qInfo() << "MainWindowController::conncet ";
        m_mainWindowModel->connect();
        this->m_mainWindowView->m_connectSubMenu->setEnabled(false);
        this->m_mainWindowView->m_disconnectSubMenu->setEnabled(true);
    });

    // File->Disconnect functionality
    QObject::connect(m_mainWindowView->m_disconnectSubMenu, &QAction::triggered, this, [this](){
        qInfo() << "MainWindowController::disconnect ";
        this->m_mainWindowModel->disconnect();
        this->m_mainWindowView->m_connectSubMenu->setEnabled(true);
        this->m_mainWindowView->m_disconnectSubMenu->setEnabled(false);

        // for now, destruct and re constrcut of treeWidget, in the future, just clean
        //terminateTreeMenuWidget();
        const unsigned int l_key = ControllerFactory::CONTROLLER_TYPE::TREE_MENU_WIDGET_CONTROLLER;
        terminateController(l_key);

        // init again
        // create
        initController(l_key);

        // set as central
        setCentralWidget(m_controllersDB->value(l_key)->getView());
    });

    // File->Exit functionality
    QObject::connect(m_mainWindowView->m_exitSubMenu, &QAction::triggered, this, &QWidget::close);
}


void MainWindowController::createDockWindows()
{
//    QDockWidget *dock2 = new QDockWidget(tr("Heap::MxTelemetry.Msg Chart"), this);
//    //dock2->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
//    QListWidget *customerList;
//    customerList = new QListWidget(dock2);
//    customerList->addItems(QStringList()
//            << "John Doe, Harmony Enterprises, 12 Lakeside, Ambleton"
//            << "Jane Doe, Memorabilia, 23 Watersedge, Beaton"
//            << "Tammy Shea, Tiblanka, 38 Sea Views, Carlton"
//            << "Tim Sheen, Caraba Gifts, 48 Ocean Way, Deal"
//            << "Sol Harvey, Chicos Coffee, 53 New Springs, Eccleston"
//            << "Sally Hobart, Tiroli Tea, 67 Long River, Fedula");
//    dock2->setWidget(customerList);
//    addDockWidget(Qt::RightDockWidgetArea, dock2);

    QtCharts::QLineSeries *upper_series = new QtCharts::QLineSeries();
    QtCharts::QLineSeries *lower_series = new QtCharts::QLineSeries();

    *upper_series << QPointF(1, 5) << QPointF(3, 7); //<< QPointF(7, 6) << QPointF(9, 7) << QPointF(12, 6)
             //<< QPointF(16, 7) << QPointF(18, 5);
    *lower_series << QPointF(1, 0) << QPointF(3, 0); //<< QPointF(7, 0) << QPointF(9, 0) << QPointF(12, 0)
             //<< QPointF(16, 0) << QPointF(18, 0);

    QAreaSeries *series = new QAreaSeries(upper_series, lower_series);
    //series->setName("Batman");
    QPen pen(0x059605);
    pen.setWidth(3);
    series->setPen(pen);

    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
    gradient.setColorAt(0.0, 0x3cc63c);
    gradient.setColorAt(1.0, 0x26f626);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    series->setBrush(gradient);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Heap::MxTelemetry.Msg");
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setRange(0, 200);
    chart->axes(Qt::Vertical).first()->setRange(0, 100);

    class chartViewSized : public QChartView {
    public:

        chartViewSized(QChart *chart, QWidget *parent = nullptr):
            QChartView(chart) {}

        QSize sizeHint() const override final
        {
            return QSize(600,600);
        };

    protected:
        void keyPressEvent(QKeyEvent *event)
        {
            QAreaSeries *series = (QAreaSeries *) chart()->series().first();
            switch (event->key()) {
            case Qt::Key_1:
                qDebug() << "Qt::Key_1:";
                for (auto i = 0;i < series->lowerSeries()->count(); i++) {
                    qDebug() << "upperSeries["<< i <<"]" << series->upperSeries()->at(i).x() << series->upperSeries()->at(i).y();
                    qDebug() << "lowerSeries["<< i <<"]" << series->lowerSeries()->at(i).x() << series->lowerSeries()->at(i).y();
                }
                // 1. get last point x and y, and append the new point
                double u_x = series->upperSeries()->at(series->upperSeries()->count()-1).x();
                double u_y = series->upperSeries()->at(series->upperSeries()->count()-1).y();

                if (u_y > 100) {u_y = 24;}

                series->upperSeries()->append(QPointF(u_x+1, u_y+1));
                series->lowerSeries()->append(QPointF(u_x+1, 0));
                //chart()->axes(Qt::Horizontal).first()->setRange(0, u_x+1);
                //chart()->axes(Qt::Vertical).first()->setRange(0, u_y+1);
                break;
            }
        }
     private:
         QAreaSeries *series;

    };
    QDockWidget *dock = new QDockWidget(tr("Heap::MxTelemetry.Msg Graphic Chart"), this);
    //dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QChartView *chartView = new chartViewSized(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->sizeHint();
    dock->setWidget(chartView);
    addDockWidget(Qt::RightDockWidgetArea, dock);

}


void MainWindowController::dockWindowsManager(const unsigned int a_dockWindowType,
                                              const QString& a_mxTelemetryTypeName,
                                              const QString& a_mxTelemetryInstanceName) noexcept
{
    qDebug() << "dockWindowsManager" << a_dockWindowType << a_mxTelemetryTypeName << a_mxTelemetryInstanceName;

    // NOTICE - GRAPH IS NOT SUPPORTED YET
    auto l_dockWindowType = ControllerFactory::CONTROLLER_TYPE::TABLE_DOCK_WINDOW_CONTROLLER;
    //

    // sanity check
    if (!m_controllersDB->contains(l_dockWindowType))
    {
        qCritical() << "dockWindowsManager called with invalid a_dockWindowType:"
                    << l_dockWindowType
                    << a_mxTelemetryTypeName
                    << a_mxTelemetryInstanceName
                    << "returning...";
        return;
    }

    // only for readability
    DockWindowController* l_dockWindowController = static_cast<DockWindowController*>(m_controllersDB->value(l_dockWindowType));

    unsigned int l_action = DockWindowController::ACTIONS::NO_ACTION;
    QDockWidget* l_dockWidget = nullptr;

    l_dockWindowController->handleUserSelection(l_action, l_dockWidget, findChildren<QDockWidget *>(), a_mxTelemetryTypeName, a_mxTelemetryInstanceName);

    switch (l_action) {
    case DockWindowController::ACTIONS::NO_ACTION:
        break;
    case DockWindowController::ACTIONS::ADD:
        if (!l_dockWidget) {qCritical() << "DockWindowController::ACTIONS::DO_ADD recived l_dockWidget=nullptr!, doing nothing"
                                        << a_dockWindowType << a_mxTelemetryTypeName << a_mxTelemetryInstanceName; return;}
        l_dockWidget->setParent(this);
        // handle orientation
        addDockWidget(Qt::RightDockWidgetArea, l_dockWidget, Qt::Horizontal);
        break;
    default:
        qCritical() << "Recived unknown action:" << l_action << "doing nothing"
                    << a_dockWindowType << a_mxTelemetryTypeName << a_mxTelemetryInstanceName;
        break;
    }
}


