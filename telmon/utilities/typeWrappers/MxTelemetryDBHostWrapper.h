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


#ifndef MXTELEMETRYDBHOSTWRAPPER_H
#define MXTELEMETRYDBHOSTWRAPPER_H


#include "utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


/**
 * @brief This class is a
 * wrapper for struct ZdbHost::Telemetry in Zdb.hpp
 * This should be the only place in the app that
 * this data type releated actions should be written
 * i.e. setting priorites, all other places should
 * be updated according to this end point
 */
class MxTelemetryDBHostWrapper : public MxTelemetryGeneralWrapper
{
private:
    // Private Constructor
    MxTelemetryDBHostWrapper();
    virtual ~MxTelemetryDBHostWrapper() override final;

    // Stop the compiler generating methods of copy the object
    MxTelemetryDBHostWrapper(MxTelemetryDBHostWrapper const& copy);            // Not Implemented
    MxTelemetryDBHostWrapper& operator=(MxTelemetryDBHostWrapper const& copy); // Not Implemented


protected:
    friend class MxTelemetryTypeWrappersFactory;
    // protected so only friend class can access // to be tested
    // Not part of the inferface
    static MxTelemetryDBHostWrapper& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static MxTelemetryDBHostWrapper m_instance;
        return m_instance;
    }


    void initTableList() noexcept override final;
    void initChartList() noexcept override final;
    void initActiveDataSet() noexcept override final;
    QPair<void*, int> getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;

public:
    // must correspond to struct index
    enum DBHostMxTelemetryStructIndex { e_ip,    e_id,     e_priority,
                                        e_port,  e_voted,  e_state};

    double getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    void getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept override final;
};


#endif // MXTELEMETRYDBHOSTWRAPPER_H








