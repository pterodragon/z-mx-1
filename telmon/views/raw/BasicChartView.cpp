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


#include "ZmHeap.hpp"
#include "BasicChartView.h"


BasicChartView::BasicChartView(QChart *a_chart, QWidget *a_parent):
    QChartView(a_chart, a_parent),
    m_seriesVertical(new QLineSeries()),   // delete as part of QChart object destructor
    m_seriesHorizontal(new QLineSeries()),  // delete as part of QChart object destructor
    m_xAxisNumberOfPoints(10),
    m_activeData{LOCAL_ZmHeapTelemetry::size, LOCAL_ZmHeapTelemetry::alignment}
{
    qDebug() << "BasicChartView::BasicChartView";

    initMenuBar();
    createActions();

    // // trying new
    m_axisArray[CHART_AXIS::X] = new QValueAxis;
    m_axisArray[CHART_AXIS::Y_LEFT] = new QValueAxis;
    m_axisArray[CHART_AXIS::Y_RIGHT] = new QValueAxis;

    m_seriesArray[SERIES::SERIES_RIGHT] = new QSplineSeries;
    m_seriesArray[SERIES::SERIES_LEFT] = new QSplineSeries;

    getAxes(CHART_AXIS::X).setTickCount(10);

    // Adds the axis axis to the chart aligned as specified by alignment.
    // The chart takes the ownership of the axis.
    chart()->addAxis(&getAxes(CHART_AXIS::X), Qt::AlignBottom);

    m_axisArray[CHART_AXIS::X]->setRange(0, 10);

    // iterate over {SERIES::SERIES_LEFT, SERIES::SERIES_RIGHT}
    for (unsigned int l_curSeries = 0; l_curSeries < BasicChartView::SERIES::SERIES_N; l_curSeries++)
    {
        initSeries(l_curSeries);
    }


    // to make the chart look nicer
    setRenderHint(QPainter::Antialiasing);

}


BasicChartView::~BasicChartView()
{
    qDebug() << "BasicChartView::~BasicChartView";
    delete m_seriesVertical;
    delete m_seriesHorizontal;
    delete this->chart();
}


void BasicChartView::initSeries(const unsigned int a_series) noexcept
{
    unsigned int l_axis;
    (a_series == SERIES::SERIES_LEFT) ? (l_axis = CHART_AXIS::Y_LEFT) : (l_axis = CHART_AXIS::Y_RIGHT);

    Qt::Alignment l_alignment;
    (a_series == SERIES::SERIES_LEFT) ? (l_alignment = Qt::AlignLeft) : (l_alignment = Qt::AlignRight);

    chart()->addSeries(&getSeries(a_series));

    // init the series Y axis with same color
    auto l_rightSeriesColor = getSeries(a_series).color();
    getAxes(l_axis).setLinePenColor(l_rightSeriesColor);

    // attach to chart and associate with corresponding axis
    chart()->addAxis(&getAxes(l_axis), l_alignment);
    getSeries(a_series).attachAxis(&getAxes(l_axis));
    getSeries(a_series).attachAxis(&getAxes(CHART_AXIS::X));

    m_axisArray[l_axis]->setRange(0, 100); // todo: should be according to type

    const auto l_seriesName = localZmHeapTelemetry_ValueToString(m_activeData[a_series]);
    getSeries(a_series).setName(QString::fromStdString(l_seriesName));

}


void BasicChartView::createActions() noexcept
{

    auto l_rightSeriesMenuActions = m_rightSeriesMenu->actions();
    for (int i = 0; i < l_rightSeriesMenuActions.size(); i++)
    {
        QObject::connect(l_rightSeriesMenuActions.at(i), &QAction::triggered, this, [this, i](){
            this->changeSeriesData(SERIES::SERIES_RIGHT ,i);
        });
    }

    auto l_leftSeriesMenuActions  = m_leftSeriesMenu->actions();
    for (int i = 0; i < l_leftSeriesMenuActions.size(); i++)
    {
        QObject::connect(l_leftSeriesMenuActions.at(i), &QAction::triggered, this, [this, i](){
            this->changeSeriesData(SERIES::SERIES_LEFT ,i);
        });
    }
//    // File->Connect functionality
//    QObject::connect(m_mainWindowView->m_connectSubMenu, &QAction::triggered, this, [this](){
//        qInfo() << "MainWindowController::conncet ";
//        m_mainWindowModel->connect();
//        this->m_mainWindowView->m_connectSubMenu->setEnabled(false);
//        this->m_mainWindowView->m_disconnectSubMenu->setEnabled(true);
//    });

//    // File->Disconnect functionality
//    QObject::connect(m_mainWindowView->m_disconnectSubMenu, &QAction::triggered, this, [this](){
//        qInfo() << "MainWindowController::disconnect ";
//        this->m_mainWindowModel->disconnect();
//        this->m_mainWindowView->m_connectSubMenu->setEnabled(true);
//        this->m_mainWindowView->m_disconnectSubMenu->setEnabled(false);

//        // for now, destruct and re constrcut of treeWidget, in the future, just clean
//        //terminateTreeMenuWidget();
//        const unsigned int l_key = ControllerFactory::CONTROLLER_TYPE::TREE_MENU_WIDGET_CONTROLLER;
//        terminateController(l_key);

//        // init again
//        // create
//        initController(l_key);

//        // set as central
//        setCentralWidget(m_controllersDB->value(l_key)->getView());
//    });

//    // File->Exit functionality
//    QObject::connect(m_mainWindowView->m_exitSubMenu, &QAction::triggered, this, &QWidget::close);
}


void BasicChartView::changeSeriesData(const unsigned int a_series, const int data_type) noexcept
{
    const int l_oldDataType = static_cast<int>(m_activeData[a_series]);

    // set new active data
    m_activeData[a_series] = static_cast<unsigned int>(data_type);

    // set new range
    // TODO

    // set new name
    const auto l_seriesName = localZmHeapTelemetry_ValueToString(m_activeData[a_series]);
    getSeries(a_series).setName(QString::fromStdString(l_seriesName));

    // flush all current data
    getSeries(a_series).clear();

    // disable/enable this option from menu
    QMenu* l_menu;
    (a_series == SERIES::SERIES_LEFT) ? (l_menu = m_leftSeriesMenu) : (l_menu =  m_rightSeriesMenu);
    l_menu->actions().at(l_oldDataType)->setEnabled(true);
    l_menu->actions().at(static_cast<int>(data_type))->setDisabled(true);

}


QSize BasicChartView::sizeHint() const
{
    return QSize(600,600);
}


void BasicChartView::setUpdateFunction( std::function<void(BasicChartView* a_this,
                                                      void* a_mxTelemetryMsg)>   a_lambda )
{
    m_lambda = a_lambda;
}


void BasicChartView::updateData(ZmHeapTelemetry a_pair)
{
    m_lambda(this, &a_pair);
}


int BasicChartView::getVerticalAxesRange() const noexcept
{
    return m_xAxisNumberOfPoints;
}


void BasicChartView::setVerticalAxesRange(int a_range) noexcept
{
    m_xAxisNumberOfPoints = a_range;
}


QValueAxis& BasicChartView::getAxes(const unsigned int a_axis)
{
    if ( a_axis >= CHART_AXIS::CHART_AXIS_N )
    {
        qWarning() << "calling getAxes with invalid value:"
                   << a_axis
                   << "returning X Axis as default";
        return *(m_axisArray[CHART_AXIS::X]);
    }

    return *(m_axisArray[a_axis]);
}


const QValueAxis& BasicChartView::getAxes(const unsigned int a_axis) const
{
    if ( a_axis >= CHART_AXIS::CHART_AXIS_N )
    {
        qWarning() << "calling getAxes with invalid value:"
                   << a_axis
                   << "returning X Axis as default";
        return *(m_axisArray[CHART_AXIS::X]);
    }

    return *(m_axisArray[a_axis]);
}


QSplineSeries& BasicChartView::getSeries(const unsigned int a_series)
{
    return *(m_seriesArray[a_series]);
}


const QSplineSeries& BasicChartView::getSeries(const unsigned int a_series) const
{
    return *(m_seriesArray[a_series]);
}


bool BasicChartView::isExceedingLimits(const unsigned int a_series) const noexcept
{
    auto l_count = (m_seriesArray[a_series])->count();
    return l_count >= m_xAxisNumberOfPoints;
}


void BasicChartView::removeFromSeriesFirstPoint(const unsigned int a_series) noexcept
{
    if (m_seriesArray[a_series]->count() > 0)
    {
        m_seriesArray[a_series]->remove(0);
    } else {
        qWarning() << "removeFromSeriesFirstPoint was called while number of points is 0";
    }
}


void  BasicChartView::shiftLeft(const unsigned int a_series) noexcept
{
    const auto l_series = m_seriesArray[a_series];
    const auto l_count = l_series->count();

    // shift all point left
    qreal upper_x = 0;
    qreal upper_y = 0;

    for (int i = 0; i < l_count; i++)
    {
        //get curret point x
        upper_x = l_series->at(i).x() - 2;
        upper_y = l_series->at(i).y();
        l_series->replace(i, upper_x, upper_y);
    }

}


/**
 * @brief append point to the end of given series
 * @param lPoint
 */
void BasicChartView::appendPoint(const QPointF& a_point, const unsigned int a_series) noexcept
{
    m_seriesArray[a_series]->append(a_point);
}


void BasicChartView::initMenuBar() noexcept
{
    // initilization here for readiablity
    // Notice: no need to delete, i will deleted with object
    m_boxLayout = new QVBoxLayout(this);
    m_menuBar = new QMenuBar;
    m_settingsMenu = new QMenu(QObject::tr("Settings"));
    m_rightSeriesMenu = new QMenu(QObject::tr("Right Y-axis data"));
    m_leftSeriesMenu = new QMenu(QObject::tr("Left Y-axis data"));


    // init series menu options
    for (unsigned int i = 0; i < LOCAL_ZmHeapTelemetry::LOCAL_ZmHeapTelemetry_N; i++)
    {
        const auto l_name = localZmHeapTelemetry_ValueToString(i);
        m_rightSeriesMenu->addAction(new QAction(QObject::tr(l_name.c_str())));
        m_leftSeriesMenu->addAction(new QAction(QObject::tr(l_name.c_str())));
    }

    m_menuBar->addMenu(m_settingsMenu);
    m_settingsMenu->addMenu(m_rightSeriesMenu);
    m_settingsMenu->addMenu(m_leftSeriesMenu);

    // to do
    /**
    m_settingsMenu->addMenu(new QMenu("X-axis time span"));
    // later add support for changing color
    m_settingsMenu->addMenu(new QMenu("Right series color"));
    m_settingsMenu->addMenu(new QMenu("Left series color"));

    m_settingsMenu->addAction("Exit");
    */


    this->layout()->setMenuBar(m_menuBar);
}


std::string BasicChartView::localZmHeapTelemetry_ValueToString(const unsigned int a_val) const noexcept
{
    std::string l_result = "";
    switch (a_val) {
    case LOCAL_ZmHeapTelemetry::size:
        l_result = "size";
        break;
    case LOCAL_ZmHeapTelemetry::alignment:
        l_result = "alignment";
        break;
    case LOCAL_ZmHeapTelemetry::partition:
        l_result = "partition";
        break;
    case LOCAL_ZmHeapTelemetry::sharded:
        l_result = "sharded";
        break;
    case LOCAL_ZmHeapTelemetry::cacheSize:
        l_result = "cache size";
        break;
    case LOCAL_ZmHeapTelemetry::cpuset:
        l_result = "cpuset";
        break;
    case LOCAL_ZmHeapTelemetry::cacheAllocs:
        l_result = "cache allocs";
        break;
    case LOCAL_ZmHeapTelemetry::heapAllocs:
        l_result = "heap allocs";
        break;
    case LOCAL_ZmHeapTelemetry::frees:
        l_result = "frees";
        break;
    case LOCAL_ZmHeapTelemetry::allocated:
        l_result = "allocated";
        break;
    case LOCAL_ZmHeapTelemetry::none:
        l_result = "none";
        break;
    default:
        l_result = "default";
        break;
    }
    return l_result;
}


uint64_t BasicChartView::getData(const ZmHeapTelemetry& a_strcut, unsigned int a_index) const noexcept
{
    uint64_t l_result;
    switch (a_index) {
    case LOCAL_ZmHeapTelemetry::size:
        l_result = a_strcut.size;
        break;
    case LOCAL_ZmHeapTelemetry::alignment:
        l_result = a_strcut.alignment;
        break;
    case LOCAL_ZmHeapTelemetry::partition:
        l_result = a_strcut.partition;
        break;
    case LOCAL_ZmHeapTelemetry::sharded:
        l_result = a_strcut.sharded;
        break;
    case LOCAL_ZmHeapTelemetry::cacheSize:
        l_result = a_strcut.cacheSize;
        break;
    case LOCAL_ZmHeapTelemetry::cpuset:
        l_result = a_strcut.cpuset;
        break;
    case LOCAL_ZmHeapTelemetry::cacheAllocs:
        l_result = a_strcut.cacheAllocs;
        break;
    case LOCAL_ZmHeapTelemetry::heapAllocs:
        l_result = a_strcut.heapAllocs;
        break;
    case LOCAL_ZmHeapTelemetry::frees:
        l_result = a_strcut.frees;
        break;
    case LOCAL_ZmHeapTelemetry::allocated:
        l_result = (a_strcut.cacheAllocs + a_strcut.heapAllocs - a_strcut.frees);
        break;
    case LOCAL_ZmHeapTelemetry::none:
        l_result = 0;
        break;
    default:
        l_result = 0;
        break;
    }
    return l_result;
}


unsigned int BasicChartView::getActiveDataType(const unsigned int a_series) const noexcept
{
    return m_activeData[a_series];
}
















