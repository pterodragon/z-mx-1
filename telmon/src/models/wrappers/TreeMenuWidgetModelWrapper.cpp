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

#include "src/models/wrappers/TreeMenuWidgetModelWrapper.h"
#include "src/models/raw/TreeModel.h"
#include "src/factories/MxTelemetryTypeWrappersFactory.h"
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


TreeMenuWidgetModelWrapper::TreeMenuWidgetModelWrapper():
    DataSubscriber (QString("TreeMenuWidgetModelWrapper")),
    m_treeModel(new TreeModel("MxTelemetry::Types")),
    m_mxTelemetryTypeFirstTreeLevelPosition(0),
    m_mxTelemetryTypeFirstTreeLevelPositionArray(new int[static_cast<unsigned long>(MxTelemetryGeneralWrapper::mxTypeSize())])
{

}


TreeMenuWidgetModelWrapper::~TreeMenuWidgetModelWrapper()
{
    delete [] m_mxTelemetryTypeFirstTreeLevelPositionArray;
    delete m_treeModel;
}


QAbstractItemModel* TreeMenuWidgetModelWrapper::getModel() const noexcept
{
    return static_cast<QAbstractItemModel*>(m_treeModel);
}


/**
 * Algorithm:
 * 1. parse a_msg, get l_msgHeaderName and l_msgDataID
 *    l_msgHeaderName denotes to which child associate l_msgDataID
 * 2. Check if the pair<l_msgHeaderName, l_msgDataID> is in m_localContainer
 *    if yes, return // because model is already updated.
 * .  if no, go to 3.
 * 3.  --SKIPPED-- Generate l_msgHeaderDescription for l_msgHeaderName
 * 4. Check if pair <l_msgHeaderName, l_msgHeaderDescription> is inside m_localContainer
 *    if yes, go to 7, if not go to 5
 * 5. insert pair <l_msgHeaderName, l_msgHeaderDescription> to m_localContainer
 * 6. insert pair <l_msgHeaderName, l_msgHeaderDescription> to model under root index
 * 7. insert pair <l_msgHeaderName, l_msgDataID> to m_localContainer
 * 8. get the model index for parent of pair: <l_msgHeaderName, l_msgDataID>
 * 9. insert pair <l_msgHeaderName, l_msgDataID> under model index for <l_msgHeaderName, l_msgHeaderDescription>
 *
 * In case of failure, calling function should print the given msg
 */
void TreeMenuWidgetModelWrapper::update(void* a_mxTelemetryMsg)
{
    // step 1
    const auto  l_mxTelemetryTypeIndex = MxTelemetryGeneralWrapper::getMsgHeaderType(a_mxTelemetryMsg);
    const auto* l_wrapper = MxTelemetryTypeWrappersFactory::getInstance().getMxTelemetryWrapper(l_mxTelemetryTypeIndex);
    if (!l_wrapper)
    {
        qCritical() << getName()
                    << __PRETTY_FUNCTION__
                    << "Unkown mxType:"
                    << l_mxTelemetryTypeIndex;
        return; // STATUS::UNKNOWN_MSG_HEADER;
    }

    const QString l_msgDataID = l_wrapper->getPrimaryKey(a_mxTelemetryMsg);
    const auto l_msgHeaderName = QString(MxTelemetryGeneralWrapper::getMsgHeaderName(a_mxTelemetryMsg));
    auto l_pairHeaderNameHeaderData = qMakePair(l_msgHeaderName, l_msgDataID);

    // TODO: protect by mutex
    // use https://doc.qt.io/archives/qt-4.8/qmutexlocker.html#details
    // step 2
    if (m_localContainer.count(l_pairHeaderNameHeaderData))
    {
        return; //Status::ALREADY_IN_VIEW;
    }

    // step 3 -- skipped the generate part, for now use empty header descriptions
    const QString l_headerDescription = QString();
    auto l_pairHeaderNameHeaderDescription = qMakePair(l_msgHeaderName, l_headerDescription);

    // step 4
    if (!m_localContainer.count(l_pairHeaderNameHeaderDescription))
    {
        // step 5
        if (!(m_localContainer.insert(l_pairHeaderNameHeaderDescription)).second) {
            qCritical() << "Failed to insert data to local container, step 5, printing some information:"
                        << l_pairHeaderNameHeaderData
                        << l_pairHeaderNameHeaderDescription;
            return; //Status::FAILED_TO_APPEND_TO_LOCAL_CONTAINER;
        }

        // set the correspoding index so we can know where to insert instances of that type;
        m_mxTelemetryTypeFirstTreeLevelPositionArray[l_mxTelemetryTypeIndex] = m_mxTelemetryTypeFirstTreeLevelPosition;
        m_mxTelemetryTypeFirstTreeLevelPosition++;

        // step 6
        const QModelIndex l_modelIndexForRoot = QModelIndex();
        if (!m_treeModel->insertRow(APPEND_TO_THE_END, l_modelIndexForRoot, l_msgHeaderName, l_headerDescription)) {
            qCritical() << "Failed to insert data to model, step 6, printing all information:"
                        << l_pairHeaderNameHeaderData
                        << l_pairHeaderNameHeaderDescription;
            // try to undo -- that is, keep model data structure same as local data structure
            if (m_localContainer.erase(l_pairHeaderNameHeaderDescription) == 1) {
                qCritical() << "Some recover succeded ";
            } else {
                qCritical() << "Some recover failed ";
            }
            return ;//Status::FAILED_TO_APPEND_TO_MODEL;
        }
    }

    // step 7
    if (!(m_localContainer.insert(l_pairHeaderNameHeaderData)).second) {
        qCritical() << "Failed to insert data to local container, step 7, printing some information:"
                    << l_pairHeaderNameHeaderData
                    << l_pairHeaderNameHeaderDescription;
        // Too difficult to undo!
        return ;//Status::FAILED_TO_APPEND_TO_LOCAL_CONTAINER;
    }

    // step 8
    const int l_currentTypePositionInTree = m_mxTelemetryTypeFirstTreeLevelPositionArray[l_mxTelemetryTypeIndex];
    const QModelIndex l_modelIndexForCurrentParent = m_treeModel->index(l_currentTypePositionInTree, 0, QModelIndex());

    // step 9
    if (!m_treeModel->insertRow(APPEND_TO_THE_END, l_modelIndexForCurrentParent, l_msgDataID, QString())) {
        qCritical() << "Failed to insert data to model, step 9, printing all information:"
                    << l_pairHeaderNameHeaderData
                    << l_pairHeaderNameHeaderDescription;
        // try to undo -- that is, keep model data structure same as local data structure
        if (m_localContainer.erase(l_pairHeaderNameHeaderData) == 1) {
            qCritical() << "Some recover succeded ";
        } else {
            qCritical() << "Some recover failed ";
        }
        return ;//Status::FAILED_TO_APPEND_TO_MODEL;
    }

    // From design view point:
    // 1. did not want to turn the model wrapper to QObject in order to use signal
    // 2. also did not want model to know about controller, to keep them independent
    // Solution: we will use TreeModel (which is QObject) for signal ability
    // 3. it must be signal as we dont want the update thread to create all the controllers and models
    //    etc. right now it is the main thread that do it,
    //    if the main thread will become slow, we can think on solution in the future
    // and connect both in controller
    {
        m_treeModel->notifyOfDataInertion(l_mxTelemetryTypeIndex, l_msgDataID);
    }

    return ;//Status::APPENEDED_TO_VIEW;
}


/**
 * @brief this function is used in order to determine
 *        if the user has selected one of the MxTelemetry::Types subtype
 *        if so, return true
 * @return
 */
bool TreeMenuWidgetModelWrapper::isMxTelemetryTypeSelected(const QModelIndex& a_index)
{
    return (!m_treeModel->isParentIsRootItem(a_index));
}



