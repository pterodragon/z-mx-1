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


#include "src/views/raw/BasicChartView.h"
#include "BasicChartController.h"
#include "src/models/raw/BasicChartModel.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "src/factories/MxTelemetryTypeWrappersFactory.h"
#include "src/distributors/DataDistributor.h"
#include "src/subscribers/ChartSubscriber.h"




BasicChartController::BasicChartController(DataDistributor& a_dataDistributor,
                                           const int a_associatedTelemetryType,
                                           const QString& a_associatedTelemetryInstanceName,
                                           QObject* a_parent):
    BasicController(a_dataDistributor, a_parent),
    m_className(new QString("BasicChartController::" +
                            MxTelemetryGeneralWrapper::fromMxTypeValueToName(a_associatedTelemetryType)
                            + "::" +
                            a_associatedTelemetryInstanceName)),
    m_basicChartModel(new BasicChartModel(a_associatedTelemetryType,
                                          a_associatedTelemetryInstanceName)),
    m_viewsContainer(new QVector<BasicChartView*>)
{
    //qInfo() << QString("::" + *m_className);

    // number of views policy: number of fields in charts without "none" field
    m_maxViewsAllowed = MxTelemetryTypeWrappersFactory::getInstance().
            getMxTelemetryWrapper(m_basicChartModel->getAssociatedTelemetryType())->getChartList().size() - 1;

    //subscribe model-subscriber
    m_dataDistributor.subscribe(m_basicChartModel->getAssociatedTelemetryType(),
                                m_basicChartModel->getSubscriber());

}


BasicChartController::~BasicChartController()
{
    //qInfo() << QString("::~" + *m_className);

    // order of init/finialize(opposite direction):
    // Model -> subscriber -> subscribe(subscriber)
    // Model is responsible for deleting subscriber
    m_dataDistributor.unsubscribe(m_basicChartModel->getAssociatedTelemetryType(),
                                  m_basicChartModel->getSubscriber());

    delete m_basicChartModel;
    m_basicChartModel = nullptr;

    // delete views
    for (int i = 0; i < m_viewsContainer->size(); ++i) {
        finalizeView((*m_viewsContainer)[i]);
    }
    delete m_viewsContainer;
    m_viewsContainer = nullptr;

    delete m_className;
    m_className = nullptr;
}


void* BasicChartController::getModel()
{
    return m_basicChartModel;
}


QAbstractItemView* BasicChartController::getView()
{
    // todo
    return nullptr;
}


const QString BasicChartController::getClassName() const noexcept
{
    return *m_className;
}


QWidget* BasicChartController::initView(bool a_reachedMaxAllowedViews,
                                    QWidget* a_parent)  noexcept
{
    // sanity check:
    // because when we insert the last allowed view
    // we already return a_reachedMaxAllowedViews <- true
    // and this function should not be called until the user has closed
    // at least one view
    a_reachedMaxAllowedViews = (m_viewsContainer->size() == m_maxViewsAllowed);
    if (a_reachedMaxAllowedViews)
    {
        qCritical() << *m_className
                  << __PRETTY_FUNCTION__
                  << "called when max number of views already active"
                     "invalid sequence, doing nothing";
        return nullptr;
    }
    auto* l_newChartView = new BasicChartView(*m_basicChartModel,
                                              QString("BasicChartView::" +
                                                      MxTelemetryGeneralWrapper::fromMxTypeValueToName(m_basicChartModel->getAssociatedTelemetryType())
                                                      + "::" +
                                                      m_basicChartModel->getAssociatedTelemetryInstanceName()),
                                              a_parent);
    m_viewsContainer->append(l_newChartView);
    a_reachedMaxAllowedViews = (m_viewsContainer->size() == m_maxViewsAllowed);
    return l_newChartView;
}

void BasicChartController::finalizeView(QWidget* a_view) noexcept
{
    auto* l_result = dynamic_cast<BasicChartView*>(a_view);
    if (l_result == nullptr) {
        qCritical() << *m_className
                    << __PRETTY_FUNCTION__
                    << "given param is not view!!";
        return;
    }
    finalizeView(l_result);
}


void BasicChartController::finalizeView(BasicChartView* a_view) noexcept
{
    const auto l_pos = m_viewsContainer->indexOf(a_view);
    if (l_pos == -1)
    {
        qCritical() << *m_className
                    << __PRETTY_FUNCTION__
                    << "called with view that does not exists in container"
                       "invalid sequence, doing nothing";
    }

    (*m_viewsContainer)[l_pos]->setParent(nullptr);
    delete (*m_viewsContainer)[l_pos];
    (*m_viewsContainer)[l_pos] = nullptr;
    m_viewsContainer->remove(l_pos);
}

/**
 * @brief BasicChartController::updateViewCharts
 *
 * Objects and goals:
 * 1. DataSubscriber : part of the update sequence, gets the corresponding data
 * and forward it to model
 * 2. Model : stores the data
 * 3. View : Only draws data, get it from model
 * 4. Controller: manager of interaction between above objects
 *
 * The problem: update phase is much faster than drawing phase
 * 1. can we lose data? Yes, only last data drawn by View is accounted.
 * --> and id there are no views? then: Time(drawing phase) = 0
 * that is, update thread will keep pushing data in.
 *
 * Solution:
 * 1. Model data container has two modes:
 * A. NOT_READ_BY_VIEW (= initial)
 * B. READ_BY_VIEW
 *
 *
 * DS send singal to main thread when finished inserting data
 * Two modes: NOT_READ_BY_VIEW, READ_BY_VIEW
 *
 *
 *
 */
void BasicChartController::updateViewCharts() noexcept
{
    for (int i = 0; i < m_viewsContainer->size(); i++) {
        (*m_viewsContainer)[i]->updateChart();
    }
}


