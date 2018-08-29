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

// MxMD JNI

#include <jni.h>

#include <ZJNI.hpp>

#include <MxMD.hpp>

#include <MxMDSecurityJNI.hpp>
#include <MxMDOrderBookJNI.hpp>
#include <MxMDPxLevelJNI.hpp>
#include <MxMDOrderJNI.hpp>
#include <MxMDTradeJNI.hpp>

#include <MxMDL1DataJNI.hpp>

#include <MxMDSecHandlerJNI.hpp>

namespace MxMDSecHandlerJNI {
  // event handlers
  ZJNI::JavaMethod updatedSecurityFn[] = {
    { "updatedSecurity", "()Lcom/shardmx/mxmd/MxMDSecEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDSecurity;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod updatedOrderBookFn[] = {
    { "updatedOrderBook", "()Lcom/shardmx/mxmd/MxMDOBEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod l1Fn[] = {
    { "l1", "()Lcom/shardmx/mxmd/MxMDLevel1Fn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;Lcom/shardmx/mxmd/MxMDL1Data;)V" }
  };
  ZJNI::JavaMethod addMktLevelFn[] = {
    { "addMktLevel", "()Lcom/shardmx/mxmd/MxMDPxLevelFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDPxLevel;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod updatedMktLevelFn[] = {
    { "updatedMktLevel", "()Lcom/shardmx/mxmd/MxMDPxLevelFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDPxLevel;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod deletedMktLevelFn[] = {
    { "deletedMktLevel", "()Lcom/shardmx/mxmd/MxMDPxLevelFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDPxLevel;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod addPxLevelFn[] = {
    { "addPxLevel", "()Lcom/shardmx/mxmd/MxMDPxLevelFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDPxLevel;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod updatedPxLevelFn[] = {
    { "updatedPxLevel", "()Lcom/shardmx/mxmd/MxMDPxLevelFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDPxLevel;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod deletedPxLevelFn[] = {
    { "deletedPxLevel", "()Lcom/shardmx/mxmd/MxMDPxLevelFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDPxLevel;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod l2Fn[] = {
    { "l2", "()Lcom/shardmx/mxmd/MxMDOBEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod addOrderFn[] = {
    { "addOrder", "()Lcom/shardmx/mxmd/MxMDOrderFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrder;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod modifiedOrderFn[] = {
    { "modifiedOrder", "()Lcom/shardmx/mxmd/MxMDOrderFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrder;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod canceledOrderFn[] = {
    { "canceledOrder", "()Lcom/shardmx/mxmd/MxMDOrderFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrder;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod addTradeFn[] = {
    { "addTrade", "()Lcom/shardmx/mxmd/MxMDTradeFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDTrade;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod correctedTradeFn[] = {
    { "correctedTrade", "()Lcom/shardmx/mxmd/MxMDTradeFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDTrade;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod canceledTradeFn[] = {
    { "canceledTrade", "()Lcom/shardmx/mxmd/MxMDTradeFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDTrade;Ljava/time/Instant;)V" }
  };
}

ZmRef<MxMDSecHandler> MxMDSecHandlerJNI::j2c(JNIEnv *env, jobject handler_)
{
  ZmRef<MxMDSecHandler> handler = new MxMDSecHandler();

#ifdef lambda1
#undef lambda1
#endif
#ifdef lambda2
#undef lambda2
#endif
#define lambda1(method, arg1, ...) \
  if (auto fn = ZJNI::localRef( \
	env, env->CallObjectMethod(handler_, method ## Fn[0].mid))) \
    handler->method ## Fn( \
	[fn = ZJNI::globalRef(env, fn)](arg1) { \
      if (JNIEnv *env = ZJNI::env()) \
	env->CallVoidMethod(fn, method ## Fn[1].mid, __VA_ARGS__); })
#define lambda2(method, arg1, arg2, ...) \
  if (auto fn = ZJNI::localRef( \
	env, env->CallObjectMethod(handler_, method ## Fn[0].mid))) \
    handler->method ## Fn( \
	[fn = ZJNI::globalRef(env, fn)](arg1, arg2) { \
      if (JNIEnv *env = ZJNI::env()) \
	env->CallVoidMethod(fn, method ## Fn[1].mid, __VA_ARGS__); })

  lambda2(updatedSecurity,
      MxMDSecurity *sec, MxDateTime stamp,
      MxMDSecurityJNI::ctor(env, sec), ZJNI::t2j(env, stamp));
  lambda2(updatedOrderBook,
      MxMDOrderBook *ob, MxDateTime stamp,
      MxMDOrderBookJNI::ctor(env, ob), ZJNI::t2j(env, stamp));
  lambda2(l1,
      MxMDOrderBook *ob, const MxMDL1Data &l1Data,
      MxMDOrderBookJNI::ctor(env, ob), MxMDL1DataJNI::ctor(env, l1Data));
  lambda2(addMktLevel,
      MxMDPxLevel *pxLevel, MxDateTime stamp,
      MxMDPxLevelJNI::ctor(env, pxLevel), ZJNI::t2j(env, stamp));
  lambda2(updatedMktLevel,
      MxMDPxLevel *pxLevel, MxDateTime stamp,
      MxMDPxLevelJNI::ctor(env, pxLevel), ZJNI::t2j(env, stamp));
  lambda2(deletedMktLevel,
      MxMDPxLevel *pxLevel, MxDateTime stamp,
      MxMDPxLevelJNI::ctor(env, pxLevel), ZJNI::t2j(env, stamp));
  lambda2(addPxLevel,
      MxMDPxLevel *pxLevel, MxDateTime stamp,
      MxMDPxLevelJNI::ctor(env, pxLevel), ZJNI::t2j(env, stamp));
  lambda2(updatedPxLevel,
      MxMDPxLevel *pxLevel, MxDateTime stamp,
      MxMDPxLevelJNI::ctor(env, pxLevel), ZJNI::t2j(env, stamp));
  lambda2(deletedPxLevel,
      MxMDPxLevel *pxLevel, MxDateTime stamp,
      MxMDPxLevelJNI::ctor(env, pxLevel), ZJNI::t2j(env, stamp));
  lambda2(l2,
      MxMDOrderBook *ob, MxDateTime stamp,
      MxMDOrderBookJNI::ctor(env, ob), ZJNI::t2j(env, stamp));
  lambda2(addOrder,
      MxMDOrder *order, MxDateTime stamp,
      MxMDOrderJNI::ctor(env, order), ZJNI::t2j(env, stamp));
  lambda2(modifiedOrder,
      MxMDOrder *order, MxDateTime stamp,
      MxMDOrderJNI::ctor(env, order), ZJNI::t2j(env, stamp));
  lambda2(deletedOrder,
      MxMDOrder *order, MxDateTime stamp,
      MxMDOrderJNI::ctor(env, order), ZJNI::t2j(env, stamp));
  lambda2(addTrade,
      MxMDTrade *trade, MxDateTime stamp,
      MxMDTradeJNI::ctor(env, trade), ZJNI::t2j(env, stamp));
  lambda2(correctedTrade,
      MxMDTrade *trade, MxDateTime stamp,
      MxMDTradeJNI::ctor(env, trade), ZJNI::t2j(env, stamp));
  lambda2(canceledTrade,
      MxMDTrade *trade, MxDateTime stamp,
      MxMDTradeJNI::ctor(env, trade), ZJNI::t2j(env, stamp));

#undef lambda1
#undef lambda2

  return handler;
}

static int bindHandlerFn(JNIEnv *env, jclass c, ZJNI::JavaMethod *fn)
{
  if (ZJNI::bind(env, c, fn, 1) < 0) return -1;
  ZuStringN<80> fn_c =
    ZuString(&fn[0].signature[3], strlen(fn[0].signature) - 4);
  return ZJNI::bind(env, fn_c, &fn[1], 1);
}

int MxMDSecHandlerJNI::bind(JNIEnv *env)
{
  jclass c;
  if (!(c = env->FindClass("com/shardmx/mxmd/MxMDSecHandler"))) return -1;
  if (bindHandlerFn(env, c, updatedSecurityFn) < 0) return -1;
  if (bindHandlerFn(env, c, updatedOrderBookFn) < 0) return -1;
  if (bindHandlerFn(env, c, l1Fn) < 0) return -1;
  if (bindHandlerFn(env, c, addMktLevelFn) < 0) return -1;
  if (bindHandlerFn(env, c, updatedMktLevelFn) < 0) return -1;
  if (bindHandlerFn(env, c, deletedMktLevelFn) < 0) return -1;
  if (bindHandlerFn(env, c, addPxLevelFn) < 0) return -1;
  if (bindHandlerFn(env, c, updatedPxLevelFn) < 0) return -1;
  if (bindHandlerFn(env, c, deletedPxLevelFn) < 0) return -1;
  if (bindHandlerFn(env, c, l2Fn) < 0) return -1;
  if (bindHandlerFn(env, c, addOrderFn) < 0) return -1;
  if (bindHandlerFn(env, c, modifiedOrderFn) < 0) return -1;
  if (bindHandlerFn(env, c, canceledOrderFn) < 0) return -1;
  if (bindHandlerFn(env, c, addTradeFn) < 0) return -1;
  if (bindHandlerFn(env, c, correctedTradeFn) < 0) return -1;
  if (bindHandlerFn(env, c, canceledTradeFn) < 0) return -1;
  env->DeleteLocalRef((jobject)c);
  return 0;
}

void MxMDSecHandlerJNI::final(JNIEnv *env) { }
