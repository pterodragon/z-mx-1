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



#ifndef BASICCHARTVIEW_H
#define BASICCHARTVIEW_H

#include "QtCharts"

class ZmHeapTelemetry;

class BasicChartView : public QChartView
{
public:
    BasicChartView(QChart *chart, QWidget *parent = nullptr);
    virtual ~BasicChartView() override;

    void setUpdateFunction( std::function<void(BasicChartView* a_this, void* a_mxTelemetryMsg)>   a_lambda );

    //public only for testing
    int m_verticalAxesRange;

public slots:
    void updateData(ZmHeapTelemetry a_pair);


protected:
    // for now
    QSize sizeHint() const override final;
    QLineSeries *m_seriesVertical;
    QLineSeries *m_seriesHorizontal;

    // update data
    std::function<void(BasicChartView* a_this, void* a_mxTelemetryMsg)> m_lambda;

private:
//    int m_verticalAxesRange;
};

#endif // BASICCHARTVIEW_H


