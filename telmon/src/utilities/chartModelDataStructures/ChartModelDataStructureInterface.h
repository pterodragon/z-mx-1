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

#ifndef CHARTMODELDATASTRUCTUREINTERFACE_H
#define CHARTMODELDATASTRUCTUREINTERFACE_H

#include "QList"

//#include <array>
class QMutex;

template <class T>
class ChartModelDataStructureInterface
{
public:
    ChartModelDataStructureInterface(const int a_containerSize);
    virtual ~ChartModelDataStructureInterface();



    /**
     * @brief Inserts value at the beginning, moving all items one position up,
     * and removes any items that exceed the container size
     * This function is thread safe
     */
    virtual void prependKeepingSize(const T) noexcept = 0;

    /**
     * @brief return the size of the container
     * This function locks the container
     * @return
     */
    virtual int size() const noexcept = 0;

    /**
     * @brief Removes the last item
     * This function locks the container
     */
    //virtual void removeLast() noexcept = 0;

    /**
     * @brief return an array which contains items from beginIdx to endIdx
     * If index is out of border, will return -1
     * This function locks the container
     */
//    template<size_t N>
//    std::array<T, N> getItems(const int beginIdx, const int endIdx) noexcept;
    QList<T> getItems(const int beginIdx, const int endIdx) noexcept;


    /**
     * @brief setContainerSize
     * This function is thread safe
     * @param a_size
     */
    void setContainerSize(const int) noexcept;

    /**
     * @brief getContainerSize
     * This function is thread safe
     * @return
     */
    int getContainerSize() const noexcept;


protected:
    // used for implementing getItems
    virtual void lock() noexcept = 0;
    virtual void unlock() noexcept = 0;

    int m_size;

    // used for implementing getItems
    // does not lock the conatiner, works under the assumption
    // that lock and unlock were called before/after
    // if index is out of border, return -1;
    virtual T at(const int index) const noexcept = 0;

private:
    QMutex* m_sizeMutex;
};

#include "ChartModelDataStructureInterfaceGenericPart.h"

#endif // CHARTMODELDATASTRUCTUREINTERFACE_H
