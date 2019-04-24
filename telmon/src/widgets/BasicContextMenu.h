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


#ifndef BASICCONTEXTMENU_H
#define BASICCONTEXTMENU_H

#include "QMenu"

/**
 * @brief The BasicContextMenu class
 * Implemented this class to provide tool tip tin tree view context menu
 * please read more
 *  https://stackoverflow.com/questions/27161122/qtooltip-for-qactions-in-qmenu
 */
class BasicContextMenu : public QMenu
{
    Q_OBJECT
public:
    explicit BasicContextMenu(QWidget* a_parent = nullptr) : BasicContextMenu(QString(), a_parent) {}
    explicit BasicContextMenu(const QString& a_title, QWidget* a_parent = nullptr);

    virtual void popup(const QPoint &p,
                       const int a_mxTelemetryType,
                       const bool a_tableFlag,
                       const bool a_chartFlag,
                       QAction *atAction = nullptr);

protected:
    virtual bool event (QEvent * e) override;
};

#endif // BASICCONTEXTMENU_H
