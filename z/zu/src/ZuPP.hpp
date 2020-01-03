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

// C/C++ pre-processor macros

#ifndef ZuPP_HPP
#define ZuPP_HPP

// use "ZuPP_Strip1 ZuPP_Strip2(x)" to strip x of parentheses
#define ZuPP_Strip1(...) __VA_ARGS__ 
#define ZuPP_Strip2(x) x 

#define ZuPP_Eval(...) ZuPP_Eval32(__VA_ARGS__)
// #define ZuPP_Eval(...) ZuPP_Eval1024(__VA_ARGS__)
// #define ZuPP_Eval1024(...) ZuPP_Eval512(ZuPP_Eval512(__VA_ARGS__))
// #define ZuPP_Eval512(...) ZuPP_Eval256(ZuPP_Eval256(__VA_ARGS__))
// #define ZuPP_Eval256(...) ZuPP_Eval128(ZuPP_Eval128(__VA_ARGS__))
// #define ZuPP_Eval128(...) ZuPP_Eval64(ZuPP_Eval64(__VA_ARGS__))
// #define ZuPP_Eval64(...) ZuPP_Eval32(ZuPP_Eval32(__VA_ARGS__))
#define ZuPP_Eval32(...) ZuPP_Eval16(ZuPP_Eval16(__VA_ARGS__))
#define ZuPP_Eval16(...) ZuPP_Eval8(ZuPP_Eval8(__VA_ARGS__))
#define ZuPP_Eval8(...) ZuPP_Eval4(ZuPP_Eval4(__VA_ARGS__))
#define ZuPP_Eval4(...) ZuPP_Eval2(ZuPP_Eval2(__VA_ARGS__))
#define ZuPP_Eval2(...) ZuPP_Eval1(ZuPP_Eval1(__VA_ARGS__))
#define ZuPP_Eval1(...) __VA_ARGS__

#define ZuPP_Empty()

#define ZuPP_Defer(x) x ZuPP_Empty ZuPP_Empty()()

// map expansions - the *Comma versions suppress trailing commas

#define ZuPP_Map(map, first, ...) \
  map(first) __VA_OPT__(ZuPP_Defer(ZuPP_Map_)()(map, __VA_ARGS__))
#define ZuPP_Map_() ZuPP_Map

#define ZuPP_MapComma(map, first, ...) \
  map(first) __VA_OPT__(, ZuPP_Defer(ZuPP_MapComma_)()(map, __VA_ARGS__))
#define ZuPP_MapComma_() ZuPP_MapComma

#define ZuPP_MapArg(map, arg, first, ...) \
  map(arg, first) __VA_OPT__(ZuPP_Defer(ZuPP_MapArg_)()(map, arg, __VA_ARGS__))
#define ZuPP_MapArg_() ZuPP_MapArg

#define ZuPP_MapArgComma(map, arg, first, ...) \
  map(arg, first) \
  __VA_OPT__(, ZuPP_Defer(ZuPP_MapArgComma_)()(map, arg, __VA_ARGS__))
#define ZuPP_MapArgComma_() ZuPP_MapArgComma

#define ZuPP_MapIndex(map, i, first, ...) \
  map(i, first) \
  __VA_OPT__(ZuPP_Defer(ZuPP_MapIndex_)()(map, (i + 1), __VA_ARGS__))
#define ZuPP_MapIndex_() ZuPP_MapIndex

#define ZuPP_MapIndexComma(map, i, first, ...) \
  map(i, first) \
  __VA_OPT__(, ZuPP_Defer(ZuPP_MapIndexComma_)()(map, (i + 1), __VA_ARGS__))
#define ZuPP_MapIndexCommaComma_() ZuPP_MapIndexComma

#endif /* ZuPP_HPP */
