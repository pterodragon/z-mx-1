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
#define ZmBackTrace_DL
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
friend struct ZmSingletonCtor<ZmBackTrace_Mgr>;
friend class ZmBackTrace;
friend struct ZmBackTrace_MgrInit;
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
    if (module.length() > 24)
      s << "..." << ZuString(module.data() + module.length() - 21, 21);
    else
      s << module;
#if defined(__GNUC__) || defined(linux)
    m_demangle = symbol;
    s << '(' << m_demangle << ")";
#else
    s << '(' << symbol << ")";
#endif
    if (file && line) s << ' ' << file << ':' << ZuBoxed(line);
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
friend class BFD_Find;
friend class BFD;

  class BFD {
  friend class BFD_Find;
  private:
    BFD() : m_next(0), m_name(0), m_handle(0), m_symbols(0) { }
    ~BFD() {
      if (m_name) free((void *)m_name);
      if (m_symbols) free(m_symbols);
      if (m_handle) bfd_close(m_handle);
    }

    int load(ZmBackTrace_Mgr *mgr, const char *name, uintptr_t base) {
      m_name = strdup(name);
      m_base = base;
      if (!(m_handle = bfd_openr(m_name, 0))) {
	free((void *)m_name);
	return -1;
      }
      if (!(bfd_check_format(m_handle, bfd_object) &&
	    bfd_check_format_matches(m_handle, bfd_object, 0) &&
	    (bfd_get_file_flags(m_handle) & HAS_SYMS))) {
	bfd_close(m_handle);
	m_handle = 0;
	goto ret;
      }
      {
	unsigned int i;
	if (bfd_read_minisymbols(m_handle, 0, (void **)&m_symbols, &i) <= 0 &&
	    bfd_read_minisymbols(m_handle, 1, (void **)&m_symbols, &i) < 0) {
	  if (m_symbols) { free(m_symbols); m_symbols = 0; }
	  bfd_close(m_handle);
	  m_handle = 0;
	}
      }
    ret:
      m_next = mgr->m_bfd, mgr->m_bfd = this;
      return 0;
    }

  public:
    void final(ZmBackTrace_Mgr *mgr) { final_(mgr->m_bfd); }

  private:
    void final_(BFD *&prev) {
      if (m_next) m_next->final_(m_next);
      prev = 0;
      delete this;
    }

    BFD			*m_next;
    const char		*m_name;
    bfd			*m_handle;
    uintptr_t		m_base;
    asymbol		**m_symbols;
  };

  class BFD_Find {
  public:
    struct Info {
      uintptr_t		addr;
      const char	*name;
      const char	*func;
      const char	*file;
      unsigned		line;
    };

    BFD_Find(ZmBackTrace_Mgr *mgr, uintptr_t addr) :
	m_mgr(mgr), m_addr(addr), m_bfd(0), m_func(0) { }

    Info operator ()() {
      uintptr_t base;
#ifdef linux
      Dl_info info{0};
      dladdr((void *)m_addr, &info);
      base = (uintptr_t)info.dli_fbase;
#endif
#ifdef _WIN32
      base = (*m_mgr->m_symGetModuleBase64)(GetCurrentProcess(), m_addr);
#endif
      if (!base) goto notfound;
      m_bfd = m_mgr->m_bfd;
      while (m_bfd && m_bfd->m_base != base) m_bfd = m_bfd->m_next;
      if (!m_bfd) {
#ifdef linux
	m_bfd = new BFD();
	if (m_bfd->load(m_mgr, info.dli_fname, base) < 0) {
	  delete m_bfd;
	  m_bfd = 0;
	  goto notfound;
	}
#endif
#ifdef _WIN32
	if (!GetModuleFileNameA(
	      (HINSTANCE)base, m_mgr->m_nameBuf, sizeof(m_mgr->m_nameBuf) - 1))
	  goto notfound;
	m_mgr->m_nameBuf[sizeof(m_mgr->m_nameBuf) - 1] = 0;
	m_bfd = new BFD();
	if (m_bfd->load(m_mgr, m_mgr->m_nameBuf, base) < 0) {
	  delete m_bfd;
	  m_bfd = 0;
	  goto notfound;
	}
#endif
      }
      if (!m_bfd->m_handle) goto notfound;
      bfd_map_over_sections(m_bfd->m_handle, &BFD_Find::lookup_, this);
      if (!m_func) goto notfound;
      return Info{m_addr, m_bfd->m_name, m_func, m_file, m_line};
notfound:
      return Info{0};
    }

  private:
    static void lookup_(bfd *handle, asection *sec, void *context) {
      ((BFD_Find *)context)->lookup(sec);
    }
    void lookup(asection *sec) {
      if (m_func) return;
      if ((bfd_get_section_flags(m_bfd->m_handle, sec) &
	    (SEC_ALLOC | SEC_CODE)) != (SEC_ALLOC | SEC_CODE)) return;
      bfd_vma vma = bfd_get_section_vma(m_bfd->m_handle, sec);
      bfd_size_type size = bfd_get_section_size(sec);
      uintptr_t addr = m_addr;
      if ((bfd_vma)addr < vma || (bfd_vma)addr >= vma + size) {
	addr -= m_bfd->m_base;
	if ((bfd_vma)addr < vma || (bfd_vma)addr >= vma + size) return;
      }
      bool found = false;
      if (m_bfd->m_symbols)
	found = bfd_find_nearest_line(m_bfd->m_handle, sec, m_bfd->m_symbols,
	  (bfd_vma)addr - vma, &m_file, &m_func, &m_line);
#if 0
      if (!found && m_bfd->m_dsymbols)
	found = bfd_find_nearest_line(m_bfd->m_handle, sec, m_bfd->m_dsymbols,
	    (bfd_vma)addr - vma, &file, &m_func, &line);
#endif
      if (!found) m_func = 0;
    }

    ZmBackTrace_Mgr	*m_mgr;
    uintptr_t		m_addr;
    BFD			*m_bfd;
    const char		*m_func;
    const char		*m_file;
    unsigned		m_line;
  };

  bool printFrame_bfd(ZmStream &s, void *addr) {
    BFD_Find::Info info = BFD_Find(this, (uintptr_t)addr)();
    if (!info.addr) return false;
    printFrame_info(s, info.addr, info.name, info.func, info.file, info.line);
    return true;
  }
#endif

  static ZmBackTrace_Mgr *instance();

  void init();

  using Lock = ZmPLock;
  using Guard = ZmGuard<Lock>;

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
  BFD				*m_bfd;
#endif
};

ZmBackTrace_Mgr *ZmBackTrace_Mgr::instance()
{
  return ZmSingleton<ZmBackTrace_Mgr>::instance();
}

struct ZmBackTrace_MgrInit {
  ZmBackTrace_MgrInit() {
    //printf("ZmBackTrace_Mgr::instance() = %p\n", ZmBackTrace_Mgr::instance()); fflush(stdout);
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
  void **frames_ = (void **)alloca(sizeof(void *) * (ZmBackTrace_DEPTH + skip));
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

#if defined(_WIN64)
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

    while (
	(*m_stackWalk64)(machineType, GetCurrentProcess(), GetCurrentThread(),
	  &stackFrame, &context, 0, m_symFunctionTableAccess64,
	  m_symGetModuleBase64, 0) &&
	n < ZmBackTrace_DEPTH + skip) {
      if (n >= skip) frames[n - skip] = (void *)(stackFrame.AddrPC.Offset);
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

  const char *module;
  int moduleLen;
  if ((moduleLen = GetModuleFileNameA(
	  (HINSTANCE)si->ModBase, m_nameBuf, sizeof(m_nameBuf) - 1))) {
    module = m_nameBuf;
    m_nameBuf[moduleLen] = 0;
  } else {
    module = "?";
    moduleLen = 1;
  }
  printFrame_info(s, (uintptr_t)addr - (uintptr_t)si->ModBase,
      ZuString(module, moduleLen), ZuString(si->Name, si->NameLen), "", 0);
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
