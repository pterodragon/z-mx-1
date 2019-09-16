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

#include <MxMDLibJNI.hpp>
#include <MxMDFeedJNI.hpp>
#include <MxMDVenueJNI.hpp>
#include <MxMDTickSizeTblJNI.hpp>

#include <MxMDInstrumentJNI.hpp>
#include <MxMDOrderBookJNI.hpp>

#include <MxMDExceptionJNI.hpp>
#include <MxMDTickSizeJNI.hpp>
#include <MxMDSegmentJNI.hpp>

#include <MxMDLibHandlerJNI.hpp>

namespace MxMDLibHandlerJNI {
  // event handlers
  ZJNI::JavaMethod exceptionFn[] = {
    { "exception", "()Lcom/shardmx/mxmd/MxMDExceptionFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDLib;Lcom/shardmx/mxmd/MxMDException;)V" }
  };
  ZJNI::JavaMethod connectedFn[] = {
    { "connected", "()Lcom/shardmx/mxmd/MxMDFeedFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDFeed;)V" }
  };
  ZJNI::JavaMethod disconnectedFn[] = {
    { "disconnected", "()Lcom/shardmx/mxmd/MxMDFeedFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDFeed;)V" }
  };
  ZJNI::JavaMethod eofFn[] = {
    { "eof", "()Lcom/shardmx/mxmd/MxMDLibFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDLib;)V" }
  };
  ZJNI::JavaMethod refDataLoadedFn[] = {
    { "refDataLoaded", "()Lcom/shardmx/mxmd/MxMDVenueFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDVenue;)V" }
  };
  ZJNI::JavaMethod addTickSizeTblFn[] = {
    { "addTickSizeTbl", "()Lcom/shardmx/mxmd/MxMDTickSizeTblFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDTickSizeTbl;)V" }
  };
  ZJNI::JavaMethod resetTickSizeTblFn[] = {
    { "resetTickSizeTbl", "()Lcom/shardmx/mxmd/MxMDTickSizeTblFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDTickSizeTbl;)V" }
  };
  ZJNI::JavaMethod addTickSizeFn[] = {
    { "addTickSize", "()Lcom/shardmx/mxmd/MxMDTickSizeFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDTickSizeTbl;"
	"Lcom/shardmx/mxmd/MxMDTickSize;)V" }
  };
  ZJNI::JavaMethod addInstrumentFn[] = {
    { "addInstrument", "()Lcom/shardmx/mxmd/MxMDInstrEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDInstrument;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod updatedInstrumentFn[] = {
    { "updatedInstrument", "()Lcom/shardmx/mxmd/MxMDInstrEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDInstrument;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod addOrderBookFn[] = {
    { "addOrderBook", "()Lcom/shardmx/mxmd/MxMDOBEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod updatedOrderBookFn[] = {
    { "updatedOrderBook", "()Lcom/shardmx/mxmd/MxMDOBEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod deletedOrderBookFn[] = {
    { "deletedOrderBook", "()Lcom/shardmx/mxmd/MxMDOBEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod tradingSessionFn[] = {
    { "tradingSession", "()Lcom/shardmx/mxmd/MxMDTradingSessionFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDVenue;Lcom/shardmx/mxmd/MxMDSegment;)V" }
  };
  ZJNI::JavaMethod timerFn[] = {
    { "timer", "()Lcom/shardmx/mxmd/MxMDTimerFn;" },
    { "fn", "(Ljava/time/Instant;)Ljava/time/Instant;" }
  };
}

ZmRef<MxMDLibHandler> MxMDLibHandlerJNI::j2c(
    JNIEnv *env, jobject obj, bool dlr)
{
  ZmRef<MxMDLibHandler> handler = new MxMDLibHandler();

  if (ZuUnlikely(!obj)) return handler;

#ifdef lambda1
#undef lambda1
#endif
#ifdef lambda2
#undef lambda2
#endif
#define lambda1(method, arg0, ...) \
  if (auto fn = ZJNI::localRef( \
	env, env->CallObjectMethod(obj, method ## Fn[0].mid))) \
    handler->method ## Fn( \
	[fn = ZJNI::globalRef(env, fn)](arg0) { \
      if (JNIEnv *env = ZJNI::env()) { \
	env->PushLocalFrame(1); \
	env->CallVoidMethod(fn, method ## Fn[1].mid, __VA_ARGS__); \
	env->PopLocalFrame(0); \
      } \
    })
#define lambda2(method, arg0, arg1, ...) \
  if (auto fn = ZJNI::localRef( \
	env, env->CallObjectMethod(obj, method ## Fn[0].mid))) \
    handler->method ## Fn( \
	[fn = ZJNI::globalRef(env, fn)](arg0, arg1) { \
      if (JNIEnv *env = ZJNI::env()) { \
	env->PushLocalFrame(2); \
	env->CallVoidMethod(fn, method ## Fn[1].mid, __VA_ARGS__); \
	env->PopLocalFrame(0); \
      } \
    })

  lambda2(exception,
      MxMDLib *md, ZmRef<ZeEvent> e,
      MxMDLibJNI::instance_(),
      MxMDExceptionJNI::ctor(env, e));
  lambda1(connected,
      MxMDFeed *feed,
      MxMDFeedJNI::ctor(env, feed));
  lambda1(disconnected,
      MxMDFeed *feed,
      MxMDFeedJNI::ctor(env, feed));
  lambda1(eof, MxMDLib *md, MxMDLibJNI::instance_());
  lambda1(refDataLoaded,
      MxMDVenue *venue,
      MxMDVenueJNI::ctor(env, venue));
  lambda1(addTickSizeTbl,
      MxMDTickSizeTbl *tbl,
      MxMDTickSizeTblJNI::ctor(env, tbl));
  lambda1(resetTickSizeTbl,
      MxMDTickSizeTbl *tbl,
      MxMDTickSizeTblJNI::ctor(env, tbl));
  lambda2(addTickSize,
      MxMDTickSizeTbl *tbl, const MxMDTickSize &ts,
      MxMDTickSizeTblJNI::ctor(env, tbl),
      MxMDTickSizeJNI::ctor(env, ts));
  lambda2(addInstrument,
      MxMDInstrument *instr, MxDateTime stamp,
      MxMDInstrumentJNI::ctor(env, instr), ZJNI::t2j(env, stamp));
  lambda2(updatedInstrument,
      MxMDInstrument *instr, MxDateTime stamp,
      MxMDInstrumentJNI::ctor(env, instr), ZJNI::t2j(env, stamp));
  lambda2(addOrderBook,
      MxMDOrderBook *ob, MxDateTime stamp,
      MxMDOrderBookJNI::ctor(env, ob), ZJNI::t2j(env, stamp));
  lambda2(updatedOrderBook,
      MxMDOrderBook *ob, MxDateTime stamp,
      MxMDOrderBookJNI::ctor(env, ob), ZJNI::t2j(env, stamp));
  lambda2(deletedOrderBook,
      MxMDOrderBook *ob, MxDateTime stamp,
      MxMDOrderBookJNI::ctor(env, ob), ZJNI::t2j(env, stamp));
  lambda2(tradingSession,
      MxMDVenue *venue, MxMDSegment segment,
      MxMDVenueJNI::ctor(env, venue),
      MxMDSegmentJNI::ctor(env, segment));

#undef lambda1
#undef lambda2

  if (auto fn = ZJNI::localRef(
	env, env->CallObjectMethod(obj, timerFn[0].mid)))
    handler->timerFn(
	[fn = ZJNI::globalRef(env, fn)](MxDateTime stamp, MxDateTime &next_) {
      if (JNIEnv *env = ZJNI::env())
	next_ = ZJNI::j2t(env, env->CallObjectMethod(
	      fn, timerFn[1].mid, ZJNI::t2j(env, stamp)), true);
    });

  if (dlr) env->DeleteLocalRef(obj);
  return handler;
}

static int bindHandlerFn(JNIEnv *env, jclass c, ZJNI::JavaMethod *fn)
{
  if (ZJNI::bind(env, c, fn, 1) < 0) return -1;
  ZuStringN<80> fn_c =
    ZuString(&fn[0].signature[3], strlen(fn[0].signature) - 4);
  return ZJNI::bind(env, fn_c, &fn[1], 1);
}

int MxMDLibHandlerJNI::bind(JNIEnv *env)
{
  jclass c;
  if (!(c = env->FindClass("com/shardmx/mxmd/MxMDLibHandler"))) return -1;
  if (bindHandlerFn(env, c, exceptionFn) < 0) return -1;
  if (bindHandlerFn(env, c, connectedFn) < 0) return -1;
  if (bindHandlerFn(env, c, disconnectedFn) < 0) return -1;
  if (bindHandlerFn(env, c, eofFn) < 0) return -1;
  if (bindHandlerFn(env, c, refDataLoadedFn) < 0) return -1;
  if (bindHandlerFn(env, c, addTickSizeTblFn) < 0) return -1;
  if (bindHandlerFn(env, c, resetTickSizeTblFn) < 0) return -1;
  if (bindHandlerFn(env, c, addTickSizeFn) < 0) return -1;
  if (bindHandlerFn(env, c, addInstrumentFn) < 0) return -1;
  if (bindHandlerFn(env, c, updatedInstrumentFn) < 0) return -1;
  if (bindHandlerFn(env, c, addOrderBookFn) < 0) return -1;
  if (bindHandlerFn(env, c, updatedOrderBookFn) < 0) return -1;
  if (bindHandlerFn(env, c, deletedOrderBookFn) < 0) return -1;
  if (bindHandlerFn(env, c, tradingSessionFn) < 0) return -1;
  if (bindHandlerFn(env, c, timerFn) < 0) return -1;
  env->DeleteLocalRef((jobject)c);
  return 0;
}

void MxMDLibHandlerJNI::final(JNIEnv *env) { }
