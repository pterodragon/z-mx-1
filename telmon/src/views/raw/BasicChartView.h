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


    void setUpdateFunction( std::function<void(BasicChartView* a_this, void* a_mxTelemetryMsg)>   a_lambda );


    /**
     * @brief used to get the current data type the series is tracking
     * @param a_series
     * @returns
     */
    int getActiveDataType(const unsigned int a_series) const noexcept;


    /**
     * @brief set to true/false to start/stop drawing the chart
     * @param a_status
     */
    void setDrawChartFlag(const bool a_status) noexcept {m_drawChartFlag = a_status;}


    /**
     * @brief get wheter the widget is drawing chart or not
     * @return
     */
    bool getDrawChartFlag() const noexcept {return m_drawChartFlag;}


    void setXAxisSpan(const int) noexcept;


    int getXAxisSpan() const noexcept {return static_cast<int>(m_axisArray[CHART_AXIS::X]->max());}


public slots:
    void updateData(ZmHeapTelemetry a_pair);
    void updateData(ZmHashTelemetry a_pair);
    void updateData(ZmThreadTelemetry a_pair);
    void updateData(ZiMxTelemetry a_pair);
    void updateData(ZiCxnTelemetry a_pair);     // SOCKET
    void updateData(MxTelemetry::Queue a_pair); // inside MxTelemetry
    void updateData(MxTelemetry::Engine a_pair);
    void updateData(MxAnyLink::Telemetry);
    void updateData(ZdbEnv::Telemetry a_pair);
    void updateData(ZdbHost::Telemetry a_pair);
    void updateData(ZdbAny::Telemetry a_pair);


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

    // member for menu
    QVBoxLayout *m_boxLayout;
    QMenuBar* m_menuBar;
    QMenu* m_settingsMenu;
    QMenu* m_rightSeriesMenu;
    QMenu* m_leftSeriesMenu;


    // addnig mouse move event functionialy
    void mouseMoveEvent(QMouseEvent * event) override final;
    void mousePressEvent(QMouseEvent * event) override final;
    void mouseReleaseEvent(QMouseEvent * event) override final;

private:

    /**
     * @brief set reference X coordiante
     * @param a_x
     */
    void setReferenceX(const int a_x) noexcept {m_xReferenceCoordiante = a_x;}
    int getReferenceX() const noexcept {return m_xReferenceCoordiante;}


    /**
     * @brief repaintChart
     */
    void repaintChart() noexcept;


    /**
     * @brief isSeriesIsNull
     * @param a_series
     * @return
     */
    bool isSeriesIsNull(const int a_series) const noexcept;


    void initSeries() noexcept;


    // update data
    std::function<void(BasicChartView* a_this, void* a_mxTelemetryMsg)> m_lambda;


    // array storing the axes
    QValueAxis* m_axisArray[3];


    // array for storing the series
    QSplineSeries* m_seriesArray[2];


    void updateVerticalAxisRange(const double a_data) noexcept;


    // will be used to access the right MxTelemetryType via factory
    int m_associatedTelemetryType;

    std::array<int, NUMBER_OF_Y_AXIS> m_activeData;


    // represents the string names ordered by priority
    const QVector<QString>& m_chartFields;


    // Data strucutre to hold all chart points
    QVector<QVector<double>*>* m_chartDataContainer;


    /**
     * @brief initChartDataContainer
     */
    void initChartDataContainer() const noexcept;


    int m_delayIndicator; // indicator of delay


    /**
     * @brief functio update the local data container with recent telemetryMsg
     * @param a_mxTelemetryMsg
     */
    void addDataToChartDataContainer(void* a_mxTelemetryMsg) const noexcept;


    // indicates if we should draw the chart or not
    // that is done by updating the relevant series
    bool m_drawChartFlag;

    // used to store to above varaible in some cases
    // should be used only by mouseReleaseEvent() and mouseReleaseEvent()
    bool m_drawChartFlagStoreVariable;

    int m_xReferenceCoordiante;

    void handleMouseEventHelper(const int) noexcept;
};

#endif // BASICCHARTVIEW_H















