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
#include "src/models/wrappers/ChartDockWidgetModelWrapper.h"
#include "src/controllers/BasicChartController.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"
#include "src/factories/MxTelemetryTypeWrappersFactory.h"



ChartWidgetDockWindowController::ChartWidgetDockWindowController(DataDistributor& a_dataDistributor,
                                                                 QObject* a_parent):
    DockWindowController(a_dataDistributor, " Chart", a_parent),
    m_chartDockWidgetModelWrapper(new ChartDockWidgetModelWrapper(a_dataDistributor)),
    m_className(new QString("ChartWidgetDockWindowController")),
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
    delete m_chartDockWidgetModelWrapper;

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

    delete m_className;
    m_className = nullptr;
    qDebug() << "    ~ChartWidgetDockWindowController() end";
}


void* ChartWidgetDockWindowController::getModel()
{
    return m_chartDockWidgetModelWrapper;
}


QAbstractItemView*  ChartWidgetDockWindowController::getView()
{
    return nullptr;
}


// DockWindowController interface
void ChartWidgetDockWindowController::handleUserSelection(unsigned int& a_action,
                                                          QDockWidget*& a_dockWidget,
                                                          const QList<QDockWidget *>& a_currentDockList,
                                                          const QString& a_mxTelemetryTypeName,
                                                          const QString& a_mxTelemetryInstanceName,
                                                          const int a_mxTelemetryType) noexcept
{
//    // constrct widget name
//    // for now, using same name for object and chart header
//    const QString l_objectName = a_mxTelemetryTypeName + "::" + a_mxTelemetryInstanceName + m_dockWindowName;

//    // is dock widget already exists?
//    QDockWidget* l_dock = nullptr;
//    bool l_contains = isDockWidgetExists(a_currentDockList, l_objectName, l_dock);

//    // handle case that dock widget already exists
//    if (l_contains)
//    {
//        qDebug() << m_dockWindowName << l_objectName << "already exists";
//        // out policy for table, only one at a time
//        a_action = DockWindowController::ACTIONS::NO_ACTION;
//        a_dockWidget = nullptr;
//        l_dock->activateWindow();
//        return;
//    }

//    qDebug() << "constrcuting" << m_dockWindowName << l_objectName;


//    //handle case that dock widget not exists
//    a_action = DockWindowController::ACTIONS::ADD_TO_RIGHT;

//    // construct the new dock
//    a_dockWidget = new BasicDockWidget(l_objectName,
//                                  m_chartDockWidgetModelWrapper,
//                                  a_mxTelemetryTypeName,
//                                  a_mxTelemetryInstanceName,
//                                  // notice about the parent - will be set later
//                                  nullptr);


//    static_cast<BasicDockWidget*>(a_dockWidget)->hideTitleBar();

//    // create the chart view and subscribe
//    QChartView *l_chartView = m_chartDockWidgetModelWrapper->initChartWidget(a_mxTelemetryTypeName,
//                                                                          a_mxTelemetryInstanceName);

//    // associate dock widget with chart view
//    a_dockWidget->setWidget(l_chartView);

    // * * * new * * *
        // construct the new dock
        auto* l_dockWidget = new ChartDockWidget(a_mxTelemetryType,
                                           a_mxTelemetryInstanceName,
                                           nullptr  //parent will be set later
                                           );
        // a_dockWidget->setAttribute(Qt::WA_DeleteOnClose);       // delete when user close the window

        // get the container
        auto* l_controller = getController(a_mxTelemetryType, a_mxTelemetryInstanceName);

        bool l_reachedMaxAllowedViews = false;;
        auto* l_result = l_controller->initView(l_reachedMaxAllowedViews, a_dockWidget);

        if (l_result == nullptr)
        {
            delete a_dockWidget;
            a_action = ACTIONS::NO_ACTION;
        }

        a_action = DockWindowController::ACTIONS::ADD_TO_RIGHT;
        l_dockWidget->setController(l_controller);
        l_dockWidget->setWidget(l_result);

        a_dockWidget = l_dockWidget;
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
    if (a_mxTelemetryType < 0
            or
        a_mxTelemetryType > MxTelemetryGeneralWrapper::mxTypeSize())
    {
        qCritical() << *m_className
                    << __PRETTY_FUNCTION__
                    << "Invalid mxTelemetryType:"
                    << a_mxTelemetryType
                    << "mxTelemetryInstanceName:"
                    << a_mxTelemetryInstanceName;
        return nullptr;
    }

    // get from continer
    return (*m_DB).at(a_mxTelemetryType)->value(a_mxTelemetryInstanceName, nullptr);
}






