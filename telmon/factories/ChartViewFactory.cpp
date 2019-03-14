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

            // iterate over {SERIES::SERIES_LEFT, SERIES::SERIES_RIGHT}
            for (unsigned int l_curSeries = 0; l_curSeries < BasicChartView::SERIES::SERIES_N; l_curSeries++)
            {
                const unsigned int l_activeDataType = a_this->getActiveDataType(l_curSeries);
                if (l_activeDataType == BasicChartView::LOCAL_ZmHeapTelemetry::none)
                {
                    // nothing to do for this chart
                    continue;
                }

                // remove first point if exceeding the limit of points
                if (a_this->isExceedingLimits(l_curSeries))
                {
                       a_this->removeFromSeriesFirstPoint(l_curSeries);
                }

                // shift all point left
                a_this->shiftLeft(l_curSeries);

                // get the current value according the user choice
                const auto l_newPoint = QPointF(a_this->getVerticalAxesRange(),
                                                a_this->getData(*local, l_activeDataType));

                // append point
                a_this->appendPoint(l_newPoint, l_curSeries);
            }
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
