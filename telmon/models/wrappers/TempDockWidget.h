


#ifndef TEMPDOCKWIDGET_H
#define TEMPDOCKWIDGET_H

#include "QDockWidget"

class TempModelWrapper;

class TempDockWidget : public QDockWidget
{
public:
    TempDockWidget(const QString &title, TempModelWrapper* a_tempModelWrapper,
                   const QString a_mxTelemetryTypeName,
                   const QString a_mxTelemetryInstanceNameQWidget,
                   QWidget* parent = nullptr);
    virtual ~TempDockWidget();

    const QString& getMxTelemetryTypeName() const noexcept;
    const QString& getMxTelemetryInstanceName() const noexcept;
    virtual void closeEvent(QCloseEvent *event);

private:
    TempModelWrapper* m_tempModelWrapper;
    QString m_mxTelemetryTypeName;
    QString m_mxTelemetryInstanceName;
};

#endif // TEMPDOCKWIDGET_H
