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


#include "MxTelemetry.hpp"
#include "DataDistributor.h"

DataDistributor::DataDistributor()
{

}

DataDistributor::~DataDistributor()
{

}


QString DataDistributor::fromMxTypeValueToName(const unsigned int a_type) const noexcept
{
    return QString(MxTelemetry::Type::name(a_type));
}


unsigned int DataDistributor::fromMxTypeNameToValue(const QString& a_name) const noexcept
{
    return static_cast<unsigned int>(MxTelemetry::Type::lookup(a_name.toStdString().c_str()));
}


unsigned int DataDistributor::mxTypeSize() const noexcept
{
    return MxTelemetry::Type::N;
}
