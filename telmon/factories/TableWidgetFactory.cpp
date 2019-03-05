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
#include "models/wrappers/TempTable.h"

TableWidgetFactory::TableWidgetFactory()
{

}


TempTable* TableWidgetFactory::getTableWidget(const int a_tableType, const QString& a_mxTelemetryInstanceName) const noexcept
{
    TempTable* l_result = nullptr;
    switch (a_tableType)
    {
    case MxTelemetry::Type::Heap:
        l_result = new TempTable(QList<QString>({"Data"}),
                                 QList<QString>({"size",       "alignment",  "partition",   "sharded",
                                                 "cacheSize",  "cpuset X",   "cacheAllocs", "heapAllocs",
                                                 "frees"}),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::HashTbl:
        l_result = new TempTable(QList<QString>({"Data"}),
                                 QList<QString>({"linear",        "bits",     "1 << bits", "cBits",
                                                 "1 << cBits",    "count",    "resized",   "loadFactor",
                                                 "effLoadFactor", "nodeSize"}),
                                 a_mxTelemetryInstanceName);

        break;
    case MxTelemetry::Type::Thread:
        l_result = new TempTable(QList<QString>({"Data"}),
                                 QList<QString>({"id",       "tid",       "cpuUsage X",  "cpuset X",
                                                 "priority", "stackSize", "partition",   "main",
                                                 "detached"}),
                                 a_mxTelemetryInstanceName);
        break;
    default:
        //todo - print warning
        //l_result = nullptr;
        break;
    }
    return l_result;
}

