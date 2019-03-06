



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


QString TempTableSubscriber::ZiIPTypeToQString(const ZiIP a_ZiIP) const noexcept
{
    //todo if readlAll clean the stream, no need to constrcut stream every time!!
    QTextStream l_out;
    a_ZiIP.print(l_out);
    return l_out.readAll();

    /** get the local ip -- OLD

    const uint32_t l_addrLocal = static_cast<uint32_t>(ntohl(l_data.localIP.s_addr));
    l_list.append(QString::number(
                      ZuBoxed(((static_cast<uint32_t>(l_addrLocal)) >> 24) & 1UL) << '.' <<
                      ZuBoxed(((static_cast<uint32_t>(l_addrLocal)) >> 16) & 1UL) << '.' <<
                      ZuBoxed(((static_cast<uint32_t>(l_addrLocal)) >> 8 ) & 1UL) << '.' <<
                      ZuBoxed(((static_cast<uint32_t>(l_addrLocal))      ) & 1UL)
                                 )
                  );
    */
}



