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

#include <MxMDPubSub.hpp>
#include <MxMDStream.hpp>

namespace MxMDPubSub {
  // == Message Buffers ==

  template <class Derived, int Size_> class Msg : public ZmPolymorph {
    Msg(const Msg &);
    Msg &operator =(const Msg &); // prevent mis-use

  public:
    enum { Size = Size_ };

    typedef ZmFn<Derived *, ZiIOContext &> Fn;

    inline Msg() : m_length(0) { }

    inline const Fn &fn() const { return m_fn; }
    inline Fn &fn() { return m_fn; }

    inline unsigned length() const { return m_length; }
    inline void length(unsigned i) { m_length = i; }
    inline const char *data() const { return &m_buf[0]; }
    inline char *data() { return &m_buf[0]; }

    inline const void *ptr() const { return (const void *)&m_buf[0]; }
    inline void *ptr() { return (void *)&m_buf[0]; }

    template <typename T> inline const T &as() const {
      const T *ZuMayAlias(ptr) = (const T *)&m_buf[0];
      return *ptr;
    }
    template <typename T> inline T &as() {
      T *ZuMayAlias(ptr) = (T *)&m_buf[0];
      return *ptr;
    }

  protected:
    Fn		m_fn;
    unsigned	m_length;
    char	m_buf[Size];
  };

  namespace UDP {
    template <class Derived, unsigned Size = 1472>
    class Msg : public MxMDPubSub::Msg<Derived, Size> {
    public:
      typedef typename MxMDPubSub::Msg<Derived, Size> Base;
      typedef typename Base::Fn Fn;
      typedef MxMDStream::Frame Frame;

      inline Msg() { }

      template <typename T> inline void *out(uint64_t seqNo) {
	unsigned length = sizeof(UDPPktHdr) + sizeof(Frame) + sizeof(T);
        if (length >= Size) return nullptr;
        this->length(length);
        new (this->ptr()) UDPPktHdr{{}, seqNo, 1};
	Frame *frame = (Frame *)(this->data() + sizeof(UDPPktHdr));
	frame->len = sizeof(T);
	frame->type = T::Code;
        return (void *)&frame[1];
      }

      void resendReq(ZuString session, uint64_t seqNo, uint16_t count) {
	this->length(sizeof(UDPResendReq));
	UDPResendReq &req = this->template as<UDPResendReq>();
	req.session = session;
	req.seqNo = seqNo;
	req.count = count;
      }

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
	this->length(io.offset + io.length);
	this->m_fn(static_cast<Derived *>(this), io);
      }

    public:
      void recv(ZiIOContext &io) {
	io.init(ZiIOFn::Member<&Msg::rcvd_>::fn(ZmMkRef(this)),
	    this->data(), Size, 0);
      }

      bool scan() {
	UDPPktHdr &pktHdr = this->template as<UDPPktHdr>();
	char *end = this->data() + Size;
	char *ptr = (char *)pktHdr.ptr();
	unsigned n = pktHdr.count;
	for (unsigned i = 0; i < n; i++) {
	  unsigned length = ((Frame *)ptr)->len;
	  ptr += sizeof(Frame) + length;
	  if (ptr > end) { n = i; ptr = end; break; }
	  if (ptr == end) { n = i + 1; break; }
	}
	this->length(ptr - this->data());
	if (pktHdr.count > n) {
	  pktHdr.count = n;
	  return true;
	}
	return false;
      }

      inline const StringL<10> &session() const {
	return this->template as<UDPPktHdr>().session;
      }

      inline uint64_t seqNo() const {
	return this->template as<UDPPktHdr>().seqNo;
      }
      inline uint16_t count() const {
	return this->template as<UDPPktHdr>().count;
      }

      inline unsigned clipHead(unsigned n) {
	UDPPktHdr &pktHdr = this->template as<UDPPktHdr>();
	unsigned count = pktHdr.count;
	if (ZuUnlikely(!n)) return count;
	if (ZuUnlikely(count < n)) {
	  this->length(0);
	  return pktHdr.count = 0;
	}
	char *end = this->data() + this->length();
	char *ptr = (char *)pktHdr.ptr();
	for (unsigned i = 0; i < n; i++) {
	  unsigned length = ((Frame *)ptr)->len;
	  ptr += sizeof(Frame) + length;
	  if (ZuUnlikely(ptr > end)) {
	    this->length(0);
	    return pktHdr.count = 0;
	  }
	}
	if (end > ptr) memmove(pktHdr.ptr(), ptr, end - ptr);
	this->length(sizeof(UDPPktHdr) + end - ptr);
	pktHdr.seqNo += n;
	return pktHdr.count -= n;
      }
      inline unsigned clipTail(unsigned n) {
	UDPPktHdr &pktHdr = this->template as<UDPPktHdr>();
	unsigned count = pktHdr.count;
	if (ZuUnlikely(!n)) return count;
	if (ZuUnlikely(count < n)) {
	  this->length(0);
	  return pktHdr.count = 0;
	}
	char *end = this->data() + this->length();
	char *ptr = (char *)pktHdr.ptr();
	n = count - n;
	for (unsigned i = 0; i < n; i++) {
	  unsigned length = ((Frame *)ptr)->len;
	  ptr += sizeof(Frame) + length;
	  if (ZuUnlikely(ptr > end)) {
	    this->length(0);
	    return pktHdr.count = 0;
	  }
	}
	this->length(ptr - this->data());
	return pktHdr.count = n;
      }

      inline void write(const Msg &msg) {
	UDPPktHdr &pktHdr = this->template as<UDPPktHdr>();
	uint64_t seqNo = pktHdr.seqNo;
	unsigned count = pktHdr.count;
	char *ptr = (char *)pktHdr.ptr();

	const UDPPktHdr &pktHdr_ = msg.template as<UDPPktHdr>();
	uint64_t seqNo_ = pktHdr_.seqNo;
	unsigned count_ = pktHdr_.count;
	char *ptr_ = (char *)pktHdr_.ptr();

	if (seqNo_ < seqNo) return;
	if ((seqNo_ - seqNo) + count_ > count) return;
	memcpy(ptr + (seqNo_ - seqNo), ptr_, count_);
      }

      class Iterator;
    friend class Iterator;
      class Iterator {
      public:
        inline Iterator(const Msg &msg) :
	    m_msg(msg), m_ptr(m_msg.data() + sizeof(UDPPktHdr)) { }

	inline Iterator(const Iterator &i) :
	  m_msg(i.m_msg), m_ptr(i.m_ptr) { }
	inline ~Iterator() { }

	inline const Frame *iterate() {
	  const char *end = m_msg.data() + m_msg.length();
	  if (m_ptr >= end) return 0;
	  const Frame *frame = (const Frame *)m_ptr;
	  m_ptr += sizeof(Frame) + frame->len;
	  return frame;
	}

      private:
        const Msg	&m_msg;
	const char	*m_ptr;
      };

    public:
      inline const ZiSockAddr &addr() const { return m_addr; }
      inline ZiSockAddr &addr() { return m_addr; }

    private:
      ZiSockAddr	m_addr;
    };

    struct OutMsg_HeapID {
      inline static const char *id() { return "MxMDPubSub.UDP.OutMsg"; }
    };
    template <class Derived, class Heap>
    struct OutMsg_ : public Msg<Derived>, public Heap { };
    typedef ZmHeap<OutMsg_HeapID, sizeof(OutMsg_<ZuNull, ZuNull>)> OutMsg_Heap;
    struct OutMsg : public OutMsg_<OutMsg, OutMsg_Heap> { };

    struct InMsg : public Msg<InMsg> {
    public:
      ZuInline void owner(void *o) { m_owner = o; }
      ZuInline void *owner() const { return m_owner; }

    private:
      void	*m_owner = 0;
    };

    struct InQueueFn {
      typedef uint64_t Key;
      inline InQueueFn(const InMsg &msg) : m_msg(const_cast<InMsg &>(msg)) { }

      inline Key key() const { return m_msg.seqNo(); }
      inline unsigned length() const { return m_msg.count(); }
      inline unsigned clipHead(unsigned n) { return m_msg.clipHead(n); }
      inline unsigned clipTail(unsigned n) { return m_msg.clipTail(n); }
      inline void write(const InQueueFn &msg) { m_msg.write(msg.m_msg); }

    private:
      InMsg	&m_msg;
    };
    struct InQueue_HeapID {
      inline static const char *id() { return "MxMDPubSub.UDP.InQueue"; }
    };
    typedef ZmPQueue<InMsg,
	      ZmPQueueFn<InQueueFn,
		ZmPQueueNodeIsItem<true,
		  ZmPQueueObject<ZmPolymorph,
		    ZmPQueueLock<ZmNoLock,
		      ZmPQueueHeapID<InQueue_HeapID> > > > > > InQueue;
    typedef InQueue::Node InQMsg;
    typedef InQueue::Gap Gap;
  };

  namespace TCP {
    template <class Derived, unsigned Size = (1<<16)>
    class Msg : public MxMDPubSub::Msg<Derived, Size> {
    public:
      typedef typename MxMDPubSub::Msg<Derived, Size> Base;
      typedef typename Base::Fn Fn;
      typedef MxMDStream::Frame Frame;

      inline Msg() { }

      template <typename T> inline void *out() {
	unsigned length = sizeof(Frame) + sizeof(T);
	if (length >= Size) return nullptr;
	this->length(length);
	Frame *frame = new (this->ptr()) Frame{sizeof(T), T::Code};
	return (void *)&frame[1];
      }

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
	this->length(io.offset += io.length);
	while (this->length() >= sizeof(Frame)) {
	  Frame &frame = this->template as<Frame>();
	  unsigned msgLen = sizeof(Frame) + frame.len;
	  if (ZuUnlikely(msgLen > Size)) {
	    ZeLOG(Error, "received corrupt TCP message");
	    io.disconnect();
	    return;
	  }
	  if (ZuLikely(this->length() < msgLen)) return;
	  this->m_fn(static_cast<Derived *>(this), io);
	  if (ZuUnlikely(io.completed())) return;
	  if (io.offset = this->length() - msgLen)
	    memmove(this->data(), this->data() + msgLen, io.offset);
	  this->length(io.offset);
	}
      }

    public:
      void recv(ZiIOContext &io) {
	io.init(ZiIOFn::Member<&Msg::rcvd_>::fn(ZmMkRef(this)),
	    this->data(), Size, 0);
      }
    };

    struct OutMsg_HeapID {
      inline static const char *id() { return "MxMDPubSub.TCP.OutMsg"; }
    };
    template <class Derived, class Heap>
    struct OutMsg_ : public Msg<Derived>, public Heap { };
    typedef ZmHeap<OutMsg_HeapID, sizeof(OutMsg_<ZuNull, ZuNull>)> OutMsg_Heap;
    struct OutMsg : public OutMsg_<OutMsg, OutMsg_Heap> { };

    struct OutHBMsg : public Msg<OutHBMsg, sizeof(TCPHdr)> { };

    struct InMsg : public Msg<InMsg> { };
  };
}

#endif /* MxMDMsg_HPP */
