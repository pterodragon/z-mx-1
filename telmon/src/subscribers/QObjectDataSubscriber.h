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


#ifndef QOBJECTDATASUBSCRIBER_H
#define QOBJECTDATASUBSCRIBER_H




#include "QObject"
#include "DataSubscriber.h"
#include <functional>


/**
 * @brief The QObjectDataSubscriber class
 * added the following capabilites:
 * 1. singal
 * 2. lambda function support via update
 */
class QObjectDataSubscriber : public QObject, public DataSubscriber
{
     Q_OBJECT
public:
    QObjectDataSubscriber(const QString& a_subscriberName);
    virtual ~QObjectDataSubscriber();

    // DataSubscriber interface - BEGIN
    virtual void update(void* a_mxTelemetryMsg) = 0;
    // DataSubscriber interface - END


    virtual const QString& getName() const noexcept;
    virtual void setName(const QString& a_subscriberName) noexcept;


    // this two function are used to set the specific table the
    // subscriber is connected too
    virtual const QString& getAssociatedObjectName() const noexcept;
    virtual void setAssociatedObjesctName(QString a_associateObjectName) noexcept;


    std::string getCurrentTime() const noexcept;


    void setUpdateFunction( std::function<void(QObjectDataSubscriber* a_this,
                                               void* a_mxTelemetryMsg)> a_lambda );


    /**
     * @brief  called during the update sequence to make sure current
     *         subscriber is associated to object (table, chart etc).
     *         (object <--> assocaite; publisher <--> register)
     *         if subscriber is not associated and update is called,
     *         something went wrong in the logic, because nothing is
     *         being updated
     * @return true if associated
     */
    bool isAssociatedWithObject() const noexcept;


    /**
     * @brief the subscriber is called for every message of MxTelemetryType::X
     *        for example, there can be 3 different Heap instances
     *        So we make sure msg is associaed with correct type
     * @return true if associated
     */
    bool isTelemtryInstanceNameMatchsObjectName(const QString& a_instanceName) const noexcept;


protected:
    QString m_subscriberName;      // m_name
    QString m_associateObjectName; // m_tableName

};

#endif // QOBJECTDATASUBSCRIBER_H









