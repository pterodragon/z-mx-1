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

#include "BasicChartModel.h"

#include "src/factories/MxTelemetryTypeWrappersFactory.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"

#include "src/subscribers/ChartSubscriber.h"

#include "src/utilities/chartModelDataStructures/ChartModelDataStructureInterface.h"

//TODO: if using more than one type, should be moved to factory
#include "src/utilities/chartModelDataStructures/ChartModelDataStructureQListImpl.h"


BasicChartModel::BasicChartModel(const int a_associatedTelemetryType,
                                 const QString& a_associatedTelemetryInstanceName):
    // set up names
    m_associatedTelemetryType(a_associatedTelemetryType),
    m_associatedTelemetryInstanceName(new QString(a_associatedTelemetryInstanceName)),
    m_className(new QString("BasicChartModel::" +
                            MxTelemetryGeneralWrapper::fromMxTypeValueToName(m_associatedTelemetryType)
                            + "::" +
                            *m_associatedTelemetryInstanceName)),
    m_chartDataContainer(new QVector<ChartModelDataStructureInterface<int>*>),

    m_subscriber(new ChartSubscriber(a_associatedTelemetryType,
                                     a_associatedTelemetryInstanceName,
                                     this))
{
    qInfo() << QString("::" + *m_className);
    // * * *  initChartDataContainer * * * //
    // we do not need field for "none", that is why we remove 1
    const int l_numberOfFields = getChartList().size() - 1;
    for (int i = 0; i < l_numberOfFields; i++)
    {
        m_chartDataContainer->insert(i, new ChartModelDataStructureQListImpl<int>(DATA_MAX_ALLOWED_SIZE));
    }
}


BasicChartModel::~BasicChartModel()
{
    qInfo() << QString("::~" + *m_className);
    delete m_subscriber;
    m_subscriber = nullptr;

    delete m_className;
    m_className = nullptr;

    for (int i = 0; i < m_chartDataContainer->size(); i++) {
        delete (*m_chartDataContainer)[i];
        (*m_chartDataContainer)[i] = nullptr;
    }

    delete m_chartDataContainer;
    m_chartDataContainer = nullptr;

    delete m_associatedTelemetryInstanceName;
    m_associatedTelemetryInstanceName = nullptr;
}


const QString BasicChartModel::getAssociatedTelemetryInstanceName() const noexcept
{
    return *m_associatedTelemetryInstanceName;
}


int BasicChartModel::getAssociatedTelemetryType() const noexcept
{
    return m_associatedTelemetryType;
}


const std::array<int, BasicChartModel::NUMBER_OF_Y_AXIS>& BasicChartModel::getActiveDataSet() const noexcept
{
    return MxTelemetryTypeWrappersFactory::getInstance().
            getMxTelemetryWrapper(m_associatedTelemetryType)->getActiveDataSet();
}


const QVector<QString>& BasicChartModel::getChartList() const noexcept
{
    return MxTelemetryTypeWrappersFactory::getInstance().
            getMxTelemetryWrapper(m_associatedTelemetryType)->getChartList();
}


int BasicChartModel::getSize(const int a_index) const noexcept
{
    const int l_numberOfFields = getChartList().size() - 1; // -1 for "none"
    if (!(a_index < l_numberOfFields)) { return 0;}

    return m_chartDataContainer->at(a_index)->size();
}


QList<int> BasicChartModel::getData(const int a_index, const int a_beginBegin, const int a_endIndex) const noexcept
{
    return m_chartDataContainer->at(a_index)->getItems(a_beginBegin, a_endIndex);
}


bool BasicChartModel::isSeriesIsNull(const int a_series) const noexcept
{
    return MxTelemetryTypeWrappersFactory::getInstance().
            getMxTelemetryWrapper(m_associatedTelemetryType)->
            isDataTypeNotUsed(a_series);
}


ChartSubscriber*  BasicChartModel::getSubscriber() noexcept
{
    return m_subscriber;
}

/**destructor
 * @brief NOTICE: this function is NOT called from GUI thread!
 * @param mxTelemetryType
 */
void BasicChartModel::update(void* mxTelemetryType) noexcept
{
    const auto* l_wrapper = MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(m_associatedTelemetryType);
    // Notice we must remove 1 because we dont support "none" field
    const int l_fields = l_wrapper->getChartList().size() - 1;
    for (int i = 0; i < l_fields; i++)
    {
        int l_strcutData = l_wrapper->getDataForChart<int>(mxTelemetryType, i);
                        // only for testing
        //const auto l_strcutData = rand() % 100;

        m_chartDataContainer->at(i)->prependKeepingSize(l_strcutData);
    }
}
