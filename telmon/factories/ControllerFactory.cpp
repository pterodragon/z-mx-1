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


#include "ControllerFactory.h"
#include "controllers/TreeMenuWidgetController.h"
#include "controllers/TableWidgetDockWindowController.h"

ControllerFactory::ControllerFactory()
{

}


BasicController* ControllerFactory::getController(const unsigned int a_type,
                                                  DataDistributor& a_dataDistributor,
                                                  MainWindowController& a_mainWindowController) const noexcept
{
    BasicController* l_result = nullptr;
    switch (a_type)
    {
        case CONTROLLER_TYPE::TREE_MENU_WIDGET_CONTROLLER:
            l_result = new TreeMenuWidgetController(a_dataDistributor, a_mainWindowController);
            break;
        case CONTROLLER_TYPE::TABLE_DOCK_WINDOW_CONTROLLER:
            l_result = new TableWidgetDockWindowController(a_dataDistributor);
            break;
        default:
            //todo - print warning
            l_result = nullptr;
            break;
    }
    return l_result;
}
