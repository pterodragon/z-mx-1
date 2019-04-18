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



#ifndef MXTELEMETRYHEAPWRAPPER_H
#define MXTELEMETRYHEAPWRAPPER_H



#include "src/utilities/typeWrappers/MxTelemetryGeneralWrapper.h"


/**
 * @brief The MxTelemetryHeapWrapper class
 * Wrapper for struct ZmHeapTelemetry in ZmHeap.hpp
 * This should be the only place in the app that
 * this data type releated actions should be written
 * i.e. setting priorites, all other places should
 * be updated according to this end point
 */
class MxTelemetryHeapWrapper : public MxTelemetryGeneralWrapper
{
private:
    // Private Constructor
    MxTelemetryHeapWrapper();
    virtual ~MxTelemetryHeapWrapper() override final;

    // Stop the compiler generating methods of copy the object
    MxTelemetryHeapWrapper(MxTelemetryHeapWrapper const& copy);            // Not Implemented
    MxTelemetryHeapWrapper& operator=(MxTelemetryHeapWrapper const& copy); // Not Implemented

protected:
    friend class MxTelemetryTypeWrappersFactory;
    // protected so only friend class can access // to be tested
    // Not part of the inferface
    static MxTelemetryHeapWrapper& getInstance()
    {
        // The only instance
        // Guaranteed to be lazy initialized
        // Guaranteed that it will be destroyed correctly
        static MxTelemetryHeapWrapper m_instance;
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
    // to do generic function which calculates tabs
//    template <int N>
//    class array_string
//    {
//    public:
//      char _array[N + 1];

//    public:
//        array_string()
//        {
//            for (int i = 0; i < N; i++) {
//                _array[i] = 'a';
//            }
//            _array[N] = '\0';
//        }

//    };

//    template <int N1, int N2>
//      constexpr auto connect() -> array_string<N1 + N2>
//    {
//      return array_string<N1 + N2>();
//    }
    // inline static const QString _Frees       = _NewLine + "frees:"       /*+ _Tab*/ + QString(array_string<10>()._array);
    // read more
    // https://www.google.co.jp/search?client=ubuntu&hs=naL&q=connecting+chars+in+constexpr+function&spell=1&sa=X&ved=0ahUKEwiz9Kyqs9bhAhUkE6YKHThnCwkQBQgpKAA&biw=927&bih=953
    // https://akrzemi1.wordpress.com/2017/06/28/compile-time-string-concatenation/
    // https://stackoverflow.com/questions/28708497/constexpr-to-concatenate-two-or-more-char-strings
    // https://stackoverflow.com/questions/39199564/constexpr-c-string-concatenation-parameters-used-in-a-constexpr-context


    // Tags
    inline static const QString _padding    = _4S;
    inline static const QString _Title       =  _Bold_begin + "Heap::";
    inline static const QString _Time        = _NewLine + "time:"        + _Tab + _padding;
    inline static const QString _Alignemnt   = _NewLine + "alignment:"   + _3S  + _padding;
    inline static const QString _Partition   = _NewLine + "partition:"   + _3S  + _padding;
    inline static const QString _Sharded     = _NewLine + "sharded:"     + _5S  + _padding;
    inline static const QString _CacheSize   = _NewLine + "cacheSize:"   + _3S  + _padding;
    inline static const QString _CacheAllocs = _NewLine + "cacheAllocs:" + _S   + _padding;
    inline static const QString _HeapAllocs  = _NewLine + "heapAllocs:"  + _2S  + _padding;
    inline static const QString _Frees       = _NewLine + "frees:"       + _7S  + _padding;
    inline static const QString _Allocated   = _NewLine + "allocated:"   + _3S  + _padding;



public:

    /**
     * @brief The ZmHeapTelemetryStructIndex enum corresponds to following:
     *
     *  struct ZmHeapTelemetry { // graphable
     *    ZmIDString	id;         // primary key
     *    uint64_t	cacheSize;      // static
     *    uint64_t	cpuset;         // static
     *    uint64_t	cacheAllocs;	// dynamic(*)
     *    uint64_t	heapAllocs;     // dynamic(*)
     *    uint64_t	frees;          // dynamic
     *    uint32_t	size;           // static
     *    uint16_t	partition;      // static
     *    uint8_t	sharded;        // static
     *    uint8_t	alignment;      // static
     *  };
     *
     */
    enum ZmHeapTelemetryStructIndex {e_id, e_cacheSize, e_cpuset, e_cacheAllocs, e_heapAllocs,
                                     e_frees, e_size, e_partition, e_sharded, e_alignment};


    int _getDataForChart(void* const a_mxTelemetryMsg, const int a_index) const noexcept override final;
    void _getDataForTable(void* const a_mxTelemetryMsg, QLinkedList<QString>& a_result) const noexcept override final;
    virtual const QString _getDataForTabelQLabel(void* const a_mxTelemetryMsg) const noexcept override final;

};


#endif // MXTELEMETRYHEAPWRAPPER_H
