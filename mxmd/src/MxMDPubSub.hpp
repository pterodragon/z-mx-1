//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

// Copyright (c) 2007-2013 Core Technologies Ltd. (Hong Kong)
// All rights reserved

#ifndef MxMDPubSub_HPP
#define MxMDPubSub_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <ZuByteSwap.hpp>

#include <MxBase.hpp>
#include <MxMD.hpp>

#pragma pack(push, 1)

namespace MxMDPubSub {

  // == Vocabulary Types ==

  typedef uint8_t UInt8;
  typedef uint16_t UInt16;
  typedef uint32_t UInt32;
  typedef uint64_t UInt64;
// ITCH prices have MIN as sentinel null value
  typedef ZuBoxN(int32_t, INT_MIN) Px32;
  typedef ZuBoxN(int64_t, LLONG_MIN) Px64;
// ITCH quantities have 0 as sentinel null value
  typedef ZuBox0(uint32_t) Qty32;
  typedef ZuBox0(uint64_t) Qty64;
  typedef ZuBigEndian<UInt16>::T UInt16N;
  typedef ZuBigEndian<UInt32>::T UInt32N;
  typedef ZuBigEndian<UInt64>::T UInt64N;
  typedef ZuBigEndian<Px32>::T Px32N;
  typedef ZuBigEndian<Px64>::T Px64N;
  typedef ZuBigEndian<Qty32>::T Qty32N;
  typedef ZuBigEndian<Qty64>::T Qty64N;

  template <unsigned N> class StringL { // ' '-padded left-aligned
  public:
    inline StringL() { memset(m_c, ' ', N); }
    inline StringL(const StringL &i) { memcpy(m_c, i.m_c, N); }
    inline StringL &operator =(const StringL &i) {
      if (this != &i) memcpy(m_c, i.m_c, N);
      return *this;
    }

    template <typename S>
    inline StringL(const S &s) { set(s); }
    template <typename S>
    inline StringL &operator =(const S &s) { set(s); return *this; }

    inline operator const char *() const { return data(); }

    inline operator ZuString() const { return get(); }

    inline const char *data() const {
      return m_c[0] == ' ' ? (const char *)0 : m_c;
    }
    inline unsigned length() const {
      unsigned i = 0;
      if (m_c[i] == ' ') return 0;
      while (++i < N && m_c[i] != ' ');
      return i;
    }

    inline void set(ZuString s) {
      set(s.data(), s.length());
    }
    inline void set(const char *data, unsigned length) {
      if (!data || !length) { memset(m_c, ' ', N); return; }
      if (length > N) length = N;
      memcpy(m_c, data, length);
      if (length < N) memset(m_c + length, ' ', N - length);
    }
    inline ZuString get() const {
      unsigned i = 0;
      if (m_c[i] == ' ') return ZuString();
      while (++i < N && m_c[i] != ' ');
      return ZuString(m_c, i);
    }

  private:
    char	m_c[N];
  };

  template <unsigned N> class StringR { // ' '-padded right-aligned
  public:
    inline StringR() { memset(m_c, ' ', N); }
    inline StringR(const StringR &i) { memcpy(m_c, i.m_c, N); }
    inline StringR &operator =(const StringR &i) {
      if (this != &i) memcpy(m_c, i.m_c, N);
      return *this;
    }

    template <typename S>
    inline StringR(const S &s) { set(s); }
    template <typename S>
    inline StringR &operator =(const S &s) { set(s); return *this; }

    inline operator ZuString() const { return get(); }

    inline operator const char *() const { return data(); }

    inline const char *data() const {
      return m_c[N - 1] == ' ' ? (const char *)0 : m_c;
      if (m_c[N - 1] == ' ') return 0;
      int i = N - 1;
      if (m_c[i] == ' ') return 0;
      while (--i >= 0 && m_c[i] != ' '); ++i;
      return &m_c[i];
    }
    inline unsigned length() const {
      int i = N - 1; // yes, signed
      if (m_c[i] == ' ') return 0;
      while (--i >= 0 && m_c[i] != ' '); ++i;
      return N - i;
    }

    inline void set(ZuString s) {
      set(s.data(), s.length());
    }
    inline void set(const char *data, unsigned length) {
      if (!data || !length) { memset(m_c, ' ', N); return; }
      if (length > N) length = N;
      if (length < N) memset(m_c, ' ', N - length);
      memcpy(m_c + N - length, data, length);
    }
    inline ZuString get() const {
      int i = N - 1;
      if (m_c[i] == ' ') return ZuString();
      while (--i >= 0 && m_c[i] != ' '); ++i;
      return ZuString(m_c + i, N - i);
    }

  private:
    char	m_c[N];
  };

} // namespace MxMDPubSub

template <unsigned N>
struct ZuTraits<MxMDPubSub::StringL<N> > :
    public ZuGenericTraits<MxMDPubSub::StringL<N> > {
  enum { IsPOD = 1, IsString = 1 };
  typedef char Elem;
  inline static const char *data(const MxMDPubSub::StringL<N> &s) {
    return s.data();
  }
  inline static unsigned length(const MxMDPubSub::StringL<N> &s) {
    return s.length();
  }
};

template <unsigned N>
struct ZuTraits<MxMDPubSub::StringR<N> > :
    public ZuGenericTraits<MxMDPubSub::StringR<N> > {
  enum { IsPOD = 1, IsString = 1 };
  typedef char Elem;
  inline static const char *data(const MxMDPubSub::StringR<N> &s) {
    return s.data();
  }
  inline static unsigned length(const MxMDPubSub::StringR<N> &s) {
    return s.length();
  }
};

namespace MxMDPubSub {

  template <unsigned> struct UIntC_I { typedef MxUInt T; };
  template <> struct UIntC_I<20> { typedef MxUInt64 T; };
  template <unsigned N> class UIntC { // ' '-padded right-aligned unsigned

  public:
    typedef typename UIntC_I<N>::T I;
    typedef typename I::T U;

    inline UIntC() { memset(m_i, ' ', N); }
    inline UIntC(const UIntC &i) { memcpy(m_i, i.m_i, N); }
    inline UIntC &operator =(const UIntC &i) {
      if (this != &i) memcpy(m_i, i.m_i, N);
      return *this;
    }

    template <typename T>
    inline UIntC(const T &i) { set(I(i)); }
    template <typename T>
    inline UIntC &operator =(const T &i) { set(I(i)); return *this; }

    inline UIntC(const I &i) { set(i); }
    inline UIntC &operator =(const I &i) { set(i); return *this; }

    inline operator I() const { return get(); }
    inline operator U() const { return get(); }

    inline void set(I i) {
      i.fmt(ZuFmt::Right<N, ' '>()).print(m_i);
    }
    inline I get() const {
      I i;
      i.scan(ZuFmt::Right<N, ' '>(), ZuString(m_i, N));
      return i;
    }

  private:
    char	m_i[N];
  };

} // namespace MxMDPubSub

template <int N>
struct ZuTraits<MxMDPubSub::UIntC<N> > :
    public ZuTraits<typename MxMDPubSub::UIntC<N>::I> {
  typedef MxMDPubSub::UIntC<N> T;
  enum { IsBoxed = 0, IsPrimitive = 0 };
};

namespace MxMDPubSub {

  // == Enums ==

  namespace TCP {
    struct MsgType {
      enum {
	Debug = '+',		// C <> S
	Login = 'L',		// C -> S
	LoginAck = 'A',		// C <- S
	LoginRej = 'J',		// C <- S
	Logout = 'O',		// C -> S
	SeqData = 'S',		// C <- S
	UnseqData = 'U',	// C -> S
	ServerHB = 'H',		// C <- S
	ClientHB = 'R',		// C -> S
	EndOfSession = 'Z',	// C <- S
      };
    };
  };

  struct MsgType {
    enum {
      Time = 'T',		// Time
      SysEvent = 'S',		// System Event
      OBState = 'O',		// Order Book State
      AddOrder = 'A',		// Add Order
      ModifyOrder = 'U',	// Order Replace
      CancelOrder = 'D',	// Order Delete
      Fill = 'E',		// Order Executed
      FillPx = 'C',		// Order Executed at Price
      AddTrade = 'P',		// Trade
      Match = 'Z',		// Equilibrium Price Update
      Snapshot = 'G'		// End of Snapshot
    };
  };

  struct Bool { enum _ { True = 'Y', False = 'N' }; };

  struct Side { enum _ { Buy = 'B', Sell = 'S' }; };

  struct SysEventCode {
    enum _ {
      Open = 'O',	// first message of trading day
      Close = 'C'	// last message of trading day
    };
  };

  // == UDP Headers ==

  // if count is zero, this is a heartbeat
  // if count is 0xffff, this is a end-of-session message (actual count is zero)
  // in both cases, seqNo is next expected seqNo
  struct UDPPktHdr {		// UDP Packet Header
    StringL<10>	session;	// session ID
    UInt64N	seqNo;		// sequence number of next msg (first in packet)
    UInt16N	count;		// #msgs in this packet

    inline void *ptr() { return (void *)&this[1]; }
    inline const void *ptr() const { return (const void *)&this[1]; }

    template <typename T> inline T &as() {
      T *ZuMayAlias(ptr) = (T *)&this[1];
      return *ptr;
    }
    template <typename T> inline const T &as() const {
      const T *ZuMayAlias(ptr) = (const T *)&this[1];
      return *ptr;
    }
  };

  struct UDPHdr {		// UDP Message Header
    UInt16N	length;		// message length (inclusive of type)
    UInt8	type;		// message type (business level)

    inline const void *ptr() const { return (const void *)&this[1]; }
    template <typename T> inline const T &as() const {
      const T *ZuMayAlias(ptr) = (const T *)&this[1];
      return *ptr;
    }
  };

  typedef UDPPktHdr UDPResendReq;

  // == TCP Headers ==

  struct TCPHdr {		// TCP Message Header
    UInt16N	length;		// message length (inclusive of type)
    UInt8	type;		// message type (TCP level)

    inline const void *ptr() const { return (void *)&this[1]; }
    template <typename T> inline const T &as() const {
      const T *ZuMayAlias(ptr) = (const T *)&this[1];
      return *ptr;
    }
  };

  // == TCP Messages ==

  namespace TCP {

    struct Debug {			// + - TCP Debug
      enum { TCPCode = MsgType::Debug };

      inline const char *data() const { return (const char *)this; } 
    };

    struct Login {			// L - TCP Login
      StringL<6>	username;
      StringL<10>	password;
      StringR<10>	session;
      UIntC<20>		seqNo;

      enum { TCPCode = MsgType::Login };
    };

    struct LoginAck {			// A - TCP Login Accepted
      StringR<10>	session;
      UIntC<20>		seqNo;

      enum { TCPCode = MsgType::LoginAck };
    };

    struct LoginRej {			// J - TCP Login Rejected
      StringL<10>	code;

      enum { TCPCode = MsgType::LoginRej };
    };

    struct Logout {			// O - TCP Logout
      enum { TCPCode = MsgType::Logout };
    };

    struct SeqData {			// S - Sequenced Data
      UInt8		type;		// business message type

      inline const void *ptr() const { return (const void *)&this[1]; }
      template <typename T> inline const T &as() const {
	const T *ZuMayAlias(ptr) = (const T *)&this[1];
	return *ptr;
      }

      enum { TCPCode = MsgType::SeqData };
    };

    struct UnseqData {			// U - Unsequenced Data
      UInt8		type;		// business message type

      inline const void *ptr() const { return (const void *)&this[1]; }
      template <typename T> inline const T &as() const {
	const T *ZuMayAlias(ptr) = (const T *)&this[1];
	return *ptr;
      }

      enum { TCPCode = MsgType::UnseqData };
    };

    struct ServerHB {			// H - Server Heartbeat
      enum { TCPCode = MsgType::ServerHB };
    };

    struct ClientHB {			// R - Client Heartbeat
      enum { TCPCode = MsgType::ClientHB };
    };

    struct EndOfSession {		// Z - End of Session
      enum { TCPCode = MsgType::EndOfSession };
    };
  };

  // == Business Messages ==

  struct Time {				// T - Time
    UInt32N		secs;

    enum { Code = MsgType::Time };
  };

  struct Snapshot {			// G - End of Snapshot
    UIntC<20>		seqNo;

    enum { Code = MsgType::Snapshot };
  };

} // namespace MxMDPubSub

#pragma pack(pop)

#endif /* MxMDPubSub_HPP */
