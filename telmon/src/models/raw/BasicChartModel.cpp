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


BasicChartModel::BasicChartModel(const int a_associatedTelemetryType,
                                 const QString& a_associatedTelemetryInstanceName):
    // set up names
    m_associatedTelemetryType(a_associatedTelemetryType),
    m_associatedTelemetryInstanceName(new QString(a_associatedTelemetryInstanceName)),
    m_className(new QString("BasicChartModel::" +
                            MxTelemetryGeneralWrapper::fromMxTypeValueToName(m_associatedTelemetryType)
                            + "::" +
                            *m_associatedTelemetryInstanceName)),


    m_chartDataContainer(new QVector<QVector<int>*>),
    m_subscriber(new ChartSubscriber(a_associatedTelemetryType,
                                     a_associatedTelemetryInstanceName,
                                     this))
{
//    qInfo() << QString("::" + *m_className);
    // * * *  initChartDataContainer * * * //
    // we do not need field for "none", that is why we remove 1
    const int l_numberOfFields = getChartList().size() - 1;
    for (int i = 0; i < l_numberOfFields; i++)
    {
        m_chartDataContainer->insert(i, new QVector<int>);
    }
}


BasicChartModel::~BasicChartModel()
{
//    qInfo() << QString("::~" + *m_className);
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


//void BasicChartModel::setDefaultUpdateDataFunction() noexcept
//{
//    m_lambda = [] ( BasicChartModel* a_this,
//                   const QVector<int>& a_vector) -> void
//    {
//        //sanity check
//        if (a_vector.size() != a_this->m_chartDataContainer->size())
//        {
//            qCritical() << *a_this->m_className
//                        <<  __PRETTY_FUNCTION__
//                        << "a_vector.size():"
//                        << a_vector.size()
//                        << "does not match m_chartDataContainer->size():"
//                        << a_this->m_chartDataContainer->size()
//                        << "doing nothing";
//            return;
//        }

//        for (int i = 0; i < a_vpublic QObjectector.size(); ++i) {

//            //const auto l_data = a_this->m_chartDataContainer->at(i);

//            // only for testing
//            const auto l_data = rand() % 100;

//            // insert the data to be the first, pushing all the data inside one index up
//            a_this->m_chartDataContainer->at(i)->prepend(l_data);
//        }

//        // Notice:
//        // do not    };
//    } subscribe before m_basicChartView is set
//        // resposibility of controller

//        // draw chart only if flag is set
//        if (!(a_this->m_basicChartView->getDrawChartFlag())) {return;}

//        a_this->m_basicChartView->repaintChart();
//    };
//}

//void BasicChartModel::addDataToChartDataContainer(const QVector<int>& a_vector) const noexcept
//{
//    // whats need to be done here
//    // all below should be deleted
//    // and we just insert to m_charttDataContainer exactly as vector
//    // there should be matching of 1-1 between the sizes

//    // we do not need field for "none", that is why we remove 1
//    const int l_numberOfFields = getChartList().size() - 1;

//    // remove points if crossing the limit
//    // todo

//    for (int i = 0; i < l_numberOfFields; ++i)
//    {
//        // get the data
////        const auto l_data = MxTelemetryTypeWrappersFactory::getInstance().
////                getMxTelemetryWrapper(m_associatedTelemetryType).
////                getDataForChart(a_mxTelemetryMsg, i);

//        // only for testing
//        const auto l_data = rand() % 100;

//        // insert the data to be the first, pushing all the data inside one index up
//        m_chartDataContainer->at(i)->prepend(l_data);
//    }
//}


int BasicChartModel::getChartDataContainerSize() const noexcept
{
    return m_chartDataContainer->at(0)->size();
}


int BasicChartModel::getData(const int a_dataType, const int a_pos) const noexcept
{
    return m_chartDataContainer->at(a_dataType)->at(a_pos);
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
//        const auto l_strcutData = rand() % 100;

        // insert the data to be the first, pushing all the data inside one index up
        // TODO maybe need mutex array!!!
//        qDebug() << "added at pos:" << i << "data:" << l_strcutData;
        m_chartDataContainer->at(i)->prepend(l_strcutData);
    }
}


