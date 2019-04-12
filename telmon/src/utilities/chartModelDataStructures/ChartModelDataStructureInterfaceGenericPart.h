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


//#ifndef CHARTMODELDATASTRUCTUREINTERFACEGENERICPART_H
//#define CHARTMODELDATASTRUCTUREINTERFACEGENERICPART_H

#include "ChartModelDataStructureInterface.h"
#include "QMutex"

template <class T>
ChartModelDataStructureInterface<T>::ChartModelDataStructureInterface(const int a_containerSize):
    m_size(a_containerSize),
    m_sizeMutex(new QMutex)
{

}

template <class T>
ChartModelDataStructureInterface<T>::~ChartModelDataStructureInterface()
{
    delete m_sizeMutex;
    m_sizeMutex = nullptr;
}


template <class T>
void ChartModelDataStructureInterface<T>::setContainerSize(const int a_size) noexcept
{
    QMutexLocker locker(m_sizeMutex);
    m_sizeMutex = a_size;
}


template <class T>
int ChartModelDataStructureInterface<T>::getContainerSize() const noexcept
{
    QMutexLocker locker(m_sizeMutex);
    return m_sizeMutex;
}


template <class T>
//template <size_t N>
QList<T> ChartModelDataStructureInterface<T>::getItems(const int beginIdx, const int endIdx) noexcept
{
    //std::array<T, N> l_result;
    QList<T> l_result;
    lock();
    {
        for (int i = beginIdx; i < endIdx; i++)
        {
            l_result.append(at(i));
        }
    }
    unlock();
    return l_result;
}


//#endif // CHARTMODELDATASTRUCTUREINTERFACEGENERICPART_H
