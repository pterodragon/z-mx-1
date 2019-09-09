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

#ifndef MXTELEMETRYENGINEWRAPPER_H
#define MXTELEMETRYENGINEWRAPPER_H


#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"



/**
 * @brief This class is a
 * wrapper for struct MxEngine::Telemetry in MxEngine.hpp
 * This should be the only place in the app that
 * this data type releated actions should be written
 * i.e. setting priorites, all other places should
 * be updated according to this end point
 */
class MxTelemetryEngineWrapper : public MxTelemetryGeneralWrapper
{
private:
    // Private Constructor
    MxTelemetryEngineWrapper();
    virtual ~MxTelemetryEngineWrapper() override final;

    // Stop the compiler generating methods of copy the object
    MxTelemetryEngineWrapper(MxTelemetryEngineWrapper const& copy);            // Not Implemented
    MxTelemetryEngineWrapper& operator=(MxTelemetryEngineWrapper const& copy); // Not Implemented


protected:
    friend class MxTelemetryTypeWrappersFactory;
    // protected so only friend class can access // to be tested
    // Not part of the inferface
    static MxTelemetryEngineWrapper& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static MxTelemetryEngineWrapper m_instance;
        return m_instance;
    }


    void initTableList() noexcept override final;
    void initChartList() noexcept override final;
    void initActiveDataSet() noexcept override final;
    QPair<void*, int> getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    const QString _getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept override final;

    // For  HTML Text
    inline static const QString _padding   = _3S;
    inline static const QString _title     = _Bold_begin + "Engine::";

    inline static const QString _time      = _NewLine    + "time:"      + _6S + _padding;
    inline static const QString _state     = _NewLine    + "state:"     + _5S + _padding;
    inline static const QString _nLinks    = _NewLine    + "nLinks:"    + _4S + _padding;
    inline static const QString _up        = _NewLine    + "up:"        + _Tab+ _padding;
    inline static const QString _down      = _NewLine    + "down:"      + _6S + _padding;

    inline static const QString _disabled  = _NewLine    + "disabled:"  + _2S + _padding;
    inline static const QString _transient = _NewLine    + "transient:" + _S + _padding;
    inline static const QString _reconn    = _NewLine    + "reconn:"    + _4S + _padding;
    inline static const QString _failed    = _NewLine    + "failed:"    + _4S + _padding;
    inline static const QString _mxID      = _NewLine    + "mxID:"      + _6S + _padding;

    inline static const QString _rxThread  = _NewLine    + "rxThread:"  + _2S + _padding;
    inline static const QString _txThread  = _NewLine    + "txThread:"  + _2S + _padding;

    const QString& getColor(const int a_state) const noexcept;

public:
    /**
     * @brief The EngineMxTelemetryStructIndex enum corresponds to following:
     *
     *  struct MxEngine::Telemetry { // no graph
     *    MxID		id;         // primary key
     *    MxID		mxID;		// static
     *    uint32_t	down;		// dynamic
     *    uint32_t	disabled;	// dynamic
     *    uint32_t	transient;	// dynamic
     *    uint32_t	up;         // dynamic
     *    uint32_t	reconn;		// dynamic
     *    uint32_t	failed;		// dynamic
     *    uint16_t	nLinks;		// static
     *    uint8_t	rxThread;	// static
     *    uint8_t	txThread;	// static
     *    uint8_t	state;		// RAG - MxEngineState::rag(i) - MxRAG namespace return
     *  };
     *
     */
    enum EngineMxTelemetryStructIndex {e_id,       e_mxID,    e_down,      e_disabled,
                                      e_transient, e_up,      e_reconn,    e_failed,
                                      e_nLinks,    e_rxThread, e_txThread, e_state};

    int _getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;

    void _getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept override final;
    virtual const QString _getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept override final;
};


#endif // MXTELEMETRYENGINEWRAPPER_H
