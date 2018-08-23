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

#ifndef ZvCmd_HPP
#define ZvCmd_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#include <ZuString.hpp>
#include <ZuByteSwap.hpp>

#include <ZmObject.hpp>
#include <ZmRef.hpp>

#include <ZtArray.hpp>
#include <ZtString.hpp>
#include <ZtRegex.hpp>

#include <ZiMultiplex.hpp>

#include <ZeLog.hpp>

#include <ZvRegexError.hpp>
#include <ZvMultiplexCf.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

class ZvCmdLine;

class ZvCmd_Msg : public ZmPolymorph {
friend class ZvCmdLine;

  typedef typename ZuBigEndian<int32_t>::T Int32N;

  struct HdrData {
    Int32N	msgLen;
    Int32N	dataLen;
    Int32N	flags;
    Int32N	seqNo;
  };

  struct Header : public HdrData {
    Header() : HdrData{sizeof(Header), 0, 0, 0} { }

    inline int cmdLen() { return msgLen - dataLen - sizeof(Header); }
  };

  struct Body : public Header {
    Body() { }

    inline void cmd(ZtArray<char> s) {
      m_cmd = s;
      msgLen += s.length();
    }

    inline void data(ZtArray<char> s) {
      m_data = s;
      unsigned n = s.length();
      dataLen = n;
      msgLen += n;
    }

    inline void extend() {
      m_cmd.length(cmdLen());
      m_data.length(dataLen);
    }

    ZtArray<char>	m_cmd;
    ZtArray<char>	m_data;
  };

public:
  // recv messages
  ZvCmd_Msg(ZvCmdLine *line) : m_line(line) { }

  ZvCmd_Msg(int32_t flags, uint32_t seqNo, ZuString msg, ZuString data) {
    m_payload.flags = flags;
    m_payload.seqNo = seqNo;
    m_payload.cmd(msg);
    m_payload.data(data);
  }

  ZvCmd_Msg(uint32_t seqNo, ZuString cmd, ZuString data) {
    m_payload.seqNo = seqNo;
    m_payload.cmd(cmd);
    m_payload.data(data);
  }

  inline void line(ZvCmdLine *line) { m_line = line; }

  inline void *header() { return (void *)&m_payload; }
  inline int headerLen() { return sizeof(Header); }

  inline int dataLen() { return m_payload.dataLen; }
  inline int cmdLen() { return m_payload.cmdLen(); }
  inline int msgLen() { return m_payload.msgLen; }
  inline uint32_t seqNo() { return m_payload.seqNo; }
  inline void extend() { m_payload.extend(); }

  inline void *cmdPtr() { return m_payload.m_cmd.data(); }
  inline void *dataPtr() { return m_payload.m_data.data(); }

  inline const ZtArray<char> &cmd() const { return m_payload.m_cmd; }
  inline const ZtArray<char> &data() const { return m_payload.m_data; }
  inline int flags() { return m_payload.flags; }

  void recv(ZiIOContext &io);
  void send();

private:
  void recvHdr_(ZiIOContext &io);
  void recvCmd_(ZiIOContext &io);
  void recvData_(ZiIOContext &io);
  void rcvd_(ZiIOContext &io);
  void rcvd_();

  void send_(ZiIOContext &io);
  void sendHdr_(ZiIOContext &io);
  void sendCmd_(ZiIOContext &io);
  void sendData_(ZiIOContext &io);
  void sent_();

  Body			m_payload;
  ZiVec			m_vecs[3]; // header/cmd/data
  ZmRef<ZvCmdLine>	m_line;
};

struct ZvInvocation {
  ZuInline ZvInvocation(uint32_t seqNo, ZuString cmd, ZuString data) :
      m_seqNo(seqNo), m_cmd(cmd), m_data(data) { }

  ZuInline uint32_t seqNo() const { return m_seqNo; }
  ZuInline const ZtString &cmd() const { return m_cmd; }
  ZuInline const ZtArray<char> &data() const { return m_data; }

private:
  uint32_t		m_seqNo;
  ZtString		m_cmd;
  ZtArray<char>		m_data;
};

struct ZvCmd {
  enum Flags { 
    Success	= 0x0001,
    Fail	= 0x0002,
    Log		= 0x0004,
    Error	= 0x0008
  };

  ZuInline ZvCmd(unsigned f) : m_flags(f) { }

  template <typename S> inline void print(S &s) const {
    bool o = 0;
    if (m_flags & Success) { o = 1; s << "Success"; }
    if (m_flags & Fail) { if (o) s << ' '; o = 1; s << "Fail"; }
    if (m_flags & Log) { if (o) s << ' '; o = 1; s << "Log"; }
    if (m_flags & Error) { if (o) s << ' '; s << "Error"; }
  }

private:
  unsigned	m_flags;
};
template <> struct ZuPrint<ZvCmd> : public ZuPrintFn { };

typedef ZuBox<unsigned> ZvCmdFlags;

struct ZvAnswer {
  inline ZvAnswer() : m_flags(0), m_seqNo(0) { }

  template <typename Message, typename Data>
  inline ZvAnswer(ZvCmdFlags flags, uint32_t seqNo,
      Message &&message, Data &&data) :
    m_flags(flags), m_seqNo(seqNo),
    m_event(ZeEVENT(Info, ZuFwd<Message>(message))),
    m_data(ZuFwd<Data>(data)) { }

  inline ZvAnswer(uint32_t seqNo) : m_flags(0), m_seqNo(seqNo) { }

  template <typename Message>
  inline void make(
      ZvCmdFlags flags, int severity, const char *filename,
      int lineNumber, const char *function, Message &&message) {
    m_flags = flags;
    m_event = new ZeEvent(severity, filename, lineNumber, function,
	[message = ZuFwd<Message>(message)](const ZeEvent &, ZmStream &s) {
	  s << message;
	});
  }

  inline void make(ZvCmdFlags flags, ZeEvent *event) {
    m_flags = flags;
    m_event = event;
  }

  template <typename Data>
  inline typename ZuIsString<Data, void>::T
  make(ZvCmdFlags flags, const Data &data) {
    m_flags = flags;
    m_data = data;
  }

  inline ZvCmdFlags flags() const { return m_flags; }
  inline uint32_t seqNo() const { return m_seqNo; }
  struct Message;
friend struct Message;
  struct Message {
    template <typename S> ZuInline void print(S &s) const {
      if (ZuLikely(a.m_event)) s << a.m_event->message();
    }
    const ZvAnswer	&a;
  };
  ZuInline Message message() const { return Message{*this}; }
  inline ZmRef<ZeEvent> event() const { return m_event; }
  inline const ZtArray<char> &data() const { return m_data; }

  ZvCmdFlags		m_flags;
  uint32_t		m_seqNo;
  ZmRef<ZeEvent>	m_event;
  ZtArray<char>		m_data;
};
template <> struct ZuPrint<ZvAnswer::Message> : public ZuPrintFn { };

#define ZvAnswerArgs __FILE__, __LINE__, ZuFnName

typedef ZmFn<ZvCmdLine *>					ZvCmdConnFn;
typedef ZmFn<ZvCmdLine *>					ZvCmdDiscFn;
typedef ZmFn<ZvCmdLine *>					ZvCmdAckSentFn;
typedef ZmFn<ZvCmdLine *, const ZvAnswer &>			ZvCmdAckRcvdFn;
typedef ZmFn<ZvCmdLine *, const ZvInvocation &>			ZvCmdSentFn;
typedef ZmFn<ZvCmdLine *, const ZvInvocation &, ZvAnswer &>	ZvCmdRcvdFn;

class ZvAPI ZvCmdLine : public ZiConnection {
public:
  inline ZvCmdLine(
      ZiMultiplex *mx, const ZiCxnInfo &ci,
      bool incoming, unsigned timeout,
      ZvCmdConnFn connFn, ZvCmdDiscFn discFn,
      ZvCmdSentFn sentFn, ZvCmdRcvdFn rcvdFn,
      ZvCmdAckSentFn ackSentFn, ZvCmdAckRcvdFn ackRcvdFn) :
    ZiConnection(mx, ci),
    m_incoming(incoming), m_connFn(connFn), m_discFn(discFn),
    m_sentFn(sentFn), m_rcvdFn(rcvdFn),
    m_ackSentFn(ackSentFn), m_ackRcvdFn(ackRcvdFn),
    m_timeoutDisconnect(timeout) { }

  ~ZvCmdLine() { }

  inline bool incoming() { return m_incoming; }

  void connected(ZiIOContext &io);
  void disconnected();

  void failed(bool transient);

  void send(ZvCmd_Msg *msg);
  void send(ZuString msg, uint32_t seqNo);
  void send(const ZvAnswer &ans);
  void sent(ZvCmd_Msg *msg);

  void rcvd(ZvCmd_Msg *msg);

private:
  void scheduleTimeout();
  void cancelTimeout();
  void timeout();

  void warn(const char *op, int result, ZeError e);

  bool				m_incoming;

  ZvCmdConnFn			m_connFn;
  ZvCmdDiscFn			m_discFn;
  ZvCmdSentFn			m_sentFn;
  ZvCmdRcvdFn			m_rcvdFn;
  ZvCmdAckSentFn		m_ackSentFn;
  ZvCmdAckRcvdFn		m_ackRcvdFn;

  ZmLock			m_lock;
  ZmScheduler::Timer		m_timeout;
  const unsigned		m_timeoutDisconnect;
};

class ZvAPI ZvCmdClient : public ZmPolymorph {
public:
  ZvCmdClient(unsigned timeout = 10):
    m_mx(0), m_busy(false), m_timeoutDisconnect(timeout) { }

  void init(ZiMultiplex *mx, ZvCf *cf, ZvCmdConnFn connFn,
	    ZvCmdDiscFn discFn, ZvCmdAckRcvdFn ackRcvdFn);
  void final();

  void connect(ZiIP ip, int port);
  void connect();

  void failed(bool transient);

  void stop();

  void send(ZuString s, uint32_t seqNo = 0);

  template <typename S>
  void stdinData(S &&s) { m_stdinData = ZuFwd<S>(s); }

private:

  ZiConnection *connected(const ZiCxnInfo &info);
  void disconnected(ZvCmdLine *line);

  void sent(ZvCmdLine *line, const ZvInvocation &inv);
  void ackd(ZvCmdLine *line, const ZvAnswer &ans);

  typedef ZmLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmList<ZmRef<ZvCmd_Msg>, ZmListLock<ZmNoLock> > MsgList;

  ZiMultiplex		*m_mx;
  ZmRef<ZvCmdLine>	m_line;
  ZvCmdConnFn		m_connFn;
  ZvCmdDiscFn		m_discFn;
  ZvCmdAckRcvdFn	m_ackRcvdFn;

  Lock			m_lock;
    bool		  m_busy;
    MsgList		  m_queue;

  int			m_reconnectFreq;
  ZiIP			m_remoteIP;
  int			m_remotePort;
  ZiIP			m_localIP;
  int			m_localPort;
  const unsigned	m_timeoutDisconnect;
  ZtArray<char>		m_stdinData;
};

class ZvAPI ZvCmdServer : public ZmPolymorph {
public:
  ZvCmdServer() : m_mx(0), m_listening(false) { }

  void init(ZvCf *cf,
      ZiMultiplex *mx, ZvCmdDiscFn discFn, ZvCmdRcvdFn rcvdFn);
  void final();

  inline ZiMultiplex *mx() { return m_mx; }

  void start();
  void stop();

private:
  void listen();
  void listening(const ZiListenInfo &info);
  void failed(bool transient);
  ZiConnection *accepted(const ZiCxnInfo &info);

  void connected(ZvCmdLine *line);
  void ackSent(ZvCmdLine *line);

  ZiMultiplex		*m_mx;
  ZmSemaphore		m_started;
  bool			m_listening;

  int			m_rebindFreq;
  ZiIP			m_ip;
  int			m_port;
  int			m_nAccepts;
  ZvCmdDiscFn		m_discFn;
  ZvCmdRcvdFn		m_rcvdFn;
};

class ZvAPI ZvCmdObject : public ZmObject {
template <typename, typename> friend struct ZuConversionFriend;

  ZvCmdObject();
  ZvCmdObject &operator=(const ZvCmdObject &);

  struct CmdData;
friend struct CmdData;

  struct CmdOption;
friend struct CmdOption;

  struct CmdOption : public ZmObject {
    CmdOption(ZuString shortName, ZuString longName,
	ZuString longType, ZuString optName, ZuString description) :
      m_shortName(shortName), m_longName(longName), m_longType(longType),
      m_optName(optName), m_description(description) { }
    
    ZtString m_shortName;
    ZtString m_longName;
    ZtString m_longType;
    ZtString m_optName;
    ZtString m_description;
  };

  struct OptionGroup : public ZmObject {
    typedef ZtArray<ZmRef<CmdOption> > OptionArray;
    OptionArray m_options;
    ZtString	m_header;
  };

  static ZmRef<CmdOption> HelpOption;

  struct CmdData {
    friend class ZvCmdObject;

    CmdData() { m_optGrps.push(ZmRef<OptionGroup>(new OptionGroup())); }

    // throws ZvInvalidEnum if optName is not one of "flag", "scalar", "multi"
    void option(ZuString shortName, ZuString longName,
	ZuString longType, ZuString optName, ZuString description) {
      int v = ZvEnum<ZvCf::OptTypes::Map>::instance()->s2v(
	  "ZvCmdData::option", optName, -1);
      if (v < 0) throw ZtSprintf("ZvCmdData::option - option type is not set");
      
      // at least one name must be given and shortname must be 1 char
      if ((shortName.length() && shortName.length() != 1) ||
	  (!shortName.length() && !longName.length()))
	throw ZtSprintf("ZvCmdData::option - invalid short/long name combo "
			"\"%s\"/\"%s\"", shortName.data(), longName.data());
      push(ZmRef<CmdOption>(new CmdOption(
	      shortName, longName, longType, optName, description)));
    }

    template <typename Header>
    void group(const Header &header) {
      ZmRef<OptionGroup> group = new OptionGroup();
      group->m_header = header;
      m_optGrps.push(group);
    }

    void usage(ZuString usage) { if (usage) m_usages.push(usage); }
    void brief(ZuString s) { m_brief = s; }
    void manuscript(ZuString s) { if (s) m_manuscripts.push(s); }

  protected:
    inline void push(CmdOption *opt) {
      m_optGrps[m_optGrps.length() - 1]->m_options.push(opt);
    }
    // sort options by short then long name.  called by cmd object on init
    void sort();
    
    void wordWrap(
	ZtString &dst, const char *src, int srcLen,
	int lineLen = 78, int used = 0, int indent = 0);

    // generate usage string. 
    // format taken from http://www.gnu.org/software/help2man
    ZtString toGNUString(ZuString name);

    void clean();

    ZtArray<ZtString>		        m_usages;
    ZtArray<ZmRef<OptionGroup> >	m_optGrps;
    ZtString			        m_brief;
    // each item begins a stanza
    ZtArray<ZtString>		        m_manuscripts;
  };

public:
  virtual ~ZvCmdObject() { }

  struct NameAccessor : public ZuAccessor<ZvCmdObject *, ZtString> {
    static inline const ZtString &value(const ZvCmdObject *o) {
      return o->name();
    }
  };
  inline const ZtString &name() const { return m_name; }
  inline const ZtString &syntax() const { return m_syntax; }
  inline const ZtString &usage() const { return m_usage; }

  virtual void exec(ZvCf *cf, ZuString data, ZvAnswer &ans) {
    result(ans, ZvCmd::Fail | ZvCmd::Log, "not implemented", 
	   ZvAnswerArgs, Ze::Error);
  }

protected:
  inline void result(ZvAnswer &ans, ZvCmdFlags flags, ZuString msg,
      const char *filename, int lineNumber,
      const char *function, int severity = Ze::Warning) {
    ans.make(flags, severity, filename, lineNumber, function,
	ZtSprintf("\"%s\": %.*s", name().data(), msg.length(), msg.data()));
  }

  inline void result(ZvAnswer &ans, ZvCmdFlags flags, ZeEvent *event) {
    ans.make(flags, event->severity(), event->filename(), event->lineNumber(),
	event->function(),
	ZtSprintf("\"%s\": %s", name().data(),
	  (ZtString() << event->message()).data()));
  }

  inline void result(ZvAnswer &ans, ZvCmdFlags flags, ZuString blob) {
    ans.make(flags, blob);
  }

  template <typename Name> 
  ZvCmdObject(Name &&name) : m_name(ZuFwd<Name>(name)) { 
    m_syntax << name << " {\n";
  }

  // provide access so subclasses can update options/syntax
  inline CmdData &data() { return m_data; }
  
  template <typename ShortName, typename LongName, typename LongType,
            typename OptName, typename Description>
  inline void option(const ShortName &shortName, const LongName &longName,
		     const LongType &longType, const OptName &optName, 
		     const Description &description) {
    m_data.option(shortName, longName, longType, optName, description);
  }

  template <typename S>
  inline void group(S &&s) { m_data.group(ZuFwd<S>(s)); }

  template <typename S> 
  inline void usage(S &&s) { m_data.usage(ZuFwd<S>(s)); }

  template <typename S> 
  inline void brief(S &&s) { m_data.brief(ZuFwd<S>(s)); }

  template <typename S> 
  inline void manuscript(S &&s) { m_data.manuscript(ZuFwd<S>(s)); }

  // subclasses MUST call this before they exit the ctor
  void compile();

private:
  ZtString		m_name;
  ZtString		m_syntax;
  ZtString		m_usage;
  CmdData		m_data; // compiles to usage and syntax
};

struct ZvCmdHash_HeapID {
  inline static const char *id() { return "ZvCmdHash"; }
};
typedef ZmHash<ZmRef<ZvCmdObject>,
	  ZmHashIndex<ZvCmdObject::NameAccessor,
	    ZmHashObject<ZuNull,
	      ZmHashBase<ZmObject,
		ZmHashHeapID<ZvCmdHash_HeapID> > > > > ZvCmdHash;

// Tag a class as providing ZvCmdObjects
struct ZvCmdProvider { virtual void provideCmds(ZvCmdHash *) { } };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZvCmd_HPP */
