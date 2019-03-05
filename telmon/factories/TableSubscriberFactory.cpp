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
    switch (a_type)
    {
        case MxTelemetry::Type::Heap:
            l_result = new TempTableSubscriber("HeapSubscriber");

            l_result->setUpdateFunction([](TempTableSubscriber* a_this, void* a_mxTelemetryMsg) -> void {
                if (a_this->m_tableName.isNull() || a_this->m_tableName.isEmpty())
                {
                    qWarning() << a_this->m_name << "is not registered to any table, please setTableName or unsubscribe, returning..."
                               << "table name is:" << a_this->getTableName();
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
                QLinkedList<QString> a_list;

                a_list.append(QString::number(l_data.size));
                a_list.append(QString::number(ZuBoxed(l_data.alignment)));
                a_list.append(QString::number(ZuBoxed(l_data.partition)));
                a_list.append(QString::number(ZuBoxed(l_data.sharded)));
                a_list.append(QString::number(l_data.cacheSize));

                // how to print the cputest? ZmBitmap(l_data.cpuset)
                a_list.append(QString::number(l_data.cpuset));
                a_list.append(QString::number(l_data.cacheAllocs));
                a_list.append(QString::number(l_data.heapAllocs));
                a_list.append(QString::number(l_data.frees));


                emit a_this->updateDone(a_list);
            });

            break;
        case MxTelemetry::Type::HashTbl:
            l_result = new TempTableSubscriber("HashTblSubscriber");

            l_result->setUpdateFunction([](TempTableSubscriber* a_this, void* a_mxTelemetryMsg) -> void {
                if (a_this->m_tableName.isNull() || a_this->m_tableName.isEmpty())
                {
                    qWarning() << a_this->m_name << "is not registered to any table, please setTableName or unsubscribe, returning..."
                               << "table name is:" << a_this->getTableName();
                    return;
                }

                const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::HashTbl>();

                // make sure msg is of current instance
                if (a_this->getTableName() != QString(l_data.id.data()))
                {
                    return;
                }

                QLinkedList<QString> a_list;

                a_list.append(QString::number(ZuBoxed(l_data.linear)));
                a_list.append(QString::number(ZuBoxed(l_data.bits)));
                a_list.append(QString::number(ZuBoxed((static_cast<uint64_t>(1))<<l_data.bits)));
                a_list.append(QString::number(ZuBoxed(l_data.cBits)));
                a_list.append(QString::number(ZuBoxed((static_cast<uint64_t>(1))<<l_data.cBits)));

                a_list.append(QString::number(l_data.count));
                a_list.append(QString::number(l_data.resized));
                a_list.append(QString::number(static_cast<double>(l_data.loadFactor   / 16.0)));
                a_list.append(QString::number(static_cast<double>(l_data.effLoadFactor / 16.0)));
                a_list.append(QString::number(l_data.nodeSize));

                emit a_this->updateDone(a_list);
            });

            break;
        case MxTelemetry::Type::Thread:

        l_result = new TempTableSubscriber("ThreadSubscriber");

        l_result->setUpdateFunction([](TempTableSubscriber* a_this, void* a_mxTelemetryMsg) -> void {
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

            QLinkedList<QString> a_list;

            a_list.append(QString::number(l_data.id));
            a_list.append(QString::number(l_data.tid));
            // how to print cpu usage? ZuBoxed(data.cpuUsage * 100.0).fmt(ZuFmt::FP<2>())
            a_list.append(QString::number(ZuBoxed(l_data.cpuUsage)));
            // how to print ZmBitmap(data.cpuset) ??
            a_list.append(QString::number(l_data.cpuset));

            a_list.append(QString::number(ZuBoxed(l_data.priority)));
            a_list.append(QString::number(l_data.stackSize));
            a_list.append(QString::number(ZuBoxed(l_data.partition)));
            a_list.append(QString::number(ZuBoxed(l_data.main)));
            a_list.append(QString::number(ZuBoxed(l_data.detached)));

            emit a_this->updateDone(a_list);
        });
            break;
        default:
            //todo - print warning
            l_result = nullptr;
            break;
    }
    return l_result;



}
