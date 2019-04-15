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

#ifndef CHARTWIDGETDOCKWINDOWMODEL_H
#define CHARTWIDGETDOCKWINDOWMODEL_H

template <class T, class V>
class QPair;

template <class T>
class QVector;

class BasicChartView;
class BasicChartController;

class QString;

class ChartWidgetDockWindowModel
{
public:
    ChartWidgetDockWindowModel();
    ~ChartWidgetDockWindowModel();

    void addToActiveCharts(QPair<BasicChartController*, BasicChartView*>)     noexcept;

    void removeFromActiveCharts(QPair<BasicChartController*,BasicChartView*>) noexcept;
    void removeFromActiveCharts(QPair<void*, void*>)                          noexcept;

    // QPair<Before last, Last>
    QPair<QPair<BasicChartController*, BasicChartView*>,
          QPair<BasicChartController*, BasicChartView*>>
    getLastTwoCharts() const noexcept;

private:
    const QString* m_className;

    /**
     * @brief holds avaiable views, such that the last
     * view is always the most lowest one the screen.
     * So make sure the X-axis of only the lowest
     * view is active.
     *
     * eacj view has also is associated controller
     */
    QVector<QPair<BasicChartController*, BasicChartView*>>* m_activeCharts;
};

#endif // CHARTWIDGETDOCKWINDOWMODEL_H
