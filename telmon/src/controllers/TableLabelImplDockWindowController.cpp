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


#include "TableLabelImplDockWindowController.h"
#include "src/widgets/TableQLabelWidgetDockWidget.h"

#include "QLabel"

TableLabelImplDockWindowController::TableLabelImplDockWindowController(DataDistributor& a_dataDistributor,
                                                                       QObject* a_parent):
    DockWindowController(a_dataDistributor,
                         QString("TableLabelImplDockWindowController"),
                         a_parent)
{

}


TableLabelImplDockWindowController::~TableLabelImplDockWindowController()
{

}


std::pair<QDockWidget*, int> TableLabelImplDockWindowController::handleUserSelection(
                                                             const QList<QDockWidget *>&,
                                                             const QString& a_mxTelemetryInstanceName,
                                                             const int a_mxTelemetryType) noexcept
{
    auto l_dock = new TableQLabelWidgetDockWidget(a_mxTelemetryType,
                                                  a_mxTelemetryInstanceName,
                                                  m_dataDistributor,
                                                  nullptr);

    // close functioanility
    connect(l_dock, &TableQLabelWidgetDockWidget::destroyed, this, [this](QObject* ){
        emit closeDockWidget(DockWindowController::ACTIONS::REMOVE_FROM_CENTER);
    });

    return std::make_pair(l_dock, ACTIONS::ADD_TO_CENTER);
}







