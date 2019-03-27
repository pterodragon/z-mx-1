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
#include "src/models/wrappers/BasicDockWidget.h"


TableWidgetDockWindowController::TableWidgetDockWindowController(DataDistributor& a_dataDistributor):
    DockWindowController(a_dataDistributor, " Table"),
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


QAbstractItemModel* TableWidgetDockWindowController::getModel()
{
    //TODO
    return nullptr;
}


QAbstractItemView*  TableWidgetDockWindowController::getView()
{
    //TODO
    return nullptr;
}


// DockWindowController interface
void TableWidgetDockWindowController::handleUserSelection(unsigned int& a_action,
                                                          QDockWidget*& a_dockWidget,
                                                          Qt::Orientation& a_orientation,
                                                          const QList<QDockWidget *>& a_currentDockList,
                                                          const QString& a_mxTelemetryTypeName,
                                                          const QString& a_mxTelemetryInstanceName) noexcept
{
    // constrct widget name
    // for now, using same name for object and table header
    const QString l_objectName = a_mxTelemetryTypeName + "::" + a_mxTelemetryInstanceName + m_dockWindowName;

    // is dock widget already exists?
    QDockWidget* l_dock = nullptr;
    bool l_contains = isDockWidgetExists(a_currentDockList, l_objectName, l_dock);

    // handle case that dock widget already exists
    if (l_contains)
    {
        qDebug() << m_dockWindowName << l_objectName << "already exists";
        // out policy for table, only one at a time
        a_action = DockWindowController::ACTIONS::NO_ACTION;
        a_dockWidget = nullptr;
        l_dock->activateWindow();
        return;
    }
    qDebug() << "constrcuting" << m_dockWindowName << l_objectName;

    // set orientation
    a_orientation = Qt::Orientation::Horizontal;

    //handle case that dock widget not exists
    a_action = DockWindowController::ACTIONS::ADD_TO_CENTER;

    // construct the new dock
    //a_widget = new QDockWidget(l_objectName, nullptr);
    a_dockWidget = new BasicDockWidget(l_objectName,
                                  m_tableDockWidgetModelWrapper,
                                  a_mxTelemetryTypeName,
                                  a_mxTelemetryInstanceName,
                                  nullptr);

    // set dock properties
    a_dockWidget->setAttribute(Qt::WA_DeleteOnClose);       // delete when user close the window
    a_dockWidget->setAllowedAreas(Qt::RightDockWidgetArea); // allocate to the right of the window

    // create the table
     QTableWidget* l_table = m_tableDockWidgetModelWrapper->getTable(a_mxTelemetryTypeName, a_mxTelemetryInstanceName);

     l_table->setParent(l_dock);

    // associate with l_dock;
    a_dockWidget->setWidget(l_table);

    //register to datadistributor
}




/**
// good example
//QObject::connect(l_table, SIGNAL(processingDone()), this, SLOT()); //compiling example
//     QObject::connect(this, &QObject::objectNameChanged, this, [](){});
//     QObject::connect(m_mainWindowView->m_connectSubMenu, &QAction::triggered, this, [this](){
//         qInfo() << "MainWindowController::conncet ";
//         m_mainWindowModel->connect();
//         this->m_mainWindowView->m_connectSubMenu->setEnabled(false);
//         this->m_mainWindowView->m_disconnectSubMenu->setEnabled(true);
//     });
 */






