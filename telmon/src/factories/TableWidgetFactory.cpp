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
#include "src/factories/TableWidgetFactory.h"
#include "src/models/wrappers/BasicTableWidget.h"
#include "QDebug"

#include "src/factories/MxTelemetryTypeWrappersFactory.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"

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
                                 MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Heap).getTableList(),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::HashTbl:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::HashTbl).getTableList(),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Thread:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Thread).getTableList(),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Multiplexer:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Multiplexer).getTableList(),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Socket:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Socket).getTableList(),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Queue:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Queue).getTableList(),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Engine:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Engine).getTableList(),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::Link:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Link).getTableList(),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DBEnv:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::DBEnv).getTableList(),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DBHost:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::DBHost).getTableList(),
                                 a_mxTelemetryInstanceName);
        break;
    case MxTelemetry::Type::DB:
        l_result = new BasicTableWidget(QList<QString>({"Data"}),
                                 MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::DB).getTableList(),
                                 a_mxTelemetryInstanceName);
        break;
    default:
        qWarning() << "unknown MxTelemetry::Type:" << a_tableType<< "request, returning...";
        break;
    }
    return l_result;
}

