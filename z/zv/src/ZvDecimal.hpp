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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// flatbuffer load/save for ZuDecimal

#ifndef ZvDecimal_HPP
#define ZvDecimal_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZuDecimal.hpp>

#include <zlib/Zfb.hpp>

#include <zlib/decimal_fbs.h>

namespace Zfb::Save {
  template <typename B>
  ZuInline auto decimal(B &b, const ZuDecimal &v) {
    return fbs::CreateDecimal(b, (uint64_t)(v>>64), (uint64_t)v);
  }
}
namespace Zfb::Load {
  ZuInline ZuDecimal decimal(const fbs::Decimal *v) {
    return ZuDecimal{ZuDecimal::NoScale, (((int128_t)(v->h()))<<64) | v->l()};
  }
}

#endif /* ZvDecimal_HPP */
