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



#ifndef CHARTDOCKWIDGET_H
#define CHARTDOCKWIDGET_H

#include "QDockWidget"
class BasicChartController;
class BasicChartView;

class ChartDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    ChartDockWidget(const int a_mxTelemetryType,
                    const QString& a_mxTelemetryInstance,
                    BasicChartView* a_setWidget,
                    BasicChartController* const a_controller,
                    const int a_closeActionType,
                    QWidget *parent);
    virtual ~ChartDockWidget() override;

    //public and const, so no need for getter and setters
    const int m_mxTelemetryType;
    const QString* m_mxTelemetryInstance;
    const QString* m_className;

    virtual void closeEvent(QCloseEvent *event) override;

signals:
    void closeAction(int,
                     void*, // controller
                     void*  // view
                     );

private:
    bool m_isTitleBarHidden;
    void hideTitleBar() noexcept;

    BasicChartController* const m_chartController; // DO NOT DELETE
    const int m_closeActionType;
};

#endif // CHARTDOCKWIDGET_H
