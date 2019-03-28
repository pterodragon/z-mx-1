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

#include "src/controllers/TreeMenuWidgetController.h"
#include "src/models/wrappers/TreeMenuWidgetModelWrapper.h"
#include "src/views/raw/TreeView.h"
#include "src/distributors/DataDistributor.h"
#include "QMenu"
#include "QDebug"
#include "src/controllers/MainWindowController.h"
#include "src/factories/ControllerFactory.h" // only for enums
#include "src/models/raw/TreeModel.h"
#include "src/widgets/BasicContextMenu.h"

TreeMenuWidgetController::TreeMenuWidgetController(DataDistributor& a_dataDistributor,
                                                   MainWindowController& a_MainWindowController,
                                                   QObject* a_parent ):
    BasicController(a_dataDistributor, a_parent),
    m_treeMenuWidgetModelWrapper(new TreeMenuWidgetModelWrapper()),
    m_treeView (new TreeView),
    m_MainWindowController(a_MainWindowController)
{
    m_treeView->setModel(getModel());

    createActions();

    m_treeView->setMainWindow(static_cast<QWidget*>(this->parent()));

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

        // get the telemetryType index
        const auto l_MxTelemetryTypeIndex = m_dataDistributor.fromMxTypeNameToValue(indexes.first().parent().data().toString());

        // pop up menu
        // Notice: static cast to call my own implementation of popup
        static_cast<BasicContextMenu*>(this->m_treeView->m_contextMenu)->popup(QCursor::pos(), l_MxTelemetryTypeIndex);
    } );

}


void TreeMenuWidgetController::handleContextMenuAction(const QString& a_actionName,
                                                       const unsigned int a_dockWindowType) const noexcept
{
    //  get selected item in view
    QModelIndexList indexes = this->m_treeView->selectionModel()->selectedIndexes();

    // get some parameters
    const QString      l_actionName = a_actionName;
    const QString&     l_childName  = this->m_treeMenuWidgetModelWrapper->getModel()->data(indexes.first()).toString();
    const QModelIndex& l_parent     = this->m_treeMenuWidgetModelWrapper->getModel()->parent(indexes.first());
    const QString&     l_parentName = l_parent.isValid() ?
                                        this->m_treeMenuWidgetModelWrapper->getModel()->data(l_parent).toString()
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

