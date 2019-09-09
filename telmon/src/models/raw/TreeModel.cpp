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



#include "TreeModel.h"

#include "TreeItem.h"
#include "QDebug"
#include "QTime"

//TreeModel::TreeModel(const char* a_header0, const char* a_header1, QObject *parent)
TreeModel::TreeModel(const char* a_header0, QObject *parent)
    : QAbstractItemModel(parent),
      // Important notice only the rootItem parent is allowed to be nullptr
      rootItem(new TreeItem(new QList<QVariant>({a_header0/**, a_header1*/}), nullptr))
{

}


TreeModel::~TreeModel()
{
    // Notice: no need to delete rootData, will be deleted by ~TreeItem
    delete rootItem;
}


TreeItem *TreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid())
    {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
        {
            return item;
        }
    }
    return rootItem;
}


bool TreeModel::isParentIsRootItem(const QModelIndex &index) const
{
    if (index.isValid())
    {
        return rootItem == static_cast<TreeItem*>(index.internalPointer())->parent();

    }
    return false;
}


int TreeModel::rowCount(const QModelIndex &a_parent) const
{
    return getItem(a_parent)->childCount();
}


int TreeModel::columnCount(const QModelIndex &) const
{
    // all items are defined to have the same number of columns
    return rootItem->columnCount();
}


// To dive into
Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return nullptr;
    }

    return Qt::ItemIsSelectable | QAbstractItemModel::flags(index);
}


/**
 * @brief function that provide indexes for views and delegates
 * to use when accessing data.
 * Indexes are created for other components when they are
 * referenced by their row and column numbers, and their
 * parent model index. If an invalid model index is specified
 * as the parent, it is uo to the model to return an index
 * that corresponds to a top-level in the model.
 * see
 * 1. http://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html
 * index() section
 * @param rows
 * @param column
 * @param parent
 * @return
 */
QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
    {
            return QModelIndex();
    }

    TreeItem *parentItem = getItem(parent);

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
    {
        return createIndex(row, column, childItem);
    }
    else
    {
        return QModelIndex();
    }
}


QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    // if invalid index, return the index of the first item
    if (!index.isValid())
    {
        return QModelIndex();
    }
    // else index is valid

    // translate index to childTreeItem
    TreeItem *childItem = getItem(index);
    TreeItem *parentItem = childItem->parent();

    // if the parent is root, return the root index
    if (parentItem == rootItem)
    {
        return QModelIndex();
    }

    // else return model index for parent item with location as mentioned
     return createIndex(parentItem->childNumber(), 0, parentItem);
}


// Returns the data stored under the given role for the item referred to by the index.
// see https://doc.qt.io/qt-5/qabstractitemmodel.html#data
QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    TreeItem *item = getItem(index);

    //qDebug() << "TreeModel::data, index=" << index;

    return item->data(index.column());
}



QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

//# # # # # # # # # # #

bool TreeModel::insertColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
    success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool TreeModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    TreeItem *parentItem = getItem(parent);
    bool l_status;

    // if position is -1, insert in the end
    if (position == -1)
    {
        position = parentItem->childCount();
    }

    beginInsertRows(parent, position, position + rows - 1);
    l_status = parentItem->insertChildren(position, rows, rootItem->columnCount());
    endInsertRows();

    return l_status;
}

bool TreeModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginRemoveColumns(parent, position, position + columns - 1);
    success = rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if (rootItem->columnCount() == 0)
        removeRows(0, rowCount());

    return success;
}


bool TreeModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    TreeItem *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}


bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    TreeItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result)
        emit dataChanged(index, index, {role});

    return result;
}


bool TreeModel::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

// # # # # Not part of the QAbstractItemModel inheritance

bool TreeModel::insertRow(const int a_position, const QModelIndex &a_parent,
                          const QString& a_col0Data, const QString& a_col1Data)
{
    TreeItem *parentItem = getItem(a_parent);
    bool l_status;
    int l_pos = a_position;

    // if position is -1, insert in the end
    if (a_position == -1)
    {
        l_pos = parentItem->childCount();
    }

    beginInsertRows(a_parent, a_position, a_position);
    l_status = parentItem->insertChildItem(l_pos, a_col0Data, a_col1Data);
    endInsertRows();

    emit resizeColumnToContent(1);

    return l_status;
}


void TreeModel::notifyOfDataInertion(const int a_mxTelemetryTypeName,
                                     const QString& a_mxTelemetryTypeInnstance) noexcept
{
    emit notifyOfDataInsertionSignal(QPair<const int, const QString>(a_mxTelemetryTypeName,
                                                                     a_mxTelemetryTypeInnstance));
}






















