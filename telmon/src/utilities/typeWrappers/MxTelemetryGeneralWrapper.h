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
#include "QDebug"

template <class T>
class QList;

class QString;

template <class T>
class QVector;

template <class T, class H>
struct QPair;

template <class T>
class QLinkedList;


/**
 * @brief The MxTelemetryGeneralWrapper class
 * General interfacet
 *
 * could not create the interface as singleton
 */
class MxTelemetryGeneralWrapper
{
public:
    enum CONVERT_FRON {type_uint64_t, type_uint32_t, type_uint16_t, type_uint8_t,
                                      type_int32_t,                 type_int8_t,
                       type_none,
                       type_c_char,
                       type_uintptr_t,
                       type_double,
                       type_ZiIP};

    // the following struct is used to indicate cases which require calculation
    // rather than just getting MxTypeStruct specific value
    // they are organized here and shared between all types to make the
    // switch cases handling easier in inhernting classes
    // Note: this enum minimum value must be larger than the max value
    // of all the structs representing the mxTypes in inhernting classes
    // right now the largest is |DBMxTelemetryStructIndex| = 17
    // so we will begin from 40
    enum OTHER_ACTIONS {GET_CURRENT_TIME=40,
                        HEAP_MXTYPE_CALCULATE_ALLOCATED,
                        HASH_TBL_MXTYPE_CALCULATE_SLOTS, HASH_TBL_MXTYPE_CALCULATE_LOCKS};

    enum Type {Heap,
                HashTbl,
                Thread,
                Multiplexer,
                Socket,
                Queue,
                Engine,
                Link,
                DBEnv,
                DBHost,
                DB,
                Unknown};



    MxTelemetryGeneralWrapper();
    virtual ~MxTelemetryGeneralWrapper();
    // Stop the compiler generating methods of copy the object
    MxTelemetryGeneralWrapper(MxTelemetryGeneralWrapper const& copy);            // Not Implemented
    MxTelemetryGeneralWrapper& operator=(MxTelemetryGeneralWrapper const& copy); // Not Implemented

    static int fromMxTypeToTelMonType(const int) noexcept;

    const QList<QString>& getTableList() const noexcept;
    const QVector<QString>& getChartList() const noexcept;

    /**
     * @brief getActiveDataSet 0=Y-Left, 1=Y-right
     * @return
     */
    const std::array<int, 2>&  getActiveDataSet() const noexcept;


    /**
     * @brief function to be used by chart
     *  we represent unused data type by "none"
     * @param a_index
     * @return
     */
    bool isDataTypeNotUsed(const int a_index) const noexcept;


    void getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept;

    const QString getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept;

    /**
     * @brief isChartOptionEnabledInContextMenu
     * if "m_chartList" only includes "none", that is, size is 1, return false;
     * @return
     */
    bool isChartOptionEnabledInContextMenu() const noexcept;


    /**
     * @brief getPrimaryKey aka name
     * @param a_mxTelemetryMsg
     * @return
     */
    const QString getPrimaryKey(void* const) const noexcept;


    bool isChartFunctionalityEnabled() const noexcept;


    // service functions
    template <class T>
    T typeConvertor(const QPair<void*, int>& a_param) const noexcept;

    std::string getCurrentTime() const noexcept;
    static const QString getCurrentTimeQTImpl(const QString& a_format) noexcept;
    inline static const QString TIME_FORMAT__hh_mm_ss = "hh:mm:ss";

    template <class T>
    QString streamToQString(const T& a_toStream) const noexcept;

    template <class T>
    const std::string streamToStdString(const T& a_toStream) const noexcept;

    template <class T>
    const T getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept;

    static QString fromMxTypeValueToName(const int a_type) noexcept;
    inline static const QString MX_NAME_DELIMITER  = "::";

    static int fromMxTypeNameToValue(const QString& a_name) noexcept;

    static int mxTypeSize() noexcept;

    static const char* getMsgHeaderName(void* const a_mxTelemetryMsg) noexcept;

    static int getMsgHeaderType(void* const a_mxTelemetryMsg) noexcept;

    // For HTML Text
    inline static const QString _S  = "&nbsp;";
    inline static const QString _2S = _S  + _S;
    inline static const QString _3S = _2S + _S;
    inline static const QString _4S = _2S + _2S;
    inline static const QString _5S = _4S + _S;
    inline static const QString _6S = _4S + _2S;
    inline static const QString _7S = _5S + _2S;
    inline static const QString _Tab = _4S  + _4S;
    inline static const QString _Bold_begin = "<b>";
    inline static const QString _Bold_end   =  "</b>";
    inline static const QString _NewLine   =  "<br>";

    // colors
    inline static const QString _color_off    =  "<font color=\"#bec0c2\">";
    inline static const QString _color_green  =  "<font color=\"green\">";
    inline static const QString _color_amber  =  "<font color=\"#FFBF00\">";
    inline static const QString _color_red    =  "<font color=\"red\">";
    inline static const QString _font_end     =  "</font>";


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


    void setClassName(const std::string& a_className) noexcept;


    bool isIndexInChartPriorityToHeapIndexContainer(const int a_index) const noexcept;


    // QPair(void*, int); void* = the type, int = to convert
    QPair<void*, int> virtual getMxTelemetryDataType(void* const a_mxTelemetryMsg, const int a_index) const noexcept;

    // used in case of retrieving data which is callculated from some values in the struct
    QPair<void*, int> virtual getMxTelemetryDataType(void* const a_mxTelemetryMsg,
                                                     const int a_index,
                                                     void* a_otherResult) const noexcept;

    std::stringstream& getStream() const noexcept {return *m_stream;}


    QList<QString>* m_tableList; // sorted by priorites
    QVector<QString>* m_chartList; // sorted by priorites
    std::array<int, 2> m_activeDataSet; // "2" represent number of axis
    QVector<int>* m_chartPriorityToStructIndex;
    QVector<int>* m_tablePriorityToStructIndex;
    QString* m_className;
    std::stringstream* m_stream; // assistance in translation to some types

    static const char* NAME_DELIMITER;

    void* mxTelemetryMsgDataRetriever(void* const a_mxTelemetryMsg) const noexcept;

    /**
     * @brief get data for index by !CHART PRIORITY!
     *        we translate the corresponding index used by chart
     *        to the corresponding data type in type a_mxTelemetryMsg
     * @param a_mxTelemetryMsg
     * @param a_index
     * @return
     */
    int virtual _getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept = 0;

    /**
     * @brief getDataForTable
     * Notes regarding implemantation:
     * 1.
     * why QLinkedList?
     *   --> constant time insertions and removals
     *   --> iteration is the same as QList
     *
     * 2.
     * Qt containers does not support move semantics,
     * that is why we dont use std::move to move into the container
     * for more information see:
     * A. https://stackoverflow.com/questions/32584665/why-do-qts-container-classes-not-allow-movable-non-copyable-element-types
     * B. https://stackoverflow.com/questions/44217527/why-does-qt-not-support-move-only-qlist
     *
     *
     *
     * @param a_mxTelemetryMsg
     * @param a_result
     */
    void virtual _getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept  = 0;

    /**
     * @brief _getName
     * @return
     */
    virtual const QString _getPrimaryKey(void* const) const noexcept = 0;

    virtual const QString _getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept = 0;
};

// 1. supress warning Wunused template function
// 2. used for the generic functions
// 3. see https://stackoverflow.com/questions/10632251/undefined-reference-to-template-function
#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapperGenericPart.h"



#endif // MXTELEMETRYGENERALWRAPPER_H







