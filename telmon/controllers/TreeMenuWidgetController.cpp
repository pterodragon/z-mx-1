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

#include "controllers/TreeMenuWidgetController.h"
#include "models/wrappers/TreeMenuWidgetModelWrapper.h"
#include "views/raw/TreeView.h"
#include "distributors/DataDistributor.h"
#include "QMenu"
#include "QDebug"
#include "controllers/MainWindowController.h"
#include "factories/ControllerFactory.h" // only for enums

TreeMenuWidgetController::TreeMenuWidgetController(DataDistributor& a_dataDistributor, MainWindowController& a_MainWindowController):
    BasicController(a_dataDistributor),
    m_treeMenuWidgetModelWrapper(new TreeMenuWidgetModelWrapper()),
    m_treeView (new TreeView),
    m_MainWindowController(a_MainWindowController)
{
    m_treeView->setModel(getModel());

    createActions();

    //subscribe on initilization
    m_dataDistributor.subscribeAll(m_treeMenuWidgetModelWrapper);
}

TreeMenuWidgetController::~TreeMenuWidgetController()
{
    //unsubscribe on destruction
    m_dataDistributor.unsubscribeAll(m_treeMenuWidgetModelWrapper);

    delete m_treeMenuWidgetModelWrapper;
    delete m_treeView;
}


QAbstractItemModel* TreeMenuWidgetController::getModel()
{
    return m_treeMenuWidgetModelWrapper->getModel();
}

QAbstractItemView* TreeMenuWidgetController::getView()
{
    //return m_treeView;
    return static_cast<QAbstractItemView*>(m_treeView);
}

void TreeMenuWidgetController::createActions() noexcept
{
    // adding context menu functionality
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    // contextMenu->firstAction functionality
    m_treeView->connect(m_treeView->m_firstAction, &QAction::triggered, this, [this](){

        const QString l_actionName = m_treeView->m_firstAction->text();
        handleContextMenuAction(l_actionName, ControllerFactory::CONTROLLER_TYPE::TABLE_DOCK_WINDOW_CONTROLLER);

    });


    // contextMenu->m_secondAction functionality
    m_treeView->connect(m_treeView->m_secondAction, &QAction::triggered, this, [this]() {

        const QString l_actionName = m_treeView->m_secondAction->text();
        handleContextMenuAction(l_actionName, ControllerFactory::CONTROLLER_TYPE::GRAPH_DOCK_WINDOW_CONTROLLER);

    });

    // contextMenu view/display functionality
    m_treeView->connect(m_treeView, &TreeView::customContextMenuRequested, this, [this](){
        //  get selected item in view
        QModelIndexList indexes = this->m_treeView->selectionModel()->selectedIndexes();

        // prevent displaying the menu without a selected index
        if (indexes.isEmpty() ) { return;}

        //  prevent displaying the menu to one of MxTelemetry::types (i.e. Heap, HashTbl..)
        if (!this->m_treeMenuWidgetModelWrapper->isMxTelemetryTypeSelected(indexes.first())) {return;};

        // pop up menu
        this->m_treeView->m_contextMenu->popup(QCursor::pos());
    } );
}


void TreeMenuWidgetController::handleContextMenuAction(const QString& a_actionName,
                                                       const unsigned int a_dockWindowType) const noexcept
{
    //  get selected item in view
    QModelIndexList indexes = this->m_treeView->selectionModel()->selectedIndexes();

    // get some parameters
    const QString l_actionName = a_actionName;
    const QString l_childName  = this->m_treeMenuWidgetModelWrapper->getModel()->data(indexes.first()).toString();
    const QModelIndex l_parent =  this->m_treeMenuWidgetModelWrapper->getModel()->parent(indexes.first());
    const QString l_parentName = l_parent.isValid() ? this->m_treeMenuWidgetModelWrapper->getModel()->data(l_parent).toString()
                                                    : QString();
    // sanity check:
    if (l_parentName.isNull()) {
        qWarning() << "Action:"
                   << l_actionName
                   << "Child:"
                   << l_childName
                   << "Called with invalid parent, returning...";
        return;
    }

    m_MainWindowController.dockWindowsManager(a_dockWindowType, l_parentName, l_childName);
}

