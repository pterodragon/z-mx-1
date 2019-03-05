


#include "models/wrappers/TempDockWidget.h"
#include "QDebug"
#include "QTableWidget"
#include "QLayout"
#include "models/wrappers/TempModelWrapper.h"
#include "QCloseEvent"

TempDockWidget::TempDockWidget(const QString &title, TempModelWrapper* a_tempModelWrapper,
                               const QString a_mxTelemetryTypeName,
                               const QString a_mxTelemetryInstanceNameQWidget,
                               QWidget *parent):
    QDockWidget (title, parent),
    m_tempModelWrapper(a_tempModelWrapper),
    m_mxTelemetryTypeName(a_mxTelemetryTypeName),
    m_mxTelemetryInstanceName(a_mxTelemetryInstanceNameQWidget)
{

}


TempDockWidget::~TempDockWidget()
{
    qDebug() << "    ~TempDockWidget() -BEGIN";
    //unsubscribe
    if (m_tempModelWrapper)
    {
        m_tempModelWrapper->unsubscribe(m_mxTelemetryTypeName, m_mxTelemetryInstanceName);
        //m_tempModelWrapper = nullptr;
    }

    // dont delete the table!
    if (this->widget() != nullptr) {this->widget()->setParent(nullptr);}
    setWidget(nullptr);
    qDebug() << "    ~TempDockWidget() -END";

}


const QString& TempDockWidget::getMxTelemetryTypeName() const noexcept
{
    return m_mxTelemetryTypeName;
}


const QString& TempDockWidget::getMxTelemetryInstanceName() const noexcept
{
    return m_mxTelemetryInstanceName;
}


void TempDockWidget::closeEvent(QCloseEvent *event)
{

    event->accept();

    /**
     * I had some wired problem, which may be related to linkage order
     * even when i tell the program to close this DockWidget explicitly
     * it will not call the destrcutor, but only in the end
     */
    delete this;
}




