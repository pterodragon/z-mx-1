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


#ifndef MXTELEMETRYZMTHREADWRAPPER_H
#define MXTELEMETRYZMTHREADWRAPPER_H


#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


/**
 * @brief This class is a
 * wrapper for struct ZmThreadTelemetry in ZmThread.hpp
 * This should be the only place in the app that
 * this data type releated actions should be written
 * i.e. setting priorites, all other places should
 * be updated according to this end point
 */
class MxTelemetryZmThreadWrapper : public MxTelemetryGeneralWrapper
{
private:
    // Private Constructor
    MxTelemetryZmThreadWrapper();
    virtual ~MxTelemetryZmThreadWrapper() override final;

    // Stop the compiler generating methods of copy the object
    MxTelemetryZmThreadWrapper(MxTelemetryZmThreadWrapper const& copy);            // Not Implemented
    MxTelemetryZmThreadWrapper& operator=(MxTelemetryZmThreadWrapper const& copy); // Not Implemented


protected:
    friend class MxTelemetryTypeWrappersFactory;
    // protected so only friend class can access // to be tested
    // Not part of the inferface
    static MxTelemetryZmThreadWrapper& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static MxTelemetryZmThreadWrapper m_instance;
        return m_instance;
    }


    void initTableList() noexcept override final;
    void initChartList() noexcept override final;
    void initActiveDataSet() noexcept override final;

    QPair<void*, int> getMxTelemetryDataType(void* const a_mxTelemetryMsg,
                                                     const int a_index,
                                                     void* a_otherResult) const noexcept override final;


public:
    /**
     * @brief The ZmThreadTelemetryStructIndex enum corresponds to following:
     *
     *  struct ZmThreadTelemetry {  // graphable
     *    ZmThreadName	name;		// static
     *    uint64_t	tid;            // primary key
     *    uint64_t	stackSize;      // static
     *    uint64_t	cpuset;         // dynamic (not graphable) E.g. "1-4"
     *    double	cpuUsage;       // dynamic(*)
     *    int32_t	id;             // static - thread mgr ID, -ve if unset
     *    int32_t	priority;       // static
     *    uint16_t	partition;      // static
     *    uint8_t	main;           // static
     *    uint8_t	detached;       // static
     *  };
     *
     */
    enum ZmThreadTelemetryStructIndex {e_name, e_tid, e_stackSize, e_cpuset, e_cpuUsage,
                                       e_id, e_priority, e_partition, e_main, e_detached};

    double getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    void getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept override final;

    /**
     * @brief  MxTelemetry::Thread name is *name* + *tid*
     * @param a_name
     * @param a_tid
     * @return
     */
    virtual const std::string getPrimaryKey(const std::initializer_list<std::string>& a_list) const noexcept override final;

};
#endif // MXTELEMETRYZMTHREADWRAPPER_H





