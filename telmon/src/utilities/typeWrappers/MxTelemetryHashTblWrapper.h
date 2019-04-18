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


#ifndef MXTELEMETRYHASHTBLWRAPPER_H
#define MXTELEMETRYHASHTBLWRAPPER_H


#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"



/**
 * @brief The MxTelemetryHashTblWrapper class
 * Wrapper for struct ZmHashTelemetry in ZmHashMgr.hpp
 * This should be the only place in the app that
 * this data type releated actions should be written
 * i.e. setting priorites, all other places should
 * be updated according to this end point
 */
class MxTelemetryHashTblWrapper : public MxTelemetryGeneralWrapper
{
private:
    // Private Constructor
    MxTelemetryHashTblWrapper();
    virtual ~MxTelemetryHashTblWrapper() override final;

    // Stop the compiler generating methods of copy the object
    MxTelemetryHashTblWrapper(MxTelemetryHashTblWrapper const& copy);            // Not Implemented
    MxTelemetryHashTblWrapper& operator=(MxTelemetryHashTblWrapper const& copy); // Not Implemented


protected:
    friend class MxTelemetryTypeWrappersFactory;
    // protected so only friend class can access // to be tested
    // Not part of the inferface
    static MxTelemetryHashTblWrapper& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static MxTelemetryHashTblWrapper m_instance;
        return m_instance;
    }


    void initTableList() noexcept override final;
    void initChartList() noexcept override final;
    void initActiveDataSet() noexcept override final;

    QPair<void*, int> getMxTelemetryDataType(void* const a_mxTelemetryMsg,
                                                     const int a_index,
                                                     void* a_otherResult) const noexcept override final;

    const QString _getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept override final;

    // For  HTML Text
    inline static const QString _padding    = _2S;
    inline static const QString _Title         = _Bold_begin + "HashTbl::";
    inline static const QString _Time          = _NewLine + "time:"          + _Tab + _2S + _padding;
    inline static const QString _linear        = _NewLine + "linear:"        + _Tab       + _padding;
    inline static const QString _bits          = _NewLine + "bits:"          + _Tab + _2S + _padding;
    inline static const QString _slots         = _NewLine + "slots:"         + _Tab + _S  + _padding;
    inline static const QString _cBits         = _NewLine + "cBits:"         + _Tab + _S  + _padding;
    inline static const QString _locks         = _NewLine + "locks:"         + _Tab + _S  + _padding;
    inline static const QString _count         = _NewLine + "count:"         + _Tab + _S  + _padding;
    inline static const QString _resized       = _NewLine + "resized:"       + _7S + _padding;
    inline static const QString _loadFactor    = _NewLine + "loadFactor:"    + _4S + _padding;
    inline static const QString _effLoadFactor = _NewLine + "effLoadFactor:" + _S  + _padding; // LONGEST
    inline static const QString _nodeSize      = _NewLine + "nodeSize:"      + _6S + _padding;

public:
    /**
     * @brief The ZmHashTblTelemetryStructIndex enum corresponds to following:
     *
     * struct ZmHashTelemetry {    // graphable
     *    ZmIDString	id;		   // primary key
     *    uintptr_t	addr;		   // primary key
     *    uint32_t	nodeSize;	   // static
     *    uint32_t	loadFactor;	   // static - (double)N / 16.0
     *    uint32_t	count;		   // dynamic(*)
     *    uint32_t	effLoadFactor; // dynamic(*) - (double)N / 16.0
     *    uint32_t	resized;	   // dynamic (not graphable)
     *    uint8_t	bits;		   // static
     *    uint8_t	cBits;		   // static
     *    uint8_t	linear;		   // static
     *  };
     *
     */
    enum ZmHashTblTelemetryStructIndex {e_id,            e_addr,    e_nodeSize, e_loadFactor, e_count,
                                        e_effLoadFactor, e_resized, e_bits,     e_cBits,      e_linear};

    int _getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    void _getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept override final;
    virtual const QString _getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept override final;

};




#endif // MXTELEMETRYHASHTBLWRAPPER_H
