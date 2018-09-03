//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

// Copyright (c) 2007-2013 Core Technologies Ltd. (Hong Kong)
// All rights reserved

#ifndef MxMDMsg_HPP
#define MxMDMsg_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <ZiIP.hpp>
#include <ZiMultiplex.hpp>

#include <ZmPQueue.hpp>

#include <MxBase.hpp>
#include <MxMD.hpp>
#include <MxQueue.hpp>

#include <MxMDStream.hpp>

namespace MxMDPubSub {
  using namespace MxMDStream;

  // == Message Buffers ==
  template <class Derived>
  class Msg {
    Msg(const Msg &) = delete;
    Msg &operator =(const Msg &) = delete; // prevent mis-use

  public:
    typedef ZmFn<Derived *, ZiIOContext &> Fn;
    typedef MxMDStream::Frame Frame;

    inline Msg() :
      m_msg(new MxQMsg(MxFlags{}, ZmTime{}, new ZuPOD<MxMDStream::MsgData>)) { }

    inline const Fn &fn() const { return m_fn; }
    inline Fn &fn() { return m_fn; }

      template <typename T> inline void *out() {
	MsgData &data = m_msg->payload->as<MsgData>();
	Frame *frame = data.frame();
	//Frame *frame = this->frame();

	frame->len = sizeof(T);
	frame->type = T::Code;

        //return (void *)this->buf();
	return (void *)data.buf();
      }

      void resendReq(uint64_t seqNo, uint32_t count) {
	auto rr = new (this->template out<MxMDStream::ResendReq>())
	  MxMDStream::ResendReq{seqNo, count};
      }

    inline const char *data() const {
      //return (const char *)(this->frame());
      return (const char *)(m_msg->payload->as<MsgData>().frame());
    }
    inline char *data() {
      //return (char *)(this->frame());
      return (char *)(m_msg->payload->as<MsgData>().frame());
    }

    inline const void *ptr() const {
      //return (const void *)(this->frame());
      return (const void *)(m_msg->payload->as<MsgData>().frame());
    }
    inline void *ptr() {
      //return (void *)(this->frame());
      return (void *)(m_msg->payload->as<MsgData>().frame());
    }

    inline unsigned length() {
      return m_msg->payload->as<MsgData>().length();
    }

    template <typename T> inline const T &as() const {
      //const T *ZuMayAlias(ptr) = (const T *)(this->frame());
      const T *ZuMayAlias(ptr) = (const T *)(m_msg->payload->as<MsgData>().frame());
      return *ptr;
    }
    template <typename T> inline T &as() {
      //T *ZuMayAlias(ptr) = (T *)(this->frame());
      T *ZuMayAlias(ptr) = (T *)(m_msg->payload->as<MsgData>().frame());
      return *ptr;
    }

  protected:
    Fn			m_fn;
    ZmRef<MxQMsg>	m_msg;
  };

  namespace UDP {
    using namespace MxMDStream;

    template <class Derived>
    class Msg : public MxMDPubSub::Msg<Derived>, public ZmPolymorph {
    public:
      typedef MxMDPubSub::Msg<Derived> Base;
      typedef MxMDStream::Frame Frame;

      inline Msg() { }

    private:
      void send_(ZiIOContext &io) {
	io.init(ZiIOFn::Member<&Msg::sent_>::fn(ZmMkRef(this)),
	    this->data(), this->length(), 0, m_addr);
      }
      void sent_(ZiIOContext &io) {
	if (ZuUnlikely((io.offset += io.length) < io.size)) return;
	this->m_fn(static_cast<Derived *>(this), io);
      }
    public:
      void send(ZiConnection *cxn, const ZiSockAddr &addr) {
	this->m_addr = addr;
	cxn->send(ZiIOFn::Member<&Msg::send_>::fn(ZmMkRef(this)));
      }

    private:
      void rcvd_(ZiIOContext &io) {
	      
	std::cout << "Msg rcvd_ -----------------" << std::endl;
	      
	      
	//this->length(io.offset + io.length);
	this->m_fn(static_cast<Derived *>(this), io);
      }

    public:
      void recv(ZiIOContext &io) {
	io.init(ZiIOFn::Member<&Msg::rcvd_>::fn(ZmMkRef(this)),
	    this->data(), sizeof(Frame) + sizeof(MxMDStream::Buf), 0);
      }

      bool scan() { /* FIXME - sanity check frame() and buf() */ return true; }

   public:
      inline const ZiSockAddr &addr() const { return m_addr; }
      inline ZiSockAddr &addr() { return m_addr; }

    private:
      ZiSockAddr	m_addr;
    };

    struct OutMsg_HeapID {
      inline static const char *id() { return "MxMDPubSub.UDP.OutMsg"; }
    };
    template <typename Heap>
    struct OutMsg_ : public Heap, public Msg<OutMsg_<Heap>> { };    
    typedef ZmHeap<OutMsg_HeapID, sizeof(OutMsg_<ZuNull>)> OutMsg_Heap;    
    struct OutMsg : public OutMsg_<OutMsg_Heap> { };

    struct InMsg : public Msg<InMsg> {
    public:
      ZuInline void owner(void *o) { m_owner = o; }
      ZuInline void *owner() const { return m_owner; }

    private:
      void	*m_owner = 0;
    };
  };

  namespace TCP {
    template <class Derived>
    class Msg : public MxMDPubSub::Msg<Derived>, public ZmPolymorph {
    public:
      typedef typename MxMDPubSub::Msg<Derived> Base;
      //typedef typename Base::Fn Fn;
      typedef MxMDStream::Frame Frame;

      inline Msg() { }

    private:
      void send_(ZiIOContext &io) {
	io.init(ZiIOFn::Member<&Msg::sent_>::fn(ZmMkRef(this)),
	    this->data(), this->length(), 0);
      }
      void sent_(ZiIOContext &io) {
	if (ZuUnlikely((io.offset += io.length) < io.size)) return;
	this->m_fn(static_cast<Derived *>(this), io);
      }
    public:
      void send(ZiConnection *cxn) {
	cxn->send(ZiIOFn::Member<&Msg::send_>::fn(ZmMkRef(this)));
      }

    private:
      void rcvd_(ZiIOContext &io) {
	      
	      
	 //std::cout << "****** rcvd_ **************" << std::endl;
	      
	      
	//this->length(io.offset += io.length);
	auto len = io.offset += io.length;

	//while (this->length() >= sizeof(Frame)) {
	while (len >= sizeof(Frame)) {
	  Frame &frame = this->template as<Frame>();
	  unsigned msgLen = sizeof(Frame) + frame.len;
	  if (ZuUnlikely(msgLen > (sizeof(Frame) + sizeof(MxMDStream::Buf)))) {
	    ZeLOG(Error, "received corrupt TCP message");
	    io.disconnect();
	    return;
	  }
	  //if (ZuLikely(this->length() < msgLen)) return;
	  if (ZuLikely(len < msgLen)) return;
	  this->m_fn(static_cast<Derived *>(this), io);
	  if (ZuUnlikely(io.completed())) return;
	  //if (io.offset = this->length() - msgLen)
	  if (io.offset = len - msgLen)
	    memmove(this->data(), this->data() + msgLen, io.offset);
	  //this->length(io.offset);
	  len = io.offset;
	}
      }

    public:
      void recv(ZiIOContext &io) {
	io.init(ZiIOFn::Member<&Msg::rcvd_>::fn(ZmMkRef(this)),
	    this->data(), sizeof(Frame) + sizeof(MxMDStream::Buf), 0);
      }
    };

    struct OutMsg_HeapID {
      inline static const char *id() { return "MxMDPubSub.TCP.OutMsg"; }
    };
    template <class Derived>
    struct OutMsg_ : public Msg<Derived> { };
    //typedef ZmHeap<OutMsg_HeapID, sizeof(OutMsg_<ZuNull, ZuNull>)> OutMsg_Heap;
    struct OutMsg : public OutMsg_<OutMsg> { };

    //struct OutHBMsg : public Msg<OutHBMsg, sizeof(TCPHdr)> { };

    struct InMsg : public Msg<InMsg> { };
  };
}

#endif /* MxMDMsg_HPP */
