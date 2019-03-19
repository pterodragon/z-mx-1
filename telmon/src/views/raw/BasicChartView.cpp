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
#include "ZmHashMgr.hpp"

#include "BasicChartView.h"

#include "src/factories/MxTelemetryTypeWrappersFactory.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


BasicChartView::BasicChartView(QChart *a_chart,
                               const int a_associatedTelemetryType,
                               QWidget *a_parent):
    QChartView(a_chart, a_parent),
    m_associatedTelemetryType(a_associatedTelemetryType),
    m_activeData(MxTelemetryTypeWrappersFactory::getInstance().
                 getMxTelemetryWrapper(m_associatedTelemetryType).getActiveDataSet()),
    m_db(MxTelemetryTypeWrappersFactory::getInstance().
                          getMxTelemetryWrapper(m_associatedTelemetryType).getChartList())
{
    qDebug() << "BasicChartView::BasicChartView  --BEGIN--";


    m_axisArray[CHART_AXIS::X] = new QValueAxis;
    m_axisArray[CHART_AXIS::Y_LEFT] = new QValueAxis;
    m_axisArray[CHART_AXIS::Y_RIGHT] = new QValueAxis;
    m_seriesArray[SERIES::SERIES_RIGHT] = new QSplineSeries;
    m_seriesArray[SERIES::SERIES_LEFT] = new QSplineSeries;

    setDefaultUpdateDataFunction();
    initMenuBar();
    createActions();

    // Adds the axis axis to the chart aligned as specified by alignment.
    // The chart takes the ownership of the axis.
    chart()->addAxis(&getAxes(CHART_AXIS::X), Qt::AlignBottom);
    getAxes(CHART_AXIS::X).setTickCount(10);
    m_axisArray[CHART_AXIS::X]->setRange(0, 10);

    initSeries();

    // to make the chart look nicer
    setRenderHint(QPainter::Antialiasing);
     qDebug() << "BasicChartView::BasicChartView  --END--";

}


BasicChartView::~BasicChartView()
{
    qDebug() << "BasicChartView::~BasicChartView";
    delete this->chart();
}


void BasicChartView::initSeries() noexcept
{
     // iterate over {SERIES::SERIES_LEFT, SERIES::SERIES_RIGHT}
    for (unsigned int a_series = 0; a_series < BasicChartView::SERIES::SERIES_N; a_series++)
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

        const auto l_seriesName = m_db.at(m_activeData[a_series]);
        getSeries(a_series).setName(l_seriesName);
    }
}


void BasicChartView::initMenuBar() noexcept
{
    // initilization here for readiablity
    // Notice: no need to delete, it will be deleted with object
    m_boxLayout = new QVBoxLayout(this);
    m_menuBar = new QMenuBar;
    m_settingsMenu = new QMenu(QObject::tr("Settings"));
    m_rightSeriesMenu = new QMenu(QObject::tr("Right Y-axis data"));
    m_leftSeriesMenu = new QMenu(QObject::tr("Left Y-axis data"));


    // init series menu options
    for (int i = 0; i < m_db.size(); ++i) {
        m_rightSeriesMenu->addAction(new QAction((m_db.at(i))));
        m_leftSeriesMenu->addAction( new QAction((m_db.at(i))));
    }


    m_menuBar->addMenu(m_settingsMenu);
    m_settingsMenu->addMenu(m_rightSeriesMenu);
    m_settingsMenu->addMenu(m_leftSeriesMenu);

    /** TODO
    m_settingsMenu->addMenu(new QMenu("X-axis time span"));
    // later add support for changing color
    m_settingsMenu->addMenu(new QMenu("Right series color"));
    m_settingsMenu->addMenu(new QMenu("Left series color"));

    m_settingsMenu->addAction("Exit");
    */

    this->layout()->setMenuBar(m_menuBar);
}


void BasicChartView::createActions() noexcept
{
    // Settings->Right Y-axis data functionality
    auto l_rightSeriesMenuActions = m_rightSeriesMenu->actions();
    for (int i = 0; i < l_rightSeriesMenuActions.size(); i++)
    {
        QObject::connect(l_rightSeriesMenuActions.at(i), &QAction::triggered, this, [this, i](){
            this->changeSeriesData(SERIES::SERIES_RIGHT ,i);
        });
    }

    // Settings->Left Y-axis data functionality
    auto l_leftSeriesMenuActions  = m_leftSeriesMenu->actions();
    for (int i = 0; i < l_leftSeriesMenuActions.size(); i++)
    {
        QObject::connect(l_leftSeriesMenuActions.at(i), &QAction::triggered, this, [this, i](){
            this->changeSeriesData(SERIES::SERIES_LEFT ,i);
        });
    }

//    // File->Exit functionality
//    QObject::connect(m_mainWindowView->m_exitSubMenu, &QAction::triggered, this, &QWidget::close);
}


void BasicChartView::changeSeriesData(const unsigned int a_series, const int data_type) noexcept
{
    qDebug() << "BasicChartView::changeSeriesData"
             << "a_series" << a_series
             << "old" << m_activeData[a_series]
             << "new" << data_type;
    const int l_oldDataType = m_activeData[a_series];

    // set new active data
    m_activeData[a_series] = static_cast<int>(data_type);

    // set new name
    const auto l_seriesName = m_db.at((m_activeData[a_series]));
    getSeries(a_series).setName(l_seriesName);

    // flush all current data
    getSeries(a_series).clear();

    // disable/enable this option from menu
    QMenu* l_menu;
    (a_series == SERIES::SERIES_LEFT) ? (l_menu = m_leftSeriesMenu) : (l_menu =  m_rightSeriesMenu);
    l_menu->actions().at(l_oldDataType)->setEnabled(true);
    l_menu->actions().at(data_type)->setDisabled(true);

}


//QSize BasicChartView::sizeHint() const
//{
//    return QSize(600,600);
//}


void BasicChartView::setUpdateFunction( std::function<void(BasicChartView* a_this,
                                                      void* a_mxTelemetryMsg)>   a_lambda )
{
    m_lambda = a_lambda;
}


void BasicChartView::updateData(ZmHeapTelemetry a_pair)
{
    m_lambda(this, &a_pair);
}


void BasicChartView::updateData(ZmHashTelemetry a_pair)
{
    m_lambda(this, &a_pair);
}

void BasicChartView::updateData(ZmThreadTelemetry a_pair)
{
    m_lambda(this, &a_pair);
}

void BasicChartView::updateData(ZiMxTelemetry a_pair)
{
    m_lambda(this, &a_pair);
}

void BasicChartView::updateData(ZiCxnTelemetry a_pair)
{
    m_lambda(this, &a_pair);
}

void BasicChartView::updateData(MxTelemetry::Queue a_pair)
{
    m_lambda(this, &a_pair);
}

void BasicChartView::updateData(MxEngine::Telemetry a_pair)
{
    m_lambda(this, &a_pair);
}

void BasicChartView::updateData(MxAnyLink::Telemetry a_pair)
{
    m_lambda(this, &a_pair);
}

void BasicChartView::updateData(ZdbEnv::Telemetry a_pair)
{
    m_lambda(this, &a_pair);
}

void BasicChartView::updateData(ZdbHost::Telemetry a_pair)
{
    m_lambda(this, &a_pair);
}

void BasicChartView::updateData(ZdbAny::Telemetry a_pair)
{
    m_lambda(this, &a_pair);
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
    const int l_count = (m_seriesArray[a_series])->count();
    return l_count >= static_cast<int>(getVerticalAxesRange());
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


int BasicChartView::getActiveDataType(const unsigned int a_series) const noexcept
{
    return m_activeData[a_series];
}


void BasicChartView::setDefaultUpdateDataFunction() noexcept
{
    m_lambda = [] ( BasicChartView* a_this,
                    void* a_mxTelemetryMsg) -> void
    {
        // iterate over {SERIES::SERIES_LEFT, SERIES::SERIES_RIGHT}
        for (unsigned int l_curSeries = 0; l_curSeries < BasicChartView::SERIES::SERIES_N; l_curSeries++)
        {
            const int l_activeDataType = a_this->getActiveDataType(l_curSeries);

            const bool l_notUsed = MxTelemetryTypeWrappersFactory::getInstance().
                                      getMxTelemetryWrapper(a_this->m_associatedTelemetryType).
                                        isDataTypeNotUsed(l_activeDataType);
            if (l_notUsed) { continue; }

            // remove first point if exceeding the limit of points
            if (a_this->isExceedingLimits(l_curSeries))
            {
                   a_this->removeFromSeriesFirstPoint(l_curSeries);
            }

            // shift all point left
            a_this->shiftLeft(l_curSeries);

            // get the current value according the user choice
            const auto l_data = MxTelemetryTypeWrappersFactory::getInstance().
                    getMxTelemetryWrapper(a_this->m_associatedTelemetryType).
                      getDataForChart(a_mxTelemetryMsg, l_activeDataType);
            const auto l_newPoint = QPointF(a_this->getVerticalAxesRange(),
                                            l_data);

            // update range if necessary
            {
                unsigned int l_axisIndex;
                (l_curSeries == SERIES::SERIES_LEFT) ? (l_axisIndex = CHART_AXIS::Y_LEFT) : (l_axisIndex = CHART_AXIS::Y_RIGHT);
                a_this->updateAxisMaxRange(l_axisIndex, l_data);
            }


//            const auto l_newPoint = QPointF(a_this->getVerticalAxesRange(),
//                                            a_this->getData(a_mxTelemetryMsg, l_activeDataType));

            // append point
            a_this->appendPoint(l_newPoint, l_curSeries);

        }
    };
}


void BasicChartView::updateAxisMaxRange(const unsigned int a_axis, const double a_data) noexcept
{
    auto maxValue = qMax(getAxes(CHART_AXIS::Y_LEFT).max(), getAxes(CHART_AXIS::Y_RIGHT).max());
    if (maxValue < a_data)
    {
        const auto l_newMax = a_data + floor(a_data / 10);
        getAxes(CHART_AXIS::Y_LEFT).setMax(l_newMax);
        getAxes(CHART_AXIS::Y_RIGHT).setMax(l_newMax);
    }
}


qreal BasicChartView::getVerticalAxesRange() const noexcept
{
    return m_axisArray[CHART_AXIS::X]->max();
}











