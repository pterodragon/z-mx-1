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
#include "QSplitter"
#include "src/widgets/BasicTextWidget.h"
#include "QInputDialog"
#include "QHostAddress"
#include "QFile"


#include "QDebug" // perhaps remove after testing

MainWindowController::MainWindowController(QWidget *parent) :
    QMainWindow(parent),
    m_mainWindowModel(new MainWindowModel()),
    m_mainWindowView(new MainWindowView(this)),
    m_controllersDB(new QMap<unsigned int, BasicController*>()) // key, value
{
    qDebug() << "MainWindowController() - Begin";
    setAppearance(); // loading the css file
    m_mainWindowView->initCentralLook();
    m_mainWindowView->initMenuBar();
    m_mainWindowView->setGeometry();
    m_mainWindowView->setWindowTitle();

    //init Tree Menu Widget
    const unsigned int l_key = ControllerFactory::CONTROLLER_TYPE::TREE_MENU_WIDGET_CONTROLLER;

    // create
    initController(l_key);

    m_mainWindowView->m_treeWidgetSplitter->addWidget(m_controllersDB->value(l_key)->getView());

    setCentralWidget(m_mainWindowView->m_centralWidget);

    // init Table Dock Windows controller
    initController(ControllerFactory::CONTROLLER_TYPE::TABLE_DOCK_WINDOW_CONTROLLER);

    // init Graph Dock Windows controller
    initController(ControllerFactory::CONTROLLER_TYPE::GRAPH_DOCK_WINDOW_CONTROLLER);

    createActions();

    // set dock windows behavioe
    // The default value is AnimatedDocks | AllowTabbedDocks.
    //setDockOptions(dockOptions() & VerticalTabs);
    setDockNestingEnabled(true);
    qDebug() << "MainWindowController() - End";
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
    m_controllersDB->insert( a_key, ControllerFactory::getInstance().getController(a_key, *m_mainWindowModel->getDataDistributor(), *this, this));
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
         this->m_mainWindowView->m_treeWidgetSplitter->addWidget(m_controllersDB->value(l_key)->getView());

        setCentralWidget( this->m_mainWindowView->m_centralWidget);
    });

    // File->Settings functionality
    // for now we use QInputDialog
    // https://doc.qt.io/qt-5/qinputdialog.html#details
    // in the future should implement
    // normal dialog window
    // https://doc.qt.io/qt-5/examples-dialogs.html
    QObject::connect(m_mainWindowView->m_settingsSubMenu, &QAction::triggered, this, [this](){

        qInfo() << "MainWindowController::settings ";

        bool l_ok;
        QString text = QInputDialog::getText(this,
                                             tr("Settings"),
                                             tr("Please insert: [ip:port]: \n"
                                                "(In case of invalid data will use: [127.0.0.1:19300])"),
                                             QLineEdit::Normal,
                                             "127.0.0.1:19300",
                                             &l_ok);

        if (!l_ok) {return;}

        // check that user input is ok:
        // check that user entered ":" between ip and port
        QStringList l_IP_and_Port = text.split(':');
        if (l_IP_and_Port.size() != 2) {return;}

        // check IP
        QHostAddress l_addres;
        if (!l_addres.setAddress(l_IP_and_Port[0])) {return;}

        //check PORT
        // max port 65535
        if (l_IP_and_Port[1].toInt() > 65535
                or
            l_IP_and_Port[1].toInt() < 0)
        {return;}

        // success, set the ip:port
        this->m_mainWindowModel->setIP(l_IP_and_Port[0].toLocal8Bit().data());
        this->m_mainWindowModel->setPort(l_IP_and_Port[1].toLocal8Bit().data());

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

    // sanity check
    if (!m_controllersDB->contains(a_dockWindowType))
    {
        qCritical() << "dockWindowsManager called with invalid a_dockWindowType:"
                    << a_dockWindowType
                    << a_mxTelemetryTypeName
                    << a_mxTelemetryInstanceName
                    << "returning...";
        return;
    }

    // only for readability
    DockWindowController* l_dockWindowController = static_cast<DockWindowController*>(m_controllersDB->value(a_dockWindowType));

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

    QSplitter* l_parentWidget = nullptr;

    switch (l_action) {
    case DockWindowController::ACTIONS::NO_ACTION:
        return;
    case DockWindowController::ACTIONS::ADD_TO_CENTER:
        if (m_mainWindowView->m_welcomeScreen->isActiveWindow()) {m_mainWindowView->m_welcomeScreen->hide();}
        l_parentWidget =m_mainWindowView->m_tablesWidgetSplitter;
        break;
    case DockWindowController::ACTIONS::ADD_TO_RIGHT:
        if (m_mainWindowView->m_chartAreaFiller->isActiveWindow()) {m_mainWindowView->m_chartAreaFiller->hide();}
        l_parentWidget = m_mainWindowView->m_chartsWidgetSplitter;
        break;
    default:
        qCritical() << __func__
                    << "Recived unknown action:"
                    << l_action
                    << "doing nothing!"
                    << a_dockWindowType
                    << a_mxTelemetryTypeName
                    << a_mxTelemetryInstanceName;
        return;
    }

    // another sanity check
    if (!l_dockWidget)
    {
        qCritical() << __func__
                    << "Action" << l_action
                    << "l_dockWidget=nullptr!, doing nothing!"
                    << a_dockWindowType
                    << a_mxTelemetryTypeName
                    << a_mxTelemetryInstanceName;
        return;
    }


    l_parentWidget->addWidget(l_dockWidget);
    l_dockWidget->setParent(l_parentWidget);
}


void MainWindowController::setAppearance(const QString& a_pathToFile) noexcept
{
    // most of the time we will use dafault value
    QString l_pathTofile = DEFAULT_STYLE_PATH;

    // notice: the default QString is Null and Empty
    // we use logical: (not (A AND B)) equals (not(A) or not(B))
    if (!a_pathToFile.isNull() or !a_pathToFile.isEmpty())
    {
        l_pathTofile = a_pathToFile;
    }

    // we check file exists
    if ( !QFile::exists(l_pathTofile) )
    {
        qWarning() << __PRETTY_FUNCTION__
                   << "file does not EXISTS:"
                   << l_pathTofile;
        return;
    }

    // open the file and use
    QFile l_file(l_pathTofile);
    if ( !l_file.open(QFile::ReadOnly) )
    {
        qWarning() << __PRETTY_FUNCTION__
                   << "could not OPEN:"
                   << l_pathTofile
                   << l_file.errorString();
        return;
    }

    const QString StyleSheet = QLatin1String(l_file.readAll());
    this->setStyleSheet(StyleSheet);
    l_file.close();
}


