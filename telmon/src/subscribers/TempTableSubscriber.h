















#ifndef TEMPTABLESUBSCRIBER_H
#define TEMPTABLESUBSCRIBER_H

#include "QObject"
#include "DataSubscriber.h"
#include <functional>

class ZmBitmap;
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

    void setUpdateFunction( std::function<void(TempTableSubscriber* a_this,
                                               void* a_mxTelemetryMsg)> a_lambda );

    std::string getCurrentTime() const noexcept;

    QString ZmBitmapToQString(const ZmBitmap& a_zmBitMap) const noexcept;

    QString ZiIPTypeToQString(const ZiIP& a_ZiIP) const noexcept;

    QString ZiCxnFlagsTypeToQString(const uint32_t a_ZiCxnFlags) const noexcept;

    /**
     * @brief  called during the update sequence to make sure current
     *         subscriber is associated to table.
     *         (table <--> assocaite; publisher <--> registere)
     *         if subscriber is not associated and update is called,
     *         something went wrong in the logic, because what are
     *         we updating ? it just a waste of cpu
     * @return true if assiciated
     */
    bool isAssociatedWithTable() const noexcept;

    /**
     * @brief the subscriber is called for every message of type
     *        for example, there can be 3 different Heaps
     *        that is decided in run time and per server
     *        So we make sure that the specific type corresponds
     *        To the table
     * @return
     */
    bool isTelemtryInstanceNameMatchsTableName(const QString& a_instanceName) const noexcept;


signals:
    // must be compatible with qRegisterMetaType in constructor!
    void updateDone(QLinkedList<QString>);

private:
        const QString m_name;
        QString m_tableName;
        std::stringstream *m_stream;


        // update function
        //void (*m_updateFunc)(TempTableSubscriber* a_this, void* a_mxTelemetryMsg);
        std::function<void(TempTableSubscriber* a_this, void* a_mxTelemetryMsg)> m_lambda;
};

#endif // TEMPTABLESUBSCRIBER_H




















