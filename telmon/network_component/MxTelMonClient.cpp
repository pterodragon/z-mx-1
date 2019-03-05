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


#include "MxTelMonClient.h"
#include "distributors/DataDistributor.h"

MxTelMonClient::MxTelMonClient():
    m_dataDistributor(nullptr)
{

}

MxTelMonClient::~MxTelMonClient()
{

}

void MxTelMonClient::initDataDistributor(DataDistributor* a_dataDistributor) noexcept
{
    m_dataDistributor = a_dataDistributor;
}

void MxTelMonClient::process(ZmRef<MxTelemetry::Msg> msg) {

    using namespace MxTelemetry;
    //std::cout << "##########process()\n";
    // you can use ZuMv(msg)

//    std::cout << Type::name(msg->hdr().type);
//    switch ((int)msg->hdr().type) {
//      case Type::Heap: {
//    const auto &data = msg->as<Heap>();
//    std::cout << "  id: " << data.id
//      << "  cacheSize: " << data.cacheSize
//      << "  cpuset: " << data.cpuset
//      << "  cacheAllocs: " << data.cacheAllocs
//      << "  heapAllocs: " << data.heapAllocs
//      << "  frees: " << data.frees
//      << "  allocated: " << data.allocated
//      << "  maxAllocated: " << data.maxAllocated
//      << "  size: " << data.size
//      << "  partition: " << ZuBoxed(data.partition)
//      << "  sharded: " << ZuBoxed(data.sharded)
//      << "  alignment: " << ZuBoxed(data.alignment) << '\n' << std::flush;
//      } break;
//    }

    if (m_dataDistributor)
    {
        m_dataDistributor->notify( static_cast<void*>(msg.ptr()));
    }


}



