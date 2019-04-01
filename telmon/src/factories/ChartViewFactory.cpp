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


#include "MxTelemetry.hpp"
#include "ChartViewFactory.h"
#include "QDebug"
#include "QtCharts"
#include "src/views/raw/BasicChartView.h"


ChartViewFactory::ChartViewFactory()
{

}


QChartView* ChartViewFactory::getChartView(const int a_mxType, const QString& a_mxTelemetryInstanceName) const noexcept
{
    // create the chart
    QChart *l_chart = new QChart();
    l_chart->setTitle(a_mxTelemetryInstanceName + " Chart");
    BasicChartView* l_result = nullptr;

    switch (a_mxType)
    {
    case MxTelemetry::Type::Heap:
        l_result = new BasicChartView(l_chart, MxTelemetry::Type::Heap, a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::HashTbl:
        l_result = new BasicChartView(l_chart, MxTelemetry::Type::HashTbl, a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Thread:
        l_result = new BasicChartView(l_chart, MxTelemetry::Type::Thread, a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Multiplexer:
        l_result = new BasicChartView(l_chart, MxTelemetry::Type::Multiplexer, a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Socket:
        l_result = new BasicChartView(l_chart,
                                      MxTelemetry::Type::Socket,
                                      // there is wierd bug that does not represent the follow char '<' so we replace
                                      QString(a_mxTelemetryInstanceName).replace(a_mxTelemetryInstanceName.indexOf('<'), 1, "/"));
        break;
    case MxTelemetry::Type::Queue:
        l_result = new BasicChartView(l_chart, MxTelemetry::Type::Queue, a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Engine:
        l_result = new BasicChartView(l_chart, MxTelemetry::Type::Engine, a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Link:
        l_result = new BasicChartView(l_chart, MxTelemetry::Type::Link, a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DBEnv:
        l_result = new BasicChartView(l_chart, MxTelemetry::Type::DBEnv, a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DBHost:
        l_result = new BasicChartView(l_chart, MxTelemetry::Type::DBHost, a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DB:
        l_result = new BasicChartView(l_chart, MxTelemetry::Type::DB, a_mxTelemetryInstanceName);
        break;
    default:
        qCritical() << "ChartViewFactory"
                    << __PRETTY_FUNCTION__
                    <<  "unknown MxTelemetry::Type:"
                    << a_mxType
                    << "request, returning...";
        return l_result;
        break;
    }
    l_result->setRenderHint(QPainter::Antialiasing);
    return l_result;
}




