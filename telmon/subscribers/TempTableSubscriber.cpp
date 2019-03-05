



#include "MxTelemetry.hpp"
#include "TempTableSubscriber.h"
#include "QLinkedList"
#include "QDebug"


TempTableSubscriber::TempTableSubscriber(const QString a_name):
    m_name(a_name),
    m_tableName(QString()) // set null QString
{

}


TempTableSubscriber::~TempTableSubscriber()
{

};


const QString& TempTableSubscriber::getName() const noexcept
{
    return m_name;
}


const QString& TempTableSubscriber::getTableName() const noexcept
{
    return m_tableName;
}


void TempTableSubscriber::setTableName(QString a_name) noexcept
{
    m_tableName = a_name;
}



void TempTableSubscriber::update(void* a_mxTelemetryMsg)
{

     m_lambda(this, a_mxTelemetryMsg);
     return;

    if (m_tableName.isNull() || m_tableName.isEmpty())
    {
        qWarning() << m_name << "is not registered to any table, please setTableName or unsubscribe, returning...";
        return;
    }

    //qDebug() << "tempTable::update!";
    //is there is a function which called updated all table values?
    // 1. https://stackoverflow.com/questions/13605141/best-strategy-to-update-qtableview-with-data-in-real-time-from-different-threads
    // 2. https://stackoverflow.com/questions/4031168/qtableview-is-extremely-slow-even-for-only-3000-rows
    //for now leave performace, just make it work
    const auto &l_data = (static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg))->as<MxTelemetry::Heap>();

    // make sure msg is of current instance
    if (getTableName() != QString((l_data.id.data())))
    {
        return;
    }

    //TODO - convert to pointer
    // QLinkedList
    // constant time insertions and removals.
    // iteration is the same as QList
    QLinkedList<QString> a_list;

    a_list.append(QString::number(l_data.cacheSize));
    a_list.append(QString::number(l_data.cpuset));
    a_list.append(QString::number(l_data.cacheAllocs));
    a_list.append(QString::number(l_data.heapAllocs));
    a_list.append(QString::number(l_data.frees));

   // a_list.append(QString::number(l_data.allocated));
    //a_list.append(QString::number(l_data.maxAllocated));
    a_list.append(QString::number(l_data.size));
    a_list.append(QString::number(l_data.partition));
    a_list.append(QString::number(l_data.sharded));

    a_list.append(QString::number(l_data.alignment));

    emit updateDone(a_list);
}



void TempTableSubscriber::setUpdateFunction( std::function<void(TempTableSubscriber* a_this, void* a_mxTelemetryMsg)> a_lambda ) {
    m_lambda = a_lambda;
}



