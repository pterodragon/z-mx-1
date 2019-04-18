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


#ifndef TABLEQLABELWIDGETDOCKWIDGET_H
#define TABLEQLABELWIDGETDOCKWIDGET_H

class DataDistributor;
class TabelQLabelSubscriber;

#include "src/widgets/BasicDockWidget.h"

class TableQLabelWidgetDockWidget : public BasicDockWidget
{
    Q_OBJECT
public:
    TableQLabelWidgetDockWidget(const int       a_mxTelemetryTypeName,
                                const QString&  a_mxTelemetryInstanceName,
                                DataDistributor& a_dataDistributor,
                                QWidget*        a_parent,
                                Qt::WindowFlags a_flags = Qt::WindowFlags());
    virtual ~TableQLabelWidgetDockWidget() override;
    virtual void closeEvent(QCloseEvent *event) override final;

signals:
    void closeAction();

protected:
    DataDistributor* m_dataDistributor;
    TabelQLabelSubscriber* m_subscriber;
};

#endif // TABLEQLABELWIDGETDOCKWIDGET_H
