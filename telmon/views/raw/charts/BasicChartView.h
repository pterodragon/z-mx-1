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

#include "MxTelemetry.hpp"

#include "QtCharts"



class BasicChartView : public QChartView
{
public:
    BasicChartView(QChart *chart,
                   const int a_associatedTelemetryType,
                   QWidget *parent = nullptr);
    virtual ~BasicChartView() override;

    enum CHART_AXIS {X, Y_LEFT, Y_RIGHT, CHART_AXIS_N};
    enum SERIES {SERIES_LEFT, SERIES_RIGHT, SERIES_N};
    static const int NUMBER_OF_Y_AXIS = 2;



    /**
     * @brief implement in each inherting class to get type
     * @param a_val
     * @return
     */
    //std::string virtual localTypeValueToString(const unsigned int a_val) const noexcept = 0;


    /**
     * @brief used to check for each sub class if given type is none
     *        because each subclass underlying telemetry struct is of different size
     * @param a_activeType
     * @return
     */
    //bool virtual isGivenTypeNotUsed(const unsigned int a_activeType) const noexcept = 0;


    /**
     * @brief get specific msg sub field according to request
     * @param a_mxTelemetryMsg
     * @param location
     * @return qreal because QPointF gets qreal as params
     *         Notice: qreal is QT typedef for double
     */
    //qreal virtual getData(void* const a_mxTelemetryMsg, const unsigned int location) const noexcept = 0;


    // denotes the time range <--> amount of points
    qreal getVerticalAxesRange() const noexcept;


    void setUpdateFunction( std::function<void(BasicChartView* a_this, void* a_mxTelemetryMsg)>   a_lambda );


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
     * @brief used to get the current data type the series is tracking
     * @param a_series
     * @returns
     */
    int getActiveDataType(const unsigned int a_series) const noexcept;


public slots:
    void updateData(ZmHeapTelemetry a_pair);
    void updateData(ZmHashTelemetry a_pair);
    void updateData(ZmThreadTelemetry a_pair);
    void updateData(ZiMxTelemetry a_pair);
    void updateData(ZiCxnTelemetry a_pair); // SOCKET
    void updateData(MxTelemetry::Queue a_pair); // inside MxTelemetry
    void updateData(MxTelemetry::Engine a_pair);
    void updateData(MxTelemetry::Link a_pair);
    void updateData(MxTelemetry::DBEnv a_pair);
    void updateData(MxTelemetry::DBHost a_pair);
    void updateData(MxTelemetry::DB a_pair);


protected:


    /**
     * @brief init the menu bar for the dock
     * i.e. "Settings" etc...
     * virtual so inherent classes can change style
     * for behavior change -> adjust createActions below
     */
    void initMenuBar() noexcept;


    /**
     * @brief create actions for each option in menu bar
     * virtual so inherent classes can change behavior
     * see also initMenuBar()
     */
    void createActions() noexcept;


    /**
     * @brief changeSeriesData according to Y axis and data selected
     */
    void changeSeriesData(const unsigned int a_series, const int data_type) noexcept;

    /**
     * @brief used to set update data function
     * This function is generic and compatible with all sub classess
     * if sub classes implement required functions
     */
    void setDefaultUpdateDataFunction() noexcept;

    QValueAxis& getAxes(const unsigned int a_axe);
    const QValueAxis& getAxes(const unsigned int a_axe) const;

    QSplineSeries& getSeries(const unsigned int a_series);
    const QSplineSeries& getSeries(const unsigned int a_series) const;

    // for now
    //QSize virtual sizeHint() const override final;

    // update data
    std::function<void(BasicChartView* a_this, void* a_mxTelemetryMsg)> m_lambda;

    // array storing the axes, see the enum for understaind why 3
    QValueAxis* m_axisArray[3];

    // array for storing the series, see the enum for understaind why 2
    QSplineSeries* m_seriesArray[2];


    void initSeries() noexcept;

    // member for menu
    QVBoxLayout *m_boxLayout;
    QMenuBar* m_menuBar;
    QMenu* m_settingsMenu;
    QMenu* m_rightSeriesMenu;
    QMenu* m_leftSeriesMenu;

private:
    // will be used to access the right MxTelemetryType via factory
    int m_associatedTelemetryType;

    std::array<int, NUMBER_OF_Y_AXIS> m_activeData;

    // represents the string names ordered by priority
    const QVector<QString>& m_db;
};

#endif // BASICCHARTVIEW_H


