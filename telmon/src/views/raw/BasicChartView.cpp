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
                               const QString& a_chartTitle,
                               const bool a_chartTitleVisibility,
                               QWidget *a_parent):
    QChartView(a_chart, a_parent),
    m_contextMenu( new QMenu(this)),
    m_menuYLeft(nullptr),
    m_menuYRight(nullptr),
    m_titleLabel(new QLabel(this)), // no need to delete, we be deleted by parent
    m_startStopButton(new QPushButton("Stop", this)),
    m_axisArray   { new QValueAxis,    new QValueAxis, new QValueAxis},
    m_seriesArray { new QSplineSeries, new QSplineSeries},
    m_associatedTelemetryType(a_associatedTelemetryType),
    m_activeData(MxTelemetryTypeWrappersFactory::getInstance().
                 getMxTelemetryWrapper(m_associatedTelemetryType).getActiveDataSet()),
    m_chartFields(MxTelemetryTypeWrappersFactory::getInstance().
                          getMxTelemetryWrapper(m_associatedTelemetryType).getChartList()),
    m_chartDataContainer(new QVector<QVector<double>*>),
    m_delayIndicator(0),        // there is no delay on startup
    m_drawChartFlag(true),      // we draw chart on widget start up
    m_xReferenceCoordiante(0),  // just default value
    m_chartTitleVisibility(a_chartTitleVisibility),
    m_chartTitle(new QString(a_chartTitle))
{
    initChartDataContainer();
    setDefaultUpdateDataFunction();
    initContextMenu();
    createActions();

    // Adds the axis axis to the chart aligned as specified by alignment.
    // The chart takes the ownership of the axis.
    QValueAxis* l_axis = &getAxes(CHART_AXIS::X);
    chart()->addAxis(l_axis, Qt::AlignBottom);
    l_axis->setLabelFormat("%d"); // set labels format
    l_axis->setRange(1, 60); // set number of points to track on the chart
    l_axis->setLabelsVisible(false);

    l_axis = &getAxes(CHART_AXIS::Y_LEFT);
    l_axis->setTickCount(5); // divide Y axis to 4 parts
    l_axis->setLabelFormat("%d");

    l_axis = &getAxes(CHART_AXIS::Y_RIGHT);
    l_axis->setTickCount(5);
    l_axis->setLabelFormat("%d");
    l_axis->setVisible(false);

    initSeries();

    setRenderHint(QPainter::Antialiasing); // make the chart look nicer
    setDragMode(DragMode::ScrollHandDrag); //set drag mode

    // set theme
    //chart()->setTheme(QChart::ChartThemeDark);

    // no need for shadow in charts representation
    chart()->setDropShadowEnabled(false);

    // set not margin, so utilize all window size
    chart()->setMargins(QMargins(0,0,0,0));

    initLegend();
    initChartLabel();
    initStartStopButton();


}


BasicChartView::~BasicChartView()
{
    qInfo() << "BasicChartView::~BasicChartView";

    for (int i = 0; i < m_chartDataContainer->size(); i++) {
        delete (*m_chartDataContainer)[i];
        (*m_chartDataContainer)[i] = nullptr;
    }
    delete m_chartDataContainer;
    m_chartDataContainer = nullptr;

    delete m_chartTitle;
    m_chartTitle = nullptr;

    delete this->chart();
}


void BasicChartView::initChartDataContainer() const noexcept
{
    // we do not need field for "none", that is why we remove 1
    const int l_numberOfFields = m_chartFields.size() - 1;

    for (int i = 0; i < l_numberOfFields; i++)
    {
        m_chartDataContainer->insert(i, new QVector<double>);
    }
}


void BasicChartView::addDataToChartDataContainer(void* a_mxTelemetryMsg) const noexcept
{
    // we do not need field for "none", that is why we remove 1
    const int l_numberOfFields = m_chartFields.size() - 1;

    // remove points if crossing the limit


    for (int i = 0; i < l_numberOfFields; ++i)
    {
        // get the data
//        const auto l_data = MxTelemetryTypeWrappersFactory::getInstance().
//                getMxTelemetryWrapper(m_associatedTelemetryType).
//                getDataForChart(a_mxTelemetryMsg, i);

        // only for testing
        const auto l_data = rand() % 100;

        // insert the data to be the first, pushing all the data inside one index up
        m_chartDataContainer->at(i)->prepend(l_data);
    }
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
        //auto l_rightSeriesColor = getSeries(a_series).color();
        //getAxes(l_axis).setLinePenColor(l_rightSeriesColor);

        // attach to chart and associate with corresponding axis
        chart()->addAxis(&getAxes(l_axis), l_alignment);
        getSeries(a_series).attachAxis(&getAxes(l_axis));
        getSeries(a_series).attachAxis(&getAxes(CHART_AXIS::X));

        m_axisArray[l_axis]->setRange(0, 100); // set default range

        const auto l_seriesName = m_chartFields.at(m_activeData[a_series]);
        getSeries(a_series).setName(l_seriesName);
    }
}


void BasicChartView::handleStartStopFunctionality() noexcept
{
    if (this->getDrawChartFlag())
    {
        this->setDrawChartFlag(false);
        m_startStopButton->setText("Start");
        this->m_contextMenu->actions().at(0)->setText("Start");
    } else {
        this->setDrawChartFlag(true);
        m_startStopButton->setText("Stop");
        this->m_contextMenu->actions().at(0)->setText("Stop");
    }
}


void BasicChartView::initContextMenu() noexcept
{
    // init start/stop action
    m_contextMenu->addAction("Stop", [this](){
        this->handleStartStopFunctionality();
    });

    m_contextMenu->addSeparator();

    // init "Data" menu
    auto l_dataMenu = new QMenu(QObject::tr("Data"), m_contextMenu);

    // init Y Axis
    m_menuYLeft = new QMenu("Y Left", l_dataMenu);
    l_dataMenu->addMenu(m_menuYLeft);

    m_menuYRight = new QMenu("Y Right", l_dataMenu);
    l_dataMenu->addMenu(m_menuYRight);

    // init series menu actions
    for (int i = 0; i < m_chartFields.size(); ++i) {

        m_menuYLeft->addAction( m_chartFields.at(i), [this, i]() {
            this->changeSeriesData(SERIES::SERIES_LEFT ,i);
        });

        m_menuYRight->addAction( m_chartFields.at(i), [this, i]() {
            this->changeSeriesData(SERIES::SERIES_RIGHT ,i);
        });
    }

    // set default option disabled for each axis menu
    for (unsigned int l_series = 0; l_series < SERIES::SERIES_N; l_series++)
    {
        (l_series == SERIES::SERIES_LEFT)
                ? ( m_menuYLeft->actions(). at(m_activeData[l_series])->setDisabled(true))
                : ( m_menuYRight->actions().at(m_activeData[l_series])->setDisabled(true));
    }


    m_contextMenu->addMenu(l_dataMenu);

    // in the future, add appearance option

    m_contextMenu->addSeparator();

    // init close action
    m_contextMenu->addAction( "Close", [this]() {

        // delete the dock widget
        delete this->parent();

        //clean data container
        // we do not need field for "none", that is why we remove 1
        const int l_numberOfFields = this->m_chartFields.size() - 1;
        for (int i = 0; i < l_numberOfFields; ++i)
        {
            this->m_chartDataContainer->at(i)->clear();
        }

        // clean series
        for (unsigned  int i = 0; i < SERIES::SERIES_N; i++)
        {
            getSeries(i).clear();
        }

    });
}


void BasicChartView::createActions() noexcept
{
    // start/stop button functionality
    QObject::connect(m_startStopButton, &QAbstractButton::clicked, this, [this]() {
        this->handleStartStopFunctionality();
    });

    // contextMenu view/display functionality
    this->setContextMenuPolicy(Qt::CustomContextMenu); // set custom menu policy
    QObject::connect(this, &QChartView::customContextMenuRequested, this, [this](const QPoint a_pos)
    {
        this->m_contextMenu->exec(mapToGlobal(a_pos));
    } );
}


void BasicChartView::changeSeriesData(const unsigned int a_series, const int data_type) noexcept
{
    const int l_oldDataType = m_activeData[a_series];

    // set new active data
    m_activeData[a_series] = static_cast<int>(data_type);

    // set new name
    const auto l_seriesName = m_chartFields.at((m_activeData[a_series]));
    getSeries(a_series).setName(l_seriesName);

    // flush all current data
    getSeries(a_series).clear();

    // disable/enable this option from menu
    QMenu* l_menu = nullptr;
    (a_series == SERIES::SERIES_LEFT) ? (l_menu = m_menuYLeft) : (l_menu =  m_menuYRight);
    l_menu->actions().at(l_oldDataType)->setEnabled(true);
    l_menu->actions().at(data_type)->setDisabled(true);
}


void BasicChartView::setUpdateFunction( std::function<void(BasicChartView* a_this,
                                                      void* a_mxTelemetryMsg)>   a_lambda )
{
    m_lambda = a_lambda;
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


int BasicChartView::getActiveDataType(const unsigned int a_series) const noexcept
{
    return m_activeData[a_series];
}


void BasicChartView::setDefaultUpdateDataFunction() noexcept
{
    m_lambda = [] ( BasicChartView* a_this,
                    void* a_mxTelemetryMsg) -> void
    {
        // to do: in the future, could be done in update thread and not in main thread
        //we can think about thread just sending the correct series or something like that
        a_this->addDataToChartDataContainer(a_mxTelemetryMsg);

        // draw chart only if flag is set
        if (!(a_this->getDrawChartFlag())) {return;}

        a_this->repaintChart();
    };
}


void BasicChartView::updateVerticalAxisRange(const double a_data) noexcept
{
    const auto l_maxValue = qMax(getAxes(CHART_AXIS::Y_LEFT).max(), getAxes(CHART_AXIS::Y_RIGHT).max());

    // if a_data is bigger than current top max value, we update accordingly
    if (l_maxValue <= a_data)
    {
        //qDebug() << "increase";
        const auto l_newMax = a_data + floor(a_data / 10);
        getAxes(CHART_AXIS::Y_LEFT).setMax(l_newMax);
        m_axisArray[CHART_AXIS::Y_LEFT]->applyNiceNumbers();

        getAxes(CHART_AXIS::Y_RIGHT).setMax(l_newMax);
        m_axisArray[CHART_AXIS::Y_RIGHT]->applyNiceNumbers();

        m_axisArray[CHART_AXIS::Y_LEFT]->setTickCount(5);
        m_axisArray[CHART_AXIS::Y_RIGHT]->setTickCount(5);
    }

    // on the other hand, if a_data is less than 10% of current max,
    // we decrease max value
    const auto l_newMax = floor(l_maxValue * 0.10);
    if (l_newMax > a_data)
    {

        getAxes(CHART_AXIS::Y_LEFT).setMax(l_newMax);
        m_axisArray[CHART_AXIS::Y_LEFT]->applyNiceNumbers();

        getAxes(CHART_AXIS::Y_RIGHT).setMax(l_newMax);
        m_axisArray[CHART_AXIS::Y_RIGHT]->applyNiceNumbers();

        m_axisArray[CHART_AXIS::Y_LEFT]->setTickCount(5);
        m_axisArray[CHART_AXIS::Y_RIGHT]->setTickCount(5);
    }
}


void BasicChartView::mouseMoveEvent(QMouseEvent * event)
{
    // handle the event only if the left mouse button is pressed
    if (event->buttons() ==  Qt::MouseButton::LeftButton)
    {
        //get the x coordinate of the mouse location
        const int l_newXCoordinate = static_cast<int>(chart()->mapToValue(event->pos()).x());
        handleMouseEventHelper(l_newXCoordinate);
    }
    QGraphicsView::mouseMoveEvent(event);
}


void BasicChartView::handleMouseEventHelper(const int a_x /** the new x cooredinate */ ) noexcept
{
    // if the new coordinate and the current x coordiante difference is zero
    // there is nothing to do
    if (a_x == getReferenceX()) {/**qDebug() << "difference is 0";*/ return;}

    // else: repaint series according to new x
    const int l_difference = getReferenceX() - a_x;
    setReferenceX(a_x);

    // update m_delayIndicator accordingly
    //Notice:
    // 1. if *l_difference* is negative, user is moving right
    // 2. else, user is moving left
    if (l_difference < 0)
    {
        // check if we are already on the rightest allowed
        const int l_rightBorderLimit = ((getXAxisSpan() - 1) * (-1));
        if ( m_delayIndicator == l_rightBorderLimit )
        {
            return;
        }

        // compute new m_delatIndicator
        // make sure no crossing the limit
        // this behavior make sure that even if we are on the rightest border,
        // we will still show one point
        m_delayIndicator = qMax(l_rightBorderLimit, m_delayIndicator + l_difference);
    } else { // that is l_difference > 0
        // check if we are already in the leftest allowed
        const int l_leftBorderLimit = (m_chartDataContainer->at(0)->size() - 1);
        if (m_delayIndicator == l_leftBorderLimit)
        {
            return;
        }
        // compute new m_delatIndicator
        // make sure no crossing the limit
        // this behavior make sure that even if we are on the leftest border,
        // we will still show one point
        m_delayIndicator = qMin(l_leftBorderLimit, m_delayIndicator + l_difference);
    }
    repaintChart();
}


void BasicChartView::repaintChart() noexcept
{
    // should be 1 and not 0 because we later multiply it in some values,
    // and you can not multiply by 0
    double l_Y_coordianteMaxPointBothSeries = 1;

    // iterate over series, that is, iterate over {SERIES::SERIES_LEFT, SERIES::SERIES_RIGHT}
    for (unsigned int l_curSeries = 0;
         l_curSeries < BasicChartView::SERIES::SERIES_N;
         l_curSeries++)
    {
        // if current series is "none", then skip
        const int l_activeDataTypeIndex = getActiveDataType(l_curSeries);
        if (isSeriesIsNull(l_activeDataTypeIndex)) { continue; }

        // clear current series
        m_seriesArray[l_curSeries]->clear();

        // build up new series
        for (int i = 0; i < getXAxisSpan(); i++)
        {
            //consider delay indicator
            int l_index = m_delayIndicator + i;

            // if (value < 0) that means the user moved chart to right Limit
            // so we skip negative points to give it nice look
            if (l_index < 0) {/**qDebug() << "skip point" << l_index << "right limit border";*/ continue;}

            // if (value >= size()) that means the user moved chart left
            // so we skip this points, to give it nicer look
            // Notice: which data type container size we check is irrelevant because all
            // of them should be in the same size
            if (l_index >= m_chartDataContainer->at(l_activeDataTypeIndex)->size())
            {/**qDebug() << "skip point" << l_index << "left limit border";*/ continue;}

            // else, constrcut the point
            const auto l_point_Y = m_chartDataContainer->at(l_activeDataTypeIndex)->at(l_index);
            const auto l_point_X = getXAxisSpan() - i;
            m_seriesArray[l_curSeries]->insert(i, QPointF(l_point_X, l_point_Y));

            //update max Y
            l_Y_coordianteMaxPointBothSeries = qMax(l_Y_coordianteMaxPointBothSeries, l_point_Y);
        }
    }

    // update axis max range
    updateVerticalAxisRange(l_Y_coordianteMaxPointBothSeries);
}


bool BasicChartView::isSeriesIsNull(const int a_series) const noexcept
{
    return MxTelemetryTypeWrappersFactory::getInstance().
            getMxTelemetryWrapper(m_associatedTelemetryType).
            isDataTypeNotUsed(a_series);
}


void BasicChartView::mousePressEvent(QMouseEvent * event)
{
    if (event->button() ==  Qt::MouseButton::LeftButton)
    {
        // store the current status of
        m_drawChartFlagStoreVariable = getDrawChartFlag();

        // stop drawing the chart according to data updates
        setDrawChartFlag(false);

        // store the current x position of the chart for reference
        setReferenceX(static_cast<int>(chart()->mapToValue(event->pos()).x()));
    }

    QGraphicsView::mousePressEvent(event);
}


void BasicChartView::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->button() ==  Qt::MouseButton::LeftButton)
    {

        //qDebug() << "mouseReleaseEvent, event->pos()" << event->pos();
        //qDebug() << "chart()->mapToPosition(event->pos())" << chart()->mapToValue(event->pos());
        //updateDelayIndicator(m_mousePressEventXCoordiante - static_cast<int>(chart()->mapToValue(event->pos()).x()));
        //qDebug() << "chart()->mapToPosition(event->pos())" << chart()->mapToPosition(event->pos());

        //return previous status of
        setDrawChartFlag(m_drawChartFlagStoreVariable);
    }
    QGraphicsView::mouseReleaseEvent(event);
}


void BasicChartView::setXAxisSpan(const int a_num) noexcept
{
    m_axisArray[CHART_AXIS::X]->setRange(1, a_num);
}


void BasicChartView::setChartTitleVisiblity(const bool a_visibility) noexcept
{
    m_chartTitleVisibility = a_visibility;

    if ( m_chartTitleVisibility )
    {
        chart()->setTitle(*m_chartTitle + "Chart");
    } else {
        chart()->setTitle("");
    }
}


bool BasicChartView::getChartTitleVisiblity() const noexcept
{
    return m_chartTitleVisibility;
}


QString& BasicChartView::getChartTitle() const noexcept
{
    return *m_chartTitle;
}


void BasicChartView::initLegend() noexcept
{
    auto l_leg = chart()->legend();

    // we would like to disconnect the legend from the chart layout
    // this why we can position the legend on/in the chart
    // if the legend is inside the chart layout, the layout
    // treat them as different object and i could not embed
    // the legend in the background of the chart
    if ( l_leg->isAttachedToChart())
    {
        l_leg->detachFromChart();
    }

    // get chart size
    const auto l_width = this->size().width();
    const auto l_height = this->size().height();

    // set legend position in the upper left corner
    // hight = 0.02 * chart_height
    // width = 0.06 * chart_width
    l_leg->setPos(this->size().width()  * 6 / 100,
                  this->size().height() * 2 / 100);

    // resize legend
    l_leg->resize(l_width,l_height);

}


void BasicChartView::initChartLabel() noexcept
{
    // we dont use the base chart title widget, because i could not
    // embed it in the chart
    setChartTitleVisiblity(m_chartTitleVisibility);
    // thereforem we use our own label

    m_titleLabel->setText(*m_chartTitle);

    // set  width as chart width
    const auto l_width = this->width();
    m_titleLabel->setMaximumWidth(l_width);
    m_titleLabel->setMinimumWidth(l_width);

    // scale content to avaiable size
    m_titleLabel->setScaledContents(true);

    //
    QFont font = chart()->legend()->font();
    m_titleLabel->setFont(font);
}


void BasicChartView::resizeEvent(QResizeEvent *event)
{
    resizeEventLegend(event->size());
    resizeEventTitle();
    resizeEventStartStopButton();

    QChartView::resizeEvent(event);
}


void BasicChartView::resizeEventLegend(const QSize& a_size /*event size*/) noexcept
{
    // Notice:
    // 1. if the legend is too big, for example, covering the all chart
    // it seems that the hand will not change when trying to move the chart
    // change i mean from just hand will switched to pressing hand
    // from tests, sees the minimum height of legend is 42, so i am using it.
    chart()->legend()->resize(QSize(a_size.width(), 42));

    // hide/show legend if the width and height are too low
    if (a_size.height() < 139)
    {
        if (chart()->legend()->isVisible())
        {
            chart()->legend()->setVisible(false);
        }
    } else {
        if (!chart()->legend()->isVisible())
        {
            chart()->legend()->setVisible(true);
        }
    }
}


void BasicChartView::resizeEventTitle() noexcept
{
    const auto l_legendPosition = chart()->legend()->pos().toPoint();

    // set the title above the legend, we use the following factor:
    const auto l_factor = (chart()->legend()->font().pointSize() - 4);

    // set the title in the center of width, we use the following:
    const auto l_centerX = this->width() / 3;

    m_titleLabel->move(l_centerX,
                       l_legendPosition.y() - l_factor);
}


void BasicChartView::initStartStopButton() noexcept
{
    // set similar font to legend font
    auto l_font = chart()->legend()->font();
    l_font.setPointSize(9);
    m_startStopButton->setFont(l_font);
    m_startStopButton->resize(30,20); //set button size
}


void BasicChartView::resizeEventStartStopButton() noexcept
{
    // Notice: must execute after resizeEventLegend
    if (chart()->legend()->isVisible())
    {
        if (!m_startStopButton->isVisible())
        {
            m_startStopButton->setVisible(true);
        }

    } else {
        if (m_startStopButton->isVisible())
        {
            m_startStopButton->setVisible(false);
        }
        // no need to move if legend is hidden
        return;
    }

    // move
    const auto l_legendPosition = chart()->legend()->pos().toPoint();
    m_startStopButton->move(l_legendPosition.x() + 10,
                            l_legendPosition.y() + 32);
}


// # # # # updateData functions # # # #//
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




