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


#ifndef TREEMODEL_H
#define TREEMODEL_H


#include <QAbstractItemModel>

class TreeItem;

// editable tree model
// http://doc.qt.io/qt-5/qtwidgets-itemviews-editabletreemodel-example.html

// about qt models
// http://doc.qt.io/qt-5/model-view-programming.html#model-classes

// http://doc.qt.io/qt-5/qabstractitemmodel.html#details
// http://doc.qt.io/qt-5/model-view-programming.html#model-subclassing-reference
// http://doc.qt.io/qt-5/model-view-programming.html#models
class TreeModel : public QAbstractItemModel//, QtTreeModelInterface
{
    Q_OBJECT

public:
    explicit TreeModel(const char* a_header0, QObject *parent = nullptr);
    ~TreeModel() override;

    // provide support for read only tree  - Begin//
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    /**
     * @brief returns the number of child items for the TreeItem that corresponds
     * to a given model index, or the number of top-level items if an invalid index
     *  is specified
     * @param parent
     * @return
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    // provide support for read only tree  - End//


    TreeItem* getRootItem() const noexcept;

    // provide support for editing and resizing  - Begin//
    // http://doc.qt.io/qt-5/qtwidgets-itemviews-editabletreemodel-example.html

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    bool setHeaderData(int section, Qt::Orientation orientation,
                       const QVariant &value, int role = Qt::EditRole) override;

    bool insertColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;

    // # # # # Not part of the QAbstractItemModel inheritance
    bool insertRow(const int position, const QModelIndex &parent,
                   const QString& a_col0Data, const QString& a_col1Data);

    // provide support for editing and resizing  - End//

    // for other usages
    bool isParentIsRootItem(const QModelIndex &index) const;
    void notifyOfDataInertion(const int, const QString&) noexcept;

signals:
    void resizeColumnToContent(int columns);

    /**
     * @brief send pair of MxTelemetryType and MxTelemetryInstance
     */
    void notifyOfDataInsertionSignal(const QPair<const int,const QString>);

private:
    TreeItem *rootItem;

    // provide support for editing and resizing  - Begin//
    TreeItem *getItem(const QModelIndex &index) const;

    // provide support for editing and resizing  - End//
};


#endif // TREEMODEL_H
