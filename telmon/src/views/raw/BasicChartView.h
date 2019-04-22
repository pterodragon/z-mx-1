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

#include "QtCharts/QChartView"
#include "src/models/raw/BasicChartModel.h"


QT_CHARTS_BEGIN_NAMESPACE
class QChart;
QT_CHARTS_END_NAMESPACE

QT_CHARTS_USE_NAMESPACE

#include "QValueAxis"
#include "QSplineSeries"

class BasicChartController;
class ChartViewCustomizer;
class ChartViewContextMenu;
class QLabel;
class QPushButton;
class ChartViewLabelsManagerWidget;

class BasicChartView : public QChartView
{
    Q_OBJECT

    friend class ChartViewCustomizer;
    friend class ChartViewContextMenu;
    friend class ChartViewLabelsManagerWidget;

public:
    BasicChartView(const BasicChartModel& a_model,
                   const QString& a_chartTitle,
                   QWidget *a_parent,
                   BasicChartController& a_controller);
    virtual ~BasicChartView() override;

    enum CHART_AXIS {X, Y_LEFT, Y_RIGHT, CHART_AXIS_N};
    enum SERIES {SERIES_LEFT, SERIES_RIGHT, SERIES_N};

    // * * * Attributes getters/setters * * * //

    /**
     * @brief get current data type the series is tracking
     * @param a_series
     * @returns
     */
    int getActiveDataType(const unsigned int a_series) const noexcept;

    void setXAxisSpan(const int) noexcept;
    int getXAxisSpan() const noexcept {return static_cast<int>(m_axisArray[CHART_AXIS::X]->max());}

    bool getChartTitleVisiblity() const noexcept;
    QString& getChartTitle() const noexcept;
    void setXLablesVisibility(const bool) noexcept;

signals:
    void closeAction();

public slots:
    // * * * Update Chart Related Function * * * //
    void updateChart() noexcept;

protected:

    // states
    enum {DRAWING = 0,                   // regular state
          MANUAL_STOP,                   // user press the "stop" button
          DRAWING_L_MOUSE_CLICK_PRESS, // user pressed on graph, but still not released
                                         // while graph is running
          STOPPED_L_MOUSE_CLICK_PRESS  // user pressed on graph, but still not released
                                         // while graph is stopped
         };
     static const int DEFAULT_TICK_COUNT = 5;
     static const int DEFAULT_X_AXIS_MAX = 60;
     static const int DEFAULT_X_AXIS_MIN = 1;
     static const int VERTICAL_AXIS_RANGE_FACTOR = 10;

    // * * * Attributes getters * * * //
    QValueAxis& getAxes(const unsigned int a_axe);
    const QValueAxis& getAxes(const unsigned int a_axe) const;

    QSplineSeries& getSeries(const unsigned int a_series);
    const QSplineSeries& getSeries(const unsigned int a_series) const;


    // * * * Update Chart Related Function * * * //
    // addnig mouse move event functionialy
    void mouseMoveEvent(QMouseEvent * event) override final;
    void mousePressEvent(QMouseEvent * event) override final;
    void mouseReleaseEvent(QMouseEvent * event) override final;

    /**
     * @brief resizeEvent
     * we need to override this function in order to rearrange the legend
     * @param event
     */
    void resizeEvent(QResizeEvent *event) override;

    void repaintChart() noexcept;

    int m_state;

    void updateYAxisMax(const double a_newMax, QValueAxis& a_axis, const int a_tickCount) noexcept;
    QPair<qreal, qreal> getCurrentMaxAndMinYFromSeries(const unsigned int) const noexcept;
    //void getCurrentMinYFromSeries() const noexcept;
private:
    // * * * Init Functons * * * //
    void initSeries() noexcept;

    /**
     * @brief This function is used to set legend properties
     */
    void initLegend() noexcept;

    void initChartLabel() noexcept;
    void initStartStopButton() noexcept;


    /**
     * @brief create actions for each option in menu bar
     * virtual so inherent classes can change behavior
     * see also initMenuBar()
     */
    void createActions() noexcept;

    // * * * Update Chart Related Function * * * //
    void setReferenceXCoordinate(const int a_x) noexcept {m_xReferenceCoordiante = a_x;}
    int getReferenceX() const noexcept {return m_xReferenceCoordiante;}
    void updateVerticalAxisRange(const int a_data) noexcept;
    void updateYAxisMin(const double a_newMin,
                        QValueAxis& a_axis,
                        const int a_tickCount) noexcept;
    void updateYAxis(const QPair<qreal, qreal> a_pair,
                     QValueAxis& a_axis,
                     const int a_tickCount) noexcept;
    void updateYRangesManually() noexcept;
    void handleMouseEventHelper(const int) noexcept;

    /**
     * @brief handle legend resize event part
     * for example, if the width of the chart becomes too low,
     * will split the legend to two lines
     */
    void resizeEventLegend(const QSize&) noexcept;

    /**
     * @brief make sure the chart title is located above the legend
     */
    void resizeEventTitle() noexcept;

    /**
     * @brief hide/show button
     */
    void resizeEventStartStopButton() noexcept;

    /**
     * @brief handleStartStopFunctionality
     */
    void handleStartStopFunctionality() noexcept;

    /**
     * @brief changeSeriesData according to Y axis and data selected
     */
    void changeSeriesData(const unsigned int a_series,
                          const int data_type,
                          const QMenu* a_menu) noexcept;

    /**
     * @brief if true, show chart title
     *        else, hide the chart
     */
    void setChartTitleVisiblity(const bool) noexcept;


    int repaintOneSeries(const unsigned int l_series) noexcept;

    template <class T>
    int buildSeriesArray(const T& a_data,
                          QSplineSeries& a_series,
                          const int a_xAxisSize,
                          const int a_offset) noexcept
    {
        int l_maxY = 0;

        for (auto i = 0; i < a_data.size(); i++)
        {
            int l_x = a_xAxisSize - i + a_offset;
            int l_y = a_data.at(i);
            if (l_y == -1) {l_y = 0;} // another sanity check, should not happen
            a_series.append(QPointF(l_x, l_y));

            l_maxY = qMax(l_maxY, l_y);
        }

        return l_maxY;
    }


     // * * * Data Members * * * //
    const BasicChartModel& m_model;
    QValueAxis* m_axisArray[3];
    QSplineSeries* m_seriesArray[2];
    std::array<int, BasicChartModel::NUMBER_OF_Y_AXIS> m_activeData;
    const QVector<QString>& m_chartFields; //  ordered by priority
    int m_delayIndicator; // indicator of delay
    int m_xReferenceCoordiante;
    bool m_chartTitleVisibility;
    QString* m_chartTitle;
    QLabel* m_titleLabel;
    QPushButton* m_startStopButton;
    QTimer* m_timer;
    ChartViewCustomizer* m_designer;
    ChartViewContextMenu* m_contextMenu;
    BasicChartController& m_controller;
    QList<QDateTime> m_dataContainer;
    ChartViewLabelsManagerWidget* m_labelManager;


};

#endif // BASICCHARTVIEW_H

