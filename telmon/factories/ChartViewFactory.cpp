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
#include "views/raw/BasicChartView.h"


ChartViewFactory::ChartViewFactory()
{

}


QChartView* ChartViewFactory::getChartView(const int a_mxType, const QString& a_mxTelemetryInstanceName) const noexcept
{

    // create the chart
    QChart *l_chart = new QChart();
    l_chart->setTitle(a_mxTelemetryInstanceName + " Chart");
    BasicChartView* l_result = new BasicChartView(l_chart);


    switch (a_mxType)
    {
    case MxTelemetry::Type::Heap:


        l_result->setUpdateFunction( [] ( BasicChartView* a_this,
                                          void* a_mxTelemetryMsg) -> void
        {
            // Notice: we defiently know a_mxTelemetryMsg type !
            const ZmHeapTelemetry* local = static_cast<ZmHeapTelemetry*>(a_mxTelemetryMsg);

            // test
            qDebug() << " ! BasicChartView::updateData a_pair.first" << local->id;

            // get the series
            QAreaSeries *series = static_cast<QAreaSeries *>(a_this->chart()->series().first());

            // check not excedding the limits
            if (series->upperSeries()->count() == a_this->m_verticalAxesRange)
            {
                series->upperSeries()->remove(0);
                series->lowerSeries()->remove(0);
            }

            // shift all point left
            const int k = series->upperSeries()->count();
            qreal upper_x = 0;
            qreal upper_y = 0;
            qreal lower_x = 0;
            qreal lower_y = 0;

            for (int i = 0; i < k; i++)
            {
                //get curret point x
                upper_x = series->upperSeries()->at(i).x() - 2;
                upper_y = series->upperSeries()->at(i).y();
                series->upperSeries()->replace(i, upper_x, upper_y);

                series->lowerSeries()->replace(i,
                                               series->lowerSeries()->at(i).x() - 2,
                                               series->lowerSeries()->at(i).y());
            }

            int l_randomNumberForTesting =rand() % 100 ;
            series->setName("CPU Usage: " + QString::number(l_randomNumberForTesting));
            series->upperSeries()->append(QPointF(a_this->m_verticalAxesRange, l_randomNumberForTesting));

            //actually lower is always permenant, add until reach limit
            if (series->lowerSeries()->count() < a_this->m_verticalAxesRange)
            {
                series->lowerSeries()->append(QPointF(a_this->m_verticalAxesRange, 0));
            }

            qDebug() << "series->upperSeries().count()" << series->upperSeries()->count();
            qDebug() << "series->lowerSeries()->count()" << series->lowerSeries()->count();
            //series->upperSeries()->re
        });



        break;
    case MxTelemetry::Type::HashTbl:
//        l_result = new BasicTableWidget(QList<QString>({"Data"}),
//                                 QList<QString>({"time",   "linear",  "bits",  "slots",
//                                                 "cBits",  "cSlots",  "count", "resized",
//                                                 "loadFactor", "effLoadFactor", "nodeSize"}),
//                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Thread:

        break;
    case MxTelemetry::Type::Multiplexer:
//        l_result = new BasicTableWidget(QList<QString>({"Data"}),
//                                 QList<QString>({"time",   "state",       "nThreads",
//                                                 "priority", "partition",  "isolation",  "rxThread",
//                                                 "txThread",   "stackSize", "rxBufSize", "txBufSize"}),
//                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Socket:
//        l_result = new BasicTableWidget(QList<QString>({"Data"}),
//                                 QList<QString>({"time",      "type",      "remoteIP",  "remotePort",
//                                                 "localIP",   "localPort", "fd",        "flags",
//                                                 "mreqAddr",  "mreqIf",    "mif",       "ttl",
//                                                 "rxBufSize", "rxBufLen",  "txBufSize", "txBufLen"}),
//                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Queue:
//        l_result = new BasicTableWidget(QList<QString>({"Data"}),
//                                 QList<QString>({"time",      "type",    "full",    "size",
//                                                 "count",     "seqNo",   "inCount", "inBytes",
//                                                 "outCount",  "outBytes"}),
//                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Engine:
//        l_result = new BasicTableWidget(QList<QString>({"Data"}),
//                                 QList<QString>({"time",   "state",    "nLinks",    "up",
//                                                 "down",   "disabled", "transient", "reconn",
//                                                 "failed", "mxID",     "rxThread",  "txThread"}),
//                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Link:
//        l_result = new BasicTableWidget(QList<QString>({"Data"}),
//                                 QList<QString>({"time",  "state",   "reconnects",    "rxSeqNo",
//                                                 "txSeqNo"}),
//                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DBEnv:
//        l_result = new BasicTableWidget(QList<QString>({"Data"}),
//                                 QList<QString>({"time",        "self",            "master",           "prev",
//                                                 "next",        "state",           "active",           "recovering",
//                                                 "replicating", "nDBs",            "nHosts",           "nPeers",
//                                                 "nCxns",       "heartbeatFreq",   "heartbeatTimeout", "reconnectFreq",
//                                                 "m_dbenv",     "electionTimeout", "writeThread"}),
//                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DBHost:
//        l_result = new BasicTableWidget(QList<QString>({"Data"}),
//                                 QList<QString>({"time",  "priority",  "state",  "voted",
//                                                 "ip",    "port"}),
//                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DB:
//        l_result = new BasicTableWidget(QList<QString>({"Data"}),
//                                 QList<QString>({"id",   "recSize",    "compress",    "cacheMode",
//                                                 "cacheSize",   "path",    "fileSize",    "fileRecs",
//                                                 "filesMax",   "preAlloc",    "minRN",    "allocRN",
//                                                 "fileRN",    "cacheLoads",    "cacheMisses",    "fileLoads",
//                                                 "fileMisses"}),
//                                 a_mxTelemetryInstanceName);
        break;
    default:
        delete l_result;
        l_result = nullptr;
        qWarning() << "unknown MxTelemetry::Type:" << a_mxType<< "request, returning...";
        break;
    }
    return l_result;
}
