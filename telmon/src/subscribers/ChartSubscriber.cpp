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


#include "ChartSubscriber.h"

#include "src/models/raw/BasicChartModel.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


ChartSubscriber::ChartSubscriber(const int a_mxTelemetryType,
                                 const QString& a_instance,
                                 BasicChartModel* a_basicChartModel):
    OneMxTypeDataSubscriber(QString("ChartSubscriber::" +
                                    MxTelemetryGeneralWrapper::fromMxTypeValueToName(a_mxTelemetryType) +
                                    "::" + a_instance),
                            a_mxTelemetryType,
                            a_instance),
    m_model(a_basicChartModel)
{

}


ChartSubscriber::~ChartSubscriber()
{

};


void ChartSubscriber::update(void* a_mxTelemetryMsg)
{
    if (isMsgInstanceMatchesSubscriberInstance(a_mxTelemetryMsg))
    {
        m_model->update(a_mxTelemetryMsg);
    }
}







