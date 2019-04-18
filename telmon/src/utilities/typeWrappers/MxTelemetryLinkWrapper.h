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



#ifndef MXTELEMETRYLINKWRAPPER_H
#define MXTELEMETRYLINKWRAPPER_H

#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"



/**
 * @brief This class is a
 * wrapper for struct MxAnyLink::Telemetry in MxEngine.hpp
 * This should be the only place in the app that
 * this data type releated actions should be written
 * i.e. setting priorites, all other places should
 * be updated according to this end point
 */
class MxTelemetryLinkWrapper : public MxTelemetryGeneralWrapper
{
private:
    // Private Constructor
    MxTelemetryLinkWrapper();
    virtual ~MxTelemetryLinkWrapper() override final;

    // Stop the compiler generating methods of copy the object
    MxTelemetryLinkWrapper(MxTelemetryLinkWrapper const& copy);            // Not Implemented
    MxTelemetryLinkWrapper& operator=(MxTelemetryLinkWrapper const& copy); // Not Implemented


protected:
    friend class MxTelemetryTypeWrappersFactory;
    // protected so only friend class can access // to be tested
    // Not part of the inferface
    static MxTelemetryLinkWrapper& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static MxTelemetryLinkWrapper m_instance;
        return m_instance;
    }


    void initTableList() noexcept override final;
    void initChartList() noexcept override final;
    void initActiveDataSet() noexcept override final;
    QPair<void*, int> getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    const QString _getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept override final;

    // For  HTML Text
        inline static const QString _title       = _Bold_begin + "Link::";
        inline static const QString _time        = _NewLine + "time:"        + _Tab + _2S;
        inline static const QString _state       = _NewLine + "state:"       + _Tab + _S;
        inline static const QString _reconnects  = _NewLine + "reconnects:"  + _4S; // LONGEST
        inline static const QString _rxSeqNo     = _NewLine + "rxSeqNo:"     + _7S;
        inline static const QString _txSeqNo     = _NewLine + "txSeqNo:"     + _7S;

        const QString& getColor(const int a_state) const noexcept;

public:
    /**
     * @brief The LinkMxTelemetryStructIndex enum corresponds to following:
     *
     *  struct MxAnyLink::Telemetry { // no graph
     *    MxID		id;		    // primary key
     *    uint64_t	rxSeqNo;	// dynamic - not graphable
     *    uint64_t	txSeqNo;	// dynamic - not graphable
     *    uint32_t	reconnects;	// dynamic - not graphable
     *    uint8_t	state;		// RAG - MxLinkState::rag(i) - MxRAG
     */
    enum LinkMxTelemetryStructIndex {e_id,        e_rxSeqNo,     e_txSeqNo,
                                      e_reconnects, e_state};

    int _getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    void _getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept override final;
    virtual const QString _getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept override final;
};


#endif // MXTELEMETRYLINKWRAPPER_H
