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


#ifndef CHARTDOCKWIDGETMODELWRAPPER_H
#define CHARTDOCKWIDGETMODELWRAPPER_H

#include "DockWidgetModelWrapper.h"
#include "QtCharts"

class ChartSubscriber;

class ChartDockWidgetModelWrapper : public DockWidgetModelWrapper
{
public:
    ChartDockWidgetModelWrapper(DataDistributor& a_dataDistributor);
    virtual ~ChartDockWidgetModelWrapper();

    // # # # DockWidgetModelWrapper INTERFACE begin # # #
    void unsubscribe(const QString& a_mxTelemetryTypeName, const QString& a_mxTelemetryInstanceName) noexcept;
    // # # # DockWidgetModelWrapper INTERFACE end # # #

    QChartView* initChartWidget(const QString& a_mxTelemetryTypeName, const QString& a_mxTelemetryInstanceName);

private:
    QPair<QChartView*, ChartSubscriber*> getSubscriberPair(const int a_mxTelemetryTypeName,
                                                               const QString& a_mxTelemetryInstanceName) noexcept;

    /**
     * @brief qt does not support overload signals and slots
     *        i had to use the following
     *        https://stackoverflow.com/questions/16794695/connecting-overloaded-signals-and-slots-in-qt-5
     * @param a_pair
     * @param a_mxTelemetryTypeNameNumber
     */
    void connectSignalAndSlot(QPair<QChartView*, ChartSubscriber*>& a_pair,
                                                                 const int a_mxTelemetryTypeNameNumber) const noexcept;


    // Notes:
    // QList of all the MxType available by index, that corresponds to MxTelemetry::Type
    // QMap holds chart and corresponding subscriber per instance, for example
    // QList.at[0=Heap].value(MxTelemetry.Msg) will return the associate chart and subscriber for MxTelemetry.Msg
    QList<QMap<QString, QPair<QChartView*, ChartSubscriber*>>*>* m_subscriberDB;
    QPair<QChartView*, ChartSubscriber*> m_defaultValue;
};

#endif // CHARTDOCKWIDGETMODELWRAPPER_H
