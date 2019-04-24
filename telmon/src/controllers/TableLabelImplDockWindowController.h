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


#ifndef TABLELABELIMPLDOCKWINDOWCONTROLLER_H
#define TABLELABELIMPLDOCKWINDOWCONTROLLER_H

#include "src/controllers/DockWindowController.h"

class TableLabelImplDockWindowController : public DockWindowController
{
    Q_OBJECT
public:
    TableLabelImplDockWindowController(DataDistributor& a_dataDistributor,
                                       QObject* a_parent);
    virtual ~TableLabelImplDockWindowController() override;

    // interface BasicController
    virtual void* getModel()             override final {return nullptr;}
    virtual QAbstractItemView* getView() override final {return nullptr;}

    // interface DockWindowController
    virtual std::pair<QDockWidget*, int> handleUserSelection(const QList<QDockWidget *>& a_currentDockList,
                                                             const QString& a_mxTelemetryInstanceName,
                                                             const int a_mxTelemetryType) noexcept override final;

    virtual bool showPolicy(const int a_mxType,
                            const QString& a_mxInstance) const noexcept final override;

protected slots:
    void dockWidgetDestroyed(QObject* a_obj);

protected:
    /**
     * @brief m_activeTables, holds the names of all active tables
     */
    QSet<QString>* m_activeTables;
};

#endif // TABLELABELIMPLDOCKWINDOWCONTROLLER_H




