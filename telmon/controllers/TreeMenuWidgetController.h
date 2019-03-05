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


#ifndef TREEMENUWIDGETCONTROLLER_H
#define TREEMENUWIDGETCONTROLLER_H


#include "BasicController.h"

class TreeMenuWidgetModelWrapper;
class TreeView;
class MainWindowController;

class TreeMenuWidgetController : public BasicController
{

public:
    // interface
    TreeMenuWidgetController(DataDistributor& a_dataDistributor, MainWindowController& a_MainWindowController);
    virtual ~TreeMenuWidgetController() override;
    virtual QAbstractItemModel* getModel() override;
    virtual QAbstractItemView* getView() override;


private:
    TreeMenuWidgetModelWrapper* m_treeMenuWidgetModelWrapper;
    TreeView* m_treeView;

    // not part of the interface
    MainWindowController& m_MainWindowController;
    void handleContextMenuAction(const QString& a_actionName,
                                 const unsigned int a_dockWindowType) const noexcept; // right now we handle only two actions, Table and Chart

    void createActions() noexcept;
};

#endif // TREEMENUWIDGETCONTROLLER_H
