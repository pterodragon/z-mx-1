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

#include "TreeView.h"
#include "QMenu"
#include "QAction"
#include "QMouseEvent"
#include "QDebug"

TreeView::TreeView():
    m_mainWindow(nullptr)
{
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    initContextMenu();
}

TreeView::~TreeView()
{
    // no need to free m_contextMenu and related
}

void TreeView::initContextMenu() noexcept
{
    m_contextMenu = new QMenu(this);

    m_firstAction = new QAction(QObject::tr("Show Table"), this);
    m_firstAction->setStatusTip(QObject::tr("Open new window with data fields updated in real time"));

    m_secondAction = new QAction(QObject::tr("Show Chart"), this);
    m_secondAction->setStatusTip(QObject::tr("Open new window with graph to monitor specific data filed over time"));

    m_contextMenu->addAction(m_firstAction);
    m_contextMenu->addAction(m_secondAction);
}

void TreeView::mousePressEvent(QMouseEvent* event)
{
    QModelIndex item = indexAt(event->pos());

    if (item.isValid())
    {
        QTreeView::mousePressEvent(event);
    }
    else
    {
        clearSelection();
        selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::Select);
    }
}


void TreeView::setMainWindow(QWidget* a_mainWindow) noexcept
{
    m_mainWindow = a_mainWindow;
}


QSize TreeView::sizeHint() const
{
    if (!m_mainWindow)
        return QTreeView::sizeHint();

    const auto l_size = m_mainWindow->size();

    // after playing with it, for below values the tree widget looks good on strartup
    return  QSize(l_size.width() * 20 / 100, l_size.height() * 90 / 100);
}




