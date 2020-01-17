//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

// control of global new/delete operators

#ifndef ZuNew_HPP
#define ZuNew_HPP

#ifdef _MSC_VER
#pragma once
#endif

#if !defined(__mips64)
#include <new>	// placement new
#else
#include <stddef.h> // size_t

// don't allow standard new/delete - throw a linker error
#define ZuNew_new_Failed(N) extern void *ZuNew_##N##_FAILED(); \
  return ZuNew_##N##_FAILED()
#define ZuNew_delete_Failed(N) extern void ZuNew_##N##_FAILED(); \
  ZuNew_##N##_FAILED(); return

void *operator new(size_t sz) { ZuNew_new_Failed(new); }
void *operator new[](size_t sz) { ZuNew_new_Failed(new_array); }
void operator delete(void *ptr) { ZuNew_delete_Failed(delete); }
void operator delete[](void *ptr) { ZuNew_delete_Failed(delete_array); }

// placement new
void *operator new(size_t sz, void *ptr) { return ptr; }
void *operator new[](size_t sz, void *ptr) { return ptr; }
void operator delete(void *ptr, void *place) { }
void operator delete[](void *ptr, void *place) { }
#endif

#endif /* ZuNew_HPP */
