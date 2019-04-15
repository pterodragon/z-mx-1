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


#include "ChartWidgetDockWindowModel.h"

#include "QVector"
#include "QDebug"


ChartWidgetDockWindowModel::ChartWidgetDockWindowModel():
    m_className(new QString("ChartWidgetDockWindowModel")),
    m_activeCharts(new QVector<QPair<BasicChartController*, BasicChartView*>>)
{

}

ChartWidgetDockWindowModel::~ChartWidgetDockWindowModel()
{
    // do not delete vector content
    delete m_activeCharts;
    m_activeCharts = nullptr;

    delete m_className;
    m_className = nullptr;
}


void ChartWidgetDockWindowModel::addToActiveCharts(QPair<BasicChartController*,
                                                   BasicChartView*> a_pair) noexcept
{
    m_activeCharts->append(a_pair);
}


void ChartWidgetDockWindowModel::removeFromActiveCharts(QPair<BasicChartController*,
                                                        BasicChartView*> a_pair) noexcept
{
    const auto l_result = m_activeCharts->indexOf(a_pair);
    if (l_result == -1)
    {
        qCritical() << *m_className
                    << __PRETTY_FUNCTION__
                    << "attempt to remove nonexist view";
        return;
    }
    m_activeCharts->remove(l_result);
}

void ChartWidgetDockWindowModel::removeFromActiveCharts(QPair<void*,
                                                        void*> a_pair) noexcept
{
    removeFromActiveCharts(qMakePair(
                               static_cast<BasicChartController*>(a_pair.first),
                               static_cast<BasicChartView*>(a_pair.second)
                               )
                           );
}


QPair<QPair<BasicChartController*,BasicChartView*>,
      QPair<BasicChartController*, BasicChartView*>>
ChartWidgetDockWindowModel::getLastTwoCharts() const noexcept
{
    QPair<QPair<BasicChartController*, BasicChartView*>,
          QPair<BasicChartController*, BasicChartView*>> l_result;

    const auto l_size = m_activeCharts->size();
    if (l_size > 1)
    {
        l_result = qMakePair((*m_activeCharts)[l_size -2 ],
                             (*m_activeCharts)[l_size - 1]);
    }

    else if (l_size == 1)
    {
        l_result = qMakePair(qMakePair(nullptr, nullptr),
                             (*m_activeCharts)[0]);
    }

    else
    {
        l_result = qMakePair(qMakePair(nullptr, nullptr),
                             qMakePair(nullptr, nullptr));
    }

    return l_result;
}






