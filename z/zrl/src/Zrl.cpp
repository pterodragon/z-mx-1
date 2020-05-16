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

// command line interface using a readline-compatible library

#include <zlib/Zrl.hpp>

// #ifndef _WIN32
#include <readline/readline.h>
#include <readline/history.h>
// #endif

#if 0
#ifdef _WIN32
extern "C" {
  typedef char **rl_completion_func_t(const char *, int, int);
  typedef char *rl_compentry_func_t(const char *, int);
  typedef void rl_compentryfree_func_t(void *);

  ZuImport_Explicit const char *rl_readline_name;
  ZuImport_Explicit rl_completion_func_t *rl_attempted_completion_function;

  ZuImport_API char *readline(const char *prompt);
  ZuImport_API char **rl_completion_matches(
      const char *text, rl_compentry_func_t entry_func);
  ZuImport_API char *add_history(char *line);
}
#endif
#endif

#include <zlib/ZmSingleton.hpp>

struct Zrl_ {
  Zrl_() { }
  ~Zrl_() { }

  using Lock = ZmLock;
  using Guard = ZmGuard<Lock>;

  static Zrl_ *instance();

  void init(const char *program) {
    Guard guard(m_lock);
    rl_readline_name = program;
  }

  void stop() {
#ifdef linux
    rl_deprep_terminal();
    ::close(0);
#endif
  }

  void syntax(ZvCf *cf) {
    Guard guard(m_lock);
    m_cf = cf;
    m_completions.null();
    rl_attempted_completion_function = (rl_completion_func_t *)&Zrl_completions;
  }

  void match(const char *prefix) {
    Guard guard(m_lock);
    if (!m_cf) return;
    unsigned n;
    for (n = 0; n < 80; n++)
      if (!prefix[n] || prefix[n] == ' ' ||
	  prefix[n] == '\t' || prefix[n] == '\r' || prefix[n] == '\n') break;
    if (n == 80) return;
    m_completions.null();
    {
      ZvCf::Iterator i(m_cf, prefix);
      ZuString key;

      while (i.subset(key)) {
	if (key.length() < n ||
	    ZuString(key.data(), n) != ZuString(prefix, n)) break;
	m_completions.push(key);
      }
    }
  }

  ZuString complete(char *prefix, int state) {
    if (!state) match(prefix);
    return m_completions.shift();
  }

  ZmRef<ZvCf> cf() { return m_cf; }

  ZtString readline_(const char *prompt) {
    Guard guard(m_lock);
    char *line = ::readline(prompt);
    if (!line) throw Zrl::EndOfFile();
    if (!line[0]) { free(line); return ZtString(); }
    add_history(line);
    int length = strlen(line);
    return ZtString(line, length, length + 1);	// takes ownership
  }

  ZmRef<ZvCf> readline(const char *prompt) {
    Guard guard(m_lock);
    ZtString line = readline_(prompt);
    if (!line) return 0;
    ZmRef<ZvCf> cf = new ZvCf();
    cf->fromCLI(m_cf, line);
    return cf;
  }

  ZmLock		m_lock;
  ZmRef<ZvCf>		m_cf;
  ZtArray<ZuString>	m_completions;
};

Zrl_ *Zrl_::instance() { return ZmSingleton<Zrl_>::instance(); }

ZrlExtern char *Zrl_complete(char *prefix, int state)
{
  ZuString match = Zrl_::instance()->complete(prefix, state);
  if (!match) return 0;
  return ZtString(match).data(1);
}

ZrlExtern char **Zrl_completions(char *text, int start, int end)
{
  if (start) return 0;
  return rl_completion_matches(text, (rl_compentry_func_t *)&Zrl_complete);
}

void Zrl::init(const char *program)
{
  Zrl_::instance()->init(program);
}

void Zrl::syntax(ZvCf *cf)
{
  Zrl_::instance()->syntax(cf);
}

void Zrl::stop()
{
  Zrl_::instance()->stop();
}

ZtString Zrl::readline_(const char *prompt)
{
  return Zrl_::instance()->readline_(prompt);
}

ZmRef<ZvCf> Zrl::readline(const char *prompt)
{
  return Zrl_::instance()->readline(prompt);
}
