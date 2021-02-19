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

// stack backtrace

#include <zlib/ZmBackTrace.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib/ZmCleanup.hpp>
#include <zlib/ZmSingleton.hpp>
#include <zlib/ZmPLock.hpp>
#include <zlib/ZmGuard.hpp>

#ifdef linux
#include <execinfo.h>
#include <dlfcn.h>
#endif

#if defined(__GNUC__) || defined(linux)
#ifdef linux
#define ZmBackTrace_BFD
#endif
#ifdef _WIN32
#define ZmBackTrace_BFD
#endif
#include <zlib/ZuDemangle.hpp>
#endif

#ifdef ZmBackTrace_BFD
#define PACKAGE Zm
#ifdef linux
#include <bfd.h>
#else
#include <binutils/bfd.h>
#endif
#endif

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#ifdef _WIN32
extern "C" {
  typedef struct _SYMBOL_INFO {
    ULONG		SizeOfStruct;
    ULONG		TypeIndex;
    ULONG64		Reserved[2];
    ULONG		Index;
    ULONG		Size;
    ULONG64		ModBase;
    ULONG		Flags;
    ULONG64		Value;
    ULONG64		Address;
    ULONG		Register;
    ULONG		Scope;
    ULONG		Tag;
    ULONG		NameLen;
    ULONG		MaxNameLen;
    CHAR		Name[1];
  } SYMBOL_INFO;
  typedef enum {
    AddrMode1616,
    AddrMode1632,
    AddrModeReal,
    AddrModeFlat
  } ADDRESS_MODE;
  typedef struct _ADDRESS64 {
    DWORD64		Offset;
    WORD		Segment;
    ADDRESS_MODE	Mode;
  } ADDRESS64;
  typedef struct _STACKFRAME64 {
    ADDRESS64		AddrPC;
    ADDRESS64		AddrReturn;
    ADDRESS64		AddrFrame;
    ADDRESS64		AddrStack;
    ADDRESS64		AddrBStore;
    PVOID		FuncTableEntry;
    DWORD64		Params[4];
    BOOL		Far;
    BOOL		Virtual;
    DWORD64		Reserved[3];
  } STACKFRAME64;
  // SymOptions
  typedef DWORD (WINAPI *PSymSetOptions)(DWORD);
  // Process, UserSearchPath, LoadModules
  typedef BOOL (WINAPI *PSymInitialize)(HANDLE, PCSTR, BOOL);
  // Process
  typedef BOOL (WINAPI *PSymCleanup)(HANDLE);
  // Process, Address, Displacement, SymbolInfo
  typedef BOOL (WINAPI *PSymFromAddr)(
      HANDLE, DWORD64, DWORD64 *, SYMBOL_INFO *);
  // Process, AddrBase
  typedef PVOID (WINAPI *PSymFunctionTableAccess64)(HANDLE, DWORD64);
  // Process, Addr
  typedef DWORD64 (WINAPI *PSymGetModuleBase64)(HANDLE, DWORD64);
  // MachineType, Process, Thread, StackFrame, ContextRecord,
  // ReadMemoryRoutine, FunctionTableAccessRoutine, GetModuleBaseRoutine,
  // TranslateAddress
  typedef BOOL (WINAPI *PStackWalk64)(
      DWORD, HANDLE, HANDLE, STACKFRAME64 *, void *, void (*)(),
      PSymFunctionTableAccess64, PSymGetModuleBase64, void (*)());
  typedef WORD (WINAPI *PRtlCaptureStackBackTrace)(
      DWORD, DWORD, PVOID *, DWORD *);
}
#endif /* _WIN32 */

class ZmBackTrace_Mgr;

template <> struct ZmCleanup<ZmBackTrace_Mgr> {
  enum { Level = ZmCleanupLevel::Final };
};

struct ZmBackTrace_MgrInit;

class ZmBackTrace_Mgr {
friend ZmSingletonCtor<ZmBackTrace_Mgr>;
friend ZmBackTrace;
friend ZmBackTrace_MgrInit;
friend void ZmBackTrace_print(ZmStream &s, const ZmBackTrace &bt);

#if defined(__GNUC__) || defined(linux)
  using Demangle = ZuDemangle<ZmBackTrace_BUFSIZ>;
#endif

  ZmBackTrace_Mgr();
public:
  ~ZmBackTrace_Mgr();

private:
  void printFrame_info(ZmStream &s,
      uintptr_t addr, ZuString module, ZuString symbol,
      ZuString file, unsigned line) {
    if (module.length() > 24) {
      module.offset(module.length() - 21);
      s << "...";
    }
    s << module;
#if defined(__GNUC__) || defined(linux)
    m_demangle = symbol;
    s << '(' << m_demangle << ")";
#else
    s << '(' << symbol << ")";
#endif
    if (file && line) {
      s << ' ';
      if (file.length() > 24) {
	file.offset(file.length() - 21);
	s << "...";
      }
      s << file << ':' << ZuBoxed(line);
    }
    s << " [+" << ZuBoxed(addr).hex() << "]\n";
  }

#ifdef ZmBackTrace_DL
  bool printFrame_dl(ZmStream &s, void *addr) {
    Dl_info info{0};
    dladdr((void *)addr, &info);
    if (!info.dli_fbase || !info.dli_fname || !info.dli_sname) return false;
    printFrame_info(s, (uintptr_t)addr - (uintptr_t)info.dli_fbase,
	info.dli_fname, info.dli_sname, "", 0);
    return true;
  }
#endif

#ifdef ZmBackTrace_BFD
  class BFD_Find;
  class BFD;
friend BFD_Find;
friend BFD;

  class BFD {
  friend BFD_Find;
  private:
#ifdef _WIN32
    struct Handle {
      HANDLE	v = INVALID_HANDLE_VALUE;
      Handle() = default;
      Handle(const Handle &) = delete;
      Handle &operator =(const Handle &) = delete;
      Handle(Handle &&h) { v = h.v; h.v = INVALID_HANDLE_VALUE; }
      Handle &operator =(Handle &&h) {
	v = h.v;
	h.v = INVALID_HANDLE_VALUE;
	return *this;
      }
      ~Handle() { if (v != INVALID_HANDLE_VALUE) CloseHandle(v); }
      Handle(HANDLE v_) : v{v_} { }
      Handle &operator =(HANDLE v_) {
	this->~Handle();
	new (this) Handle{v_};
	return *this;
      }
      bool operator !() const { return v == INVALID_HANDLE_VALUE; }
      ZuOpBool
      void close() { CloseHandle(v); v = INVALID_HANDLE_VALUE; }
    };
#endif
    BFD() = default;
    ~BFD() {
      if (abfd) bfd_close(abfd);
      if (name) ::free(const_cast<char *>(name));
      if (symbols) ::free(symbols);
    }

    bool load(ZmBackTrace_Mgr *mgr, const char *name_, uintptr_t base_) {
      name = strdup(name_);
      base = base_;
#ifdef _WIN32
      // due to address randomization (ASLR), BFD doesn't obtain the same
      // base address as used in-memory; neither does BFD reliably report
      // the PE-COFF base after loading, necessitating obtaining it
      // directly from the file by navigating the DOS/PE-COFF headers
      {
	Handle handle = CreateFileA(name_,
	    GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, NULL);
	if (!handle) return false;
	{
	  char buf[0x40];
	  DWORD r;
	  OVERLAPPED o{0};
	  if (!ReadFile(handle.v, buf, 0x40, &r, &o)) return false;
	  {
	    uint32_t off = 0;
	    if (buf[0] == 'M' && buf[1] == 'Z') memcpy(&off, &buf[0x3c], 4);
	    o.Offset = off;
	  }
	  if (!ReadFile(handle.v, buf, 0x38, &r, &o)) return false;
	  if (memcmp(buf, "PE\0", 4)) return false;
	  {
	    auto magic = *reinterpret_cast<const uint16_t *>(&buf[0x18]);
	    if (magic == 0x10b) { // PE32
	      fileBase = *reinterpret_cast<const uint32_t *>(&buf[0x34]);
	    } else if (magic == 0x20b) { // PE32+, i.e. 64-bit
	      fileBase = *reinterpret_cast<const uint64_t *>(&buf[0x30]);
	    } else
	      return false;
	  }
	}
      }
#if 0
      abfd = bfd_openr_iovec(name, nullptr,
	  [](bfd *, void *this_) { return this_; }, this,
	  [](bfd *, void *this__,
	      void *buf, file_ptr n, file_ptr o_) -> file_ptr {
	    auto this_ = reinterpret_cast<BFD *>(this__);
	    DWORD r;
	    OVERLAPPED o{0};
	    o.Offset = static_cast<DWORD>(o_);
	    o.OffsetHigh = static_cast<DWORD>(o_>>32);
	    if (!ReadFile(this_->handle.v, buf, n, &r, &o)) return -1;
	    return r;
	  },
	  [](bfd *, void *this__) -> int {
	    auto this_ = reinterpret_cast<BFD *>(this__);
	    this_->handle.close();
	    return 0;
	  },
	  [](bfd *, void *this__, struct stat *s) {
	    memset(s, 0, sizeof(struct stat));
	    auto this_ = reinterpret_cast<BFD *>(this__);
	    DWORD l, h;
	    l = GetFileSize(this_->handle.v, &h);
	    s->st_size = (static_cast<uint64_t>(h)<<32) | l;
	    return 0;
	  });
#endif
#endif
      abfd = bfd_openr(name, nullptr);
      if (!abfd) {
	free(const_cast<char *>(name));
	return false;
      }
      if (!(bfd_check_format(abfd, bfd_object) &&
	    bfd_check_format_matches(abfd, bfd_object, 0) &&
	    (bfd_get_file_flags(abfd) & HAS_SYMS))) {
	bfd_close(abfd);
	abfd = nullptr;
	goto ret;
      }
      {
	unsigned int i;
	if (bfd_read_minisymbols(abfd, 0, (void **)&symbols, &i) <= 0 &&
	    bfd_read_minisymbols(abfd, 1, (void **)&symbols, &i) < 0) {
	  if (symbols) { free(symbols); symbols = nullptr; }
	  bfd_close(abfd);
	  abfd = nullptr;
	}
      }
    ret:
      next = mgr->m_bfd, mgr->m_bfd = this;
      return true;
    }

  public:
    void final(ZmBackTrace_Mgr *mgr) { final_(mgr->m_bfd); }

  private:
    void final_(BFD *&prev) {
      if (next) next->final_(next);
      prev = 0;
      delete this;
    }

    BFD			*next = nullptr;
    const char		*name = nullptr;
    bfd			*abfd = nullptr;
    uintptr_t		base = 0;
#ifdef _WIN32
    uintptr_t		fileBase = 0;
#endif
    asymbol		**symbols = nullptr;
  };

  class BFD_Find {
  public:
    struct Info {
      uintptr_t		addr = 0;
      const char	*name = nullptr;
      const char	*func = nullptr;
      const char	*file = nullptr;
      unsigned		line = 0;
    };

    BFD_Find(ZmBackTrace_Mgr *mgr, uintptr_t addr) :
	m_mgr{mgr}, m_info{.addr = addr} { }

    Info operator ()() {
      uintptr_t base;
#ifdef linux
      Dl_info dl_info{0};
      dladdr((void *)m_info.addr, &dl_info);
      base = (uintptr_t)dl_info.dli_fbase;
#endif
#ifdef _WIN32
      base = (*m_mgr->m_symGetModuleBase64)(GetCurrentProcess(), m_info.addr);
#endif
      if (!base) goto notfound;
      m_bfd = m_mgr->m_bfd;
      while (m_bfd && m_bfd->base != base) m_bfd = m_bfd->next;
      if (!m_bfd) {
	const char *name = nullptr;
#ifdef linux
	name = dl_info.dli_fname;
#endif
#ifdef _WIN32
	auto nameBuf = m_mgr->nameBuf();
	if (auto n = GetModuleFileNameA((HINSTANCE)base,
	      nameBuf.data(), nameBuf.length() - 1))
	  nameBuf.trunc(n);
	else
	  goto notfound;
	name = nameBuf.data();
#endif
	if (!name) goto notfound;
	m_bfd = new BFD();
	if (!m_bfd->load(m_mgr, name, base)) {
	  delete m_bfd;
	  m_bfd = nullptr;
	  goto notfound;
	}
      }
      if (!m_bfd->abfd) goto notfound;
      bfd_map_over_sections(
	  m_bfd->abfd,
	  [](bfd *, asection *sec, void *context) {
	    static_cast<BFD_Find *>(context)->lookup(sec);
	  },
	  this);
      if (!m_info.func) goto notfound;
      return ZuMv(m_info);
notfound:
      return {};
    }

  private:
    void lookup(asection *sec) {
      if (m_info.func) return;
      auto size = m_bfd->abfd->size;
#if 0
      std::cerr << m_bfd->name <<
	" base=0x" << ZuBoxed(m_bfd->base).hex() <<
	" size=0x" << ZuBoxed(size).hex() <<
	"\r\n" << std::flush;
#endif
      auto flags = bfd_section_flags(sec);
      auto vma = bfd_section_vma(sec);
      auto secSize = bfd_section_size(sec);
#if 0
      std::cerr << "section vma=0x" << ZuBoxed(vma).hex() << 
	  " secSize=0x" << ZuBoxed(secSize).hex() <<
	  " filepos=0x" << ZuBoxed(sec->filepos).hex() <<
	  " ALLOC=0x" << ZuBoxed(flags & SEC_ALLOC).hex() <<
	  " CODE=0x" << ZuBoxed(flags & SEC_CODE).hex() <<
	  "\r\n" << std::flush;
#endif
      if ((flags & (SEC_ALLOC | SEC_CODE)) != (SEC_ALLOC | SEC_CODE)) return;
      if (vma >= m_bfd->base && vma < m_bfd->base + size)
	vma -= m_bfd->base;
#ifdef _WIN32
      else {
	// Microsoft 32/64-bit default base addresses
#ifdef _WIN64
	bfd_vma defltExeBase = 0x140000000;
	bfd_vma defltDLLBase = 0x180000000;
#else
	bfd_vma defltExeBase = 0x400000;
	bfd_vma defltDLLBase = 0x10000000;
#endif
	if (vma >= m_bfd->fileBase && vma < m_bfd->fileBase + size)
	  vma -= m_bfd->fileBase;
	else if (vma >= defltDLLBase && vma < defltDLLBase + size)
	  vma -= defltDLLBase;
	else if (vma >= defltExeBase && vma < defltExeBase + size)
	  vma -= defltExeBase;
	else
	  return;
      }
#endif /* _WIN32 */
      auto off = static_cast<bfd_vma>(m_info.addr - m_bfd->base);
#if 0
      std::cerr << "abs_addr=0x" << ZuBoxed(m_info.addr).hex() <<
	" off=0x" << ZuBoxed(off).hex() <<
	" rel_vma=0x" << ZuBoxed(vma).hex() <<
	"\r\n" << std::flush;
#endif
      if (off < vma || off >= vma + secSize) return;
      off -= vma;
      bool found = false;
      if (m_bfd->symbols)
	found = bfd_find_nearest_line(m_bfd->abfd, sec, m_bfd->symbols,
	  off, &m_info.file, &m_info.func, &m_info.line);
      if (found)
	m_info.name = m_bfd->name;
      else
	m_info.func = nullptr;
    }

    ZmBackTrace_Mgr	*m_mgr = nullptr;
    BFD			*m_bfd = nullptr;
    Info		m_info;
  };

  bool printFrame_bfd(ZmStream &s, void *addr) {
    BFD_Find::Info info = BFD_Find{this, (uintptr_t)addr}();
    if (!info.addr) return false;
    printFrame_info(s, info.addr, info.name, info.func, info.file, info.line);
    return true;
  }
#endif

  static ZmBackTrace_Mgr *instance();

  void init();

  using Lock = ZmPLock;
  using Guard = ZmGuard<Lock>;

#ifdef _WIN32
  ZuArray<char> nameBuf() {
    return {m_nameBuf, sizeof(m_nameBuf) / sizeof(m_nameBuf[0])};
  }
#endif

  void capture(unsigned skip, void **frames);
#ifdef _WIN32
  void capture(EXCEPTION_POINTERS *exInfo, unsigned skip, void **frames);
#endif
  void print(ZmStream &s, void *const *frames);
  void printFrame(ZmStream &s, void *addr);
  bool printFrame_(ZmStream &s, void *addr);

  Lock				m_lock;
  bool				m_initialized;

#ifdef _WIN32
  HMODULE			m_dll;
  HMODULE			m_ntdll;
  PSymSetOptions		m_symSetOptions;
  PSymInitialize		m_symInitialize;
  PSymCleanup			m_symCleanup;
  PSymFromAddr			m_symFromAddr;
  PStackWalk64			m_stackWalk64;
  PSymFunctionTableAccess64	m_symFunctionTableAccess64;
  PSymGetModuleBase64		m_symGetModuleBase64;
  PRtlCaptureStackBackTrace	m_rtlCaptureStackBackTrace;
  char				m_nameBuf[ZmBackTrace_BUFSIZ]; // not MAX_PATH
#endif

#if defined(__GNUC__) || defined(linux)
  Demangle			m_demangle;
#endif

#ifdef ZmBackTrace_BFD
  BFD				*m_bfd = nullptr;
#endif
};

ZmBackTrace_Mgr *ZmBackTrace_Mgr::instance()
{
  return ZmSingleton<ZmBackTrace_Mgr>::instance();
}

struct ZmBackTrace_MgrInit {
  ZmBackTrace_MgrInit() {
    ZmBackTrace_Mgr::instance()->init();
  }
};

static ZmBackTrace_MgrInit ZmBackTrace_mgrInit;

ZmBackTrace_Mgr::ZmBackTrace_Mgr() :
  m_initialized(false)
#ifdef _WIN32
  , m_dll(0),
  m_ntdll(0),
  m_symSetOptions(0),
  m_symInitialize(0),
  m_symCleanup(0),
  m_symFromAddr(0),
  m_stackWalk64(0),
  m_symFunctionTableAccess64(0),
  m_symGetModuleBase64(0),
  m_rtlCaptureStackBackTrace(0)
#endif
#ifdef ZmBackTrace_BFD
  , m_bfd(0)
#endif
{
}

ZmBackTrace_Mgr::~ZmBackTrace_Mgr()
{
  if (m_initialized) {
#ifdef _WIN32
    // if (m_symCleanup) (*m_symCleanup)(GetCurrentProcess());
    if (m_dll) FreeLibrary(m_dll);
    if (m_ntdll) FreeLibrary(m_ntdll);
#endif
#ifdef ZmBackTrace_BFD
    if (m_bfd) m_bfd->final(this);
#endif
  }
}

void ZmBackTrace_Mgr::init()
{
  Guard guard(m_lock);

  if (m_initialized) return;

#ifdef _WIN32
  if (!(m_dll = LoadLibrary(L"dbghelp.dll"))) goto error;
  if (!(m_ntdll = LoadLibrary(L"ntdll.dll"))) goto error;

  if (!(m_symSetOptions =
	(PSymSetOptions)GetProcAddress(m_dll, "SymSetOptions"))) goto error;
  if (!(m_symInitialize =
	(PSymInitialize)GetProcAddress(m_dll, "SymInitialize"))) goto error;
  if (!(m_symCleanup =
	(PSymCleanup)GetProcAddress(m_dll, "SymCleanup"))) goto error;
  if (!(m_symFromAddr =
	(PSymFromAddr)GetProcAddress(m_dll, "SymFromAddr"))) goto error;
  if (!(m_stackWalk64 =
	(PStackWalk64)GetProcAddress(m_dll, "StackWalk64"))) goto error;
  if (!(m_symFunctionTableAccess64 = (PSymFunctionTableAccess64)
	GetProcAddress(m_dll, "SymFunctionTableAccess64")))
    goto error;
  if (!(m_symGetModuleBase64 = (PSymGetModuleBase64)
	GetProcAddress(m_dll, "SymGetModuleBase64")))
    goto error;
  if (!(m_rtlCaptureStackBackTrace = (PRtlCaptureStackBackTrace)
	GetProcAddress(m_ntdll, "RtlCaptureStackBackTrace")))
    goto error;

  (*m_symSetOptions)(0x00002000); // SYMOPT_INCLUDE_32BIT_MODULES
  (*m_symInitialize)(GetCurrentProcess(), 0, TRUE);
#endif /* _WIN32 */

#ifdef ZmBackTrace_BFD
  bfd_init();
#endif

  m_initialized = true;
  return;

#ifdef _WIN32
error:
  if (m_dll) FreeLibrary(m_dll);
  if (m_ntdll) FreeLibrary(m_ntdll);
  m_dll = 0;
  m_ntdll = 0;
  m_symSetOptions = 0;
  m_symInitialize = 0;
  m_symCleanup = 0;
  m_symFromAddr = 0;
  m_stackWalk64 = 0;
  m_symFunctionTableAccess64 = 0;
  m_symGetModuleBase64 = 0;
  m_rtlCaptureStackBackTrace = 0;
#endif /* _WIN32 */

  m_initialized = true;
}

void ZmBackTrace::capture(unsigned skip)
{
  ZmBackTrace_Mgr::instance()->capture(++skip, (void **)m_frames);
}

#ifdef _WIN32
void ZmBackTrace::capture(EXCEPTION_POINTERS *exInfo, unsigned skip)
{
  ZmBackTrace_Mgr::instance()->capture(exInfo, skip, (void **)m_frames);
}
#endif /* _WIN32 */

void ZmBackTrace_Mgr::capture(unsigned skip, void **frames)
{
  Guard guard(m_lock);

  int n = 0; // signed since ::backtrace() might return -ve

  ++skip;

#ifdef _WIN32
  if (m_rtlCaptureStackBackTrace)
    n = (*m_rtlCaptureStackBackTrace)(skip, ZmBackTrace_DEPTH, frames, 0);
#else /* _WIN32 */
#ifdef __GNUC__
  void **frames_ = ZuAlloca(frames_, (ZmBackTrace_DEPTH + skip));
  n = ::backtrace(frames_, ZmBackTrace_DEPTH + skip);
  n = n < (int)skip ? 0 : n - skip;
  memcpy(frames, frames_ + skip, sizeof(void *) * n);
#endif /* __GNUC__ */
#endif /* _WIN32 */

  if (n < ZmBackTrace_DEPTH)
    memset(frames + n, 0, sizeof(void *) * (ZmBackTrace_DEPTH - n));
}

#ifdef _WIN32
void ZmBackTrace_Mgr::capture(
    EXCEPTION_POINTERS *exInfo, unsigned skip, void **frames)
{
  Guard guard(m_lock);

  unsigned n = 0;

  if (m_stackWalk64 && m_symFunctionTableAccess64 && m_symGetModuleBase64) {
    STACKFRAME64 stackFrame;
    CONTEXT context;
    int machineType;

    memset(&stackFrame, 0, sizeof(STACKFRAME64));
    memcpy(&context, exInfo->ContextRecord, sizeof(CONTEXT));

#ifdef _WIN64
    machineType = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Offset = context.Rip;
    stackFrame.AddrFrame.Offset = context.Rbp;
    stackFrame.AddrStack.Offset = context.Rsp;
#else
    machineType = IMAGE_FILE_MACHINE_I386;
    stackFrame.AddrPC.Offset = context.Eip;
    stackFrame.AddrFrame.Offset = context.Ebp;
    stackFrame.AddrStack.Offset = context.Esp;
#endif
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Mode = AddrModeFlat;

    decltype(stackFrame.AddrStack.Offset) prevAddrStack = 0;
    while (
	(*m_stackWalk64)(machineType, GetCurrentProcess(), GetCurrentThread(),
	  &stackFrame, &context, 0, m_symFunctionTableAccess64,
	  m_symGetModuleBase64, 0) &&
	n < ZmBackTrace_DEPTH + skip) {
      if (prevAddrStack &&
	  prevAddrStack < stackFrame.AddrStack.Offset) break;
      if (n >= skip) {
	frames[n - skip] = reinterpret_cast<void *>(stackFrame.AddrPC.Offset);
      }
      n++;
    }
  }

  n = n < skip ? 0 : n - skip;

  if (n < ZmBackTrace_DEPTH)
    memset(frames + n, 0, sizeof(void *) * (ZmBackTrace_DEPTH - n));
}
#endif /* _WIN32 */

void ZmBackTrace_Mgr::printFrame(ZmStream &s, void *addr)
{
  if (!printFrame_(s, addr))
    s << '[' << ZuBoxPtr(addr).hex() << "]\n";
}

bool ZmBackTrace_Mgr::printFrame_(ZmStream &s, void *addr)
{
  if (ZuUnlikely(!addr)) return false;

#ifdef linux
#ifdef ZmBackTrace_DL
  return printFrame_dl(s, addr);
#endif
#ifdef ZmBackTrace_BFD
  return printFrame_bfd(s, addr);
#endif
#endif

#ifdef _WIN32
  if (ZuUnlikely(!m_symFromAddr)) return false;
  if (ZuUnlikely(!m_symGetModuleBase64)) return false;

  char sibuf[sizeof(SYMBOL_INFO) + 1024];
  SYMBOL_INFO *si = (SYMBOL_INFO *)&sibuf[0];

  si->SizeOfStruct = sizeof(SYMBOL_INFO);
  si->MaxNameLen = 1024;

  // MSVC DbgHelp lookup
  if (!(*m_symFromAddr)(GetCurrentProcess(), (DWORD64)addr, 0, si)) {
#ifdef ZmBackTrace_BFD
    return printFrame_bfd(s, addr);
#else
    return false;
#endif
  }

  auto nameBuf = this->nameBuf();
  ZuString module;
  if (auto n = GetModuleFileNameA(
	  (HINSTANCE)si->ModBase, nameBuf.data(), nameBuf.length() - 1)) {
    nameBuf.trunc(n);
    module = nameBuf;
  } else {
    module = "?";
  }
  printFrame_info(s, (uintptr_t)addr - (uintptr_t)si->ModBase,
      module, ZuString(si->Name, si->NameLen), "", 0);
  return true;
#endif /* _WIN32 */
}

ZmExtern void ZmBackTrace_print(ZmStream &s, const ZmBackTrace &bt)
{
  ZmBackTrace_Mgr::instance()->print(s, (void *const *)bt.frames());
}

void ZmBackTrace_Mgr::print(ZmStream &s, void *const *frames)
{
  Guard guard(m_lock);

  for (int depth = 0; depth < ZmBackTrace_DEPTH && frames[depth]; depth++)
    printFrame(s, frames[depth]);
}
