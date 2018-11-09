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

#define ZuPP_Args(...) __VA_ARGS__
#define ZuPP_Strip(x) x

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

#define ZuPP_Map(map, first, ...) \
  map(first) __VA_OPT__(ZuPP_Defer(_ZuPP_Map)()(map, __VA_ARGS__))
#define _ZuPP_Map() ZuPP_Map

#define ZuPP_Map2(map, first, second, ...) \
  map(first, second) __VA_OPT__(ZuPP_Defer(_ZuPP_Map2)()(map, __VA_ARGS__))
#define _ZuPP_Map2() ZuPP_Map2

#ifndef ZuPP_N
#define ZuPP_N 30       // max. # expansions supported
#endif

// white-space separated list expansion for repeated code
#define ZuPP_List_0(L)
#define ZuPP_List_1(L)                  L(1)
#define ZuPP_List_2(L)  ZuPP_List_1(L)  L(2)
#define ZuPP_List_3(L)  ZuPP_List_2(L)  L(3)
#define ZuPP_List_4(L)  ZuPP_List_3(L)  L(4)
#define ZuPP_List_5(L)  ZuPP_List_4(L)  L(5)
#define ZuPP_List_6(L)  ZuPP_List_5(L)  L(6)
#define ZuPP_List_7(L)  ZuPP_List_6(L)  L(7)
#define ZuPP_List_8(L)  ZuPP_List_7(L)  L(8)
#define ZuPP_List_9(L)  ZuPP_List_8(L)  L(9)
#define ZuPP_List_10(L) ZuPP_List_9(L)  L(10)
#define ZuPP_List_11(L) ZuPP_List_10(L) L(11)
#define ZuPP_List_12(L) ZuPP_List_11(L) L(12)
#define ZuPP_List_13(L) ZuPP_List_12(L) L(13)
#define ZuPP_List_14(L) ZuPP_List_13(L) L(14)
#define ZuPP_List_15(L) ZuPP_List_14(L) L(15)
#define ZuPP_List_16(L) ZuPP_List_15(L) L(16)
#define ZuPP_List_17(L) ZuPP_List_16(L) L(17)
#define ZuPP_List_18(L) ZuPP_List_17(L) L(18)
#define ZuPP_List_19(L) ZuPP_List_18(L) L(19)
#define ZuPP_List_20(L) ZuPP_List_19(L) L(20)
#define ZuPP_List_21(L) ZuPP_List_20(L) L(21)
#define ZuPP_List_22(L) ZuPP_List_21(L) L(22)
#define ZuPP_List_23(L) ZuPP_List_22(L) L(23)
#define ZuPP_List_24(L) ZuPP_List_23(L) L(24)
#define ZuPP_List_25(L) ZuPP_List_24(L) L(25)
#define ZuPP_List_26(L) ZuPP_List_25(L) L(26)
#define ZuPP_List_27(L) ZuPP_List_26(L) L(27)
#define ZuPP_List_28(L) ZuPP_List_27(L) L(28)
#define ZuPP_List_29(L) ZuPP_List_28(L) L(29)
#define ZuPP_List_30(L) ZuPP_List_29(L) L(30)

#define ZuPP_List(L, I) ZuPP_List_(L, I)
#define ZuPP_List_(L, I) ZuPP_List_##I(L)

// cpp does not support recursive macro expansion, so an identical but
// differently named macro is required for lists within lists
#define ZuPP_List1_0(L, C)
#define ZuPP_List1_1(L, C)                      L(1, C)
#define ZuPP_List1_2(L, C)  ZuPP_List1_1(L, C)  L(2, C)
#define ZuPP_List1_3(L, C)  ZuPP_List1_2(L, C)  L(3, C)
#define ZuPP_List1_4(L, C)  ZuPP_List1_3(L, C)  L(4, C)
#define ZuPP_List1_5(L, C)  ZuPP_List1_4(L, C)  L(5, C)
#define ZuPP_List1_6(L, C)  ZuPP_List1_5(L, C)  L(6, C)
#define ZuPP_List1_7(L, C)  ZuPP_List1_6(L, C)  L(7, C)
#define ZuPP_List1_8(L, C)  ZuPP_List1_7(L, C)  L(8, C)
#define ZuPP_List1_9(L, C)  ZuPP_List1_8(L, C)  L(9, C)
#define ZuPP_List1_10(L, C) ZuPP_List1_9(L, C)  L(10, C)
#define ZuPP_List1_11(L, C) ZuPP_List1_10(L, C) L(11, C)
#define ZuPP_List1_12(L, C) ZuPP_List1_11(L, C) L(12, C)
#define ZuPP_List1_13(L, C) ZuPP_List1_12(L, C) L(13, C)
#define ZuPP_List1_14(L, C) ZuPP_List1_13(L, C) L(14, C)
#define ZuPP_List1_15(L, C) ZuPP_List1_14(L, C) L(15, C)
#define ZuPP_List1_16(L, C) ZuPP_List1_15(L, C) L(16, C)
#define ZuPP_List1_17(L, C) ZuPP_List1_16(L, C) L(17, C)
#define ZuPP_List1_18(L, C) ZuPP_List1_17(L, C) L(18, C)
#define ZuPP_List1_19(L, C) ZuPP_List1_18(L, C) L(19, C)
#define ZuPP_List1_20(L, C) ZuPP_List1_19(L, C) L(20, C)
#define ZuPP_List1_21(L, C) ZuPP_List1_20(L, C) L(21, C)
#define ZuPP_List1_22(L, C) ZuPP_List1_21(L, C) L(22, C)
#define ZuPP_List1_23(L, C) ZuPP_List1_22(L, C) L(23, C)
#define ZuPP_List1_24(L, C) ZuPP_List1_23(L, C) L(24, C)
#define ZuPP_List1_25(L, C) ZuPP_List1_24(L, C) L(25, C)
#define ZuPP_List1_26(L, C) ZuPP_List1_25(L, C) L(26, C)
#define ZuPP_List1_27(L, C) ZuPP_List1_26(L, C) L(27, C)
#define ZuPP_List1_28(L, C) ZuPP_List1_27(L, C) L(28, C)
#define ZuPP_List1_29(L, C) ZuPP_List1_28(L, C) L(29, C)
#define ZuPP_List1_30(L, C) ZuPP_List1_29(L, C) L(30, C)

#define ZuPP_List1(L, C, I) ZuPP_List1_(L, C, I)
#define ZuPP_List1_(L, C, I) ZuPP_List1_##I(L, C)

// prefixed comma-separated list expansion for argument lists
#define ZuPP_PList_0(P, L)                   P
#define ZuPP_PList_1(P, L)                   P,  L(1)
#define ZuPP_PList_2(P, L)  ZuPP_PList_1(P, L),  L(2)
#define ZuPP_PList_3(P, L)  ZuPP_PList_2(P, L),  L(3)
#define ZuPP_PList_4(P, L)  ZuPP_PList_3(P, L),  L(4)
#define ZuPP_PList_5(P, L)  ZuPP_PList_4(P, L),  L(5)
#define ZuPP_PList_6(P, L)  ZuPP_PList_5(P, L),  L(6)
#define ZuPP_PList_7(P, L)  ZuPP_PList_6(P, L),  L(7)
#define ZuPP_PList_8(P, L)  ZuPP_PList_7(P, L),  L(8)
#define ZuPP_PList_9(P, L)  ZuPP_PList_8(P, L),  L(9)
#define ZuPP_PList_10(P, L) ZuPP_PList_9(P, L),  L(10)
#define ZuPP_PList_11(P, L) ZuPP_PList_10(P, L), L(11)
#define ZuPP_PList_12(P, L) ZuPP_PList_11(P, L), L(12)
#define ZuPP_PList_13(P, L) ZuPP_PList_12(P, L), L(13)
#define ZuPP_PList_14(P, L) ZuPP_PList_13(P, L), L(14)
#define ZuPP_PList_15(P, L) ZuPP_PList_14(P, L), L(15)
#define ZuPP_PList_16(P, L) ZuPP_PList_15(P, L), L(16)
#define ZuPP_PList_17(P, L) ZuPP_PList_16(P, L), L(17)
#define ZuPP_PList_18(P, L) ZuPP_PList_17(P, L), L(18)
#define ZuPP_PList_19(P, L) ZuPP_PList_18(P, L), L(19)
#define ZuPP_PList_20(P, L) ZuPP_PList_19(P, L), L(20)
#define ZuPP_PList_21(P, L) ZuPP_PList_20(P, L), L(21)
#define ZuPP_PList_22(P, L) ZuPP_PList_21(P, L), L(22)
#define ZuPP_PList_23(P, L) ZuPP_PList_22(P, L), L(23)
#define ZuPP_PList_24(P, L) ZuPP_PList_23(P, L), L(24)
#define ZuPP_PList_25(P, L) ZuPP_PList_24(P, L), L(25)
#define ZuPP_PList_26(P, L) ZuPP_PList_25(P, L), L(26)
#define ZuPP_PList_27(P, L) ZuPP_PList_26(P, L), L(27)
#define ZuPP_PList_28(P, L) ZuPP_PList_27(P, L), L(28)
#define ZuPP_PList_29(P, L) ZuPP_PList_28(P, L), L(29)
#define ZuPP_PList_30(P, L) ZuPP_PList_29(P, L), L(30)

#define ZuPP_PList(P, L, I) ZuPP_PList_(P, L, I)
#define ZuPP_PList_(P, L, I) ZuPP_PList_##I(P, L)

// white-spaced list expansion with shifted parameter
#define ZuPP_SList_1(L)
#define ZuPP_SList_2(L)                   L(1, 2)
#define ZuPP_SList_3(L)  ZuPP_SList_2(L)  L(2, 3)
#define ZuPP_SList_4(L)  ZuPP_SList_3(L)  L(3, 4)
#define ZuPP_SList_5(L)  ZuPP_SList_4(L)  L(4, 5)
#define ZuPP_SList_6(L)  ZuPP_SList_5(L)  L(5, 6)
#define ZuPP_SList_7(L)  ZuPP_SList_6(L)  L(6, 7)
#define ZuPP_SList_8(L)  ZuPP_SList_7(L)  L(7, 8)
#define ZuPP_SList_9(L)  ZuPP_SList_8(L)  L(8, 9)
#define ZuPP_SList_10(L) ZuPP_SList_9(L)  L(9, 10)
#define ZuPP_SList_11(L) ZuPP_SList_10(L) L(10, 11)
#define ZuPP_SList_12(L) ZuPP_SList_11(L) L(11, 12)
#define ZuPP_SList_13(L) ZuPP_SList_12(L) L(12, 13)
#define ZuPP_SList_14(L) ZuPP_SList_13(L) L(13, 14)
#define ZuPP_SList_15(L) ZuPP_SList_14(L) L(14, 15)
#define ZuPP_SList_16(L) ZuPP_SList_15(L) L(15, 16)
#define ZuPP_SList_17(L) ZuPP_SList_16(L) L(16, 17)
#define ZuPP_SList_18(L) ZuPP_SList_17(L) L(17, 18)
#define ZuPP_SList_19(L) ZuPP_SList_18(L) L(18, 19)
#define ZuPP_SList_20(L) ZuPP_SList_19(L) L(19, 20)
#define ZuPP_SList_21(L) ZuPP_SList_20(L) L(20, 21)
#define ZuPP_SList_22(L) ZuPP_SList_21(L) L(21, 22)
#define ZuPP_SList_23(L) ZuPP_SList_22(L) L(22, 23)
#define ZuPP_SList_24(L) ZuPP_SList_23(L) L(23, 24)
#define ZuPP_SList_25(L) ZuPP_SList_24(L) L(24, 25)
#define ZuPP_SList_26(L) ZuPP_SList_25(L) L(25, 26)
#define ZuPP_SList_27(L) ZuPP_SList_26(L) L(26, 27)
#define ZuPP_SList_28(L) ZuPP_SList_27(L) L(27, 28)
#define ZuPP_SList_29(L) ZuPP_SList_28(L) L(28, 29)
#define ZuPP_SList_30(L) ZuPP_SList_29(L) L(29, 30)

#define ZuPP_SList(L, I) ZuPP_SList_(L, I)
#define ZuPP_SList_(L, I) ZuPP_SList_##I(L)

// comma-separated list expansion for argument lists
#define ZuPP_CList_0(L)
#define ZuPP_CList_1(L)                    L(1)
#define ZuPP_CList_2(L)  ZuPP_CList_1(L),  L(2)
#define ZuPP_CList_3(L)  ZuPP_CList_2(L),  L(3)
#define ZuPP_CList_4(L)  ZuPP_CList_3(L),  L(4)
#define ZuPP_CList_5(L)  ZuPP_CList_4(L),  L(5)
#define ZuPP_CList_6(L)  ZuPP_CList_5(L),  L(6)
#define ZuPP_CList_7(L)  ZuPP_CList_6(L),  L(7)
#define ZuPP_CList_8(L)  ZuPP_CList_7(L),  L(8)
#define ZuPP_CList_9(L)  ZuPP_CList_8(L),  L(9)
#define ZuPP_CList_10(L) ZuPP_CList_9(L),  L(10)
#define ZuPP_CList_11(L) ZuPP_CList_10(L), L(11)
#define ZuPP_CList_12(L) ZuPP_CList_11(L), L(12)
#define ZuPP_CList_13(L) ZuPP_CList_12(L), L(13)
#define ZuPP_CList_14(L) ZuPP_CList_13(L), L(14)
#define ZuPP_CList_15(L) ZuPP_CList_14(L), L(15)
#define ZuPP_CList_16(L) ZuPP_CList_15(L), L(16)
#define ZuPP_CList_17(L) ZuPP_CList_16(L), L(17)
#define ZuPP_CList_18(L) ZuPP_CList_17(L), L(18)
#define ZuPP_CList_19(L) ZuPP_CList_18(L), L(19)
#define ZuPP_CList_20(L) ZuPP_CList_19(L), L(20)
#define ZuPP_CList_21(L) ZuPP_CList_20(L), L(21)
#define ZuPP_CList_22(L) ZuPP_CList_21(L), L(22)
#define ZuPP_CList_23(L) ZuPP_CList_22(L), L(23)
#define ZuPP_CList_24(L) ZuPP_CList_23(L), L(24)
#define ZuPP_CList_25(L) ZuPP_CList_24(L), L(25)
#define ZuPP_CList_26(L) ZuPP_CList_25(L), L(26)
#define ZuPP_CList_27(L) ZuPP_CList_26(L), L(27)
#define ZuPP_CList_28(L) ZuPP_CList_27(L), L(28)
#define ZuPP_CList_29(L) ZuPP_CList_28(L), L(29)
#define ZuPP_CList_30(L) ZuPP_CList_29(L), L(30)

#define ZuPP_CList(L, I) ZuPP_CList_(L, I)
#define ZuPP_CList_(L, I) ZuPP_CList_##I(L)

// comma-separated list expansion for completing (trailing) argument lists
#define ZuPP_CListT_0(L)    L(1)  ZuPP_CListT_1(L)
#if ZuPP_N > 1
#define ZuPP_CListT_1(L)  , L(2)  ZuPP_CListT_2(L)
#if ZuPP_N > 2
#define ZuPP_CListT_2(L)  , L(3)  ZuPP_CListT_3(L)
#if ZuPP_N > 3
#define ZuPP_CListT_3(L)  , L(4)  ZuPP_CListT_4(L)
#if ZuPP_N > 4
#define ZuPP_CListT_4(L)  , L(5)  ZuPP_CListT_5(L)
#if ZuPP_N > 5
#define ZuPP_CListT_5(L)  , L(6)  ZuPP_CListT_6(L)
#if ZuPP_N > 6
#define ZuPP_CListT_6(L)  , L(7)  ZuPP_CListT_7(L)
#if ZuPP_N > 7
#define ZuPP_CListT_7(L)  , L(8)  ZuPP_CListT_8(L)
#if ZuPP_N > 8
#define ZuPP_CListT_8(L)  , L(9)  ZuPP_CListT_9(L)
#if ZuPP_N > 9
#define ZuPP_CListT_9(L)  , L(10) ZuPP_CListT_10(L)
#if ZuPP_N > 10
#define ZuPP_CListT_10(L) , L(11) ZuPP_CListT_11(L)
#if ZuPP_N > 11
#define ZuPP_CListT_11(L) , L(12) ZuPP_CListT_12(L)
#if ZuPP_N > 12
#define ZuPP_CListT_12(L) , L(13) ZuPP_CListT_13(L)
#if ZuPP_N > 13
#define ZuPP_CListT_13(L) , L(14) ZuPP_CListT_14(L)
#if ZuPP_N > 14
#define ZuPP_CListT_14(L) , L(15) ZuPP_CListT_15(L)
#if ZuPP_N > 15
#define ZuPP_CListT_15(L) , L(16) ZuPP_CListT_16(L)
#if ZuPP_N > 16
#define ZuPP_CListT_16(L) , L(17) ZuPP_CListT_17(L)
#if ZuPP_N > 17
#define ZuPP_CListT_17(L) , L(18) ZuPP_CListT_18(L)
#if ZuPP_N > 18
#define ZuPP_CListT_18(L) , L(19) ZuPP_CListT_19(L)
#if ZuPP_N > 19
#define ZuPP_CListT_19(L) , L(20) ZuPP_CListT_20(L)
#if ZuPP_N > 20
#define ZuPP_CListT_20(L) , L(21) ZuPP_CListT_21(L)
#if ZuPP_N > 21
#define ZuPP_CListT_21(L) , L(22) ZuPP_CListT_22(L)
#if ZuPP_N > 22
#define ZuPP_CListT_22(L) , L(23) ZuPP_CListT_23(L)
#if ZuPP_N > 23
#define ZuPP_CListT_23(L) , L(24) ZuPP_CListT_24(L)
#if ZuPP_N > 24
#define ZuPP_CListT_24(L) , L(25) ZuPP_CListT_25(L)
#if ZuPP_N > 25
#define ZuPP_CListT_25(L) , L(26) ZuPP_CListT_26(L)
#if ZuPP_N > 26
#define ZuPP_CListT_26(L) , L(27) ZuPP_CListT_27(L)
#if ZuPP_N > 27
#define ZuPP_CListT_27(L) , L(28) ZuPP_CListT_28(L)
#if ZuPP_N > 28
#define ZuPP_CListT_28(L) , L(29) ZuPP_CListT_29(L)
#if ZuPP_N > 29
#define ZuPP_CListT_29(L) , L(30) ZuPP_CListT_30(L)
#define ZuPP_CListT_30(L)
#else
#define ZuPP_CListT_29(L)
#endif
#else
#define ZuPP_CListT_28(L)
#endif
#else
#define ZuPP_CListT_27(L)
#endif
#else
#define ZuPP_CListT_26(L)
#endif
#else
#define ZuPP_CListT_25(L)
#endif
#else
#define ZuPP_CListT_24(L)
#endif
#else
#define ZuPP_CListT_23(L)
#endif
#else
#define ZuPP_CListT_22(L)
#endif
#else
#define ZuPP_CListT_21(L)
#endif
#else
#define ZuPP_CListT_20(L)
#endif
#else
#define ZuPP_CListT_19(L)
#endif
#else
#define ZuPP_CListT_18(L)
#endif
#else
#define ZuPP_CListT_17(L)
#endif
#else
#define ZuPP_CListT_16(L)
#endif
#else
#define ZuPP_CListT_15(L)
#endif
#else
#define ZuPP_CListT_14(L)
#endif
#else
#define ZuPP_CListT_13(L)
#endif
#else
#define ZuPP_CListT_12(L)
#endif
#else
#define ZuPP_CListT_11(L)
#endif
#else
#define ZuPP_CListT_10(L)
#endif
#else
#define ZuPP_CListT_9(L)
#endif
#else
#define ZuPP_CListT_8(L)
#endif
#else
#define ZuPP_CListT_7(L)
#endif
#else
#define ZuPP_CListT_6(L)
#endif
#else
#define ZuPP_CListT_5(L)
#endif
#else
#define ZuPP_CListT_4(L)
#endif
#else
#define ZuPP_CListT_3(L)
#endif
#else
#define ZuPP_CListT_2(L)
#endif
#else
#define ZuPP_CListT_1(L)
#endif

#define ZuPP_CListT(L, I) ZuPP_CListT_(L, I)
#define ZuPP_CListT_(L, I) ZuPP_CListT_##I(L)

#endif /* ZuPP_HPP */
