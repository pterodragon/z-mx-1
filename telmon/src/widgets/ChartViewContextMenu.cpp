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


#include "ChartViewContextMenu.h"
#include "src/views/raw/BasicChartView.h"
#include "QTimer"

ChartViewContextMenu::ChartViewContextMenu(BasicChartView& a_view, const QString &title, QWidget *parent):
    QMenu(title, parent),
    m_view(a_view)
{

    // start/stop action
    {
        addAction("Stop", this, [this]() {
            this->m_view.handleStartStopFunctionality();
        });
    }

    addSeparator();

    // Data menu
    {
        m_dataMenu = new QMenu(QObject::tr("Data"), this);
        m_menuYLeft = new QMenu("Y Left", m_dataMenu);
        m_dataMenu->addMenu(m_menuYLeft);
        m_menuYRight = new QMenu("Y Right", m_dataMenu);
        m_dataMenu->addMenu(m_menuYRight);

        // init series menu actions

        for (int i = 0; i < m_view.m_chartFields.size(); ++i) {

            m_menuYLeft->addAction( m_view.m_chartFields.at(i), [this, i]() {
                m_view.changeSeriesData(BasicChartView::SERIES::SERIES_LEFT ,i, m_menuYLeft);
            });

            m_menuYRight->addAction( m_view.m_chartFields.at(i), [this, i]() {
                m_view.changeSeriesData(BasicChartView::SERIES::SERIES_RIGHT ,i, m_menuYRight);
            });
        }

        // set default option disabled for each axis menu
        for (unsigned int l_series = 0; l_series < BasicChartView::SERIES::SERIES_N; l_series++)
        {
            (l_series == BasicChartView::SERIES::SERIES_LEFT)
                    ? ( m_menuYLeft->actions(). at(m_view.m_activeData[l_series])->setDisabled(true))
                    : ( m_menuYRight->actions().at(m_view.m_activeData[l_series])->setDisabled(true));
        }


        addMenu(m_dataMenu);
    }

    // Zoom In menu
    {
        m_zoomInMenu = new QMenu(QObject::tr("Zoom In/Out"), this);


        m_xMinMenu = new QMenu("x min", m_zoomInMenu);
        for (int i = X_AXIS_MIN_FIELD_MIN_VALUE; i <= X_AXIS_MIN_FIELD_MAX_VALUE;)
        {
            int k = i;
            if (i == 0) { k = 1;}
            auto l_action =  m_xMinMenu->addAction(QString::number(i), this, &ChartViewContextMenu::setXAxisMinAction);
            l_action->setData(QVariant(k));
            i += X_AXIS_FACTOR;
        }
        m_xMinMenu->actions().at(0)->setEnabled(false);
        m_zoomInMenu->addMenu(m_xMinMenu);

        m_xMaxMenu = new QMenu("x max", m_zoomInMenu);
        for (int i = X_AXIS_MAX_FIELD_MIN_VALUE; i <= X_AXIS_MAX_FIELD_MAX_VALUE;)
        {
            auto l_action =  m_xMaxMenu->addAction(QString::number(i),this, &ChartViewContextMenu::setXAxisMaxAction);

            l_action->setData(QVariant(i));
            i += X_AXIS_FACTOR;
        }
        m_xMaxMenu->actions().at(m_xMaxMenu->actions().size()-1)->setEnabled(false);
        m_zoomInMenu->addMenu(m_xMaxMenu);

        addMenu(m_zoomInMenu);
    }

    // refresh rate menu
    {
        m_refreshRateMenu = new QMenu(QObject::tr("Refresh Rate"), this);

        m_refreshRateMenu->addAction("1 second", [this]() {
            m_view.m_timer->start(1000);
            m_refreshRateMenu->actions().at(0)->setDisabled(true);
            m_refreshRateMenu->actions().at(1)->setDisabled(false);
        });
        m_refreshRateMenu->actions().at(0)->setDisabled(true);

        m_refreshRateMenu->addAction("5 second", [this]() {
            m_view.m_timer->start(5000);
            m_refreshRateMenu->actions().at(0)->setDisabled(false);
            m_refreshRateMenu->actions().at(1)->setDisabled(true);
        });

        addMenu(m_refreshRateMenu);
    }

    addSeparator();

    // close action
    {
        addAction("Close", [this]() {
                    emit m_view.closeAction();
                });
    }
}


ChartViewContextMenu::~ChartViewContextMenu()
{

}


void ChartViewContextMenu::setXAxisMinAction()
{
    //sanity check
    auto* l_sender =  static_cast<QAction*>(QObject::sender());
    if (!l_sender) {return;}

    int l_val = l_sender->data().toInt();
    auto& l_axis = m_view.getAxes(BasicChartView::CHART_AXIS::X);
    //sanity check
    const auto l_x_Max = static_cast<int>(l_axis.max());
    if (l_val >= l_x_Max) {l_val = l_x_Max - X_AXIS_FACTOR;}
    l_axis.setMin(l_val);

    // updating the Y Axis
    m_view.updateYRangesManually();

    // enable all actions (including the previous disabled one)
    // and disable current
    setActionsStatus(*m_xMinMenu, true);
    l_sender->setEnabled(false);
}


void ChartViewContextMenu::setXAxisMaxAction()
{
    //sanity check
    auto* l_sender =  static_cast<QAction*>(QObject::sender());
    if (!l_sender) {return;}

    int l_val = l_sender->data().toInt();
    auto& l_axis = m_view.getAxes(BasicChartView::CHART_AXIS::X);
    //sanity check
    const auto l_x_Min = static_cast<int>(l_axis.min());
    if (l_val <= l_x_Min) {l_val = l_x_Min + X_AXIS_FACTOR;}
    l_axis.setMax(l_val);

    // updating the Y Axis
    m_view.updateYRangesManually();

    // enable all actions (including the previous disabled one)
    // and disable current
    setActionsStatus(*m_xMaxMenu, true);
    l_sender->setEnabled(false);
}


void ChartViewContextMenu::setActionsStatus(QMenu& a_menu, const bool a_status) noexcept
{
    foreach (auto action, a_menu.actions())
    {
        action->setEnabled(a_status);
    }
}


