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



#ifndef MXTELEMETRYDBWRAPPER_H
#define MXTELEMETRYDBWRAPPER_H


#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


/**
 * @brief This class is a
 * wrapper for struct ZdbAny::Telemetry in Zdb.hpp
 * This should be the only place in the app that
 * this data type releated actions should be written
 * i.e. setting priorites, all other places should
 * be updated according to this end point
 */
class MxTelemetryDBWrapper : public MxTelemetryGeneralWrapper
{
private:
    // Private Constructor
    MxTelemetryDBWrapper();
    virtual ~MxTelemetryDBWrapper() override final;

    // Stop the compiler generating methods of copy the object
    MxTelemetryDBWrapper(MxTelemetryDBWrapper const& copy);            // Not Implemented
    MxTelemetryDBWrapper& operator=(MxTelemetryDBWrapper const& copy); // Not Implemented


protected:
    friend class MxTelemetryTypeWrappersFactory;
    // protected so only friend class can access // to be tested
    // Not part of the inferface
    static MxTelemetryDBWrapper& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static MxTelemetryDBWrapper m_instance;
        return m_instance;
    }


    void initTableList() noexcept override final;
    void initChartList() noexcept override final;
    void initActiveDataSet() noexcept override final;
    QPair<void*, int> getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;


public:
    /**
     * @brief The DBMxTelemetryStructIndex enum corresponds to following:
     *
     *   struct ZdbAny::Telemetry {     // graphable
     *    typedef ZuStringN<124> Path;
     *    typedef ZuStringN<28> Name;
     *
     *    Path		path;
     *    Name		name;               // primary key
     *    uint64_t	fileSize;
     *    uint64_t	minRN;
     *    uint64_t	allocRN;           // dynamic
     *    uint64_t	fileRN;
     *    uint64_t	cacheLoads;	       // dynamic(*)
     *    uint64_t	cacheMisses;	   // dynamic(*)
     *    uint64_t	fileLoads;	       // dynamic
     *    uint64_t	fileMisses;	       // dynamic
     *    uint32_t	id;
     *    uint32_t	preAlloc;
     *    uint32_t	recSize;
     *    uint32_t	fileRecs;
     *    uint32_t	cacheSize;
     *    uint32_t	filesMax;
     *    uint8_t	compress;
     *    int8_t	cacheMode;
    };
     *
     */
    enum DBMxTelemetryStructIndex { e_path,        e_name,         e_fileSize,
                                    e_minRN,       e_allocRN,      e_fileRN,
                                    e_cacheLoads,  e_cacheMisses,  e_fileLoads,
                                    e_fileMisses,  e_id,           e_preAlloc,
                                    e_recSize,     e_fileRecs,     e_cacheSize,
                                    e_filesMax,    e_compress,     e_cacheMode};

    double getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    void getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept override final;
};

#endif // MXTELEMETRYDBWRAPPER_H































