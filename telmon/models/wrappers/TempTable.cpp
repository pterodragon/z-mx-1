
#include "models/wrappers/TempTable.h"
#include "QLinkedList"


TempTable::TempTable(QWidget *parent):
    QTableWidget(parent)
{

}

TempTable::TempTable(int rows, int columns, QWidget *parent):
    QTableWidget(rows, columns, parent)
{

}


TempTable::TempTable(const QList<QString> a_horizontalHeader, const QList<QString> a_verticalHeader, const QString a_objectName, QWidget *parent):
    QTableWidget(a_verticalHeader.size(), a_horizontalHeader.size(), parent)
{
    setObjectName(a_objectName);
    setHorizontalHeaderLabels(a_horizontalHeader);
    setVerticalHeaderLabels(a_verticalHeader);
}


TempTable::~TempTable()
{
    qDebug() << "~TempTable()" << objectName();
}

// todo should be provided by lambda function
void TempTable::updateData(QLinkedList<QString> a_list)
{
    //sanity check
    // a list larger than rows?

    //qDebug() << "TempTable::updateData";
    int i = 0;
    foreach (auto list, a_list)
    {
        setItem(i, 0, (new QTableWidgetItem(a_list.takeFirst())) );
        i++;
    }
    emit dataChanged(QModelIndex(), QModelIndex());
}
