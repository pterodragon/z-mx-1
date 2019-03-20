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
#include "src/factories/TableSubscriberFactory.h"
#include "src/subscribers/TableSubscriber.h"
#include "QDebug"
#include "QLinkedList"
#include "QDateTime"
#include "src/factories/MxTelemetryTypeWrappersFactory.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


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
                MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Heap)
                        .getDataForTable(const_cast<MxTelemetry::Heap*>(&l_data), l_list);

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
                MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::HashTbl)
                        .getDataForTable(const_cast<MxTelemetry::HashTbl*>(&l_data), l_list);

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
                MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Thread)
                        .getDataForTable(const_cast<MxTelemetry::Thread*>(&l_data), l_list);

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
                MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Multiplexer)
                        .getDataForTable(const_cast<MxTelemetry::Multiplexer*>(&l_data), l_list);

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
            MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Socket)
                    .getDataForTable(const_cast<MxTelemetry::Socket*>(&l_data), l_list);

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
            MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Queue)
                    .getDataForTable(const_cast<MxTelemetry::Queue*>(&l_data), l_list);

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
            MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Engine)
                    .getDataForTable(const_cast<MxTelemetry::Engine*>(&l_data), l_list);

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
            MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(MxTelemetry::Type::Link)
                    .getDataForTable(const_cast<MxTelemetry::Link*>(&l_data), l_list);

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
