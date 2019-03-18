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


#ifndef CHARTSUBSCRIBER_H
#define CHARTSUBSCRIBER_H

#include "MxTelemetry.hpp"
#include "QObjectDataSubscriber.h"


class ChartSubscriber : public QObjectDataSubscriber
{
    Q_OBJECT
public:
    ChartSubscriber(const QString& a_subscriberName);
    virtual ~ChartSubscriber() override;


    // DataSubscriber interface - BEGIN
    virtual void update(void* a_mxTelemetryMsg) override final;
    // DataSubscriber interface - END


    void setUpdateFunction( std::function<void(ChartSubscriber* a_this,
                                               void* a_mxTelemetryMsg)> a_lambda );

signals:
    // must be compatible with qRegisterMetaType! see model wraper constructor
    void updateDone(ZmHeapTelemetry);
    void updateDone(ZmHashTelemetry);
    void updateDone(ZmThreadTelemetry);
    void updateDone(ZiMxTelemetry);
    void updateDone(ZiCxnTelemetry); // SOCKET
    void updateDone(MxTelemetry::Queue); // inside MxTelemetry
    void updateDone(MxEngine::Telemetry);
    void updateDone(MxAnyLink::Telemetry);
    void updateDone(MxTelemetry::DBEnv);
    void updateDone(MxTelemetry::DBHost);
    void updateDone(MxTelemetry::DB);


protected:
    // update function
    std::function<void(ChartSubscriber* a_this, void* a_mxTelemetryMsg)> m_lambda;
};

#endif // CHARTSUBSCRIBER_H






