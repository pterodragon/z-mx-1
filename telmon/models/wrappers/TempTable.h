

#ifndef TEMPTABLE_H
#define TEMPTABLE_H



#include "QTableWidget"
#include "QDebug"
#include "distributors/DataDistributor.h"


class TempTable : public QTableWidget
{

public:
    explicit TempTable(QWidget *parent = nullptr);
    TempTable(int rows, int columns, QWidget *parent = nullptr);
    TempTable(int rows, int columns, const QString a_objectName, QWidget *parent = nullptr);
    TempTable(const QList<QString> a_horizontalHeader, const QList<QString> a_verticalHeader, const QString a_objectName, QWidget *parent = nullptr);
    virtual ~TempTable();

public slots:
    // should be provided by lambda function!
    void updateData(QLinkedList<QString> a_list);
};

#endif // TEMPTABLE_H
