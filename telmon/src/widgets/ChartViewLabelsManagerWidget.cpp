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


#include "ChartViewLabelsManagerWidget.h"
#include "QLabel"
#include "QMouseEvent"
#include "src/views/raw/BasicChartView.h"
#include <math.h>       /* floor */
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "QDateTime"

ChartViewLabelsManagerWidget::ChartViewLabelsManagerWidget(BasicChartView& a_view):
    m_view(a_view),
    m_Y_label(new QLabel(&a_view)),
    m_X_label(new QLabel(&a_view))
{
    m_Y_label->hide();
    m_X_label->hide();
}


ChartViewLabelsManagerWidget::~ChartViewLabelsManagerWidget()
{

}


void ChartViewLabelsManagerWidget::drawLabels(const QMouseEvent& a_event) noexcept
{
    // continue only one move event
    if (!isMouseMoveEvent(a_event.type())) {return;}

    // get the series value at pos
    const auto valueGivenSeries = getSeriesValue(a_event);

    // hide the label if user goes out of borders
    if (isOutOfChartBorders(valueGivenSeries.x(), valueGivenSeries.y()))
    {
        hide();
        return;
    }


    const auto l_curXPos = BasicChartView::DEFAULT_X_AXIS_MAX - static_cast<int>(floor(valueGivenSeries.x()));

    if (!isDataExistsForCurrentX(l_curXPos))
    {
        hide();
        return;
    }


    m_X_label->move(getXLabelNewPos(a_event.pos().x()));
    m_X_label->setText(getXLabelData(l_curXPos));
    m_X_label->show();


    // set X

//    m_Y_label->move(getYLabelNewPos(l_curXPos));
//    coordToPixel
//            PxielToCord
    //m_view.getSeries(BasicChartView::SERIES_LEFT).at()
//    qDebug() << "valueGivenSeries" << valueGivenSeries;
//    qDebug() << "" <<  m_view.chart()->mapToPosition(valueGivenSeries,
//                                                     &m_view.getSeries(BasicChartView::SERIES_LEFT));

//    const auto bla = m_view.getSeries(BasicChartView::SERIES_LEFT).at(l_curXPos);
//    qDebug() << "bla" << bla;
//    const auto bla2 =  m_view.chart()->mapToPosition(bla, &m_view.getSeries(BasicChartView::SERIES_LEFT));
//    qDebug() <<  bla2;
//    qDebug();
//    m_Y_label->move(5, bla2.y());
    m_Y_label->move(getYLabelNewPos(l_curXPos));
    m_Y_label->setText(QString::number(getYLabelData(l_curXPos)));
    m_Y_label->show();


}


bool ChartViewLabelsManagerWidget::isMouseMoveEvent(const QEvent::Type a_type) const noexcept
{
    return a_type == QEvent::Type::MouseMove;
}


const QPointF ChartViewLabelsManagerWidget::getSeriesValue(const QMouseEvent& a_event) const noexcept
{
    const auto l_widgetPos = a_event.localPos();
    const auto l_scenePos = m_view.mapToScene(QPoint(static_cast<int>(l_widgetPos.x()),
                                                   static_cast<int>(l_widgetPos.y())));
    const auto l_chartItemPos = m_view.chart()->mapFromScene(l_scenePos);

//    qDebug() << "l_widgetPos" << l_widgetPos;
//    qDebug() << "l_scenePos" << l_scenePos;
//    qDebug() << "l_chartItemPos" << l_chartItemPos;
//    qDebug() << "m_view.chart()->mapToValue(l_chartItemPos);" <<
//                m_view.chart()->mapToPosition(QPointF(0, 0),
//                                           &m_view.getSeries(BasicChartView::SERIES_LEFT));
//    qDebug();


    return  m_view.chart()->mapToValue(l_chartItemPos);
}


void ChartViewLabelsManagerWidget::hide() noexcept
{
    m_Y_label->hide();
    m_X_label->hide();
}


bool ChartViewLabelsManagerWidget::isOutOfChartBorders(const qreal a_x, const qreal a_y) const noexcept
{
    const auto& l_X_axis = &m_view.getAxes(BasicChartView::CHART_AXIS::X);
    if (a_x < l_X_axis->min()
            or
        a_x > l_X_axis->max())
    {
        return true;
    }

     const auto& l_Y_axis = &m_view.getAxes(BasicChartView::CHART_AXIS::Y_LEFT);
     if (a_y < l_Y_axis->min()
             or
         a_y > l_Y_axis->max())
     {
         return true;
     }

     return false;
}


bool ChartViewLabelsManagerWidget::isDataExistsForCurrentX(const qreal a_x) const noexcept
{
    // if X pos is bigger than size of array, that mean there is no series data yet
    // notice that our reference is alwats the left legend
    // another sanity check: notice: m_dataContainer.size
    // should always == getSeries(SERIES_LEFT).count()
    return
            a_x  < m_view.getSeries(BasicChartView::SERIES_LEFT).count()
            and
            a_x < m_view.m_dataContainer.count();
}


const QPoint ChartViewLabelsManagerWidget::getXLabelNewPos(const int a_curMouseXPos) const noexcept
{
    return QPoint(X_LABEL_LEFT_MARGIN + a_curMouseXPos,
                  m_view.height() - X_LABEL_BUTTON_EXTRA_MARGIN);
}


const QString ChartViewLabelsManagerWidget::getXLabelData(const int a_curXPos) const noexcept
{
    return m_view.m_dataContainer.at(a_curXPos).toString(MxTelemetryGeneralWrapper::TIME_FORMAT__hh_mm_ss);
}


const QPoint ChartViewLabelsManagerWidget::getYLabelNewPos(const int a_curXPos) const noexcept
{
        auto& l_series = m_view.getSeries(BasicChartView::SERIES_LEFT);
        const auto l_seriesDataPoint = l_series.at(a_curXPos);
        const auto l_seriesDataCoordiante =  m_view.chart()->mapToPosition(l_seriesDataPoint,
                                                         &l_series);



    return QPoint(X_LABEL_LEFT_MARGIN, static_cast<int>(l_seriesDataCoordiante.y()));
}


qreal ChartViewLabelsManagerWidget::getYLabelData(const int a_curXPos) const noexcept
{
    return m_view.getSeries(BasicChartView::SERIES_LEFT).at(a_curXPos).y();
}
















