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


#include "ChartModelDataStructureQListImpl.h"
#include "QMutex"
#include "QDateTime"

template <class T>
ChartModelDataStructureQListImpl<T>::ChartModelDataStructureQListImpl(const int a_containerSize):
    ChartModelDataStructureInterface<T>(a_containerSize),
    m_db(new QList<T>),
    m_mutex(new QMutex)
{

}


template <class T>
ChartModelDataStructureQListImpl<T>::~ChartModelDataStructureQListImpl()
{
    delete m_db;
    m_db = nullptr;

    delete m_mutex;
    m_mutex = nullptr;
}

template <class T>
int ChartModelDataStructureQListImpl<T>::size() const noexcept
{
    QMutexLocker locker(m_mutex);
    return m_db->size();
}

template <class T>
void ChartModelDataStructureQListImpl<T>::prependKeepingSize(const T a_val) noexcept
{
    QMutexLocker locker(m_mutex);

    // check size
    if (!m_db->isEmpty() and (m_db->size() > this->m_size))
    {
        m_db->removeLast();
    }

    //insert element
    m_db->prepend(a_val);
}


template <class T>
void ChartModelDataStructureQListImpl<T>::lock() noexcept
{
    m_mutex->lock();
}


template <class T>
void ChartModelDataStructureQListImpl<T>::unlock() noexcept
{
    m_mutex->unlock();
}


template <class T>
T ChartModelDataStructureQListImpl<T>::at(const int a_index) const noexcept
{
    if (m_db->isEmpty()) {return T();}
    if (a_index >= m_db->size() or a_index < 0) {return T();}
    return m_db->at(a_index);
}
