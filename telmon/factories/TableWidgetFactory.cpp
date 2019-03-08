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
#include "TableWidgetFactory.h"
#include "models/wrappers/BasicTableWidget.h"
#include "QDebug"

TableWidgetFactory::TableWidgetFactory()
{

}


BasicTableWidget* TableWidgetFactory::getTableWidget(const int a_tableType, const QString& a_mxTelemetryInstanceName) const noexcept
{
    BasicTableWidget* l_result = nullptr;
    switch (a_tableType)
    {
    case MxTelemetry::Type::Heap:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 QList<QString>({"time",       "size",       "alignment",   "partition",
                                                 "sharded",    "cacheSize",  "cpuset",    "cacheAllocs",
                                                 "heapAllocs", "frees", "allocated"}),
                                 a_mxTelemetryInstanceName);

        break;
    case MxTelemetry::Type::HashTbl:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 QList<QString>({"time",   "linear",  "bits",  "slots",
                                                 "cBits",  "cSlots",  "count", "resized",
                                                 "loadFactor", "effLoadFactor", "nodeSize"}),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Thread:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 QList<QString>({"time",   "id",       "tid",       "cpuUsage",
                                                 "cpuset", "priority", "stackSize", "partition",
                                                 "main",   "detached"}),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Multiplexer:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 QList<QString>({"time",   "state",       "nThreads",
                                                 "priority", "partition",  "isolation",  "rxThread",
                                                 "txThread",   "stackSize", "rxBufSize", "txBufSize"}),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Socket:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 QList<QString>({"time",      "type",      "remoteIP",  "remotePort",
                                                 "localIP",   "localPort", "fd",        "flags",
                                                 "mreqAddr",  "mreqIf",    "mif",       "ttl",
                                                 "rxBufSize", "rxBufLen",  "txBufSize", "txBufLen"}),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Queue:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 QList<QString>({"time",      "type",    "full",    "size",
                                                 "count",     "seqNo",   "inCount", "inBytes",
                                                 "outCount",  "outBytes"}),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Engine:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 QList<QString>({"time",   "state",    "nLinks",    "up",
                                                 "down",   "disabled", "transient", "reconn",
                                                 "failed", "mxID",     "rxThread",  "txThread"}),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Link:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 QList<QString>({"time",  "state",   "reconnects",    "rxSeqNo",
                                                 "txSeqNo"}),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DBEnv:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 QList<QString>({"time",        "self",            "master",           "prev",
                                                 "next",        "state",           "active",           "recovering",
                                                 "replicating", "nDBs",            "nHosts",           "nPeers",
                                                 "nCxns",       "heartbeatFreq",   "heartbeatTimeout", "reconnectFreq",
                                                 "m_dbenv",     "electionTimeout", "writeThread"}),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DBHost:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 QList<QString>({"time",  "priority",  "state",  "voted",
                                                 "ip",    "port"}),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DB:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 QList<QString>({"id",   "recSize",    "compress",    "cacheMode",
                                                 "cacheSize",   "path",    "fileSize",    "fileRecs",
                                                 "filesMax",   "preAlloc",    "minRN",    "allocRN",
                                                 "fileRN",    "cacheLoads",    "cacheMisses",    "fileLoads",
                                                 "fileMisses"}),
                                 a_mxTelemetryInstanceName);
        break;
    default:
        qWarning() << "unknown MxTelemetry::Type:" << a_tableType<< "request, returning...";
        break;
    }
    return l_result;
}

