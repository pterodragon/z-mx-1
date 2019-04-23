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


#ifndef CHARTVIEWDATAMANAGER_H
#define CHARTVIEWDATAMANAGER_H

class BasicChartView;

template<class T, class V>
class QPair;

class QDateTime;
class QPointF;

/**
 * @brief The ChartViewModel encapsulated the chart data for life time
 *        This class come to assist in managing the internal chart view data
 */
class ChartViewDataManager
{
public:
    ChartViewDataManager(BasicChartView& a_view);
    ~ChartViewDataManager();

    /**
     * @brief give x position on x array, get back the (x, y) data
     *        in case of invalid data, will return y=-1;
     *        This should be the only function to request data from the container
     *        because it does all the necessary checks
     *        RIGHT NOW only support left series
     * @param x
     * @param y
     * @return
     */
    const QPair<const QDateTime, const QPointF> getData(const int a_x) const noexcept;

    /**
     * @brief toInternalXCoordinate
     * translate chart X to internal X
     * assume X axis length is N -> than container size is (N + 1) // counting the 0
     * example chartX at pos k <--> container at pos container.size() - k - 1
     * DO CONSIDER IF ZOOM IN/OUT
     * @param a_x
     * @return
     */
    int toInternalXCoordinate(const int a_x) const noexcept;

    // * * * * * Zoom realted --BEGIN--* * * * * //
    // Notice: read notes in var declaration
    bool isZoom() const noexcept {return m_isZoomMin or m_isZoomMax;}

    int getZoomMinOffset() const noexcept {return m_zoomInXMinOffset;}

    /**
     * @brief val should make the following true
     * (x_min_current_coordinate + val = x_min_original_coordinate)
     * @param val
     */
    void setZoomMinOffset(const int val) noexcept;

    int getZoomMaxOffset() const noexcept {return m_zoomInXMaxOffset;}

    /**
     * @brief val should make the following true
     * (x_max_current_coordinate + val = x_max_original_coordinate)
     * @param val
     */
    void setZoomMaxOffset(const int val) noexcept;

    /**
     * @brief return the original x coordiante max value
     * @return
     */
    int getOriginalMaxX() const noexcept;
    // * * * * * Zoom realted --END--* * * * * //

protected:
    BasicChartView& m_view;

    // if x min original is 0
    // user zoom in min x to 10
    // than offset should be -10
    // if we take current X axis x coordiante and add offset we should reach original pos
    int  m_zoomInXMinOffset;
    bool m_isZoomMin;
    // if x min original is 60
    // user zoom in max x to 50
    // than offset should be +10
    // if we take current X axis x coordiante and add offset we should reach original pos
    int  m_zoomInXMaxOffset;
    bool m_isZoomMax;
};

#endif // CHARTVIEWDATAMANAGER_H
