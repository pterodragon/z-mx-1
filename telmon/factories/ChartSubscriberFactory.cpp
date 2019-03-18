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
#include "ChartSubscriberFactory.h"
#include "subscribers/ChartSubscriber.h"
#include "QList"
#include "QDebug"

ChartSubscriberFactory::ChartSubscriberFactory()
{

}


ChartSubscriber* ChartSubscriberFactory::getSubscriber(const int a_type) const noexcept
{
    ChartSubscriber* l_result = nullptr;


    const QString l_name = MxTelemetry::Type::name(a_type); // get the name, if a_type is out of range, return unknown
    l_result = new ChartSubscriber(l_name + "Subscriber");

    switch (a_type)
    {
    case MxTelemetry::Type::Heap:

        l_result->setUpdateFunction( [] ( ChartSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {

            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Heap>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.id.data())));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

            ZmHeapTelemetry l_result = std::move(l_data);

            emit a_this->updateDone(l_result);

        });
        break;

    case MxTelemetry::Type::HashTbl:

        l_result->setUpdateFunction( [] ( ChartSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::HashTbl>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.id.data())));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

            ZmHashTelemetry l_result = std::move(l_data);

            emit a_this->updateDone(l_result);
        });
        break;

    case MxTelemetry::Type::Thread:

        l_result->setUpdateFunction( [] ( ChartSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {

            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Thread>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.name.data())));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

            ZmThreadTelemetry l_result = std::move(l_data);

            emit a_this->updateDone(l_result);

        });
        break;

    case MxTelemetry::Type::Multiplexer:

        l_result->setUpdateFunction( [] ( ChartSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Multiplexer>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.id)));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

            ZiMxTelemetry l_result = std::move(l_data);

            emit a_this->updateDone(l_result);
        });

        break;

    case MxTelemetry::Type::Socket:

        l_result->setUpdateFunction( [] ( ChartSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Socket>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.mxID)));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

            ZiCxnTelemetry l_result = std::move(l_data);

            emit a_this->updateDone(l_result);

        });
        break;

    case MxTelemetry::Type::Queue:

        l_result->setUpdateFunction( [] ( ChartSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Queue>();

            {
                auto l_instanceName= QString::fromStdString(a_this->constructQueueName(l_data.id, MxTelemetry::QueueType::name(l_data.type)));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

            MxTelemetry::Queue l_result = std::move(l_data);

            emit a_this->updateDone(l_result);
        });
        break;

    case MxTelemetry::Type::Engine:

        l_result->setUpdateFunction( [] ( ChartSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Engine>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.id)));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

            MxEngine::Telemetry l_result = std::move(l_data);

            emit a_this->updateDone(l_result);

        });
        break;

    case MxTelemetry::Type::Link:

        l_result->setUpdateFunction( [] ( ChartSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Link>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.id)));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

           MxAnyLink::Telemetry l_result = std::move(l_data);

            emit a_this->updateDone(l_result);

        });
        break;

    case MxTelemetry::Type::DBEnv:

        l_result->setUpdateFunction( [] ( ChartSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::DBEnv>();

            {
                auto l_instanceName= QString((l_data.self));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

            ZdbEnv::Telemetry l_result = std::move(l_data);

             emit a_this->updateDone(l_result);
        });
        break;

    case MxTelemetry::Type::DBHost:

        l_result->setUpdateFunction( [] ( ChartSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::DBHost>();

            {
                auto l_instanceName= QString((l_data.id));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

            ZdbHost::Telemetry l_result = std::move(l_data);

             emit a_this->updateDone(l_result);

        });
        break;

    case MxTelemetry::Type::DB:

        l_result->setUpdateFunction( [] ( ChartSubscriber* a_this,
                                     void* a_mxTelemetryMsg) -> void
        {
            if (!a_this->isAssociatedWithObject()) {return;}
            const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::DB>();

            {
                auto l_instanceName= QString(ZmIDString((l_data.name)));
                if (!a_this->isTelemtryInstanceNameMatchsObjectName(l_instanceName)) {return;}
            }

            ZdbAny::Telemetry l_result = std::move(l_data);

             emit a_this->updateDone(l_result);

        });
        break;

    default:
        qWarning() << "ChartSubscriberFactory::getSubscriber() unkown type:" << a_type;
        delete l_result;
        l_result = nullptr;
        break;
    }
    return l_result;
}


