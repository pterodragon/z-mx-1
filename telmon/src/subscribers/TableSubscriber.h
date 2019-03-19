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


#ifndef TABLESUBSCRIBER_H
#define TABLESUBSCRIBER_H

#include "QObjectDataSubscriber.h"

class ZmBitmap;
class ZiIP;

class TableSubscriber : public QObjectDataSubscriber
{
    Q_OBJECT

public:
    TableSubscriber(const QString& a_subscriberName);
    virtual ~TableSubscriber() override;


    // DataSubscriber interface - BEGIN
    virtual void update(void* a_mxTelemetryMsg) override final;
    // DataSubscriber interface - END


    void setUpdateFunction( std::function<void(TableSubscriber* a_this,
                                               void* a_mxTelemetryMsg)> a_lambda );



    QString ZmBitmapToQString(const ZmBitmap& a_zmBitMap) const noexcept;


    QString ZiIPTypeToQString(const ZiIP& a_ZiIP) const noexcept;


    QString ZiCxnFlagsTypeToQString(const uint32_t a_ZiCxnFlags) const noexcept;



signals:
    // must be compatible with qRegisterMetaType!
    void updateDone(QLinkedList<QString>);

protected:
        std::stringstream *m_stream; // assistance in translation to some types

        // update function
        std::function<void(TableSubscriber* a_this, void* a_mxTelemetryMsg)> m_lambda;

};

#endif // TABLESUBSCRIBER_H
