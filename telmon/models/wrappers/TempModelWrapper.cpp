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

//#include "MxTelemetry.hpp" // move to factory later

#include "models/wrappers/TempModelWrapper.h"
#include "subscribers/TempTableSubscriber.h"
#include "models/wrappers/TempTable.h"
#include "QLinkedList"
#include "QDebug"
#include "factories/TableSubscriberFactory.h"
#include "factories/TableWidgetFactory.h"
#include "QTableWidget"


TempModelWrapper::TempModelWrapper(DataDistributor& a_dataDistributor) :
    m_dataDistributor(a_dataDistributor),
    //m_TempTableSubscriber(new TempTableSubscriber("HeapSubscriber")),
//    m_tempTable(new TempTable(QList<QString>({"Data"}), QList<QString>({"cacheSize", "cpuset",    "cacheAllocs",  "heapAllocs",
//                                                                        "frees",     "allocated", "maxAllocated", "size",
//                                                                        "partition", "sharded",   "alignment"}),  "Heap")),
    m_tableSubscriberDB(new QList<QMap<QString, QPair<TempTable*, TempTableSubscriber*>>*>)
{
    qDebug() << "TempModelWrapper::TempModelWrapper()";

    // init default value
    m_defaultValue.first = nullptr;
    m_defaultValue.second = nullptr;

    //must be compatible with the signal-slot type,
    //to enable usage in run time
    // see https://doc.qt.io/qt-5/custom-types.html
    qRegisterMetaType<QLinkedList<QString>>();

    //init the QMap inside QList in m_tableList
    for (unsigned int var = 0; var < m_dataDistributor.mxTypeSize(); ++var)
    {
        m_tableSubscriberDB->append(new QMap<QString, QPair<TempTable*, TempTableSubscriber*>>());
    }


    //connect the slots
    //QObject::connect(m_TempTableSubscriber, &TempTableSubscriber::updateDone, m_tempTable, &TempTable::updateData);
}


TempModelWrapper::~TempModelWrapper()
{
    qDebug() << "            ~TempModelWrapper() - Begin";

    // # # # clean m_tempTable # # #
    // iterate over QList
    for (auto i = m_tableSubscriberDB->begin(); i != m_tableSubscriberDB->end(); i++)
    {
        //iterate over QMap
        for (auto k = (*i)->begin(); k !=  (*i)->end(); k++)
        {
            delete (k).value().second; //delete TempTableSubscriber*
            (k).value().second = nullptr;

            // that is very important that before we delete we set differnt parent,
            // that make sure the the dock widget which cotains the table
            // will not also attempt to delete the table
            (k).value().first->setParent(nullptr);
            delete (k).value().first;   //delete TempTable*
            (k).value().first = nullptr;
        }
        delete *i; //delete QMap* at this position
    }

    delete m_tableSubscriberDB;
    qDebug() << "            ~TempModelWrapper() - End";
}


QTableWidget* TempModelWrapper::getTable(const QString& a_mxTelemetryTypeName, const QString& a_mxTelemetryInstanceName)
{
    //translate a_mxTelemetryTypeName to corresponding number
    int mxTelemetryTypeNameNumber = static_cast<int>(m_dataDistributor.fromMxTypeNameToValue(a_mxTelemetryTypeName));

    // get the corresponding pair
    auto l_pair = getTableSubscriberPair(mxTelemetryTypeNameNumber, a_mxTelemetryInstanceName);

    // sanity check
    if ( (l_pair.first == nullptr && l_pair.second != nullptr)
            ||
         (l_pair.first != nullptr && l_pair.second == nullptr) )
    {
        qCritical() << "getTableSubscriberPair returned invalid pair for:"
                    << a_mxTelemetryTypeName
                    << a_mxTelemetryInstanceName;
        return nullptr;
    }

    // everything already exists, just need to subscribe again
    if (l_pair.first != nullptr && l_pair.second != nullptr)
    {
        qDebug() << "getTable already exists!";

        //subscribe
        m_dataDistributor.subscribe(mxTelemetryTypeNameNumber, l_pair.second);

        return l_pair.first;
    }


    qDebug() << "getTable create table and subscriber for the first time!";
    // create table
    // to do: move to factory
    l_pair.first = TableWidgetFactory::getInstance().getTableWidget(mxTelemetryTypeNameNumber, a_mxTelemetryInstanceName);


    //create subscriber
    l_pair.second = TableSubscriberFactory::getInstance().getTableSubscriber(mxTelemetryTypeNameNumber);
    qDebug() << "setTableName:" << a_mxTelemetryInstanceName;
    l_pair.second->setTableName(a_mxTelemetryInstanceName); //todo -> move into constrctor

    //add to the table
    m_tableSubscriberDB->at(mxTelemetryTypeNameNumber)->insert(a_mxTelemetryInstanceName, l_pair);

    //connect signal and slot
    QObject::connect(l_pair.second, &TempTableSubscriber::updateDone, l_pair.first, &TempTable::updateData);

    // subscribe
    m_dataDistributor.subscribe(mxTelemetryTypeNameNumber, l_pair.second);
    //return the table
    return l_pair.first;
}


void TempModelWrapper::unsubscribe(const QString& a_mxTelemetryTypeName, const QString& a_mxTelemetryInstanceName) noexcept
{

    //translate a_mxTelemetryTypeName to corresponding number
    unsigned int mxTelemetryTypeNameNumber = m_dataDistributor.fromMxTypeNameToValue(a_mxTelemetryTypeName);

    auto l_tableSubscriberInstance = getTableSubscriberPair(mxTelemetryTypeNameNumber, a_mxTelemetryInstanceName).second;

    // to do --> transfer from string to enum
    m_dataDistributor.unsubscribe(mxTelemetryTypeNameNumber, l_tableSubscriberInstance);
}


QPair<TempTable*, TempTableSubscriber*> TempModelWrapper::getTableSubscriberPair(const int a_mxTelemetryTypeName,
                                                                                 const QString& a_mxTelemetryInstanceName) noexcept
{
    //get the map from the list
    auto l_map = m_tableSubscriberDB->value(a_mxTelemetryTypeName, nullptr);

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






