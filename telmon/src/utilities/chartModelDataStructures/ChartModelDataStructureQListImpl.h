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

#ifndef CHARTMODELDATASTRUCTUREQLISTIMPL_H
#define CHARTMODELDATASTRUCTUREQLISTIMPL_H

#include "src/utilities/chartModelDataStructures/ChartModelDataStructureInterface.h"

template <class T>
class QList;

template <class T>
class ChartModelDataStructureQListImpl : public ChartModelDataStructureInterface<T>
{
public:
    ChartModelDataStructureQListImpl(const int a_containerSize);
    virtual ~ChartModelDataStructureQListImpl() override;
    virtual int size() const noexcept override final;
    virtual void prependKeepingSize(const T) noexcept override final;

protected:
    virtual void lock()           noexcept override final;
    virtual void unlock()         noexcept override final;
    virtual T at(const int) const noexcept override final;

private:
    QList<T>* m_db;
    QMutex* m_mutex;
};

#include "ChartModelDataStructureQListImplGenericPart.h"

#endif // CHARTMODELDATASTRUCTUREQLISTIMPL_H
