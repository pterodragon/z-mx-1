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

// based on
// https://stackoverflow.com/questions/318064/how-do-you-declare-an-interface-in-c

#ifndef DOCKWINDOWCONTROLLER_H
#define DOCKWINDOWCONTROLLER_H

#include "BasicController.h"

class QDockWidget;


class DockWindowController : public BasicController
{
    Q_OBJECT
public:
    DockWindowController(DataDistributor& a_dataDistributor,
                         const QString& a_className,
                         QObject* a_parent);
    virtual ~DockWindowController();


    // BasicController interface
    // Stop the compiler generating methods of copy the object
    DockWindowController(DockWindowController const& copy);            // Not Implemented
    DockWindowController& operator=(DockWindowController const& copy); // Not Implemented

    virtual void* getModel()  = 0;
    virtual QAbstractItemView* getView()   = 0;


    // DockWindowController interface
    enum ACTIONS {NO_ACTION,
                  ADD_TO_CENTER,
                  ADD_TO_RIGHT,
                  REMOVE_FROM_CENTER,
                  REMOVE_FROM_RIGHT};

    virtual std::pair<QDockWidget*, int> handleUserSelection(const QList<QDockWidget *>& a_currentDockList,
                                                             const QString& a_mxTelemetryInstanceName,
                                                             const int a_mxTelemetryType) noexcept = 0;

    bool isDockWidgetExists(const QList<QDockWidget *>& a_currentDockList,
                            const QString& l_objectName,
                            QDockWidget*& a_dock) const noexcept;

    virtual void initSubController(const int mxTelemetryType,
                                   const QString& mxTelemetryInstanceName) noexcept;

signals:
    void closeDockWidget(int);
};

#endif // DOCKWINDOWCONTROLLER_H
