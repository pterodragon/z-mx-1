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




#include "ChartWidgetDockWindowController.h"
#include "QDebug"
#include "src/widgets/ChartDockWidget.h"
#include "src/controllers/BasicChartController.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "src/factories/MxTelemetryTypeWrappersFactory.h"
#include "src/models/raw/ChartWidgetDockWindowModel.h"

ChartWidgetDockWindowController::ChartWidgetDockWindowController(DataDistributor& a_dataDistributor,
                                                                 QObject* a_parent):
    DockWindowController(a_dataDistributor, QString("ChartWidgetDockWindowController"), a_parent),
    m_model(new ChartWidgetDockWindowModel() ),
    m_DB(new QVector<QMap<QString, BasicChartController*>*>)
{
    qDebug() << "    ChartWidgetDockWindowController()";

    // int internal containers in m_DB
    for (int i = 0; i < MxTelemetryGeneralWrapper::mxTypeSize(); i++) {
        m_DB->insert(i, new QMap<QString, BasicChartController*>);
    }

}


ChartWidgetDockWindowController::~ChartWidgetDockWindowController()
{
    qDebug() << "    ~ChartWidgetDockWindowController() begin";

    // clean DB
    for (int i = 0; i < m_DB->size(); ++i) {
        //clearn data inside internal container
        auto k = m_DB->value(i)->begin();
        while (k != m_DB->value(i)->end())
        {
            delete k.value();
            k.value() = nullptr;
            k++;
        }
        // clean internal contianer
        delete (*m_DB)[i];
        (*m_DB)[i] = nullptr;
    }
    delete m_DB;
    m_DB = nullptr;

    delete m_model;
    m_model = nullptr;

    qDebug() << "    ~ChartWidgetDockWindowController() end";
}


void* ChartWidgetDockWindowController::getModel()
{
    return nullptr;
}


QAbstractItemView*  ChartWidgetDockWindowController::getView()
{
    return nullptr;
}


// DockWindowController interface
std::pair<QDockWidget*, int> ChartWidgetDockWindowController::handleUserSelection(
        const QList<QDockWidget *>&,
        const QString& a_mxTelemetryInstanceName,
        const int a_mxTelemetryType) noexcept
{
    auto* l_controller = getController(a_mxTelemetryType, a_mxTelemetryInstanceName);
    auto* l_view = l_controller->initView();

    if (l_view == nullptr)
    {
        return std::make_pair(nullptr, ACTIONS::NO_ACTION);
    }

    m_model->addToActiveCharts(qMakePair(l_controller, l_view));
    rearrangeXAxisLablesOnAllViews();


    const auto a_action = ACTIONS::ADD_TO_RIGHT;
    auto* a_dockWidget = new ChartDockWidget(a_mxTelemetryType,
                                       a_mxTelemetryInstanceName,
                                       l_view,
                                       l_controller,
                                       ACTIONS::REMOVE_FROM_RIGHT,
                                       nullptr  //parent will be set later
                                       );

    connect(static_cast<ChartDockWidget*>(a_dockWidget),
            &ChartDockWidget::closeAction,
            this,
            [this](int a_type,
                   void* a_controller,
                   void* a_view)
    {
        this->m_model->removeFromActiveCharts(qMakePair(a_controller, a_view));
        this->rearrangeXAxisLablesOnAllViews();
        emit closeDockWidget(a_type);
    });

    return std::make_pair(a_dockWidget, a_action);
}


/**
 * @brief This function will be call in the init sequence, just after the user press connect
 *        its role is to create chart controller for each instance in the tree
 * @param mxTelemetryType
 * @param mxTelemetryInstanceName
 */
void ChartWidgetDockWindowController::initSubController(const int a_mxTelemetryType,
                                                        const QString& a_mxTelemetryInstanceName) noexcept
{
    // check if given mxType has chart functionality
    if (!MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(a_mxTelemetryType)->isChartFunctionalityEnabled())
    {
        return;
    }

    // sanity check, check that controller does not already exist
    // why ? because this function should be called only per controller
    if (getController(a_mxTelemetryType, a_mxTelemetryInstanceName) != nullptr)
    {
        qCritical() << *m_className
                    << __PRETTY_FUNCTION__
                    << "Invalid sequence, container already exists! a_mxTelemetryType:"
                    << a_mxTelemetryType
                    << "mxTelemetryInstanceName:"
                    << a_mxTelemetryInstanceName;
        return;
    }

    // init controller
    (*m_DB)[a_mxTelemetryType]->insert(a_mxTelemetryInstanceName,
                                       new BasicChartController(m_dataDistributor,
                                                                a_mxTelemetryType,
                                                                a_mxTelemetryInstanceName,
                                                                this));
}


BasicChartController* ChartWidgetDockWindowController::getController(
                            const int a_mxTelemetryType,
                            const QString& a_mxTelemetryInstanceName) noexcept
{
    // sanity check
    if (!MxTelemetryGeneralWrapper::isMxTypeValid(a_mxTelemetryType))
    {
        qCritical() << __PRETTY_FUNCTION__
                    << "Invalid mxTelemetryType:"
                    << a_mxTelemetryType
                    << "mxTelemetryInstanceName:"
                    << a_mxTelemetryInstanceName;
        return nullptr;
    }

    // get from continer
    return (*m_DB).at(a_mxTelemetryType)->value(a_mxTelemetryInstanceName, nullptr);
}


BasicChartController* ChartWidgetDockWindowController::getController(const int a_mxTelemetryType,
                                                                     const QString& a_mxTelemetryInstanceName) const noexcept
{
    // sanity check
    if (!MxTelemetryGeneralWrapper::isMxTypeValid(a_mxTelemetryType))
    {
        qCritical() << __PRETTY_FUNCTION__
                    << "Invalid mxTelemetryType:"
                    << a_mxTelemetryType
                    << "mxTelemetryInstanceName:"
                    << a_mxTelemetryInstanceName;
        return nullptr;
    }

    return (*m_DB).at(a_mxTelemetryType)->value(a_mxTelemetryInstanceName, nullptr);
}


void ChartWidgetDockWindowController::rearrangeXAxisLablesOnAllViews() noexcept
{
    auto l_lastTwo = this->m_model->getLastTwoCharts();
    l_lastTwo.first.first->setChartXAxisVisibility(l_lastTwo.first.second, false);
    l_lastTwo.second.first->setChartXAxisVisibility(l_lastTwo.second.second, true);
}


bool ChartWidgetDockWindowController::showPolicy(const int a_mxType,
                                                 const QString& a_mxInstance) const noexcept
{
    const auto* const l_controller = getController(a_mxType, a_mxInstance);
    if (!l_controller) {return true;}
    return !(l_controller->isReachedMaxViewAllowed());
}

