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


#ifndef MAINWINDOWVIEW_H
#define MAINWINDOWVIEW_H

class QMenu;
class QAction;
class MainWindowController;

class QSplitter;
class QHBoxLayout;
class QWidget;
class BasicTextWidget;
class StatusBarWidget;
class QString;

class MainWindowView
{
    friend class MainWindowController;

public:
    MainWindowView(MainWindowController* a_mainWindowController);
    ~MainWindowView();

    StatusBarWidget* getStatusBar() noexcept;

    void setInitialStatusBarMsg(const QString& a_msg) noexcept;

private:
    MainWindowController* m_mainWindowController;

    void setGeometry();
    void setWindowTitle() noexcept;
    // # # # # # MENUS PART # # # # #
    void initMenuBar();

    QMenu *m_fileMenu;
    QAction *m_connectSubMenu;
    QAction *m_disconnectSubMenu;
    QAction *m_settingsSubMenu;
    QAction *m_exitSubMenu;

    // # # # # # Central Look Part # # # # #
    void initCentralLook() noexcept;
    // we are going from micro to macro
    QSplitter* m_treeWidgetSplitter;
    QSplitter* m_tablesWidgetSplitter;
    QSplitter* m_chartsWidgetSplitter;

    // the following splitter is used so the user can resize
    // windows during runtime
    QSplitter* m_centralWindowSplitter;

    QHBoxLayout* m_centralWidgetLayout;

    // this widget will be the central widget of the main window
    QWidget* m_centralWidget;

    BasicTextWidget* m_welcomeScreen;
    BasicTextWidget* m_chartAreaFiller;
    // # # # # # # # # # # # # # # # # # # # # # # # # # # #

};

#endif // MAINWINDOWVIEW_H
