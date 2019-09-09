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

#ifndef MXTELEMETRYZICXNWRAPPER_H
#define MXTELEMETRYZICXNWRAPPER_H


#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


/**
 * @brief This class is a
 * wrapper for struct ZiCxnTelemetry in ZiMultiplex.hpp
 * This should be the only place in the app that
 * this data type releated actions should be written
 * i.e. setting priorites, all other places should
 * be updated according to this end point
 */
class MxTelemetryZiCxnWrapper : public MxTelemetryGeneralWrapper
{
private:
    // Private Constructor
    MxTelemetryZiCxnWrapper();
    virtual ~MxTelemetryZiCxnWrapper() override final;

    // Stop the compiler generating methods of copy the object
    MxTelemetryZiCxnWrapper(MxTelemetryZiCxnWrapper const& copy);            // Not Implemented
    MxTelemetryZiCxnWrapper& operator=(MxTelemetryZiCxnWrapper const& copy); // Not Implemented


    /**
     * @brief  MxTelemetry::Socket name depends on type
     *         switch (type) {
     *           case ZiCxnType::TCPIn:   // primary key is localIP:localPort<remoteIP:remotePort
     *           case ZiCxnType::UDP:
     *           case ZiCxnType::TCPOut:  // primary key is remoteIP:remotePort<localIP:localPort
     *          }
     * @param a_list
     * @return
     */
    const std::string __getPrimaryKey(void* const a_mxTelemetryMsg, const int a_type = smallarThanSymbol::defaultValue) const noexcept;

protected:
    friend class MxTelemetryTypeWrappersFactory;
    // protected so only friend class can access // to be tested
    // Not part of the inferface
    static MxTelemetryZiCxnWrapper& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static MxTelemetryZiCxnWrapper m_instance;
        return m_instance;
    }


    void initTableList() noexcept override final;
    void initChartList() noexcept override final;
    void initActiveDataSet() noexcept override final;

    QPair<void*, int> getMxTelemetryDataType(void* const a_mxTelemetryMsg,
                                                     const int a_index,
                                                     void* a_otherResult) const noexcept override final;
    const QString _getPrimaryKey(void* const a_mxTelemetryMsg) const noexcept override final;
    const QString _getPrimaryKey(void* const a_mxTelemetryMsg, const int) const noexcept; // version for html

    // For  HTML Text
        inline static const QString _padding    = _3S;
        inline static const QString _title      = _Bold_begin + "Socket::";
        inline static const QString _time       = _NewLine + "time:"       + _7S  + _padding;
        inline static const QString _type       = _NewLine + "type:"       + _7S  + _padding;
        inline static const QString _remoteIP   = _NewLine + "remoteIP:"   + _3S  + _padding;
        inline static const QString _remotePort = _NewLine + "remotePort:" + _S   + _padding; // LONGEST
        inline static const QString _localIP    = _NewLine + "localIP:"    + _4S  + _padding;
        inline static const QString _localPort  = _NewLine + "localPort:"  + _2S  + _padding;
        inline static const QString _fd         = _NewLine + "fd:"         + _Tab + _S + _padding;
        inline static const QString _flags      = _NewLine + "flags:"      + _6S  + _padding;
        inline static const QString _mreqAddr   = _NewLine + "mreqAddr:"   + _3S  + _padding;
        inline static const QString _mreqIf     = _NewLine + "mreqIf:"     + _5S  + _padding;
        inline static const QString _mif        = _NewLine + "mif:"        + _Tab + _padding;
        inline static const QString _ttl        = _NewLine + "ttl:"        + _Tab + _padding;
        inline static const QString _rxBufSize  = _NewLine + "rxBufSize:"  + _2S  + _padding;
        inline static const QString _rxBufLen   = _NewLine + "rxBufLen:"   + _3S  + _padding;
        inline static const QString _txBufSize  = _NewLine + "txBufSize:"  + _2S  + _padding;
        inline static const QString _txBufLen   = _NewLine + "txBufLen:"   + _3S  + _padding;

        inline static const char _regularSmallarThan[]    = "<";
        inline static const char _HTMLSmallarThan[]       = "&lt;";
        enum smallarThanSymbol {defaultValue, HTML};

        const char* smallerThan(const int arg) const noexcept {
            switch (arg) {
            case defaultValue:
                return _regularSmallarThan;
            case HTML:
                return _HTMLSmallarThan;
            default:
                return _HTMLSmallarThan;
            }
        }


    //    inline static const QString _title         = _Bold_begin + "HashTbl::";
    //    inline static const QString _time          = _NewLine + "time:"          + _Tab;
    //    inline static const QString _linear        = _NewLine + "linear:"        + _Tab;
    //    inline static const QString _bits          = _NewLine + "bits:"          + _Tab;
    //    inline static const QString _slots         = _NewLine + "slots:"         + _Tab;
    //    inline static const QString _cBits         = _NewLine + "cBits:"         + _Tab;
    //    inline static const QString _locks         = _NewLine + "locks:"         + _Tab;
    //    inline static const QString _count         = _NewLine + "count:"         + _Tab;
    //    inline static const QString _resized       = _NewLine + "resized:"       + _Tab;
    //    inline static const QString _loadFactor    = _NewLine + "loadFactor:"    + _Tab;
    //    inline static const QString _effLoadFactor = _NewLine + "effLoadFactor:" + _Tab; // LONGEST
    //    inline static const QString _nodeSize      = _NewLine + "nodeSize:"      + _Tab;

public:
    /**
     * @brief The ZiCxnMxTelemetryStructIndex enum corresponds to following:
     *
     *  struct ZiCxnTelemetry { // graphable
     *    ZuID		mxID;		// static - multiplexer ID
     *    uint64_t	socket;		// dynamic - Unix file descriptor / Winsock SOCKET
     *    uint32_t	rxBufSize;	// dynamic - getsockopt(..., SO_RCVBUF, ...)
     *    uint32_t	rxBufLen;	// dynamic(*) - ioctl(..., SIOCINQ, ...)
     *    uint32_t	txBufSize;	// dynamic - getsockopt(..., SO_SNDBUF, ...)
     *    uint32_t	txBufLen;	// dynamic(*) - ioctl(..., SIOCOUTQ, ...)
     *    uint32_t	flags;		// static - ZiCxnFlags
     *    ZiIP		mreqAddr;	// static - mreqs[0]
     *    ZiIP		mreqIf;		// static - mreqs[0]
     *    ZiIP		mif;		// static
     *    uint32_t	ttl;		// static
     *    ZiIP		localIP;	// primary key - static
     *    ZiIP		remoteIP;	// primary key - static
     *    uint16_t	localPort;	// primary key - static
     *    uint16_t	remotePort;	// primary key - static
     *    uint8_t	type;		// static - ZiCxnType
     *  };
     *
     */
    enum ZiCxnMxTelemetryStructIndex {e_mxID, e_socket, e_rxBufSize, e_rxBufLen,
                                                        e_txBufSize, e_txBufLen,
                                      e_flags, e_mreqAddr, e_mreqIf, e_mif,
                                      e_ttl, e_localIP,   e_remoteIP,
                                             e_localPort, e_remotePort,
                                      e_type};

    int _getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    void _getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept override final;
    virtual const QString _getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept override final;



};



#endif // MXTELEMETRYZICXNWRAPPER_H
