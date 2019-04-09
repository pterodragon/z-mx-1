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


#ifndef BASICCHARTCONTROLLER_H
#define BASICCHARTCONTROLLER_H

#include "src/controllers/BasicController.h"

class BasicChartModel;
class BasicChartView;


class BasicChartController : public BasicController
{
public:
    BasicChartController(DataDistributor& a_dataDistributor,
                         const int a_associatedTelemetryType,
                         const QString& a_associatedTelemetryInstanceName,
                         QObject* a_parent);
    virtual ~BasicChartController()       override;

    virtual void* getModel()              override final;
    virtual QAbstractItemView* getView()  override final;

    const QString getClassName() const noexcept;


    QWidget* initView(bool a_reachedMaxAllowedViews,
                             QWidget* a_parent /* <-- should be the dockWidget */)  noexcept;
    void finalizeView(QWidget* a_view) noexcept;
    void finalizeView(BasicChartView* a_view) noexcept;

    void updateViewCharts() noexcept;

private:
    const QString* m_className;
    int m_maxViewsAllowed;

protected:
    BasicChartModel* m_basicChartModel;
    QVector<BasicChartView*>* m_viewsContainer;
};

#endif // BASICCHARTCONTROLLER_H
