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



#ifndef BASICCHARTVIEW_H
#define BASICCHARTVIEW_H

#include "QtCharts"

class ZmHeapTelemetry;

class BasicChartView : public QChartView
{
public:
    BasicChartView(QChart *chart, QWidget *parent = nullptr);
    virtual ~BasicChartView() override;

    enum CHART_AXIS {X, Y_LEFT, Y_RIGHT, CHART_AXIS_N};
    enum SERIES {SERIES_LEFT, SERIES_RIGHT, SERIES_N};

    // should match ZmHeapTelemetry, ordered by priority, added none option for presentation
    enum LOCAL_ZmHeapTelemetry {size, alignment, partition, sharded, cacheSize,
                                cpuset, cacheAllocs, heapAllocs, frees, allocated,
                                none, LOCAL_ZmHeapTelemetry_N};

    std::string localZmHeapTelemetry_ValueToString(const unsigned int a_val) const noexcept;


    void setUpdateFunction( std::function<void(BasicChartView* a_this, void* a_mxTelemetryMsg)>   a_lambda );

    // denotes the time range <--> amount of points
    int getVerticalAxesRange() const noexcept;

    // set how many points one would like to track in the graph
    void setVerticalAxesRange(int a_range) noexcept;

    /**
     * @brief check if current points series is exceeding the amount
     * @return true/false accordingly
     */
    bool isExceedingLimits(const unsigned int a_series) const noexcept;


    void removeFromSeriesFirstPoint(const unsigned int a_series) noexcept;


    void shiftLeft(const unsigned int a_series) noexcept;

    /**
     * @brief addPoint to the !end! of given series
     * @param lPoint
     */
    void appendPoint(const QPointF& a_point, const unsigned int a_series) noexcept;

    /**
     * @brief get specific struct field according to request
     * @param a_strcut
     * @param location
     * @return
     */
    uint64_t getData(const ZmHeapTelemetry& a_strcut, unsigned int location) const noexcept;

    /**
     * @brief used to get the current data type the series is tracking
     * @param a_series
     * @returns
     */
    unsigned int getActiveDataType(const unsigned int a_series) const noexcept;


public slots:
    void updateData(ZmHeapTelemetry a_pair);


protected:
    void initMenuBar() noexcept;

    /**
     * @brief createActions for each option in menu bar
     */
    void createActions() noexcept;

    /**
     * @brief changeSeriesData according to Y axis and data selected
     */
    void changeSeriesData(const unsigned int a_series, const int data_type) noexcept;


    QValueAxis& getAxes(const unsigned int a_axe);
    const QValueAxis& getAxes(const unsigned int a_axe) const;

    QSplineSeries& getSeries(const unsigned int a_series);
    const QSplineSeries& getSeries(const unsigned int a_series) const;

    // for now
    QSize sizeHint() const override final;
    QLineSeries *m_seriesVertical;
    QLineSeries *m_seriesHorizontal;

    // update data
    std::function<void(BasicChartView* a_this, void* a_mxTelemetryMsg)> m_lambda;

    int m_xAxisNumberOfPoints;

    // array storing the axes, see the enum for understaind why 3
    QValueAxis* m_axisArray[3];

    // array for storing the series, see the enum for understaind why 2
    QSplineSeries* m_seriesArray[2];

    // corresponds to data tracked
    // must be within LOCAL_ZmHeapTelemetry values
    unsigned int m_activeData[2];

    void initSeries(const unsigned int a_series) noexcept;

    // member for menu
    QVBoxLayout *m_boxLayout;
    QMenuBar* m_menuBar;
    QMenu* m_settingsMenu;
    QMenu* m_rightSeriesMenu;
    QMenu* m_leftSeriesMenu;

private:


};

#endif // BASICCHARTVIEW_H


