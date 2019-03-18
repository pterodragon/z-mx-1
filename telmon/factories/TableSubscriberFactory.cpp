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
#include "subscribers/TableSubscriber.h"
#include "QDebug"
#include "QLinkedList"
#include "QDateTime"


TableSubscriberFactory::TableSubscriberFactory()
{

}


TableSubscriber* TableSubscriberFactory::getTableSubscriber(const int a_type) const noexcept
{
    TableSubscriber* l_result = nullptr;

    // todo, can be improved to array of constans
    const QString l_name = MxTelemetry::Type::name(a_type); // get the name, if a_type is out of range, return unknown

    switch (a_type)
    {
        case MxTelemetry::Type::Heap:
            l_result = new TableSubscriber(l_name + "Subscriber");

            /**
             * more:
             * 1. https://stackoverflow.com/questions/13605141/best-strategy-to-update-qtableview-with-data-in-real-time-from-different-threads
             * 2. https://stackoverflow.com/questions/4031168/qtableview-is-extremely-slow-even-for-only-3000-rows
             *
             *   TODO - convert to pointer
             *   QLinkedList
             *   pros constant time insertions and removals.
             *        iteration is the same as QList
            */
            l_result->setUpdateFunction( [] ( TableSubscriber* a_this,
                                              void* a_mxTelemetryMsg) -> void
            {

                if (!a_this->isAssociatedWithObject()) {return;}
                const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Heap>();

                {
                    auto l_instanceName= QString(ZmIDString((l_data.id.data())));
                    if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
                }

                QLinkedList<QString> l_list;

                l_list.append(QString::fromStdString(a_this->getCurrentTime()));
                l_list.append(QString::number(l_data.size));
                l_list.append(QString::number(ZuBoxed(l_data.alignment)));
                l_list.append(QString::number(ZuBoxed(l_data.partition)));
                l_list.append(QString::number(ZuBoxed(l_data.sharded)));

                l_list.append(QString::number(l_data.cacheSize));
                l_list.append(a_this->ZmBitmapToQString(ZmBitmap(l_data.cpuset)));
                l_list.append(QString::number(l_data.cacheAllocs));
                l_list.append(QString::number(l_data.heapAllocs));
                l_list.append(QString::number(l_data.frees));

                l_list.append(QString::number((l_data.cacheAllocs + l_data.heapAllocs - l_data.frees)));


                emit a_this->updateDone(l_list);
            });

            break;
        case MxTelemetry::Type::HashTbl:
            l_result = new TableSubscriber(l_name + "Subscriber");

            l_result->setUpdateFunction( [] ( TableSubscriber* a_this,
                                              void* a_mxTelemetryMsg) -> void
            {
                if (!a_this->isAssociatedWithObject()) {return;}
                const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::HashTbl>();

                {
                    auto l_instanceName= QString(ZmIDString((l_data.id.data())));
                    if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
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

            l_result = new TableSubscriber(l_name + "Subscriber");

            l_result->setUpdateFunction( [] ( TableSubscriber* a_this,
                                         void* a_mxTelemetryMsg) -> void
            {
                if (!a_this->isAssociatedWithObject()) {return;}
                const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Thread>();

                {
                    auto l_instanceName= QString(ZmIDString((l_data.name.data())));
                    if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
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
            l_result = new TableSubscriber(l_name + "Subscriber");
            l_result->setUpdateFunction( [] ( TableSubscriber* a_this,
                                         void* a_mxTelemetryMsg) -> void
            {
                if (!a_this->isAssociatedWithObject()) {return;}
                const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Multiplexer>();

                {
                    auto l_instanceName= QString(ZmIDString((l_data.id)));
                    if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
                }

                QLinkedList<QString> l_list;

                l_list.append(QString::fromStdString(a_this->getCurrentTime()));
                l_list.append(ZmScheduler::stateName(l_data.state));
                l_list.append(QString::number(ZuBoxed(l_data.nThreads)));
                l_list.append(QString::number(ZuBoxed(l_data.priority)));
                l_list.append(QString::number(ZuBoxed(l_data.partition)));

                l_list.append(a_this->ZmBitmapToQString(l_data.isolation));
                l_list.append(QString::number(l_data.rxThread));
                l_list.append(QString::number(l_data.txThread));
                l_list.append(QString::number(l_data.stackSize));
                l_list.append(QString::number(l_data.rxBufSize));

                l_list.append(QString::number(l_data.txBufSize));

                emit a_this->updateDone(l_list);
            });
            break;

    case MxTelemetry::Type::Socket:
        l_result = new TableSubscriber(l_name + "Subscriber");
        l_result->setUpdateFunction( [] ( TableSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Socket>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.mxID)));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }


            QLinkedList<QString> l_list;

            l_list.append(QString::fromStdString(a_this->getCurrentTime()));
            l_list.append(ZiCxnType::name(l_data.type));
            l_list.append(a_this->ZiIPTypeToQString(l_data.remoteIP));
            l_list.append(QString::number(ZuBoxed(l_data.remotePort)));
            l_list.append(a_this->ZiIPTypeToQString(l_data.localIP));

            l_list.append(QString::number(ZuBoxed(l_data.localPort)));
            l_list.append(QString::number(l_data.socket));
            l_list.append(a_this->ZiCxnFlagsTypeToQString(l_data.flags));
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
    case MxTelemetry::Type::Queue:
        l_result = new TableSubscriber(l_name + "Subscriber");
        l_result->setUpdateFunction( [] ( TableSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Queue>();

            {
                auto l_instanceName= QString::fromStdString(a_this->constructQueueName(l_data.id, MxTelemetry::QueueType::name(l_data.type)));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }


            QLinkedList<QString> l_list;

            l_list.append(QString::fromStdString(a_this->getCurrentTime()));
            l_list.append(MxTelemetry::QueueType::name(l_data.type));
            qDebug() << QString::fromStdString(a_this->constructQueueName(l_data.id, MxTelemetry::QueueType::name(l_data.type)))
                     << "l_data.full"
                     << l_data.full << "\n";
            l_list.append(QString::number(l_data.full));
            l_list.append(QString::number(l_data.size));
            l_list.append(QString::number(l_data.count));

            l_list.append(QString::number(l_data.seqNo));
            l_list.append(QString::number(l_data.inCount));
            l_list.append(QString::number(l_data.inBytes));
            l_list.append(QString::number(l_data.outCount));
            l_list.append(QString::number(l_data.outBytes));

            emit a_this->updateDone(l_list);
        });
        break;
    case MxTelemetry::Type::Engine:
        l_result = new TableSubscriber(l_name + "Subscriber");
        l_result->setUpdateFunction( [] ( TableSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Engine>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.id)));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }


            QLinkedList<QString> l_list;

            l_list.append(QString::fromStdString(a_this->getCurrentTime()));
            l_list.append(MxEngineState::name(l_data.state));
            l_list.append(QString::number(l_data.nLinks));
            l_list.append(QString::number(l_data.up));
            l_list.append(QString::number(l_data.down));

            l_list.append(QString::number(l_data.disabled));
            l_list.append(QString::number(l_data.transient));
            l_list.append(QString::number(l_data.reconn));
            l_list.append(QString::number(l_data.failed));
            l_list.append(l_data.mxID.data());

            l_list.append(QString::number(l_data.rxThread));
            l_list.append(QString::number(l_data.txThread));

            emit a_this->updateDone(l_list);
        });
        break;
    case MxTelemetry::Type::Link:
        l_result = new TableSubscriber(l_name + "Subscriber");
        l_result->setUpdateFunction( [] ( TableSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Link>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.id)));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }


            QLinkedList<QString> l_list;

            l_list.append(QString::fromStdString(a_this->getCurrentTime()));
            l_list.append(MxLinkState::name(l_data.state));
            l_list.append(QString::number(l_data.reconnects));
            l_list.append(QString::number(l_data.rxSeqNo));
            l_list.append(QString::number(l_data.txSeqNo));

            emit a_this->updateDone(l_list);
        });
        break;
    case MxTelemetry::Type::DBEnv:
        l_result = new TableSubscriber(l_name + "Subscriber");
        l_result->setUpdateFunction( [] ( TableSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::DBEnv>();

            {
                auto l_instanceName= QString((l_data.self));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

            QLinkedList<QString> l_list;

            l_list.append(QString::fromStdString(a_this->getCurrentTime()));
            l_list.append(QString::number(l_data.master));
            l_list.append(QString::number(l_data.prev));
            l_list.append(QString::number(l_data.next));
            //l_list.append(ZdbHost::stateName(l_data.state));

            l_list.append(QString::number(ZuBoxed(l_data.active)));
            l_list.append(QString::number(ZuBoxed(l_data.recovering)));
            l_list.append(QString::number(ZuBoxed(l_data.replicating)));
            l_list.append(QString::number(ZuBoxed(l_data.nDBs)));
            l_list.append(QString::number(ZuBoxed(l_data.nHosts)));

            l_list.append(QString::number(ZuBoxed(l_data.nPeers)));
            l_list.append(QString::number(ZuBoxed(l_data.nCxns)));
            l_list.append(QString::number(l_data.heartbeatFreq));
            l_list.append(QString::number(l_data.heartbeatTimeout));
            l_list.append(QString::number(l_data.reconnectFreq));

            l_list.append(QString::number(l_data.electionTimeout));
            l_list.append(QString::number(l_data.writeThread));

            emit a_this->updateDone(l_list);
        });
        break;
    case MxTelemetry::Type::DBHost:
        l_result = new TableSubscriber(l_name + "Subscriber");
        l_result->setUpdateFunction( [] ( TableSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::DBHost>();

            {
                auto l_instanceName= QString((l_data.id));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }


            QLinkedList<QString> l_list;

            //ZdbHost::stateName(l_data.state);

            l_list.append(QString::fromStdString(a_this->getCurrentTime()));
            l_list.append(QString::number(l_data.priority));
            //l_list.append(QString(ZdbHost::stateName(l_data.state)));
            l_list.append(QString::number(ZuBoxed(l_data.voted)));
            l_list.append(a_this->ZiIPTypeToQString(l_data.ip));

            l_list.append(QString::number(l_data.port));

            emit a_this->updateDone(l_list);
        });
        break;
    case MxTelemetry::Type::DB:
        l_result = new TableSubscriber(l_name + "Subscriber");
        l_result->setUpdateFunction( [] ( TableSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::DB>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.name)));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }


            QLinkedList<QString> l_list;

            l_list.append(QString::fromStdString(a_this->getCurrentTime()));
            l_list.append(QString::number(l_data.id));
            l_list.append(QString::number(l_data.recSize));
            l_list.append(QString::number(l_data.compress));
            l_list.append(ZdbCacheMode::name(l_data.cacheMode));

            l_list.append(QString::number(l_data.cacheSize));
            l_list.append(QString(l_data.path));
            l_list.append(QString::number(l_data.fileSize));
            l_list.append(QString::number(l_data.fileRecs));
            l_list.append(QString::number(l_data.filesMax));

            l_list.append(QString::number(l_data.preAlloc));
            l_list.append(QString::number(l_data.minRN));
            l_list.append(QString::number(l_data.allocRN));
            l_list.append(QString::number(l_data.fileRN));
            l_list.append(QString::number(l_data.cacheLoads));

            l_list.append(QString::number(l_data.cacheMisses));
            l_list.append(QString::number(l_data.fileLoads));
            l_list.append(QString::number(l_data.fileMisses));

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
