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



#include "MainWindowController.h"
#include "src/models/raw/MainWindowModel.h"
#include "src/views/raw/MainWindowView.h"
#include "BasicController.h"
#include "DockWindowController.h"
#include "src/factories/ControllerFactory.h"
#include "QAbstractItemView"
#include "QAction"
#include "QDockWidget"

#include "QDebug" // perhaps remove after testing

MainWindowController::MainWindowController(QWidget *parent) :
    QMainWindow(parent),
    m_mainWindowModel(new MainWindowModel()),
    m_mainWindowView(new MainWindowView(this)),
    m_controllersDB(new QMap<unsigned int, BasicController*>()) // key, value
{
    m_mainWindowView->initMenuBar();
    m_mainWindowView->setGeometry();
    m_mainWindowView->setWindowTitle();

    //init Tree Menu Widget
    const unsigned int l_key = ControllerFactory::CONTROLLER_TYPE::TREE_MENU_WIDGET_CONTROLLER;

    // create
    initController(l_key);

    // set as central
    setCentralWidget(m_controllersDB->value(l_key)->getView());

    // init Table Dock Windows controller
    initController(ControllerFactory::CONTROLLER_TYPE::TABLE_DOCK_WINDOW_CONTROLLER);

    // init Graph Dock Windows controller
    initController(ControllerFactory::CONTROLLER_TYPE::GRAPH_DOCK_WINDOW_CONTROLLER);

    createActions();

    // set dock windows behavioe
    // The default value is AnimatedDocks | AllowTabbedDocks.
    //setDockOptions(dockOptions() & VerticalTabs);
    setDockNestingEnabled(true);
}

MainWindowController::~MainWindowController()
{
    qDebug() << "~MainWindowController() - Begin";

    // destroy dock windows first
    foreach (auto child, findChildren<QDockWidget *>())
    {
        qDebug() << "closing" << child << "status:" << child->close();
    }

    for (auto i = m_controllersDB->begin(); i != m_controllersDB->end() ;i++) { delete i.value(); }
    //foreach (auto controller, m_controllersDB) { delete controller} //totest and use //qt  foreach keyword
    delete m_controllersDB;
    delete m_mainWindowModel;
    delete m_mainWindowView;
    qDebug() << "~MainWindowController() - End";
}


void MainWindowController::initController(const unsigned int a_key) noexcept
{
    // another sanity check
    if (a_key >= ControllerFactory::CONTROLLER_TYPE::SIZE)
    {
        qWarning() << "initController called with unknown controller type"
                   << a_key
                   << "returning...";
        return;
    }

    // construct controller and insert the DB of controllers
    m_controllersDB->insert( a_key, ControllerFactory::getInstance().getController(a_key, *m_mainWindowModel->getDataDistributor(), *this));
}


void MainWindowController::terminateController(const unsigned int a_key) noexcept
{
    // sanity check
    if (!m_controllersDB->contains(a_key))
    {
        qCritical() << "terminateController called with invalid key:" << a_key << "returning...";
        return;
    }

    delete m_controllersDB->value(a_key);
    if (m_controllersDB->remove(a_key) != 1)
    {
        //should never happen! (because we already checked the DB contains the key
        qCritical() << "Failed to remove key:" << a_key << "m_controllersDB, although already deleted it, something went wrong!";
    }
}


void MainWindowController::createActions() noexcept
{
    // todo - improve lambda performance by using the example from "ZmFn.hp"
    // File->Connect functionality
    QObject::connect(m_mainWindowView->m_connectSubMenu, &QAction::triggered, this, [this](){
        qInfo() << "MainWindowController::conncet ";
        m_mainWindowModel->connect();
        this->m_mainWindowView->m_connectSubMenu->setEnabled(false);
        this->m_mainWindowView->m_disconnectSubMenu->setEnabled(true);
    });

    // File->Disconnect functionality
    QObject::connect(m_mainWindowView->m_disconnectSubMenu, &QAction::triggered, this, [this](){
        qInfo() << "MainWindowController::disconnect ";
        this->m_mainWindowModel->disconnect();
        this->m_mainWindowView->m_connectSubMenu->setEnabled(true);
        this->m_mainWindowView->m_disconnectSubMenu->setEnabled(false);

        // for now, destruct and re constrcut of treeWidget, in the future, just clean
        //terminateTreeMenuWidget();
        const unsigned int l_key = ControllerFactory::CONTROLLER_TYPE::TREE_MENU_WIDGET_CONTROLLER;
        terminateController(l_key);

        // init again
        // create
        initController(l_key);

        // set as central
        setCentralWidget(m_controllersDB->value(l_key)->getView());
    });

    // File->Exit functionality
    QObject::connect(m_mainWindowView->m_exitSubMenu, &QAction::triggered, this, &QWidget::close);
}


void MainWindowController::dockWindowsManager(const unsigned int a_dockWindowType,
                                              const QString& a_mxTelemetryTypeName,
                                              const QString& a_mxTelemetryInstanceName) noexcept
{
    qDebug() << "MainWindowController::dockWindowsManage()"
             << a_dockWindowType
             << a_mxTelemetryTypeName
             << a_mxTelemetryInstanceName;

    const auto l_dockWindowType = a_dockWindowType;

    // sanity check
    if (!m_controllersDB->contains(l_dockWindowType))
    {
        qCritical() << "dockWindowsManager called with invalid a_dockWindowType:"
                    << l_dockWindowType
                    << a_mxTelemetryTypeName
                    << a_mxTelemetryInstanceName
                    << "returning...";
        return;
    }

    // only for readability
    DockWindowController* l_dockWindowController = static_cast<DockWindowController*>(m_controllersDB->value(l_dockWindowType));

    // setting default values which will be set later by handleUserSelection()
    unsigned int l_action = DockWindowController::ACTIONS::NO_ACTION;
    QDockWidget* l_dockWidget = nullptr;
    Qt::Orientation l_orientation = Qt::Vertical;

    // telling dockWindoeController to handle the selection
    l_dockWindowController->handleUserSelection(l_action,
                                                l_dockWidget,
                                                l_orientation,
                                                findChildren<QDockWidget *>(),
                                                a_mxTelemetryTypeName,
                                                a_mxTelemetryInstanceName);

    switch (l_action) {
    case DockWindowController::ACTIONS::NO_ACTION:
        break;
    case DockWindowController::ACTIONS::ADD:
        if (!l_dockWidget) {qCritical() << "DockWindowController::ACTIONS::DO_ADD recived l_dockWidget=nullptr!, doing nothing"
                                        << a_dockWindowType << a_mxTelemetryTypeName << a_mxTelemetryInstanceName; return;}
        //l_dockWidget->setWindowTitle(QString());
        l_dockWidget->setParent(this);
        // handle orientation
        addDockWidget(Qt::RightDockWidgetArea, l_dockWidget, l_orientation);
        break;
    default:
        qCritical() << "Recived unknown action:" << l_action << "doing nothing"
                    << a_dockWindowType << a_mxTelemetryTypeName << a_mxTelemetryInstanceName;
        break;
    }
}


