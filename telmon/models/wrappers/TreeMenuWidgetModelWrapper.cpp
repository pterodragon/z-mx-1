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

#include "MxTelemetry.hpp"
#include "models/wrappers/TreeMenuWidgetModelWrapper.h"
#include "models/raw/TreeModel.h"
#include "QDebug"


TreeMenuWidgetModelWrapper::TreeMenuWidgetModelWrapper():
    m_treeModel(new TreeModel("MxTelemetry::Types")),
    m_name("TreeMenuWidgetModelWrappesr")
{

}

TreeMenuWidgetModelWrapper::~TreeMenuWidgetModelWrapper()
{
    delete m_treeModel;
}


QAbstractItemModel* TreeMenuWidgetModelWrapper::getModel() const noexcept
{
    return static_cast<QAbstractItemModel*>(m_treeModel);
}


const QString& TreeMenuWidgetModelWrapper::getName() const noexcept
{
    return m_name;
}



void TreeMenuWidgetModelWrapper::update(void* a_mxTelemetryMsg)
{
    using namespace MxTelemetry;
    MxTelemetry::Msg* a_msg = static_cast<MxTelemetry::Msg*>(a_mxTelemetryMsg);

    // step 1
    const auto l_msgHeaderName = Type::name(a_msg->hdr().type);
    ZmIDString l_msgDataID;
    const int l_treeFirstLevelOrder = static_cast<int>(a_msg->hdr().type);

    switch (l_treeFirstLevelOrder) {
      case Type::Heap: {
    const auto &data = a_msg->as<Heap>();
        l_msgDataID = data.id;
      } break;
      case Type::Thread: {
    const auto &data = a_msg->as<Thread>();
        l_msgDataID = data.name;
      } break;
      case Type::Multiplexer: {
    const auto &data = a_msg->as<Multiplexer>();
        l_msgDataID = data.id;
      } break;
      case Type::Socket: {
    const auto &data = a_msg->as<Socket>();
        l_msgDataID = data.mxID;
      } break;
      case Type::Queue: {
    const auto &data = a_msg->as<Queue>();
        l_msgDataID = data.id;
      } break;
      case Type::HashTbl: {
    const auto &data = a_msg->as<HashTbl>();
        l_msgDataID = data.id;
      } break;
      case Type::Engine: {
    const auto &data = a_msg->as<Engine>();
        l_msgDataID = data.id;
    } break;
      case Type::Link: {
    const auto &data = a_msg->as<Link>();
        l_msgDataID = data.id;
  } break;
//    case Type::DBEnv: {
//  const auto &data = a_msg->as<DBEnv>();
//      l_msgDataID = static_cast<ZmIDString>(data.self);
//  } break;
//    case Type::DBHost: {
//  const auto &data = a_msg->as<DBHost>();
//      l_msgDataID = data.id;
//  } break;
//    case Type::DB: {
//  const auto &data = a_msg->as<DB>();
//      l_msgDataID = data.id;
//  } break;
      default: {
        qWarning() << "Unkown message header, ignoring:";
        return; //STATUS::UNKNOWN_MSG_HEADER;
      } break;
    }

    auto l_pairHeaderNameHeaderData = std::make_pair(l_msgHeaderName, l_msgDataID);

    // TODO: protect by mutex
    // use https://doc.qt.io/archives/qt-4.8/qmutexlocker.html#details
    // step 2
    if (m_localContainer.count(l_pairHeaderNameHeaderData))
    {
        return; //Status::ALREADY_IN_VIEW;
    }

    // step 3 -- skipped the generate part, for now use empty header descriptions
    ZmIDString l_headerDescription = EMPRTY_STRING;
    auto l_pairHeaderNameHeaderDescription = std::make_pair(l_msgHeaderName, l_headerDescription);

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

        // step 6
        QModelIndex l_modelIndexForRoot = QModelIndex();
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
    QModelIndex l_modelIndexForCurrentParent = m_treeModel->index(l_treeFirstLevelOrder,0, QModelIndex());

    // step 9
    if (!m_treeModel->insertRow(APPEND_TO_THE_END, l_modelIndexForCurrentParent, l_msgDataID, EMPRTY_STRING)) {
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
    return ;//Status::APPENEDED_TO_VIEW;
}


/**
 * @brief this function is used in order to determine
 *        if the user has selected one of the MxTelemetry::Types subtype
 *        if so, return true
 * @return
 */
bool TreeMenuWidgetModelWrapper::isMxTelemetryTypeSelected(const QModelIndex a_index)
{
    return (!m_treeModel->isParentIsRootItem(a_index));
}



