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


#ifndef TREEVIEW_H
#define TREEVIEW_H

#include "QTreeView"

class QMenu;
class QAction;

class TreeView : public QTreeView
{
    Q_OBJECT
    friend class TreeMenuWidgetController;

public:
    TreeView();
    ~TreeView() override;

    /**
     * @brief this method allows us to remove selection of index in the table
     * in case user press in some place which is not an item
     * @param event
     */
    virtual void mousePressEvent(QMouseEvent* event) final override;
    virtual QSize sizeHint() const override;
    void setMainWindow(QWidget*) noexcept;


protected:
    QMenu* m_contextMenu;
    QAction* m_firstAction;
    QAction* m_secondAction;

    void initContextMenu() noexcept;

private:
    QWidget* m_mainWindow;

};

#endif // TREEVIEW_H
