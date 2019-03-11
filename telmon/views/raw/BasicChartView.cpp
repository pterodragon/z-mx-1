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



#include "BasicChartView.h"


BasicChartView::BasicChartView(QChart *a_chart, QWidget *a_parent):
    QChartView(a_chart, a_parent),
    m_seriesVertical(new QLineSeries()),   // delete as part of QChart object destructor
    m_seriesHorizontal(new QLineSeries()),  // delete as part of QChart object destructor

    // TODO
    // 1. set default value
    // 2. change according touser behavior
    m_verticalAxesRange(10)
{
    qDebug() << "BasicChartView";

//    *m_seriesVertical << QPointF(1, 5) << QPointF(3, 7) << QPointF(7, 6) << QPointF(9, 7) << QPointF(12, 6)
//                 << QPointF(16, 7) << QPointF(18, 5);
//    *m_seriesHorizontal << QPointF(1, 3) << QPointF(3, 4) << QPointF(7, 3) << QPointF(8, 2) << QPointF(12, 3)
//                 << QPointF(16, 4) << QPointF(18, 3);



    // set series propertes
    auto l_series = new QAreaSeries(m_seriesVertical, m_seriesHorizontal);
    l_series->setName("CPU Usage");
    QPen pen(0x059605);
    pen.setWidth(2);
    l_series->setPen(pen);

    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
    gradient.setColorAt(0.0, 0x3cc63c);
    gradient.setColorAt(1.0, 0x26f626);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    l_series->setBrush(gradient);

    // QAreaSeries is deleted as part of part of QChart object destructor
    chart()->addSeries(l_series);

    chart()->createDefaultAxes();
    chart()->axes(Qt::Horizontal).first()->setRange(0, m_verticalAxesRange); // represeting 60 seconds
    chart()->axes(Qt::Vertical).first()->setRange(0, 100);

}


BasicChartView::~BasicChartView()
{
    qDebug() << "~BasicChartView";
    delete m_seriesVertical;
    delete m_seriesHorizontal;
    delete this->chart();
}


QSize BasicChartView::sizeHint() const
{
    return QSize(600,600);
}


// todo should be provided by lambda function
void BasicChartView::updateData(QLinkedList<QString> a_list)
{
    // get the series
    QAreaSeries *series = static_cast<QAreaSeries *>(chart()->series().first());

    // check not excedding the limits
    if (series->upperSeries()->count() == m_verticalAxesRange)
    {
        series->upperSeries()->remove(0);
        series->lowerSeries()->remove(0);
    }

    // shift all point left
    const int k = series->upperSeries()->count();
    qreal upper_x = 0;
    qreal upper_y = 0;
    qreal lower_x = 0;
    qreal lower_y = 0;

    for (int i = 0; i < k; i++)
    {
        //get curret point x
        upper_x = series->upperSeries()->at(i).x() - 2;
        upper_y = series->upperSeries()->at(i).y();
        series->upperSeries()->replace(i, upper_x, upper_y);

        series->lowerSeries()->replace(i,
                                       series->lowerSeries()->at(i).x() - 2,
                                       series->lowerSeries()->at(i).y());
    }

    int l_randomNumberForTesting =rand() % 100 ;
    series->setName("CPU Usage: " + QString::number(l_randomNumberForTesting));
    series->upperSeries()->append(QPointF(m_verticalAxesRange, l_randomNumberForTesting));

    //actually lower is always permenant, add until reach limit
    if (series->lowerSeries()->count() < m_verticalAxesRange)
    {
        series->lowerSeries()->append(QPointF(m_verticalAxesRange, 0));
    }

    qDebug() << "series->upperSeries().count()" << series->upperSeries()->count();
    qDebug() << "series->lowerSeries()->count()" << series->lowerSeries()->count();
    //series->upperSeries()->re



}


