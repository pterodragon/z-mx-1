



#include "MxTelemetry.hpp"
#include "TempTableSubscriber.h"
#include "QLinkedList"
#include "QDebug"

#include <sstream> //for ZmBitmapToQString


TempTableSubscriber::TempTableSubscriber(const QString a_name):
    m_name(a_name),
    m_tableName(QString()), // set null QString
    m_stream(new std::stringstream())
{

}


TempTableSubscriber::~TempTableSubscriber()
{
    delete m_stream;
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
}



void TempTableSubscriber::setUpdateFunction( std::function<void(TempTableSubscriber* a_this, void* a_mxTelemetryMsg)> a_lambda )
{
    m_lambda = a_lambda;
}



std::string TempTableSubscriber::getCurrentTime() const noexcept
{
    ZtDate::CSVFmt nowFmt;
    ZtDate now(ZtDate::Now);
    return (ZuStringN<512>() << now.csv(nowFmt)).data();
}


QString TempTableSubscriber::ZmBitmapToQString(const ZmBitmap& a_zmBitMap) const noexcept
{
    a_zmBitMap.print(*m_stream);
    const QString l_result = QString::fromStdString(m_stream->str());
    m_stream->str(std::string());
    m_stream->clear();
    return l_result;
}



QString TempTableSubscriber::ZiIPTypeToQString(const ZiIP& a_ZiIP) const noexcept
{
    a_ZiIP.print(*m_stream);
    const QString l_result = QString::fromStdString(m_stream->str());
    m_stream->str(std::string());
    m_stream->clear();
    return l_result;
}


QString TempTableSubscriber::ZiCxnFlagsTypeToQString(const uint32_t a_ZiCxnFlags) const noexcept
{
    *m_stream << ZiCxnFlags::Flags::print(a_ZiCxnFlags);
    const QString l_result = QString::fromStdString(m_stream->str());
    m_stream->str(std::string());
    m_stream->clear();
    return l_result;
}

bool TempTableSubscriber::isAssociatedWithTable() const noexcept
{
    if (m_tableName.isNull() || m_tableName.isEmpty())
    {
        qWarning() << m_name
                   << "is not registered to any table, please setTableName or unsubscribe, returning..."
                   << "table name is:"
                   << getTableName();
        return false;
    }
    return true;
}


bool TempTableSubscriber::isTelemtryInstanceNameMatchsTableName(const QString& a_instanceName) const noexcept
{
    return  (m_tableName ==  a_instanceName);
}



