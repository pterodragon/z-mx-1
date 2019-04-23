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


#include "BasicChartView.h"
#include "QtCharts"
#include "src/controllers/BasicChartController.h"
#include "src/widgets/ChartViewCustomizer.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "src/widgets/ChartViewContextMenu.h"
#include "src/widgets/ChartViewLabelsManagerWidget.h"
#include "src/widgets/ChartViewDataManager.h"

#include <stdlib.h> // abs

BasicChartView::BasicChartView(const BasicChartModel& a_model,
                               const QString& a_chartTitle,
                               QWidget *a_parent,
                               BasicChartController& a_controller):
    // no need to delete QChart()
    // from doc "the ownership of the chart is passed to the chart view"
    QChartView(new QChart(), a_parent),
    m_state(DRAWING),
    m_model(a_model),
    // no need to delete, will be delete by QChart()
    m_axisArray   { new QValueAxis,    new QValueAxis, new QValueAxis},
    m_seriesArray { new QSplineSeries, new QSplineSeries},
    m_activeData(m_model.getActiveDataSet()),
    m_chartFields(m_model.getChartList()),
    m_delayIndicator(0),        // there is no delay on startup
    m_xReferenceCoordiante(0),  // default value

    // do not show chart title visibiliy, because this build in feature
    // decrease the available chart size, therefore we will use our own chart label
    // as follows and we put it in the position we desire
    m_chartTitleVisibility(false),
    m_chartTitle(new QString(a_chartTitle)),
    m_timer(new QTimer(this)),
    m_designer(new ChartViewCustomizer(QString(
                                           QString("ChartViewCustomizer::") +
                                           MxTelemetryGeneralWrapper::fromMxTypeValueToName(
                                               m_model.getAssociatedTelemetryType())) +
                                           m_model.getAssociatedTelemetryInstanceName(),
                                       *this)),
    m_controller(a_controller),
    m_labelManager(new ChartViewLabelsManagerWidget(*this)),
    m_dataManager(new ChartViewDataManager(*this))

{
    // connect the timer
    m_timer->setTimerType(Qt::VeryCoarseTimer);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateChart()));

    m_contextMenu = new ChartViewContextMenu(*this, QString(), this);


    // Adds the axis to the chart aligned as specified by alignment.
    // The chart takes the ownership of the axis.
    QValueAxis* l_axis = &getAxes(CHART_AXIS::X);
    chart()->addAxis(l_axis, Qt::AlignBottom);
    l_axis->setLabelFormat("%d"); // set labels format
    l_axis->setRange(DEFAULT_X_AXIS_MIN, DEFAULT_X_AXIS_MAX); // set number of points to track on the chart

    l_axis = &getAxes(CHART_AXIS::Y_LEFT);
    l_axis->setTickCount(DEFAULT_TICK_COUNT);

    l_axis = &getAxes(CHART_AXIS::Y_RIGHT);
    l_axis->setTickCount(DEFAULT_TICK_COUNT);

    initSeries();

    setRenderHint(QPainter::Antialiasing); // make the chart look nicer
    setDragMode(DragMode::ScrollHandDrag); //set drag mode

    initLegend();
    initChartLabel();
    initStartStopButton();
    createActions();

    m_designer->init();


    m_timer->start(1000);
}


BasicChartView::~BasicChartView()
{
    qInfo() << QString("~" + *m_chartTitle);
    m_timer->stop();

    delete m_designer;
    m_designer = nullptr;

    delete m_chartTitle;
    m_chartTitle = nullptr;

    delete m_labelManager;
    m_labelManager = nullptr;

    delete m_dataManager;
    m_dataManager = nullptr;
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

        // attach to chart and associate with corresponding axis
        chart()->addAxis(&getAxes(l_axis), l_alignment);
        getSeries(a_series).attachAxis(&getAxes(l_axis));
        getSeries(a_series).attachAxis(&getAxes(CHART_AXIS::X));

        // init series with default values
        m_axisArray[l_axis]->setRange(0, 100); // set default range

        const auto l_seriesName = m_chartFields.at(m_activeData[a_series]);
        getSeries(a_series).setName(l_seriesName);
    }
}


void BasicChartView::handleStartStopFunctionality() noexcept
{
    if (m_state == DRAWING)
    {
        m_state = MANUAL_STOP;
        m_startStopButton->setText("Start");
        this->m_contextMenu->actions().at(0)->setText("Start");
    } else { // MANUAL_STOP
        m_state = DRAWING;
        m_startStopButton->setText("Stop");
        this->m_contextMenu->actions().at(0)->setText("Stop");
    }
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


void BasicChartView::changeSeriesData(const unsigned int a_series,
                                      const int data_type,
                                      const QMenu* a_menu) noexcept
{
    const int l_oldDataType = m_activeData[a_series];
    //const int l_oldDataType = m_model.getActiveData(a_series);

    // set new active data
    m_activeData[a_series] = static_cast<int>(data_type);

    // set new name
    const auto l_seriesName = m_chartFields.at((m_activeData[a_series]));
    getSeries(a_series).setName(l_seriesName);

    // flush all current data
    getSeries(a_series).clear();

    // disable/enable this option from menu
    a_menu->actions().at(l_oldDataType)->setEnabled(true);
    a_menu->actions().at(data_type)->setDisabled(true);
}//    qDebug() << *m_chartTitle << "updateChart() BEGIN";



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


void BasicChartView::updateVerticalAxisRange(const int a_newMax) noexcept
{
    const auto l_curMaxValue = qMax(getAxes(CHART_AXIS::Y_LEFT).max(), getAxes(CHART_AXIS::Y_RIGHT).max());

    // if a_newMax is bigger than current top max value
    if (l_curMaxValue <= a_newMax)
    {
        const auto l_newMax = a_newMax + floor(a_newMax / 10);
        updateYAxisMax(l_newMax, getAxes(CHART_AXIS::Y_LEFT),  DEFAULT_TICK_COUNT);
        updateYAxisMax(l_newMax, getAxes(CHART_AXIS::Y_RIGHT), DEFAULT_TICK_COUNT);
    }

    // on the other hand, if a_newMax is less than 10% of current max,
    // we decrease max value
    const auto l_newMax = floor(l_curMaxValue * 0.10);
    if (l_newMax > a_newMax)
    {
        updateYAxisMax(l_newMax, getAxes(CHART_AXIS::Y_LEFT),  DEFAULT_TICK_COUNT);
        updateYAxisMax(l_newMax, getAxes(CHART_AXIS::Y_RIGHT), DEFAULT_TICK_COUNT);
    }
}

void BasicChartView::updateYAxisMin(const double a_newMin,
                                    QValueAxis& a_axis,
                                    const int a_tickCount) noexcept
{
    auto l_newMin = a_newMin;
    if (l_newMin < 0) {l_newMin = 0;}
    a_axis.setMin(l_newMin);
    a_axis.applyNiceNumbers();
    a_axis.setTickCount(a_tickCount);
}


void BasicChartView::updateYAxisMax(const double a_newMax,
                                    QValueAxis& a_axis,
                                    const int a_tickCount) noexcept
{
    a_axis.setMax(a_newMax);
    a_axis.applyNiceNumbers();
    a_axis.setTickCount(a_tickCount);
}


// called as part of zoom in sequence
void BasicChartView::updateYRangesManually() noexcept
{
    const auto l_leftSeriesMax = getCurrentMaxAndMinYFromSeries(BasicChartView::SERIES_LEFT);
    const auto l_rightSeriesMax = getCurrentMaxAndMinYFromSeries(BasicChartView::SERIES_RIGHT);
    const auto l_newMax = qMax(l_leftSeriesMax.first, l_rightSeriesMax.first);
    const auto l_newMin = qMax(l_leftSeriesMax.second, l_rightSeriesMax.second);

    if (l_newMax > 0 )
    {
         updateYAxisMax(l_newMax + VERTICAL_AXIS_RANGE_FACTOR, getAxes(BasicChartView::CHART_AXIS::Y_RIGHT), DEFAULT_TICK_COUNT);
         updateYAxisMax(l_newMax + VERTICAL_AXIS_RANGE_FACTOR, getAxes(BasicChartView::CHART_AXIS::Y_LEFT), DEFAULT_TICK_COUNT);
    }
    if (l_newMin > 0 )
    {
        updateYAxisMin(l_newMin - VERTICAL_AXIS_RANGE_FACTOR, getAxes(BasicChartView::CHART_AXIS::Y_RIGHT), DEFAULT_TICK_COUNT);
        updateYAxisMin(l_newMin - VERTICAL_AXIS_RANGE_FACTOR, getAxes(BasicChartView::CHART_AXIS::Y_LEFT), DEFAULT_TICK_COUNT);
    }
}





 QPair<qreal, qreal> BasicChartView::getCurrentMaxAndMinYFromSeries(const unsigned int a_seriesIndex) const noexcept
{
    // sanity check: is none?

    QPair<qreal, qreal> l_result = qMakePair(-1, -1); // max and min
    const int l_activeDataTypeIndex = getActiveDataType(a_seriesIndex);
    if (m_model.isSeriesIsNull(l_activeDataTypeIndex)) { return l_result; }

    // else get series
    const auto& l_series = getSeries(a_seriesIndex);

    // new
    // get user input
    const auto& l_axis = getAxes(CHART_AXIS::X);
    const auto l_xBegin  = static_cast<int>(l_axis.min());
    const auto l_xEnd    = static_cast<int>(l_axis.max());
    int l_numberOfPointsMeasured = 0;

    qreal l_curMaxY = -1; // -1 indicating do nothing
    qreal l_curMinY = -1; // -1 indicating do nothing
    bool l_minFlagSetFirstTime = true;
    for (int i = 0; i < l_series.count(); i++) {
        const int l_curPointX = static_cast<int>(l_series.at(i).x());
        if (l_curPointX >= l_xBegin
                and
            l_curPointX <= l_xEnd)
        {
            if (l_minFlagSetFirstTime)
            {
                l_curMinY = l_series.at(i).y();
                l_minFlagSetFirstTime = false;
            }

            l_numberOfPointsMeasured++;
            l_curMaxY = qMax(l_curMaxY, l_series.at(i).y());
            l_curMinY = qMin(l_curMinY, l_series.at(i).y());
        }
    }
    // if there is only one point measured, dont change anything;
    if (l_numberOfPointsMeasured < 2) {return l_result;}
    l_result.first  = l_curMaxY;
    l_result.second = l_curMinY;
    return l_result;
}


void BasicChartView::mouseMoveEvent(QMouseEvent * event)
{
    // handle the el_activeDataTypeIndexvent only if the left mouse button is pressed
    if (event->buttons() ==  Qt::MouseButton::LeftButton)
    {
        //get the x coordinate of the mouse location
        const int l_newXCoordinate = static_cast<int>(chart()->mapToValue(event->pos()).x());
        handleMouseEventHelper(l_newXCoordinate);
    }

    m_labelManager->drawLabels(*event);

    QGraphicsView::mouseMoveEvent(event);
}


void BasicChartView::handleMouseEventHelper(const int a_new_X /** the new x cooredinate */ ) noexcept
{
    // three options for l_difference:
    //1. ( 0)
    //2. (+1)  --> user moves left
    //3. (-1)  --> user moves right

    // if the new coordinate and the current x coordiante difference is zero -> do nothing
    if (a_new_X == getReferenceX()) {return;}

    // else, repaint series according to new x
    const int l_difference = getReferenceX() - a_new_X;
    setReferenceXCoordinate(a_new_X);
    m_delayIndicator += l_difference;

    repaintChart();
}


void BasicChartView::repaintChart() noexcept
{
    int l_maxY = 1; // dont set as 0

    // iterate over series
    for (unsigned int l_curSeries = 0; l_curSeries < BasicChartView::SERIES::SERIES_N; l_curSeries++)
    {
        const int l_maxYForSeries = repaintOneSeries(l_curSeries);
        l_maxY = qMax(l_maxYForSeries, l_maxY);
    }
    updateVerticalAxisRange(l_maxY);
}


/**
  * Implementation Notes:
  *
  * For simplicity, assume X axis length is 10, denote X_LENGTH
  *
  * 1. Figure out which points to request from data structure
  * 2. Build the series
  *
  * There are 3 options:
  * --OPTION 1--
  * 1. user moved to the right and is not within container borders,
  *  which indicated by (m_delayIndicator < 0)
  *
  *        0 ______________ N
  *         |_____________|
  *        new  ->->       old
  *
  *   ^   user refernce
  *
  * in the most extreme point to the right, we always print at least one point
  * this point is the most updated one
  *
  * --OPTION 2--
  * 2. user is within container borders
  *
  *        0 ______________ N
  *         |_____________|
  *        new  ->->       old
  *
  *           ^   user refernce
  *
  *
  *
  * --OPTION 3--
  * 1. user moved to the left (to see history) and is not within container borders,
  *  which indicated by (m_delayIndicator >= *size*)
  *
  *        0 ______________ N
  *         |_____________|
  *        new  ->->       old
  *
  *                          ^   user refernce
  *
  */
int BasicChartView::repaintOneSeries(const unsigned int l_series) noexcept
{
    // set default max value
    int l_maxY = 0;

    // clear current series
    m_seriesArray[l_series]->clear();

    // if current series is "none" --> skip
    if (!isActive(l_series)) { return l_maxY; }

    const auto l_activeDataTypeIndex = getActiveDataType(l_series);
    const auto l_containerSize = m_model.getSize(BasicChartModel::Data, l_activeDataTypeIndex);
    if (l_containerSize == 0) {return l_maxY;} // container is empty, no data to pull

    int l_beginIndex = 0;
    int l_endIndex = 0;
    const int l_xAxisSize = getXAxisSpan() + 1;
    int l_offset = 0;

    // user moved right
    if (m_delayIndicator < 0)
    {
        // set delay indicator, so it will not grow out of propotion
        const auto l_rightBorder = (l_xAxisSize - 2) * (-1);
        if (m_delayIndicator < l_rightBorder) {m_delayIndicator = l_rightBorder;}

        // set offset as
        l_offset = m_delayIndicator;

        // Calculate request indexes:
        const int l_diff = abs(m_delayIndicator) - l_xAxisSize;
        if (l_diff < 0) {l_endIndex = qMin(abs(l_diff), l_containerSize);}
        else {l_endIndex = 1;}
    }

    // user is within container borders
    // Notice: we already know m_delayIndicator >= 0
    else if (m_delayIndicator < l_containerSize - 1 )
    {
        // Calculate request indexes:
        l_beginIndex = m_delayIndicator;
        if (l_beginIndex + l_xAxisSize >= l_containerSize) {l_endIndex = l_containerSize;}
        else {l_endIndex = l_beginIndex + l_xAxisSize;}

    }

    // user moved left
    else //m_delayIndicator >= *size* - 1
    {
        // set delay indicator, so it will not grow out of propotion
        const auto l_leftBorder = l_containerSize - 1 ;
        if (m_delayIndicator >= l_leftBorder) {m_delayIndicator = l_leftBorder - 1;}

        l_beginIndex = l_leftBorder - 1;
        l_endIndex = l_leftBorder;
    }

    // Request and Build:

    const auto l_requestedData = m_model.getData(l_activeDataTypeIndex,
                                                 l_beginIndex,
                                                 l_endIndex);
    // update only once
    if (l_series == BasicChartView::SERIES::SERIES_LEFT)
    {
        m_dataContainer = m_model.getDataTime(l_beginIndex, l_endIndex);
    }

    return buildSeriesArray(l_requestedData,  *m_seriesArray[l_series], l_xAxisSize, l_offset);
}


bool BasicChartView::isActive(const unsigned int a_series) const noexcept
{
    const auto l_activeDataTypeIndex = getActiveDataType(a_series);
    return !(m_model.isSeriesIsNull(l_activeDataTypeIndex));
}



void BasicChartView::mousePressEvent(QMouseEvent * event)
{
    if (event->button() ==  Qt::MouseButton::LeftButton)
    {
        if (m_state == MANUAL_STOP)
            {m_state = STOPPED_L_MOUSE_CLICK_PRESS;}
        else
            {m_state = DRAWING_L_MOUSE_CLICK_PRESS;}

        // store the current x position of the chart for reference
        setReferenceXCoordinate(static_cast<int>(chart()->mapToValue(event->pos()).x()));
    }
    QGraphicsView::mousePressEvent(event);
}


void BasicChartView::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->button() ==  Qt::MouseButton::LeftButton)
    {
        if (m_state == STOPPED_L_MOUSE_CLICK_PRESS)
            {m_state = MANUAL_STOP;}
        else if (m_state == DRAWING_L_MOUSE_CLICK_PRESS)
            {m_state = DRAWING;}
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
    m_titleLabel = new QLabel(*m_chartTitle, this); // no need to delete, we be deleted by parent

    // we dont use the base chart title widget, because i could not
    // embed it in the chart, thereforem we use our own label
    setChartTitleVisiblity(m_chartTitleVisibility);
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
    const auto l_centerX = (this->width() - m_titleLabel->width()) / 2;

    m_titleLabel->move(l_centerX,
                       l_legendPosition.y() - l_factor);
}


void BasicChartView::initStartStopButton() noexcept
{
    m_startStopButton = new QPushButton("Stop", this); // no need to delete, we be deleted by parent

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


void BasicChartView::updateChart() noexcept
{
    if (m_state != DRAWING) {/* qDebug() << *m_chartTitle << "updateChart() END";*/return;}

    repaintChart();
}


void BasicChartView::setXLablesVisibility(const bool a_visibility) noexcept
{
    getAxes(CHART_AXIS::X).setLabelsVisible(a_visibility);
}





