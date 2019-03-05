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


#include "MainWindowView.h"
#include "QMenu"
#include "controllers/MainWindowController.h"
#include "QAction"
#include <QMenuBar>
#include "QObject"
#include "QApplication"
#include "QScreen"
#include "QDebug"

MainWindowView::MainWindowView(MainWindowController* a_mainWindowController):
    m_mainWindowController(a_mainWindowController),
    m_fileMenu(nullptr),
    m_connectSubMenu(nullptr),
    m_disconnectSubMenu(nullptr),
    m_settingsSubMenu(nullptr),
    m_exitSubMenu(nullptr)
{
}


MainWindowView::~MainWindowView()
{
        qDebug() << "    ~MainWindowView()";
}


void MainWindowView::initMenuBar()
{
    //TODO:
    // cant it be done all dynamically?
    // read more here https://doc.qt.io/qt-5/qmenubar.html#details


    // connect functionaility
    m_connectSubMenu = new QAction(QObject::tr("&Connect"), m_mainWindowController);
    m_connectSubMenu->setStatusTip(QObject::tr("Connect to server"));

    // disconnect functionaility
    m_disconnectSubMenu = new QAction(QObject::tr("&Disconnect"), m_mainWindowController);
    m_disconnectSubMenu->setEnabled(false); // by default disabled

    //settings functionality
    m_settingsSubMenu = new QAction(QObject::tr("&Settings"), m_mainWindowController);
    m_settingsSubMenu->setEnabled(false); // disabled until implemented

    // Exit functionaility
    m_exitSubMenu = new QAction(QObject::tr("&Exit"), m_mainWindowController);

    m_fileMenu = m_mainWindowController->menuBar()->addMenu(QObject::tr("&File"));
    m_fileMenu->addAction(m_connectSubMenu);
    m_fileMenu->addAction(m_disconnectSubMenu);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_settingsSubMenu);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitSubMenu);
}



void MainWindowView::setGeometry()
{
    // set window size to one quarter of user screen
    const QRect l_screenGeometry = QApplication::primaryScreen()->geometry();
    m_mainWindowController->resize(l_screenGeometry.width() / 2, l_screenGeometry.height() / 2);

    // move to center of screen
    const int x = (l_screenGeometry.width()  - m_mainWindowController->width()) / 2;
    const int y = (l_screenGeometry.height() - m_mainWindowController->height()) / 2;
    m_mainWindowController->move(x, y);
}

void MainWindowView::setWindowTitle() noexcept
{
    m_mainWindowController->setWindowTitle("TelMon");
}

