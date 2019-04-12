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



#ifndef BASICCHARTMODEL_H
#define BASICCHARTMODEL_H

class QString;
#include <functional> // for std::array

template<class T>
class QList;

template<class T>
class QVector;

template<class T>
class ChartModelDataStructureInterface;

class ChartSubscriber;
class DataSubscriber;


class BasicChartModel
{
public:
    BasicChartModel(const int a_associatedTelemetryType,
                    const QString& a_associatedTelemetryInstanceName);
    ~BasicChartModel();

    static const int NUMBER_OF_Y_AXIS = 2;

    const QString getAssociatedTelemetryInstanceName() const noexcept;
    int getAssociatedTelemetryType() const noexcept;

    const std::array<int, NUMBER_OF_Y_AXIS>& getActiveDataSet() const noexcept;
    const QVector<QString>& getChartList() const noexcept;

    int getSize(const int a_index) const noexcept;
    QList<int> getData(const int a_index, const int a_beginBegin, const int a_endIndex) const noexcept;

    bool isSeriesIsNull(const int a_series) const noexcept;

    ChartSubscriber* getSubscriber() noexcept;

    // * * * interface with subscriber * * * //
    void update(void* mxTelemetryType) noexcept;

private:
    const int m_associatedTelemetryType;
    const QString* m_associatedTelemetryInstanceName;
    const QString* m_className;

    // Data strucutre to hold all chart points
    // about size for each type:
    // we use int = 32 bit = 4 bytes
    // 1 min    = 60 seconds = 60 * 4 = 240 bytes
    // 60 min   = 240 * 60 = 14400 bytes
    // 24 hours = 14400 * 24 = 345600 bytes
    // 345600 bytes = 337.5 KB = 0.32959 MB
    // so 60 * 60 * 24 = 86400
    // for one type
    const int DATA_MAX_ALLOWED_SIZE = 86400;
    QVector<ChartModelDataStructureInterface<int>*>* m_chartDataContainer;


//    bool isReachedMax(const int) const noexcept;

    ChartSubscriber* m_subscriber;

};



#endif // BASICCHARTMODEL_H







