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


#include "BasicTableWidget.h"
#include "QDebug"
#include "QLinkedList"

BasicTableWidget::BasicTableWidget(QWidget *parent):
    QTableWidget(parent)
{

}

BasicTableWidget::BasicTableWidget(int rows, int columns, QWidget *parent):
    QTableWidget(rows, columns, parent)
{

}


BasicTableWidget::BasicTableWidget(const QList<QString> a_horizontalHeader, const QList<QString> a_verticalHeader, const QString a_objectName, QWidget *parent):
    QTableWidget(a_verticalHeader.size(), a_horizontalHeader.size(), parent)
{
    setObjectName(a_objectName);
    setHorizontalHeaderLabels(a_horizontalHeader);
    setVerticalHeaderLabels(a_verticalHeader);
}


BasicTableWidget::~BasicTableWidget()
{
    qDebug() << "~TempTable()" << objectName();
}

// todo should be provided by lambda function
void BasicTableWidget::updateData(QLinkedList<QString> a_list)
{
    //sanity check
    // a list larger than rows?

    int i = 0;
    foreach (auto list, a_list)
    {
        setItem(i, 0, (new QTableWidgetItem(a_list.takeFirst())) );
        i++;
    }

//    emit dataChanged(QModelIndex(), QModelIndex());
}


