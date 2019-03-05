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


#ifndef TEMPMODELWRAPPER_H
#define TEMPMODELWRAPPER_H



// ugly for testing - BEGIN


#include "subscribers/DataSubscriber.h"
#include "QPair"


class DataDistributor;
class TempTable;
class QTableWidget;
class TempTableSubscriber;

template <class T>
class QList;

template <class T, class V>
class QMap;



class TempModelWrapper// : public QObject, public DataSubscriber
{
    //Q_OBJECT

public:
    TempModelWrapper(DataDistributor& a_dataDistributor);
    virtual ~TempModelWrapper();
    QTableWidget* getTable(const QString& a_mxTelemetryTypeName, const QString& a_mxTelemetryInstanceName);

    void unsubscribe(const QString& a_mxTelemetryTypeName, const QString& a_mxTelemetryInstanceName) noexcept;


private:
    DataDistributor& m_dataDistributor;

    QPair<TempTable*, TempTableSubscriber*> getTableSubscriberPair(const int a_mxTelemetryTypeName, const QString& a_mxTelemetryInstanceName) noexcept;

    // Notes:
    // QList of all the MxType available by index, that corresponds to MxTelemetry::Type
    // QMap holds table and corresponding subscriber per instance, for example
    // QList.at[0=Heap].value(MxTelemetry.Msg) will return the associate table and subscriber for MxTelemetry.Msg
    QList<QMap<QString, QPair<TempTable*, TempTableSubscriber*>>*>* m_tableSubscriberDB;
    QPair<TempTable*, TempTableSubscriber*> m_defaultValue;
};





#endif // TEMPMODELWRAPPER_H
