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


#ifndef CHARTVIEWCONTEXTMENU_H
#define CHARTVIEWCONTEXTMENU_H

#include "QMenu"
class BasicChartView;

class ChartViewContextMenu : public QMenu
{
    Q_OBJECT
public:
    ChartViewContextMenu(BasicChartView& a_view, const QString &title, QWidget *parent = nullptr);
    ~ChartViewContextMenu();

public slots:
    void setXAxisMinAction();
    void setXAxisMaxAction();

protected:
    BasicChartView& m_view;

    static const int X_AXIS_MIN_FIELD_MAX_VALUE = 55;
    static const int X_AXIS_MIN_FIELD_MIN_VALUE = 0;
    static const int X_AXIS_FACTOR = 5;
    static const int X_AXIS_MAX_FIELD_MAX_VALUE = 60;
    static const int X_AXIS_MAX_FIELD_MIN_VALUE = 5;

    QMenu* m_dataMenu;
    QMenu* m_menuYLeft;
    QMenu* m_menuYRight;
    QMenu* m_zoomInMenu;
    QMenu* m_xMinMenu;
    QMenu* m_xMaxMenu;
    QMenu* m_refreshRateMenu;

    void setActionsStatus(QMenu& a_menu, const bool a_status) noexcept;
};

#endif // CHARTVIEWCONTEXTMENU_H
