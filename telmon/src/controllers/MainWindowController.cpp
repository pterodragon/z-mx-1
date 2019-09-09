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
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "QDebug"

MainWindowController::MainWindowController(QWidget *parent) :
    QMainWindow(parent),
    m_mainWindowModel(new MainWindowModel()),
    m_mainWindowView(new MainWindowView(this)),
    m_controllersDB(new QMap<unsigned int, BasicController*>()) // key, value
{
    qInfo() << "MainWindowController() - Begin; VERSION" << CURRENT_VER;
    setAppearance(); // loading the css file
    m_mainWindowView->initCentralLook();
    m_mainWindowView->initMenuBar();
    m_mainWindowView->setGeometry();
    m_mainWindowView->setWindowTitle();

    //init Tree Menu Widget
    unsigned int l_key = ControllerFactory::CONTROLLER_TYPE::TREE_MENU_WIDGET_CONTROLLER;

    // create
    initController(l_key);

    m_mainWindowView->m_treeWidgetSplitter->addWidget(m_controllersDB->value(l_key)->getView());

    setCentralWidget(m_mainWindowView->m_centralWidget);

    // init Table Dock Windows controller
    l_key = ControllerFactory::CONTROLLER_TYPE::TABLE_LABEL_IMPL_DOCK_WINDOW_CONTROLLER;
    initController(l_key);

    // set close functinality
    auto* l_controller = static_cast<DockWindowController*>(m_controllersDB->value(l_key));
    connect(l_controller, &DockWindowController::closeDockWidget, this, &MainWindowController::closeDockWidget);

    // init Chart Dock Windows controller
    l_key = ControllerFactory::CONTROLLER_TYPE::CHART_DOCK_WINDOW_CONTROLLER;
    initController(l_key);

    // set close functinality
    l_controller = static_cast<DockWindowController*>(m_controllersDB->value(l_key));
    connect(l_controller, &DockWindowController::closeDockWidget, this, &MainWindowController::closeDockWidget);

    createActions();

    // set dock windowcloseDockWidgets behavioe
    // The default value is AnimatedDocks | AllowTabbedDocks.
    //setDockOptions(dockOptions() & VerticalTabs);
    setDockNestingEnabled(true);

    m_mainWindowView->setInitialStatusBarMsg(m_mainWindowModel->getInitialStatusBarMsg());

    qInfo() << "MainWindowController() - End";
}

MainWindowController::~MainWindowController()
{
    qInfo() << "~MainWindowController() - Begin; VERSION" << CURRENT_VER;

    // destroy dock windows first
    foreach (auto child, findChildren<QDockWidget *>())
    {
        qInfo() << "closing" << child << "status:" << child->close();
    }

    for (auto i = m_controllersDB->begin(); i != m_controllersDB->end() ;i++) { delete i.value(); }
    //foreach (auto controller, m_controllersDB) { delete controller} //totest and use //qt  foreach keyword
    delete m_controllersDB;
    delete m_mainWindowModel;
    delete m_mainWindowView;
    qInfo() << "~MainWindowController() - End";
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
        m_mainWindowModel->connect(*getStatusBar());
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
        initController(l_key);
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
        const auto m_defaultIpPort = QString(this->m_mainWindowModel->getIP() +
                                             ":" +
                                             this->m_mainWindowModel->getPort());

        QString text = QInputDialog::getText(this,
                                             tr("Settings"),
                                             tr(qPrintable(QString(
                                                               "Please insert: [ip:port]: \n"
                                                               "(In case of invalid data will use: ["  +
                                                               m_defaultIpPort                         +
                                                               "])"))),
                                             QLineEdit::Normal,
                                             m_defaultIpPort,
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


void MainWindowController::dockWindowsManagerTreeWidgetContextMenuSequence(const unsigned int a_dockWindowType,
                                                                           const QString& a_mxTelemetryInstanceName,
                                                                           const int a_mxTelemetryType) noexcept
{
    const auto l_mxTelemetryInstance = MxTelemetryGeneralWrapper::fromMxTypeValueToName(a_mxTelemetryType);

    // sanity check
    if (!m_controllersDB->contains(a_dockWindowType))
    {
        qCritical() << __PRETTY_FUNCTION__
                    << "invalid a_dockWindowType:"
                    << a_dockWindowType
                    << "other params:"
                    << l_mxTelemetryInstance
                    << a_mxTelemetryInstanceName;
        return;
    }

    // only for readability
    DockWindowController* l_dockWindowController = static_cast<DockWindowController*>(m_controllersDB->value(a_dockWindowType));

    // handleUserSelection, different behavior for table, chart etc...
    auto l_result = l_dockWindowController->handleUserSelection(
                                                findChildren<QDockWidget *>(),
                                                a_mxTelemetryInstanceName,
                                                a_mxTelemetryType);

    QSplitter* l_parentWidget = nullptr;

    switch (l_result.second) {
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
    {
        qCritical() << __PRETTY_FUNCTION__
                    << "Recived unknown action:"
                    << l_result.second
                    << "doing nothing!"
                    << a_dockWindowType
                    << l_mxTelemetryInstance
                    << a_mxTelemetryInstanceName;
    }
        return;
    }

    // another sanity check
    if (!l_result.first)
    {
        qCritical() << __PRETTY_FUNCTION__
                    << "Action" << l_result.second
                    << "l_dockWidget=nullptr!, doing nothing!"
                    << a_dockWindowType
                    << l_mxTelemetryInstance
                    << a_mxTelemetryInstanceName;
        return;
    }

    l_parentWidget->addWidget(l_result.first);
    l_result.first->setParent(l_parentWidget);
}

void MainWindowController::dockWindowsManagerFirstTimeDataInsertionToTreeSequence(
        const int a_mxTelemetryType,
        const QString& a_mxTelemetryInstanceName) noexcept
{
    // only for readability, get controller
    DockWindowController* l_dockWindowController = static_cast<DockWindowController*>(m_controllersDB->value(ControllerFactory::CONTROLLER_TYPE::CHART_DOCK_WINDOW_CONTROLLER));

    l_dockWindowController->initSubController(a_mxTelemetryType, a_mxTelemetryInstanceName);
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


void MainWindowController::closeDockWidget(const int a_type) noexcept
{
    switch (a_type) {
    case DockWindowController::ACTIONS::REMOVE_FROM_CENTER:
        if (m_mainWindowView->m_tablesWidgetSplitter->count() == 2)
        {
            m_mainWindowView->m_welcomeScreen->show();
        }
        break;
    case DockWindowController::ACTIONS::REMOVE_FROM_RIGHT:
        if (m_mainWindowView->m_chartsWidgetSplitter->count() == 2)
        {
            m_mainWindowView->m_chartAreaFiller->show();
        }
        break;
    default:
        qCritical() << __PRETTY_FUNCTION__
                    << "unknown type:" << a_type;
        break;
    }
}


bool MainWindowController::showPolicy(const unsigned int a_controllerType,
                                      const int a_mxType,
                                      const QString& a_mxInstance) const noexcept
{
    auto l_dockWindowController = static_cast<DockWindowController*>(m_controllersDB->value(a_controllerType, nullptr));
    if (!l_dockWindowController)
    {
        qCritical() << __PRETTY_FUNCTION__
                    << "a_controllerType" << a_controllerType << "does not exist in DB";
        return true;
    }
    return l_dockWindowController->showPolicy(a_mxType, a_mxInstance);
}


StatusBarWidget* MainWindowController::getStatusBar() noexcept
{
    return m_mainWindowView->getStatusBar();
}





