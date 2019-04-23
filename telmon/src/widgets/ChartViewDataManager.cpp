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


#include "src/views/raw/BasicChartView.h"
#include "ChartViewDataManager.h"
#include "QDateTime"

ChartViewDataManager::ChartViewDataManager(BasicChartView& a_view):
    m_view(a_view),
    m_zoomInXMinOffset(0),
    m_isZoomMin(false),
    m_zoomInXMaxOffset(0),
    m_isZoomMax(false)


{

}

ChartViewDataManager::~ChartViewDataManager()
{

}


const QPair<const QDateTime, const QPointF> ChartViewDataManager::getData(const int a_x) const noexcept
{
    // translate to internal representation
    const auto l_internalX = toInternalXCoordinate(a_x);

    if (l_internalX < 0 or l_internalX > getOriginalMaxX()) {return qMakePair(QDateTime(), QPointF());}

    auto l_result = qMakePair(QDateTime(), QPointF());

    // get the X value
    if (l_internalX >= m_view.m_dataContainer.count()) {return qMakePair(l_result.first, l_result.second);}
    else                                               {l_result.first = m_view.m_dataContainer.at(l_internalX);}

    // get the Y value
    const auto& l_leftSeries = m_view.getSeries((BasicChartView::SERIES_LEFT));
    if (l_internalX >= l_leftSeries.count()) { return qMakePair(l_result.first, l_result.second);}
    else                                     {l_result.second = l_leftSeries.at(l_internalX);}

    return qMakePair(l_result.first, l_result.second);

}


int ChartViewDataManager::toInternalXCoordinate(const int a_x) const noexcept
{
    const auto l_dI =  m_view.m_delayIndicator;
    if (l_dI < 0)
    {
        return getOriginalMaxX() + (l_dI - m_zoomInXMaxOffset - a_x);
    }

    return getOriginalMaxX() - m_zoomInXMaxOffset - a_x;
}


void ChartViewDataManager::setZoomMinOffset(const int val) noexcept
{
    m_zoomInXMinOffset = val;
    if (m_zoomInXMinOffset != 0) {m_isZoomMin = true;}
    else {m_isZoomMin = false;}
}


void ChartViewDataManager::setZoomMaxOffset(const int val) noexcept
{
    m_zoomInXMaxOffset = val;
    if (m_zoomInXMaxOffset != 0) {m_isZoomMax = true;}
    else {m_isZoomMax = false;}
}


int ChartViewDataManager::getOriginalMaxX() const noexcept
{
    return static_cast<int>(m_view.getAxes(BasicChartView::CHART_AXIS::X).max()) + m_zoomInXMaxOffset;
}


















