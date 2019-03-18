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



#ifndef MXTELEMETRYGENERALWRAPPER_H
#define MXTELEMETRYGENERALWRAPPER_H

#include <array> // for std::array

template <class T>
class QList;

class QString;

template <class T>
class QVector;

template <class T, class H>
class QPair;


/**
 * @brief The MxTelemetryGeneralWrapper class
 * General interfacet
 *
 * could not create the interface as singleton
 */
class MxTelemetryGeneralWrapper
{
public:
    //friend class MxTelemetryTypeWrappersFactory;
    enum REQUESTING_DATA {TABLE, CHART };
    enum CONVERT_FRON {type_uint64_t, type_uint32_t, type_uint16_t, type_uint8_t,
                       type_none, type_c_char};

    MxTelemetryGeneralWrapper();
    virtual ~MxTelemetryGeneralWrapper();
    // Stop the compiler generating methods of copy the object
    MxTelemetryGeneralWrapper(MxTelemetryGeneralWrapper const& copy);            // Not Implemented
    MxTelemetryGeneralWrapper& operator=(MxTelemetryGeneralWrapper const& copy); // Not Implemented

    const virtual QList<QString>& getTableList() const noexcept = 0;
    const virtual QVector<QString>& getChartList() const noexcept = 0;

    /**
     * @brief getActiveDataSet 0=Y-Left, 1=Y-right
     * @return
     */
    const virtual std::array<int, 2>&  getActiveDataSet() const noexcept = 0;

    /**
     * @brief function to be used by chart
     *  we represent unused data type by "none"
     * @param a_index
     * @return
     */
    virtual bool isDataTypeNotUsed(const int a_index) const noexcept = 0;

    /**
     * @brief get data for index by !CHART PRIORITY!
     *        we translate the corresponding index used by chart
     *        to the corresponding data type in type a_mxTelemetryMsg
     * @param a_mxTelemetryMsg
     * @param a_index
     * @return
     */
    double virtual getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept = 0;


    // service functions
    //template <class T>
    double typeConvertor(const QPair<void*, int>& a_param) const noexcept;

protected:
    /**
     * @brief initTableList accroding to priorites in MxTelClient
     */
    void virtual initTableList() noexcept = 0;

    /**
     * @brief initChartList according to priorites in MxTelClient
     */
    void virtual initChartList() noexcept = 0;

    /**
     * @brief initActiveDataSet which represents the
     *  Y left axis and Y right axis
     */
    void virtual initActiveDataSet() noexcept = 0;
};


//void open() {
//  open_(m_heap, "heap");
//  write_(m_heap, "time,id,size,alignment,partition,sharded,cacheSize,cpuset,cacheAllocs,heapAllocs,frees,allocated\n");
//  open_(m_hashTbl, "hashTbl");
//  write_(m_hashTbl, "time,id,linear,bits,slots,cBits,cSlots,count,resized,loadFactor,effLoadFactor,nodeSize\n");
//  open_(m_thread, "thread");
//  write_(m_thread,"time,name,id,tid,cpuUsage,cpuset,priority,stackSize,partition,main,detached\n");
//  open_(m_multiplexer, "multiplexer");
//  write_(m_multiplexer, "time,id,state,nThreads,priority,partition,isolation,rxThread,txThread,stackSize,rxBufSize,txBufSize\n");
//  open_(m_socket, "socket");
//  write_(m_socket, "time,mxID,type,remoteIP,remotePort,localIP,localPort,fd,flags,mreqAddr,mreqIf,mif,ttl,rxBufSize,rxBufLen,txBufSize,txBufLen\n");
//  open_(m_queue, "queue");
//  write_(m_queue, "time,id,type,full,size,count,seqNo,inCount,inBytes,outCount,outBytes\n");
//  open_(m_engine, "engine");
//  write_(m_engine, "time,id,state,nLinks,up,down,disabled,transient,reconn,failed,mxID,rxThread,txThread\n");
//  open_(m_link, "link");
//  write_(m_link, "time,id,state,reconnects,rxSeqNo,txSeqNo\n");
//  open_(m_dbenv, "dbenv");
//  write_(m_dbenv, "time,self,master,prev,next,state,active,recovering,replicating,nDBs,nHosts,nPeers,nCxns,heartbeatFreq,heartbeatTimeout,reconnectFreq,electionTimeout,writeThread\n");
//  open_(m_dbhost, "dbhost");
//  write_(m_dbhost, "time,id,priority,state,voted,ip,port\n");
//  open_(m_db, "db");
//  write_(m_db, "time,name,id,recSize,compress,cacheMode,cacheSize,path,fileSize,fileRecs,filesMax,preAlloc,minRN,allocRN,fileRN,cacheLoads,cacheMisses,fileLoads,fileMisses\n");
//}





#endif // MXTELEMETRYGENERALWRAPPER_H
