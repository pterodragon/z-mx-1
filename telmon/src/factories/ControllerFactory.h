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

// based on:
// https://stackoverflow.com/questions/86582/singleton-how-should-it-be-used


#ifndef CONTROLLERFACTORY_H
#define CONTROLLERFACTORY_H

class BasicController;
class DataDistributor;
class MainWindowController;
class QObject;

class ControllerFactory
{
private:
    // Private Constructor
    ControllerFactory();
    // Stop the compiler generating methods of copy the object
    ControllerFactory(ControllerFactory const& copy);            // Not Implemented
    ControllerFactory& operator=(ControllerFactory const& copy); // Not Implemented

public:
    enum CONTROLLER_TYPE {TREE_MENU_WIDGET_CONTROLLER,
                          TABLE_DOCK_WINDOW_CONTROLLER,
                          TABLE_LABEL_IMPL_DOCK_WINDOW_CONTROLLER,
                          CHART_DOCK_WINDOW_CONTROLLER,
                          SIZE };

    static ControllerFactory& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static ControllerFactory m_instance;
        return m_instance;
    }

    // Not responsible for deallocating
    BasicController* getController(const unsigned int a_type,
                                   DataDistributor& a_dataDistributor,
                                   MainWindowController& a_mainWindowController,
                                   QObject* a_parent) const noexcept;
};

#endif // CONTROLLERFACTORY_H
