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
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "src/factories/MxTelemetryTypeWrappersFactory.h"
#include "src/distributors/DataDistributor.h"
#include "src/subscribers/ChartSubscriber.h"


BasicChartController::BasicChartController(DataDistributor& a_dataDistributor,
                                           const int a_associatedTelemetryType,
                                           const QString& a_associatedTelemetryInstanceName,
                                           QObject* a_parent):
    BasicController(a_dataDistributor,
                    QString("BasicChartController::" +
                                               MxTelemetryGeneralWrapper::fromMxTypeValueToName(a_associatedTelemetryType)
                                               + "::" +
                                               a_associatedTelemetryInstanceName),
                    a_parent),
    m_basicChartModel(new BasicChartModel(a_associatedTelemetryType,
                                          a_associatedTelemetryInstanceName)),
    m_viewsContainer(new QVector<BasicChartView*>)
{
    qInfo() << QString("::" + *m_className);

    // number of views policy: number of fields in charts without "none" field
    m_maxViewsAllowed = MxTelemetryTypeWrappersFactory::getInstance().
            getMxTelemetryWrapper(m_basicChartModel->getAssociatedTelemetryType())->getChartList().size() - 1;

    //subscribe model-subscriber
    m_dataDistributor.subscribe(m_basicChartModel->getAssociatedTelemetryType(),
                                m_basicChartModel->getSubscriber());

}


BasicChartController::~BasicChartController()
{
    qInfo() << QString("::~" + *m_className);

    m_dataDistributor.unsubscribe(m_basicChartModel->getAssociatedTelemetryType(),
                                  m_basicChartModel->getSubscriber());

    delete m_basicChartModel;
    m_basicChartModel = nullptr;

    delete m_viewsContainer;
    m_viewsContainer = nullptr;
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


BasicChartView* BasicChartController::initView()  noexcept
{
    // sanity check:
    if (isReachedMaxViewAllowed())
    {
        qCritical() << *m_className
                  << __PRETTY_FUNCTION__
                  << "called when max number of views already active"
                     "invalid sequence, doing nothing";
        return nullptr;
    }

    auto l_newChartView = getChartView();

    if (!addView(l_newChartView))
    {
        qCritical() << *m_className
                  << __PRETTY_FUNCTION__
                  << "failed to add view";
        delete l_newChartView;
        l_newChartView = nullptr;
    }

    return l_newChartView;
}


bool BasicChartController::addView(BasicChartView* a_view) noexcept
{
    if (!m_viewsContainer->contains(a_view))
    {
        m_viewsContainer->prepend(a_view);
        return true;
    }
    return false;
}


bool BasicChartController::removeView(BasicChartView* a_view) noexcept
{
    return m_viewsContainer->removeOne(a_view);
}


bool BasicChartController::isReachedMaxViewAllowed() const noexcept
{
    return m_viewsContainer->size() == m_maxViewsAllowed;
}


BasicChartView* BasicChartController::getChartView() noexcept
{
    const auto l_mxType = m_basicChartModel->getAssociatedTelemetryType();
    const auto l_mxTypeName =  MxTelemetryGeneralWrapper::fromMxTypeValueToName(l_mxType);
    return new BasicChartView(*m_basicChartModel,
                              QString("BasicChartView::" +
                                      l_mxTypeName +
                                      "::" +
                                      m_basicChartModel->getAssociatedTelemetryInstanceName()),
                              nullptr,
                              *this);
}

