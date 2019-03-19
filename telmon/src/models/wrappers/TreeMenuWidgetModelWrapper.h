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


#ifndef TREEMENUWIDGETMODELWRAPPER_H
#define TREEMENUWIDGETMODELWRAPPER_H

class TreeModel;
class QAbstractItemModel;
//class QModelIndexList;

#include "ZmPlatform.hpp"


#include <set>
#include "src/subscribers/DataSubscriber.h"
#include "QString"
#include "QModelIndexList"


/**
 * @brief The class is implementation of the TreeWidgetModelInterface
 * The class wraps TreeModel and provides:
 * 1. add/remove to the tree view WITHOUT need to dive into qt details
 * 2. better performance as we hold local data structure which includes the
 * elements in the tree in easy retreive manner.
 * Before any addition to the TreeModel, we check it locally, and only if
 * data is missing from the TreeModel, we add it.
 * This design is faster because there are a lot of message from the network component
 * but most of the time, its just an update about the inner fields and not new TelemetryType
 * new ID
 *
 */
class TreeMenuWidgetModelWrapper : public DataSubscriber
{
public:
    TreeMenuWidgetModelWrapper();
    virtual ~TreeMenuWidgetModelWrapper() override;

    enum STATUS {APPENEDED_TO_VIEW,
                 FAILED_TO_APPEND_TO_MODEL,
                 ALREADY_IN_VIEW,
                 FAILED_TO_APPEND_TO_LOCAL_CONTAINER,
                 UNKNOWN_MSG_HEADER};

    QAbstractItemModel* getModel() const noexcept;

    virtual const QString& getName() const noexcept override final;
    virtual void update(void* a_mxTelemetryMsg) override final;

    bool isMxTelemetryTypeSelected(const QModelIndex a_index) ;

protected:
    TreeModel* m_treeModel;

    // chioce of container made upon
    // https://stackoverflow.com/questions/471432/in-which-scenario-do-i-use-a-particular-stl-container
    // https://stackoverflow.com/questions/181693/what-are-the-complexity-guarantees-of-the-standard-containers


    std::set<std::pair<std::string, ZmIDString>> m_localContainer;

private:
    int m_mxTelemetryTypeFirstTreeLevelPosition; // while doing reset, dont forget to set to zero
    int *m_mxTelemetryTypeFirstTreeLevelPositionArray;
    const QString  m_name;
    static constexpr auto APPEND_TO_THE_END = -1;
    static constexpr auto EMPRTY_STRING = "";

};

#endif // TREEMENUWIDGETMODELWRAPPER_H
