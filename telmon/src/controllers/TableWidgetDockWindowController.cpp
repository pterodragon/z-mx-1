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


#include "src/controllers/TableWidgetDockWindowController.h"
#include "QTableWidget"
#include "QDebug"
#include "src/models/wrappers/TableDockWidgetModelWrapper.h"
#include "src/widgets/TableWidgetDockWidget.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


TableWidgetDockWindowController::TableWidgetDockWindowController(DataDistributor& a_dataDistributor,
                                                                 QObject* a_parent):
    DockWindowController(a_dataDistributor, QString("TableWidgetDockWindowController"), a_parent),
    m_tableDockWidgetModelWrapper(new TableDockWidgetModelWrapper(a_dataDistributor))
    //m_modelWrapper(new TableWidgetDockWindowModelWrapper())
{

}


TableWidgetDockWindowController::~TableWidgetDockWindowController()
{
    qDebug() << "    ~TableWidgetDockWindowController() begin";
    delete m_tableDockWidgetModelWrapper;
    m_tableDockWidgetModelWrapper = nullptr;

    qDebug() << "    ~TableWidgetDockWindowController() end";
}


void* TableWidgetDockWindowController::getModel()
{
    return m_tableDockWidgetModelWrapper;
}


QAbstractItemView*  TableWidgetDockWindowController::getView()
{
    //TODO
    return nullptr;
}


// DockWindowController interface
std::pair<QDockWidget*, int> TableWidgetDockWindowController::handleUserSelection(
                                                          const QList<QDockWidget *>& a_currentDockList,
                                                          const QString& a_mxTelemetryInstanceName,
                                                          const int a_mxTelemetryType) noexcept
{
    // constrct widget name
    const QString l_a_mxTelemetryTypeName = MxTelemetryGeneralWrapper::fromMxTypeValueToName(a_mxTelemetryType);
    // for now, using same name for object and table header
    const QString l_objectName = l_a_mxTelemetryTypeName + "::" + a_mxTelemetryInstanceName + m_className;

    // is dock widget already exists?
    QDockWidget* l_dock = nullptr;
    bool l_contains = isDockWidgetExists(a_currentDockList, l_objectName, l_dock);

    // handle case that dock widget already exists
    if (l_contains)
    {
        qDebug() << m_className << l_objectName << "already exists";
        // out policy for table, only one at a time
        l_dock->activateWindow();
        return std::make_pair(nullptr, ACTIONS::NO_ACTION);
    }

    // construct the new dock
    auto* l_dockWidget = new TableWidgetDockWidget(m_tableDockWidgetModelWrapper,
                                                   a_mxTelemetryType,
                                                   a_mxTelemetryInstanceName,
                                                   nullptr);

    // set dock properties
    l_dockWidget->setAttribute(Qt::WA_DeleteOnClose);       // delete when user close the window

    // create the table
    QTableWidget* l_table = m_tableDockWidgetModelWrapper->getTable(l_a_mxTelemetryTypeName, a_mxTelemetryInstanceName);

    // set table parent as dock
    l_table->setParent(l_dock);

    // associate with l_dock;
    l_dockWidget->setWidget(l_table);

    return std::pair(l_dockWidget, ACTIONS::ADD_TO_CENTER);
}




