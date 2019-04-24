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


#include "BasicContextMenu.h"
#include "QHelpEvent"
#include "QToolTip"
#include "QDebug"
#include "src/factories/MxTelemetryTypeWrappersFactory.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "src/views/raw/TreeView.h"

BasicContextMenu::BasicContextMenu(const QString& a_title, QWidget* a_parent):
    QMenu(a_title, a_parent)
{

}



bool BasicContextMenu::event (QEvent * e)
{
    const QHelpEvent *helpEvent = static_cast <QHelpEvent *>(e);
     if (helpEvent->type() == QEvent::ToolTip && activeAction() != nullptr)
     {
          QToolTip::showText(helpEvent->globalPos(), activeAction()->toolTip());
     } else
     {
          QToolTip::hideText();
     }
     return QMenu::event(e);
}

void BasicContextMenu::popup(const QPoint &p,
                             const int a_mxTelemetryType,
                             const bool a_tableFlag,
                             QAction *atAction)
{
    const auto* const l_typeWrapper = MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(a_mxTelemetryType);

    // enable/disable context menu option according to telemetry
    // indexes:
    // 0 - show table
    if (a_tableFlag)
    {
        actions().at(0)->setText(TreeView::TABLE_ENABLE_LABEL);
        actions().at(0)->setEnabled(true);
    } else {
        actions().at(0)->setText(TreeView::TABLE_DISABLE_LABEL);
        actions().at(0)->setEnabled(false);
    }

    // 1 - show chart
    const auto l_chartPosbillityOption = l_typeWrapper->isChartOptionEnabledInContextMenu();
    if (l_chartPosbillityOption) {
            actions().at(1)->setText(TreeView::CHART_ENABLE_LABEL);
            actions().at(1)->setEnabled(true);
    } else {
            actions().at(1)->setText(TreeView::CHART_NOT_SUPPORTED_LABEL);
            actions().at(1)->setEnabled(false);
    }


    // in the future, allow tooltip also in disabled menu, see:
    // msg "no dynamic data to show, so this option is disabled"
    //   https://stackoverflow.com/questions/40271665/is-there-a-way-to-show-tooltip-on-disabled-qwidget

    QMenu::popup(p, atAction);
}
