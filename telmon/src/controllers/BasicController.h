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

// based on
// https://stackoverflow.com/questions/318064/how-do-you-declare-an-interface-in-c

#ifndef BASICCONTROLLER_H
#define BASICCONTROLLER_H

#include <QObject>

class QAbstractItemView;
class DataDistributor;

class BasicController : public QObject
{
    Q_OBJECT
public:
    BasicController(DataDistributor& a_dataDistributor, QObject* a_parent);
    virtual ~BasicController();
    // Stop the compiler generating methods of copy the object
    BasicController(BasicController const& copy);            // Not Implemented
    BasicController& operator=(BasicController const& copy); // Not Implemented

    virtual void* getModel() = 0;
    virtual QAbstractItemView* getView() = 0;


protected:
    DataDistributor& m_dataDistributor;
};

#endif // BASICCONTROLLER_H
