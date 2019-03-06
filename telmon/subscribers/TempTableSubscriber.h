















#ifndef TEMPTABLESUBSCRIBER_H
#define TEMPTABLESUBSCRIBER_H

#include "QObject"
#include "DataSubscriber.h"
#include <functional>

class ZiIP;

class TempTableSubscriber : public QObject, public DataSubscriber
{
    Q_OBJECT

public:
    TempTableSubscriber(const QString a_name);
    virtual ~TempTableSubscriber();

    virtual const QString& getName() const noexcept;

    virtual void update(void* a_mxTelemetryMsg);

    // this two function are used to set the specific table the
    // subscriber is connected too
    virtual const QString& getTableName() const noexcept;
    virtual void setTableName(QString a_name) noexcept;

    void setUpdateFunction( std::function<void(TempTableSubscriber* a_this, void* a_mxTelemetryMsg)> a_lambda );

    std::string getCurrentTime() const noexcept;

    QString ZiIPTypeToQString(const ZiIP a_ZiIP) const noexcept;

    const QString m_name;
    QString m_tableName;

signals:
    // must be compatible with qRegisterMetaType in constructor!
    void updateDone(QLinkedList<QString>);

private:
//        const QString m_name;
//        QString m_tableName;

        // update function
        //void (*m_updateFunc)(TempTableSubscriber* a_this, void* a_mxTelemetryMsg);
        std::function<void(TempTableSubscriber* a_this, void* a_mxTelemetryMsg)> m_lambda;
};

#endif // TEMPTABLESUBSCRIBER_H




















