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


#include "TableLabelImplDockWindowController.h"
#include "src/widgets/TableQLabelWidgetDockWidget.h"

#include "QLabel"
#include "QDebug"

TableLabelImplDockWindowController::TableLabelImplDockWindowController(DataDistributor& a_dataDistributor,
                                                                       QObject* a_parent):
    DockWindowController(a_dataDistributor,
                         QString("TableLabelImplDockWindowController"),
                         a_parent),
    m_activeTables(new QSet<QString>)
{

}


TableLabelImplDockWindowController::~TableLabelImplDockWindowController()
{
    delete m_activeTables;
    m_activeTables = nullptr;
}


std::pair<QDockWidget*, int> TableLabelImplDockWindowController::handleUserSelection(
                                                             const QList<QDockWidget *>&,
                                                             const QString& a_mxTelemetryInstanceName,
                                                             const int a_mxTelemetryType) noexcept
{
    auto l_dock = new TableQLabelWidgetDockWidget(a_mxTelemetryType,
                                                  a_mxTelemetryInstanceName,
                                                  m_dataDistributor,
                                                  nullptr);

    const QString l_dockName = l_dock->objectName();
    if (m_activeTables->contains(l_dockName))
    {
        // this logic should not be executed because of table already exist
        // user should not have the option to create another one from context menu
        qCritical() << __PRETTY_FUNCTION__
                    << "trying to create already existing table"
                    << l_dockName;
        delete l_dock;
        return std::make_pair(nullptr, ACTIONS::NO_ACTION);
    }

    m_activeTables->insert(l_dockName);

    // close functioanility
    connect(l_dock, &TableQLabelWidgetDockWidget::destroyed,
            this, &TableLabelImplDockWindowController::dockWidgetDestroyed);

    return std::make_pair(l_dock, ACTIONS::ADD_TO_CENTER);
}


void TableLabelImplDockWindowController::dockWidgetDestroyed(QObject* a_obj)
{
    QString l_dockName;
    if (a_obj) {l_dockName = a_obj->objectName();}

    if (!l_dockName.isNull())
    {
        if (m_activeTables->contains(l_dockName)) {
            m_activeTables->remove(l_dockName);
        } else {
            qCritical() << __PRETTY_FUNCTION__
                        << "trying to remove dock which is not in DB"
                           // which implies something went wrong,
                           // because this is the only place dock should be removed
                        << l_dockName;
        }
    }
    emit closeDockWidget(DockWindowController::ACTIONS::REMOVE_FROM_CENTER);
}


bool TableLabelImplDockWindowController::showPolicy(const QString& a_name) const noexcept
{
    // or show policy is only one table at a time
    return !(m_activeTables->contains(a_name));
}













