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
//#include "ZmHeap.hpp"
#include "ChartDockWidgetModelWrapper.h"
#include "QDebug"
#include "src/distributors/DataDistributor.h"
#include "src/subscribers/ChartSubscriber.h"

#include "src/views/raw/BasicChartView.h"

#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"




ChartDockWidgetModelWrapper::ChartDockWidgetModelWrapper(DataDistributor& a_dataDistributor):
    DockWidgetModelWrapper(a_dataDistributor),
     m_subscriberDB(new QList<QMap<QString, QPair<QChartView*, ChartSubscriber*>>*>)
{
    qDebug() << "GraphDockWidgetModelWrapper";

    // init default value
    m_defaultValue.first = nullptr;
    m_defaultValue.second = nullptr;

    /* Notice :
     * Example how to subscribe unique types
     * see https://forum.qt.io/topic/93039/qregistermetatype-qmap-qstring-long-long-int/9
     */
    qRegisterMetaType<ZmHeapTelemetry>("ZmHeapTelemetry");
    qRegisterMetaType<ZmHashTelemetry>("ZmHashTelemetry");
    qRegisterMetaType<ZmThreadTelemetry>("ZmThreadTelemetry");
    qRegisterMetaType<ZiMxTelemetry >("ZiMxTelemetry");
    qRegisterMetaType<ZiCxnTelemetry >("ZiCxnTelemetry");
    qRegisterMetaType<MxTelemetry::Queue>("MxTelemetry::Queue");
    qRegisterMetaType<MxEngine::Telemetry>("MxEngine::Telemetry");      // Engine
    qRegisterMetaType<MxAnyLink::Telemetry>("MxAnyLink::Telemetry");    // Link
    qRegisterMetaType<ZdbEnv::Telemetry>("ZdbEnv::Telemetry");          // DBEnv
    qRegisterMetaType<ZdbHost::Telemetry>("ZdbHost::Telemetry");        // DBHost
    qRegisterMetaType<ZdbAny::Telemetry>("ZdbAny::Telemetry");          // DB


    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<QList<uint64_t>>("QList<uint64_t>");
    qRegisterMetaType<QPair<QList<uint64_t>, QString>>("QPair<QList<uint64_t>, QString>");

    //init the QMap inside QList in m_tableList
    for (int var = 0; var < MxTelemetryGeneralWrapper::mxTypeSize(); ++var)
    {
        m_subscriberDB->append(new QMap<QString, QPair<QChartView*, ChartSubscriber*>>());
    }
}


ChartDockWidgetModelWrapper::~ChartDockWidgetModelWrapper()
{
    qDebug() << "            ~GraphDockWidgetModelWrapper() - Begin";

    // # # # clean m_subscriberDB # # #
    // iterate over QList
    for (auto i = m_subscriberDB->begin(); i != m_subscriberDB->end(); i++)
    {
        //iterate over QMap
        for (auto k = (*i)->begin(); k !=  (*i)->end(); k++)
        {
            //delete (k).value().second; //delete ChartSubscriber*
            //(k).value().second = nullptr;


            qDebug() << "Deleteing: " << (k).value().first->chart()->title();
            delete (k).value().first;   //delete BasicChartView*
            (k).value().first = nullptr;
        }
        delete *i; //delete QMap* at this position
    }

    // delete QList;
    delete m_subscriberDB;
    qDebug() << "            ~GraphDockWidgetModelWrapper() - End";
}



void ChartDockWidgetModelWrapper::unsubscribe(const QString& a_mxTelemetryTypeName,
                                              const QString& a_mxTelemetryInstanceName) noexcept
{
    //translate a_mxTelemetryTypeName to corresponding number
    const int mxTelemetryTypeNameNumber = MxTelemetryGeneralWrapper::fromMxTypeNameToValue(a_mxTelemetryTypeName);

    auto l_tableSubscriberInstance = getSubscriberPair(mxTelemetryTypeNameNumber, a_mxTelemetryInstanceName).second;

    // to do --> transfer from string to enum
    m_dataDistributor.unsubscribe(mxTelemetryTypeNameNumber, l_tableSubscriberInstance);
}


QPair<QChartView*, ChartSubscriber*> ChartDockWidgetModelWrapper::getSubscriberPair(
                                                const int a_mxTelemetryTypeName,
                                                const QString& a_mxTelemetryInstanceName) noexcept
{
    //get the map from the list
    auto l_map = m_subscriberDB->value(a_mxTelemetryTypeName, nullptr);

    //sanity check
    if (l_map == nullptr)
    {
        qCritical() << "getTableSubscriberPair called with invalid (a_mxTelemetryTypeName, a_mxTelemetryInstanceName) ("
                    << a_mxTelemetryTypeName << a_mxTelemetryInstanceName << ") returning default value";
        return m_defaultValue;
    }

    // return the pair if exists or m_defaultValue
    return l_map->value(a_mxTelemetryInstanceName, m_defaultValue);
}


QChartView* ChartDockWidgetModelWrapper::initChartWidget(const QString& a_mxTelemetryTypeName,
                                                      const QString& a_mxTelemetryInstanceName)
{
    //translate a_mxTelemetryTypeName to corresponding number
    const int l_mxTelemetryTypeNameNumber = MxTelemetryGeneralWrapper::fromMxTypeNameToValue(a_mxTelemetryTypeName);

    // get the corresponding pair
    auto l_pair = getSubscriberPair(l_mxTelemetryTypeNameNumber, a_mxTelemetryInstanceName);

    // sanity check
    if ( (l_pair.first == nullptr && l_pair.second != nullptr)
            ||
         (l_pair.first != nullptr && l_pair.second == nullptr) )
    {
        qCritical() << "getChartView returned invalid pair for:"
                    << a_mxTelemetryTypeName
                    << a_mxTelemetryInstanceName;
        return nullptr;
    }

    // everything already exists, just need to subscribe again
    if (l_pair.first != nullptr && l_pair.second != nullptr)
    {
        qDebug() << "Chart:"
                 << a_mxTelemetryTypeName
                 << a_mxTelemetryInstanceName
                 << "already exists!";

        //subscribe
        m_dataDistributor.subscribe(l_mxTelemetryTypeNameNumber, l_pair.second);

        return l_pair.first;
    }


    qDebug() << "getChartView create chart and subscriber for the first time!";
    // create tables
    //l_pair.first = ChartViewFactory::getInstance().getChartView(l_mxTelemetryTypeNameNumber, a_mxTelemetryInstanceName);


//    create subscriber
    qDebug("EXIT");
    exit(-1);
    //l_pair.second = ChartSubscriberFactory::getInstance().getSubscriber(l_mxTelemetryTypeNameNumber);

    //l_pair.second->setAssociatedObjesctName(a_mxTelemetryInstanceName); //todo -> move into constrctor

    //add to the table
    m_subscriberDB->at(l_mxTelemetryTypeNameNumber)->insert(a_mxTelemetryInstanceName, l_pair);

    //connect signal and slot
    //connectSignalAndSlot(l_pair, l_mxTelemetryTypeNameNumber);


    // subscribes
    m_dataDistributor.subscribe(l_mxTelemetryTypeNameNumber, l_pair.second);

    //return the chart
    return l_pair.first;
}












