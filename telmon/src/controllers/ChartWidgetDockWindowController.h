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


#ifndef CHARTWIDGETDOCKWINDOWCONTROLLER_H
#define CHARTWIDGETDOCKWINDOWCONTROLLER_H

#include "DockWindowController.h"

class ChartDockWidgetModelWrapper;
class BasicChartController;
template<class T>
class QVector;

class ChartWidgetDockWindowController : public DockWindowController
{
public:
    ChartWidgetDockWindowController(DataDistributor& a_dataDistributor, QObject* a_parent);
    ~ChartWidgetDockWindowController() override;

    // BasicController interface
    virtual void* getModel()  override final;
    virtual QAbstractItemView* getView()   override final;

    // DockWindowController interface
    virtual void handleUserSelection(unsigned int& a_action,
                                     QDockWidget*& a_widget,
                                     const QList<QDockWidget *>& a_currentDockList,
                                     const QString& a_mxTelemetryTypeName,
                                     const QString& a_mxTelemetryInstanceName,
                                     const int a_mxTelemetryType) noexcept override final;

    virtual void initSubController(const int mxTelemetryType,
                                   const QString& mxTelemetryInstanceName) noexcept override final;

private:
    // todo, perhaps remove
    ChartDockWidgetModelWrapper* m_chartDockWidgetModelWrapper;

    // new
    const QString* m_className;
    // most important attribute: index lookup, QVector is O(1), that is,
    //    to access the correct MxTypeInsatnces *dataContainer*;
    // *dataContainer* will be made from QMap<Key=QString, T=BasicChartController*>
    //    key lookup is O(log n)
    QVector<QMap<QString, BasicChartController*>*>* m_DB;

    BasicChartController* getController(const int mxTelemetryType,
                                        const QString& mxTelemetryInstanceName) noexcept;
};

#endif // CHARTWIDGETDOCKWINDOWCONTROLLER_H
