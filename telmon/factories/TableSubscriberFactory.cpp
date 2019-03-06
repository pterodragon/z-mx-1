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
#include "TableSubscriberFactory.h"
#include "subscribers/TempTableSubscriber.h"
#include "QDebug"
#include "QLinkedList"
#include "QDateTime"

#include <arpa/inet.h>

TableSubscriberFactory::TableSubscriberFactory()
{

}



//todo - add support for all types
//
//    MxEnumValues(Heap, HashTbl, Thread, Multiplexer, Socket, Queue,
//	Engine, Link,
//	DBEnv, DBHost, DB);
TempTableSubscriber* TableSubscriberFactory::getTableSubscriber(const int a_type) const noexcept
{
    TempTableSubscriber* l_result = nullptr;

    // todo, can be improved to array of constans
    const QString l_name = MxTelemetry::Type::name(a_type); // get the name, if a_type is out of range, return unknown

    switch (a_type)
    {
        case MxTelemetry::Type::Heap:
            l_result = new TempTableSubscriber(l_name + "Subscriber");

            l_result->setUpdateFunction( [] ( TempTableSubscriber* a_this,
                                              void* a_mxTelemetryMsg) -> void {
                if (a_this->m_tableName.isNull() || a_this->m_tableName.isEmpty())
                {
                    qWarning() << a_this->m_name
                               << "is not registered to any table, please setTableName or unsubscribe, returning..."
                               << "table name is:"
                               << a_this->getTableName();
                    return;
                }

                /**
                // more about my approach
                // 1. https://stackoverflow.com/questions/13605141/best-strategy-to-update-qtableview-with-data-in-real-time-from-different-threads
                // 2. https://stackoverflow.com/questions/4031168/qtableview-is-extremely-slow-even-for-only-3000-rows
                */

                const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Heap>();


                // make sure msg is of current instance
                if (a_this->getTableName() != QString((l_data.id.data())))
                {
                    return;
                }

                //TODO - convert to pointer
                // QLinkedList
                // constant time insertions and removals.
                // iteration is the same as QList

                QLinkedList<QString> l_list;

                l_list.append(QString::fromStdString(a_this->getCurrentTime()));
                l_list.append(QString::number(l_data.size));
                l_list.append(QString::number(ZuBoxed(l_data.alignment)));
                l_list.append(QString::number(ZuBoxed(l_data.partition)));
                l_list.append(QString::number(ZuBoxed(l_data.sharded)));

                // how to print the cputest? ZmBitmap(l_data.cpuset)
                l_list.append(QString::number(l_data.cacheSize));
                l_list.append(QString::number(l_data.cpuset));
                l_list.append(QString::number(l_data.cacheAllocs));
                l_list.append(QString::number(l_data.heapAllocs));
                l_list.append(QString::number(l_data.frees));

                l_list.append(QString::number((l_data.cacheAllocs + l_data.heapAllocs - l_data.frees)));


                emit a_this->updateDone(l_list);
            });

            break;
        case MxTelemetry::Type::HashTbl:
            l_result = new TempTableSubscriber(l_name + "Subscriber");

            l_result->setUpdateFunction( [] ( TempTableSubscriber* a_this,
                                              void* a_mxTelemetryMsg) -> void {
                if (a_this->m_tableName.isNull() || a_this->m_tableName.isEmpty())
                {
                    qWarning() << a_this->m_name
                               << "is not registered to any table, please setTableName or unsubscribe, returning..."
                               << "table name is:"
                               << a_this->getTableName();
                    return;
                }

                const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::HashTbl>();

                // make sure msg is of current instance
                if (a_this->getTableName() != QString(l_data.id.data()))
                {
                    return;
                }


                QLinkedList<QString> l_list;

                l_list.append(QString::fromStdString(a_this->getCurrentTime()));
                l_list.append(QString::number(ZuBoxed(l_data.linear)));
                l_list.append(QString::number(ZuBoxed(l_data.bits)));
                l_list.append(QString::number(ZuBoxed((static_cast<uint64_t>(1))<<l_data.bits)));
                l_list.append(QString::number(ZuBoxed(l_data.cBits)));

                l_list.append(QString::number(ZuBoxed((static_cast<uint64_t>(1))<<l_data.cBits)));
                l_list.append(QString::number(l_data.count));
                l_list.append(QString::number(l_data.resized));
                l_list.append(QString::number(static_cast<double>(l_data.loadFactor   / 16.0)));
                l_list.append(QString::number(static_cast<double>(l_data.effLoadFactor / 16.0)));

                l_list.append(QString::number(l_data.nodeSize));

                emit a_this->updateDone(l_list);
            });

            break;
        case MxTelemetry::Type::Thread:

            l_result = new TempTableSubscriber(l_name + "Subscriber");

            l_result->setUpdateFunction( [] ( TempTableSubscriber* a_this,
                                         void* a_mxTelemetryMsg) -> void {
                if (a_this->m_tableName.isNull() || a_this->m_tableName.isEmpty())
                {
                    qWarning() << a_this->m_name << "is not registered to any table, please setTableName or unsubscribe, returning..."
                               << "table name is:" << a_this->getTableName();
                    return;
                }

                const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Thread>();

                // make sure msg is of current instance
                if (a_this->getTableName() != QString(l_data.name.data()))
                {
                    return;
                }

                QLinkedList<QString> l_list;

                l_list.append(QString::fromStdString(a_this->getCurrentTime()));
                l_list.append(QString::number(l_data.id));
                l_list.append(QString::number(l_data.tid));
                l_list.append(QString::number(ZuBoxed(l_data.cpuUsage * 100.0), 'f', 2)); // take 2 digits precise
                l_list.append(QString::number(l_data.cpuset));

                l_list.append(QString::number(ZuBoxed(l_data.priority)));
                l_list.append(QString::number(l_data.stackSize));
                l_list.append(QString::number(ZuBoxed(l_data.partition)));
                l_list.append(QString::number(ZuBoxed(l_data.main)));
                l_list.append(QString::number(ZuBoxed(l_data.detached)));

                emit a_this->updateDone(l_list);
            });
            break;
        case MxTelemetry::Type::Multiplexer:
            l_result = new TempTableSubscriber(l_name + "Subscriber");
            l_result->setUpdateFunction( [] ( TempTableSubscriber* a_this,
                                         void* a_mxTelemetryMsg) -> void {

                if (a_this->m_tableName.isNull() || a_this->m_tableName.isEmpty())
                {
                    qWarning() << a_this->m_name
                               << "is not registered to any table, please setTableName or unsubscribe, returning..."
                               << "table name is:"
                               << a_this->getTableName();
                    return;
                }

                const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Multiplexer>();

                // make sure msg is of current instance
                if (a_this->getTableName() != QString::number(l_data.id))
                {
                    return;
                }

                QLinkedList<QString> l_list;

                l_list.append(QString::fromStdString(a_this->getCurrentTime()));
                l_list.append(ZmScheduler::stateName(l_data.state));
                l_list.append(QString::number(ZuBoxed(l_data.nThreads)));
                l_list.append(QString::number(ZuBoxed(l_data.priority)));
                l_list.append(QString::number(ZuBoxed(l_data.partition)));

                //l_list.append(QString::number(ZmBitmap(l_data.isolation)); TODO
                l_list.append(QString::number(l_data.isolation));
                l_list.append(QString::number(l_data.rxThread));
                l_list.append(QString::number(l_data.txThread));
                l_list.append(QString::number(l_data.stackSize));
                l_list.append(QString::number(l_data.rxBufSize));

                l_list.append(QString::number(l_data.txBufSize));

                emit a_this->updateDone(l_list);
            });
            break;
    case MxTelemetry::Type::Socket:
        l_result = new TempTableSubscriber(l_name + "Subscriber");
        l_result->setUpdateFunction( [] ( TempTableSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void {

            if (a_this->m_tableName.isNull() || a_this->m_tableName.isEmpty())
            {
                qWarning() << a_this->m_name
                           << "is not registered to any table, please setTableName or unsubscribe, returning..."
                           << "table name is:"
                           << a_this->getTableName();
                return;
            }

            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Socket>();

            // make sure msg is of current instance
            if (a_this->getTableName() != QString::number(l_data.mxID))
            {
                return;
            }

            QLinkedList<QString> l_list;

            l_list.append(QString::fromStdString(a_this->getCurrentTime()));
            l_list.append(ZiCxnType::name(l_data.type));
            l_list.append(a_this->ZiIPTypeToQString(l_data.remoteIP));
            l_list.append(QString::number(ZuBoxed(l_data.remotePort)));
            l_list.append(a_this->ZiIPTypeToQString(l_data.localIP));

            l_list.append(QString::number(ZuBoxed(l_data.localPort)));
            l_list.append(QString::number(l_data.socket));
            l_list.append(QString(ZiCxnFlags::Flags::print(l_data.flags).v));
            l_list.append(a_this->ZiIPTypeToQString(l_data.mreqAddr));
            l_list.append(a_this->ZiIPTypeToQString(l_data.mreqIf));

            l_list.append(a_this->ZiIPTypeToQString(l_data.mif));
            l_list.append(QString::number(l_data.ttl));
            l_list.append(QString::number(l_data.rxBufSize));
            l_list.append(QString::number(l_data.rxBufLen));
            l_list.append(QString::number(l_data.txBufSize));

            l_list.append(QString::number(l_data.txBufLen));
            emit a_this->updateDone(l_list);
        });
        break;
        default:
            //todo - print warning
            l_result = nullptr;
            break;
    }
    return l_result;



}
