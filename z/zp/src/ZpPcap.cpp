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

#include <ZpPcap.hpp>

// pcap dispatch callback fn
extern "C" void dispatch_cb(u_char *userarg, const pcap_pkthdr *pkthdr,
			    const u_char *packet);

ZpPcap::ZpPcap(ZiMultiplex *mx, ZvCf *cf) : 
  m_mx(mx),
  m_handles(ZmHashParams(cf->get("handleHash", false, "ZpPcap.Handles")))
{
  m_maxCapture = cf->getInt("maxCapture", 1, (1<<16) - 1, false, 100);
#ifdef ZpPcap_DEBUG
  m_debug = cf->getInt("debug", 0, 1, false, 0);
#endif
}

void ZpPcap::connect(ZpConnected connected, const ZpHandleInfo &hi)
{
  ZmAssert(!!connected);
  if (!connected) return;

  const_cast<ZpHandleInfo &>(hi).pcap(new Zp_Pcap());
  Zp_Pcap *pcap = const_cast<ZpHandleInfo &>(hi).pcap();

  ZpError err;
  ZmRef<ZpHandle> handle = 0;
  bool opened = false;

  if (!hi.filter()) {
    err = "unfiltered connection not supported";
    goto error;
  }

  if (hi.asFile() && !pcap->openFile(hi.iface(), &err)) goto error;
  else if (!pcap->create(hi.iface(), &err)) goto error;
  opened = true;

  if (!pcap->snaplen(hi.snaplen(), &err)) goto error;
  if (!pcap->timeout(hi.timeout(), &err)) goto error;
  if (!pcap->promisc(hi.promisc(), &err)) goto error;
  if (!pcap->bufferSize(hi.bufferSize(), &err)) goto error;
  // this doesnt seem to work, and somehow returns an error with no message
  // if (!pcap->nonblock(hi.nonBlocking(), &err)) goto error;
  if (!hi.asFile() && !pcap->activate(&err)) goto error;
  if (!pcap->compile(hi.filter(), &err)) goto error;

  handle = (ZpHandle *)connected(Zi::OK, hi, err);
  if (!handle) {
    pcap->close();
    pcap->finalize();
    return;
  }

  m_handles.add(handle);
  handle->connected();

  return;

error:
  if (!err.data()) err = "unknown error";
  if (err.data() && err[0])
    err.length(strlen(err.data()));

  connected(Zi::IOError, hi, err);
  if (opened) {
    pcap->close();
    pcap->finalize();
  }
}

ZpHandle::ZpHandle(ZpPcap *pcap, const ZpHandleInfo &info) :
  m_pcap(pcap), m_info(info), m_started(0)
{
#ifdef ZpPcap_DEBUG
  m_debugDrain = ZpDrain::Member<&ZpHandle::debugDrain>::fn(this);
#endif
  m_bounce.handle(this);
}

void ZpHandle::drain(ZpDrain d)
{
  if (m_drain) {
    m_drain = ZpDrain();
    m_started = 0;
  }
  m_drain = d;
  if (m_drain) start();
}

void ZpHandle::start()
{
  Zp_Pcap *pcap = m_info.pcap();
  if (!pcap) return;
  if (!m_started.xch(1)) {
    ZpDEBUG(m_pcap, "starting dispatcher...");
    m_pcap->m_mx->add(ZmFn<>::Member<&ZpHandle::dispatch>::fn(ZmMkRef(this)));
  }
}

void ZpHandle::shutdown()
{
  Zp_Pcap *pcap = m_info.pcap();
  if (!pcap) return;

  disconnect();
}

void ZpHandle::disconnect()
{
  Zp_Pcap *pcap = m_info.pcap();
  if (!pcap) { ZeLOG(Debug, "pcap is not available"); return; }
  if (m_started.xch(0)) pcap->stop();
  m_drain = ZpDrain();
  pcap->close();
  pcap->finalize();
  this->disconnected();
}

void ZpHandle::close()
{
  Zp_Pcap *pcap = m_info.pcap();
  if (!pcap) return;

  disconnect();

  m_pcap->del(m_info.filter());
}

int ZpHandle::dataLinkOffset()
{
  Zp_Pcap *pcap = m_info.pcap();
  if (!pcap) return -1;
  switch (pcap->datalink()) {
    case 1: return 14; // Ethernet
    default: return -2;
  }
}

int Zp_Pcap::datalink()
{
  return pcap_datalink(m_pcap);
}

void ZpHandle::dispatch()
{
  if (!m_started) return;
  Zp_Pcap *pcap = m_info.pcap();
  int rc = pcap->dispatch((u_char *)(&m_bounce), m_pcap->maxCapture());
  switch (rc) {
    case -1:
      ZeLOG(Error, pcap->error());
      m_started = 0;
      this->disconnect();
      break;
    case -2: // pcap_breakloop caused dispatch to stop
      ZpDEBUG(m_pcap, "breakloop encountered");
      pcap->post();
      m_started = 0;
      this->disconnect();
      break;
    default:
      m_pcap->m_mx->add(ZmFn<>::Member<&ZpHandle::dispatch>::fn(ZmMkRef(this)));
      break;
  }
}

#ifdef ZpPcap_DEBUG
void ZpHandle::debugDrain(
    ZpHandle *handle, const struct timeval *ts, const uint8_t *data, int len)
{
  ZpDEBUG(m_pcap,
      ZtHexDump(ZtSprintf("FD: %3d recv(%d)", m_info.pcap()->fileno(), len),
	(void *)data, len));
  m_drain.operator()(handle, ts, data, len);
}
#endif

int Zp_Pcap::dispatch(u_char *userdata, int count)
{
  return pcap_dispatch(m_pcap, count, dispatch_cb, userdata);
}

extern "C" void dispatch_cb(u_char *userarg, const pcap_pkthdr *pkthdr,
			    const u_char *packet)
{
  ((ZpHandle::DrainBounce *)userarg)->bounce(userarg, pkthdr, packet);
}

ZtZString Zp_Pcap::toString() const
{
  ZtZString stream;
  ZpError err;

  stream << "pcap [handle = ";
  if (!m_pcap) {
    stream << "null]";
  } else {
    stream << pcap_fileno(m_pcap) << "]";
  }

  stream << " [snaplen = ";
  if (!m_pcap) {
    stream << "0]";
  } else {
    stream << pcap_snapshot(m_pcap) << "]";
  }

  int nonBlock = pcap_getnonblock(m_pcap, err.data());
  stream << " [nonblock = ";
  if (nonBlock < 0) {
    stream << err.data() << "]";
  } else {
    stream << nonBlock << "]";
  }

  pcap_if_t *devices = 0;
  err.null();
  stream << " [devices = [";
  if (pcap_findalldevs(&devices, err.data()) < 0) {
    stream << err.data() << "]";
  } else {
    pcap_if_t *tmp = devices;
    do {
      stream << "[name = " << tmp->name
	     << "] [description = " << tmp->description
	     << "] [flags = " << tmp->flags
	     << "] [addresses = [";
      pcap_addr_t *a = tmp->addresses;
      while (a) {
	stream << "[addr = ";
	switch (a->addr->sa_family) {
	case AF_INET:
	  {
	    ZiIP ip(((sockaddr_in *)a->addr)->sin_addr);
	    stream << ip.string() << "]";
	  }
	  break;
	case AF_INET6:
	  {
	    char str[INET6_ADDRSTRLEN];
	    if (!getnameinfo(a->addr, sizeof(struct sockaddr_in6),
		  str, INET6_ADDRSTRLEN, 0, 0, NI_NUMERICHOST)) {
	      stream << str << "]";
	    } else {
	      stream << "error]";
	    }
	  }
	  break;
	default:
	  stream << "unsupported]";
	  break;
	}
	a = a->next;
      }
      stream << "] ";
    } while (tmp = tmp->next);
    stream << "]";
    pcap_freealldevs(devices);
  }
  stream << "]";
  return stream;
}

ZtZString ZpHandleInfo::toString() const
{
  ZtZString stream;
  
  stream << "ZpHandleInfo [iface = " << m_iface
	 << "] [filter = " << m_filter
	 << "] [priority = " << m_priority
	 << "] [promisc = " << m_promisc
	 << "] [snaplen = " << m_snaplen
	 << "] [timeout = " << m_timeout
	 << "] [bufferSize = " << m_bufferSize
	 << "] [nonBlocking = " << m_nonBlocking
	 << "] [asFile = " << m_asFile
	 << "]";
  return stream;
}

ZtZString ZpHandleInfo::stats()
{
  if (!m_pcap) return "";
  return m_pcap->stats();
}

ZtZString ZpHandle::stats()
{
  ZtZString stream;
  stream << m_info.toString()
	 << " " << m_info.stats();
  return stream;
}

ZtZString ZpPcap::stats()
{
  HandleHash::ReadIterator i(m_handles);
  ZtZString stream;
  stream << "ZpPcapStats [";
  while (ZmRef<ZpHandle> h = i.iterateKey()) {
    stream << h->stats() << " ";
  }
  stream << "]";
  return stream;
}
