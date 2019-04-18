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

#ifndef MXTELEMETRYQUEUEWRAPPER_H
#define MXTELEMETRYQUEUEWRAPPER_H

#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"



/**
 * @brief This class is a
 * wrapper for struct ZiCxnTelemetry in ZiMultiplex.hpp
 * This should be the only place in the app that
 * this data type releated actions should be written
 * i.e. setting priorites, all other places should
 * be updated according to this end point
 */
class MxTelemetryQueueWrapper : public MxTelemetryGeneralWrapper
{
private:
    // Private Constructor
    MxTelemetryQueueWrapper();
    virtual ~MxTelemetryQueueWrapper() override final;

    // Stop the compiler generating methods of copy the object
    MxTelemetryQueueWrapper(MxTelemetryQueueWrapper const& copy);            // Not Implemented
    MxTelemetryQueueWrapper& operator=(MxTelemetryQueueWrapper const& copy); // Not Implemented


protected:
    friend class MxTelemetryTypeWrappersFactory;
    // protected so only friend class can access // to be tested
    // Not part of the inferface
    static MxTelemetryQueueWrapper& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static MxTelemetryQueueWrapper m_instance;
        return m_instance;
    }


    void initTableList() noexcept override final;
    void initChartList() noexcept override final;
    void initActiveDataSet() noexcept override final;
    QPair<void*, int> getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;

    /**
     * @brief  MxTelemetry::Thread name is *id* + *MxTelemetry::QueueType::name(data.type)*
     * @param a_name
     * @param a_tid
     * @return
     */
    const QString _getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept override final;

    // For  HTML Text
        inline static const QString _padding    = _4S;
        inline static const QString _title     = _Bold_begin + "Queue::";
        inline static const QString _time      = _NewLine + "time:"      + _5S + _padding;
        inline static const QString _type      = _NewLine + "type:"      + _5S + _padding;
        inline static const QString _full      = _NewLine + "full:"      + _5S + _padding;
        inline static const QString _size      = _NewLine + "size:"      + _5S + _padding;
        inline static const QString _count     = _NewLine + "count:"     + _4S + _padding;
        inline static const QString _seqNo     = _NewLine + "seqNo:"     + _4S + _padding;
        inline static const QString _inCount   = _NewLine + "inCount:"   + _2S + _padding;
        inline static const QString _inBytes   = _NewLine + "inBytes:"   + _2S + _padding;
        inline static const QString _outCount  = _NewLine + "outCount:"  + _S  + _padding;
        inline static const QString _outBytes  = _NewLine + "outBytes:"  + _S  + _padding; // LONGEST

public:
    /**
     * @brief The QueueMxTelemetryStructIndex enum corresponds to following:
     *
     *  struct Queue { // graphable
     *      MxIDString	id;         // primary key - is same as Link id for Rx/Tx
     *      uint64_t	seqNo;		// dynamic - 0 for Thread, IPC
     *      uint64_t	count;		// dynamic - due to overlaps, may not equal in - out
     *      uint64_t	inCount;	// dynamic(*)
     *      uint64_t	inBytes;	// dynamic
     *      uint64_t	outCount;	// dynamic(*)
     *      uint64_t	outBytes;	// dynamic
     *      uint32_t	full;		// dynamic - not graphable - how many times queue overflowed
     *      uint32_t	size;		// static - 0 for Rx, Tx
     *      uint8_t	type;           // static - QueueType
     *   };
     *
     */
    enum QueueMxTelemetryStructIndex {e_id,      e_seqNo,    e_count,    e_inCount,
                                      e_inBytes, e_outCount, e_outBytes, e_full,
                                      e_size,    e_type};

    int _getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    void _getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept override final;
    virtual const QString _getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept override final;

};

#endif // MXTELEMETRYQUEUEWRAPPER_H
