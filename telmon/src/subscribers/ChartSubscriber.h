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


#include "OneMxTypeDataSubscriber.h"


class BasicChartModel;


class ChartSubscriber : public OneMxTypeDataSubscriber
{

public:
    ChartSubscriber(const int a_mxTelemetryType,
                    const QString& a_instance,
                    BasicChartModel* a_basicChartModel);
    virtual ~ChartSubscriber() override;
    virtual void update(void* a_mxTelemetryMsg) override final;

protected:
    BasicChartModel* m_model; // associated chart model


};

#endif // CHARTSUBSCRIBER_H






