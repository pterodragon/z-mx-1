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


#include "ZmHeap.hpp"
#include "GraphDockWidgetModelWrapper.h"
#include "QDebug"
#include "distributors/DataDistributor.h"
#include "factories/ChartViewFactory.h"
#include "factories/ChartSubscriberFactory.h"
#include "subscribers/ChartSubscriber.h"

#include "views/raw/BasicChartView.h"




GraphDockWidgetModelWrapper::GraphDockWidgetModelWrapper(DataDistributor& a_dataDistributor):
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

//    struct ZmHeapTelemetry {
//      ZmIDString	id;
//      uint64_t	cacheSize;
//      uint64_t	cpuset;
//      uint64_t	cacheAllocs;
//      uint64_t	heapAllocs;
//      uint64_t	frees;
//      uint32_t	size;
//      uint16_t	partition;
//      uint8_t	sharded;
//      uint8_t	alignment;
//    };
    qRegisterMetaType<ZmHeapTelemetry>("ZmHeapTelemetry");


    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<QList<uint64_t>>("QList<uint64_t>");
    qRegisterMetaType<QPair<QList<uint64_t>, QString>>("QPair<QList<uint64_t>, QString>");

    //init the QMap inside QList in m_tableList
    for (int var = 0; var < m_dataDistributor.mxTypeSize(); ++var)
    {
        m_subscriberDB->append(new QMap<QString, QPair<QChartView*, ChartSubscriber*>>());
    }
}


GraphDockWidgetModelWrapper::~GraphDockWidgetModelWrapper()
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



void GraphDockWidgetModelWrapper::unsubscribe(const QString& a_mxTelemetryTypeName,
                                              const QString& a_mxTelemetryInstanceName) noexcept
{
    //translate a_mxTelemetryTypeName to corresponding number
    const int mxTelemetryTypeNameNumber = m_dataDistributor.fromMxTypeNameToValue(a_mxTelemetryTypeName);

    auto l_tableSubscriberInstance = getSubscriberPair(mxTelemetryTypeNameNumber, a_mxTelemetryInstanceName).second;

    // to do --> transfer from string to enum
    m_dataDistributor.unsubscribe(mxTelemetryTypeNameNumber, l_tableSubscriberInstance);
}


QPair<QChartView*, ChartSubscriber*> GraphDockWidgetModelWrapper::getSubscriberPair(
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


QChartView* GraphDockWidgetModelWrapper::initChartWidget(const QString& a_mxTelemetryTypeName,
                                                      const QString& a_mxTelemetryInstanceName)
{
    //translate a_mxTelemetryTypeName to corresponding number
    const int mxTelemetryTypeNameNumber = static_cast<int>(m_dataDistributor.fromMxTypeNameToValue(a_mxTelemetryTypeName));

    // get the corresponding pair
    auto l_pair = getSubscriberPair(mxTelemetryTypeNameNumber, a_mxTelemetryInstanceName);

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
        qDebug() << "getChartView already exists!";

        //subscribe
        m_dataDistributor.subscribe(mxTelemetryTypeNameNumber, l_pair.second);

        return l_pair.first;
    }


    qDebug() << "getChartView create chart and subscriber for the first time!";
    // create tables
    l_pair.first = ChartViewFactory::getInstance().getChartView(mxTelemetryTypeNameNumber, a_mxTelemetryInstanceName);


//    create subscriber
    l_pair.second = ChartSubscriberFactory::getInstance().getSubscriber(mxTelemetryTypeNameNumber);
    qDebug() << "setTableName:" << a_mxTelemetryInstanceName;
    l_pair.second->setAssociatedObjesctName(a_mxTelemetryInstanceName); //todo -> move into constrctor

    //add to the table
    m_subscriberDB->at(mxTelemetryTypeNameNumber)->insert(a_mxTelemetryInstanceName, l_pair);

    //connect signal and slot
    QObject::connect(l_pair.second, &ChartSubscriber::updateDone,
                     static_cast<BasicChartView*>(l_pair.first), &BasicChartView::updateData);

    // subscribes
    m_dataDistributor.subscribe(mxTelemetryTypeNameNumber, l_pair.second);

    //return the chart
    return l_pair.first;
}
















