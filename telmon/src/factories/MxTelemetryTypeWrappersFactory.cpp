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

// based on:
// https://stackoverflow.com/questions/86582/singleton-how-should-it-be-used


#include "MxTelemetry.hpp"
#include "src/factories/MxTelemetryTypeWrappersFactory.h"
#include "QDebug"

#include "src/utilities/typeWrappers/MxTelemetryHeapWrapper.h"
#include "src/utilities/typeWrappers/MxTelemetryHashTblWrapper.h"
#include "src/utilities/typeWrappers/MxTelemetryZmThreadWrapper.h"
#include "src/utilities/typeWrappers/MxTelemetryZiMultiplexerWrapper.h"
#include "src/utilities/typeWrappers/MxTelemetryZiCxnWrapper.h"
#include "src/utilities/typeWrappers/MxTelemetryQueueWrapper.h"
#include "src/utilities/typeWrappers/MxTelemetryEngineWrapper.h"
#include "src/utilities/typeWrappers/MxTelemetryLinkWrapper.h"
#include "src/utilities/typeWrappers/MxTelemetryDBEnvWrapper.h"
#include "src/utilities/typeWrappers/MxTelemetryDBHostWrapper.h"
#include "src/utilities/typeWrappers/MxTelemetryDBWrapper.h"


MxTelemetryTypeWrappersFactory::MxTelemetryTypeWrappersFactory()
{

}


const MxTelemetryGeneralWrapper& MxTelemetryTypeWrappersFactory::getMxTelemetryWrapper(const int& a_mxType) const noexcept
{
    MxTelemetryGeneralWrapper* l_result = nullptr;
    switch (a_mxType)
    {
    case MxTelemetry::Type::Heap:
        l_result = &MxTelemetryHeapWrapper::getInstance();
        break;
    case MxTelemetry::Type::HashTbl:
        l_result = &MxTelemetryHashTblWrapper::getInstance();
        break;
    case MxTelemetry::Type::Thread:
        l_result = &MxTelemetryZmThreadWrapper::getInstance();
        break;
    case MxTelemetry::Type::Multiplexer:
        l_result = &MxTelemetryZiMultiplexerWrapper::getInstance();
        break;
    case MxTelemetry::Type::Socket:
         l_result = &MxTelemetryZiCxnWrapper::getInstance();
        break;
    case MxTelemetry::Type::Queue:
         l_result = &MxTelemetryQueueWrapper::getInstance();
        break;
    case MxTelemetry::Type::Engine:
        l_result = &MxTelemetryEngineWrapper::getInstance();
        break;
    case MxTelemetry::Type::Link:
        l_result = &MxTelemetryLinkWrapper::getInstance();
        break;
    case MxTelemetry::Type::DBEnv:
        l_result = &MxTelemetryDBEnvWrapper::getInstance();
        break;
    case MxTelemetry::Type::DBHost:
        l_result = &MxTelemetryDBHostWrapper::getInstance();
        break;
    case MxTelemetry::Type::DB:
        l_result = &MxTelemetryDBWrapper::getInstance();
        break;
    default:
        qWarning() << "unknown MxTelemetry::Type:" << a_mxType<< "request, returning...";
        break;
    }
    return *l_result;
}





