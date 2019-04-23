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


#ifndef ChartViewLabelsManagerWidget_H
#define ChartViewLabelsManagerWidget_H

#include "QEvent"


class QString;
class QLabel;
class QMouseEvent;
class BasicChartView;
class QPointF;
class QPoint;
class QGraphicsLineItem;
class QRectF;



class ChartViewLabelsManagerWidget
{
public:
    ChartViewLabelsManagerWidget(BasicChartView&);
    ~ChartViewLabelsManagerWidget();

    void drawLabels(const QMouseEvent& a_event) noexcept;

    static const int X_LABEL_LEFT_MARGIN = 5;
    static const int X_LABEL_BUTTON_EXTRA_MARGIN = 20;

    static const int MARKER_WIDTH = 2;

protected:
    BasicChartView& m_view;
    QLabel* m_Y_label; // will represent data
    QLabel* m_X_label; // will represent time

    bool isMouseMoveEvent(const QEvent::Type a_type) const noexcept;
    const QPointF translateToCoordinateSystem(const QMouseEvent& a_event) const noexcept;
    bool isOutOfChartBorders(const qreal a_x, const qreal a_y) const noexcept;
    void hide() noexcept;
    bool isDataExistsForCurrentX(const qreal a_x) const noexcept;

    const QPoint getXLabelNewPos(const int a_curMouseXPos) const noexcept;

    const QPoint getYLabelNewPos(const QPointF& a_curXPos) const noexcept;
    qreal getYLabelData(const int a_curXPos) const noexcept;

    QGraphicsLineItem* m_horizontalLine;
    QGraphicsLineItem* m_verticalLine;

    void setLines(const int a_eventXPos,
                  const QRectF& a_plotArea,
                  const int a_yLabelPos) noexcept;
};



#endif // ChartViewLabelsManagerWidget_H
