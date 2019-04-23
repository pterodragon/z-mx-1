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
#include "src/widgets/ChartViewDataManager.h"


ChartViewLabelsManagerWidget::ChartViewLabelsManagerWidget(BasicChartView& a_view):
    m_view(a_view),
    m_Y_label(new QLabel(&a_view)),
    m_X_label(new QLabel(&a_view)),
    m_horizontalLine(new QGraphicsLineItem(a_view.chart())),
    m_verticalLine(new QGraphicsLineItem(a_view.chart()))
{
    m_Y_label->setObjectName("m_Y_label");
    m_Y_label->hide();
    m_X_label->setObjectName("m_X_label");
    m_X_label->hide();

    // set lines width
    QPen l_pen;
    l_pen.setWidth(MARKER_WIDTH);
    m_horizontalLine->setPen(l_pen);
    m_verticalLine->setPen(l_pen);

}


ChartViewLabelsManagerWidget::~ChartViewLabelsManagerWidget()
{

}


void ChartViewLabelsManagerWidget::drawLabels(const QMouseEvent& a_event) noexcept
{
    // continue only one move event
    if (!isMouseMoveEvent(a_event.type())) {return;}

    // mouse pos in (x, y)
    const auto l_mousePos = translateToCoordinateSystem(a_event);

    // hide the label if user goes out of borders
    if (isOutOfChartBorders(l_mousePos.x(), l_mousePos.y()))
    {
        hide();
        return;
    }

    const auto l_result = m_view.m_dataManager->getData(static_cast<int>(floor(l_mousePos.x())));
    if (l_result.second.isNull())
    {
        hide();
        return;
    }

    m_X_label->move(getXLabelNewPos(a_event.pos().x()));
    m_X_label->setText(l_result.first.toString(MxTelemetryGeneralWrapper::TIME_FORMAT__hh_mm_ss));
    m_X_label->show();


    const auto l_newYLabelPos = getYLabelNewPos(l_result.second);
    m_Y_label->move(l_newYLabelPos);
    m_Y_label->setText(QString::number(l_result.second.y()));
    m_Y_label->show();

    setLines(a_event.pos().x(), m_view.chart()->plotArea(), l_newYLabelPos.y());
}


bool ChartViewLabelsManagerWidget::isMouseMoveEvent(const QEvent::Type a_type) const noexcept
{
    return a_type == QEvent::Type::MouseMove;
}


const QPointF ChartViewLabelsManagerWidget::translateToCoordinateSystem(const QMouseEvent& a_event) const noexcept
{
    return  m_view.chart()->mapToValue(a_event.localPos());;
}


void ChartViewLabelsManagerWidget::hide() noexcept
{
    m_Y_label->hide();
    m_X_label->hide();
    m_verticalLine->hide();
    m_horizontalLine->hide();
}


bool ChartViewLabelsManagerWidget::isOutOfChartBorders(const qreal a_x, const qreal a_y) const noexcept
{
    const auto& l_X_axis = &m_view.getAxes(BasicChartView::CHART_AXIS::X);
    if (a_x < l_X_axis->min()
            or
        a_x > l_X_axis->max() + 0.2) // take also values on border
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
    auto l_xPos = (X_LABEL_LEFT_MARGIN + a_curMouseXPos);
    const auto l_calc = l_xPos + m_X_label->width();
    if (l_calc > m_view.width()) {l_xPos -= l_calc - m_view.width();}
    return QPoint(l_xPos,
                  m_view.height() - X_LABEL_BUTTON_EXTRA_MARGIN);
}


const QPoint ChartViewLabelsManagerWidget::getYLabelNewPos(const QPointF& a_seriesPos) const noexcept
{
    auto& l_series = m_view.getSeries(BasicChartView::SERIES_LEFT);
    const auto l_seriesDataCoordiante =  m_view.chart()->mapToPosition(a_seriesPos, &l_series);
    return QPoint(X_LABEL_LEFT_MARGIN, static_cast<int>(l_seriesDataCoordiante.y()));
}


qreal ChartViewLabelsManagerWidget::getYLabelData(const int a_curXPos) const noexcept
{
    return m_view.getSeries(BasicChartView::SERIES_LEFT).at(a_curXPos).y();
}


void ChartViewLabelsManagerWidget::setLines(const int a_eventXPos,
                                            const QRectF& a_plotArea,
                                            const int a_yLabelPos) noexcept
{

    m_verticalLine->setLine(a_eventXPos, a_plotArea.y(),
                            a_eventXPos, a_plotArea.y() + a_plotArea.height());

    m_horizontalLine->setLine(a_plotArea.x(),                      a_yLabelPos,
                              a_plotArea.x() + a_plotArea.width(), a_yLabelPos);

    m_verticalLine->show();
    m_horizontalLine->show();
}













