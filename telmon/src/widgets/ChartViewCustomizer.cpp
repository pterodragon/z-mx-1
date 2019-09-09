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

#include "ChartViewCustomizer.h"
#include "src/views/raw/BasicChartView.h"
#include "QLabel"
#include "QPushButton"
#include "QGraphicsLayout"


ChartViewCustomizer::ChartViewCustomizer(QString a_className,
                                         BasicChartView& a_view):
    m_className(new QString(a_className)),
    m_view(a_view),
    m_lablesSize(SMALL_SIZE)
{

}


ChartViewCustomizer::~ChartViewCustomizer()
{
    delete m_className;
    m_className = nullptr;
}


void ChartViewCustomizer::init() noexcept
{
    // set theme
    //chart()->setTheme(QChart::ChartThemeDark);

    // Customize title
    {
        // scale content to avaiable size
        m_view.m_titleLabel->setScaledContents(true);
        m_view.m_titleLabel->setStyleSheet(
                    "QLabel { "
                    "   background-color : #404244;"
                    "   color            : #bec0c2; "
                    "}");
        m_view.m_titleLabel->setFont(m_view.chart()->legend()->font());
    }

    // Customize legend
    {
        m_view.chart()->legend()->setLabelColor(QRgb(0xbec0c2));
    }


    // Customize series
    auto l_series = &m_view.getSeries(BasicChartView::SERIES::SERIES_LEFT);
    {
        QPen pen(QRgb(0x45c6d6));
        pen.setWidth(2);
        l_series->setPen(pen);
    }

    l_series = &m_view.getSeries(BasicChartView::SERIES::SERIES_RIGHT);
    {
        QPen pen(QRgb(0x4e9a06));
        pen.setWidth(2);
        l_series->setPen(pen);
    }

    // customize button
    {
        m_view.m_startStopButton->setStyleSheet(
                    "QPushButton { "
                    "   border           :  0.1px solid #bec0c2;"
                    "   background-color : #404244; "
                    "   color            : #bec0c2; "
                    "}"
                    "QPushButton:hover {"
                    "   background-color: #1d545c;"
                    "}"
                    "QPushButton:pressed {"
                    "   background-color: #1d545c;"
                    "}"
                    );
    }

    //customize axis
    auto l_axis =  &m_view.getAxes(BasicChartView::CHART_AXIS::X);
    l_axis->setLabelsVisible(false);
    l_axis->setLabelFormat("%d");
    l_axis->labelsFont().setPixelSize(m_lablesSize);
    l_axis->setLabelsColor(QRgb(0xbec0c2));
    l_axis->setGridLineColor(QRgb(0x404244));

    l_axis = &m_view.getAxes(BasicChartView::CHART_AXIS::Y_LEFT);
    l_axis->setLabelFormat("%d");
    l_axis->labelsFont().setPixelSize(m_lablesSize);
    l_axis->setLabelsColor(QRgb(0xbec0c2));
    l_axis->setGridLineColor(QRgb(0x404244));

    l_axis = &m_view.getAxes(BasicChartView::CHART_AXIS::Y_RIGHT);
    l_axis->setLabelsVisible(false);
    l_axis->setLabelFormat("%d");
    l_axis->labelsFont().setPixelSize(m_lablesSize);
    l_axis->setLabelsColor(QRgb(0xbec0c2));
    l_axis->setGridLineColor(QRgb(0x404244));

    // set not margin, so utilize all window size
    m_view.chart()->layout()->setContentsMargins(0,0,0,0);
    m_view.chart()->setMargins(QMargins(0,0,0,0));

    // no need for shadow in charts representation
    m_view.chart()->setDropShadowEnabled(false);

    // Customize chart background
    QLinearGradient backgroundGradient;
    backgroundGradient.setStart(QPointF(0, 0));
    backgroundGradient.setFinalStop(QPointF(0, 1));
    backgroundGradient.setColorAt(0.0, QRgb(0x2E2F30));
    backgroundGradient.setColorAt(1.0, QRgb(0x2E2F30));
    backgroundGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    m_view.chart()->setBackgroundBrush(backgroundGradient);
}












