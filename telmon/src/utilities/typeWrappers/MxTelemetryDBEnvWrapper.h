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


#ifndef MXTELEMETRYDBENVWRAPPER_H
#define MXTELEMETRYDBENVWRAPPER_H


#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


/**
 * @brief This class is a
 * wrapper for struct ZdbEnv::Telemetry in Zdb.hpp
 * This should be the only place in the app that
 * this data type releated actions should be written
 * i.e. setting priorites, all other places should
 * be updated according to this end point
 */
class MxTelemetryDBEnvWrapper : public MxTelemetryGeneralWrapper
{
private:
    // Private Constructor
    MxTelemetryDBEnvWrapper();
    virtual ~MxTelemetryDBEnvWrapper() override final;

    // Stop the compiler generating methods of copy the object
    MxTelemetryDBEnvWrapper(MxTelemetryDBEnvWrapper const& copy);            // Not Implemented
    MxTelemetryDBEnvWrapper& operator=(MxTelemetryDBEnvWrapper const& copy); // Not Implemented


protected:
    friend class MxTelemetryTypeWrappersFactory;
    // protected so only friend class can access // to be tested
    // Not part of the inferface
    static MxTelemetryDBEnvWrapper& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static MxTelemetryDBEnvWrapper m_instance;
        return m_instance;
    }


    void initTableList() noexcept override final;
    void initChartList() noexcept override final;
    void initActiveDataSet() noexcept override final;
    QPair<void*, int> getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    const QString _getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept override final;

    // For  HTML Text
    inline static const QString _padding          = "";
    inline static const QString _title            = _Bold_begin + "DBHost::";
    inline static const QString _time             = _NewLine + "time:"             + _Tab + _4S  + _padding;
    inline static const QString _self             = _NewLine + "self:"             + _Tab + _4S   + _padding;
    inline static const QString _master           = _NewLine + "master:"           + _Tab + _2S   + _padding;
    inline static const QString _prev             = _NewLine + "prev:"             + _Tab + _4S   + _padding;
    inline static const QString _next             = _NewLine + "next:"             + _Tab + _4S   + _padding;
    inline static const QString _state            = _NewLine + "state:"            + _Tab + _3S   + _padding;
    inline static const QString _active           = _NewLine + "active:"           + _Tab + _3S    + _padding;
    inline static const QString _recovering       = _NewLine + "recovering:"       + _7S         + _padding; // LONGEST
    inline static const QString _replicating      = _NewLine + "replicating:"      + _6S    + _padding;
    inline static const QString _nDBs             = _NewLine + "nDBs:"             + _Tab + _4S    + _padding;

    inline static const QString _nHosts           = _NewLine + "nHosts:"           + _Tab + _2S + _padding;
    inline static const QString _nPeers           = _NewLine + "nPeers:"           + _Tab + _2S   + _padding;
    inline static const QString _nCxns            = _NewLine + "nCxns:"            + _Tab + _3S   + _padding;
    inline static const QString _heartbeatFreq    = _NewLine + "heartbeatFreq:"    + _3S   + _padding;
    inline static const QString _heartbeatTimeout = _NewLine + "heartbeatTimeout:" + _S    + _padding;
    inline static const QString _reconnectFreq    = _NewLine + "reconnectFreq:"    + _3S    + _padding; // LONGEST
    inline static const QString _electionTimeout  = _NewLine + "electionTimeout:"  + _2S    + _padding;
    inline static const QString _writeThread      = _NewLine + "writeThread:"      + _6S    + _padding;

    const QString& getColor(const int a_state) const noexcept;

public:



    /**
     * @brief The DBEnvMxTelemetryStructIndex enum corresponds to following:
     *
     *  struct ZdbEnv::Telemetry {        // not graphable
     *    uint32_t		nCxns;		      // dynamic
     *    uint32_t		heartbeatFreq;
     *    uint32_t		heartbeatTimeout;
     *    uint32_t		reconnectFreq;
     *    uint32_t		electionTimeout;
     *    uint32_t		self;             // dynamic - host ID
     *    uint32_t		master;           // dynamic - ''
     *    uint32_t		prev;             // dynamic - ''
     *    uint32_t		next;             // dynamic - ''
     *    uint16_t		writeThread;
     *    uint8_t		nHosts;
     *    uint8_t		nPeers;
     *    uint8_t		nDBs;
     *    uint8_t		state;          // dynamic - RAG - same as hosts[hostID].state
     *    uint8_t		active;         // dynamic
     *    uint8_t		recovering;     // dynamic
     *    uint8_t		replicating;	// dynamic
     *  };
     *
     */
    enum DBEnvMxTelemetryStructIndex {e_nCxns,         e_heartbeatFreq,    e_heartbeatTimeout,
                                      e_reconnectFreq, e_electionTimeout,  e_self,
                                      e_master,        e_prev,             e_next,
                                      e_writeThread,   e_nHosts,           e_nPeers,
                                      e_nDBs,          e_state,            e_active,
                                      e_recovering,    e_replicating};

    int _getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    void _getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept override final;
    virtual const QString _getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept override final;
};


#endif // MXTELEMETRYDBENVWRAPPER_H










































