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

// application configuration

#include <zlib/ZuNew.hpp>

#include <zlib/ZuBox.hpp>

#include <zlib/ZmAssert.hpp>

#include <zlib/ZePlatform.hpp>

#include <zlib/ZtICmp.hpp>
#include <zlib/ZtRegex.hpp>

#include <zlib/ZvCf.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996) // getenv warning
#endif

ZvCf::ZvCf() : m_parent(0)
{
}

ZvCf::ZvCf(ZvCf *parent) : m_parent(parent)
{
}

ZvCf::~ZvCf()
{
}

int ZvCf::fromCLI(ZvCf *syntax, ZuString line)
{
  ZtArray<ZtString> args;
  parseCLI(line, args);
  if (!args.length()) return 0;
  return fromArgs(syntax->subset(args[0], false), args);
}

int ZvCf::fromCLI(const ZvOpt *opts, ZuString line)
{
  ZtArray<ZtString> args;
  parseCLI(line, args);
  if (!args.length()) return 0;
  return fromArgs(opts, args);
}

void ZvCf::parseCLI(ZuString line, ZtArray<ZtString> &args)
{
  args.length(0);
  ZtString val;
  const auto &cliValue = ZtStaticRegexUTF8("\\G[^\"'`#;\\s]+");// \G[^"'`#;\s]+
  const auto &cliSglQuote = ZtStaticRegexUTF8("\\G'");		// \G'
  const auto &cliSglQuotedValue = ZtStaticRegexUTF8("\\G[^'`]+"); // \G[^'`]+
  const auto &cliDblQuote = ZtStaticRegexUTF8("\\G\"");		// \G"
  const auto &cliDblQuotedValue = ZtStaticRegexUTF8("\\G[^\"`]+"); // \G[^"`]+
  const auto &cliQuoted = ZtStaticRegexUTF8("\\G`.");		// \G`.
  const auto &cliWhiteSpace = ZtStaticRegexUTF8("\\G\\s+");	// \G\s+
  const auto &cliComment = ZtStaticRegexUTF8("\\G#");		// \G#
  const auto &cliSemicolon = ZtStaticRegexUTF8("\\G;");		// \G;
  ZtRegex::Captures c;
  unsigned pos = 0;

  while (pos < line.length()) {
    if (cliValue.m(line, c, pos)) {
      pos += c[1].length();
      val += c[1];
      continue;
    }
    if (cliSglQuote.m(line, c, pos)) {
      pos += c[1].length();
      while (pos < line.length()) {
	if (cliSglQuotedValue.m(line, c, pos)) {
	  pos += c[1].length();
	  val += c[1];
	  continue;
	}
	if (cliQuoted.m(line, c, pos)) {
	  pos += c[1].length();
	  val += c[1];
	  continue;
	}
	if (cliSglQuote.m(line, c, pos)) {
	  pos += c[1].length();
	  break;
	}
      }
      continue;
    }
    if (cliDblQuote.m(line, c, pos)) {
      pos += c[1].length();
      while (pos < line.length()) {
	if (cliDblQuotedValue.m(line, c, pos)) {
	  pos += c[1].length();
	  val += c[1];
	  continue;
	}
	if (cliQuoted.m(line, c, pos)) {
	  pos += c[1].length();
	  val += c[1];
	  continue;
	}
	if (cliDblQuote.m(line, c, pos)) {
	  pos += c[1].length();
	  break;
	}
      }
      continue;
    }
    if (cliQuoted.m(line, c, pos)) {
      pos += c[1].length();
      val += c[1];
      continue;
    }
    if (cliWhiteSpace.m(line, c, pos)) {
      pos += c[1].length();
      args.push(ZuMv(val));
      val.null();
      continue;
    }
    if (cliComment.m(line, c, pos)) break;
    if (cliSemicolon.m(line, c, pos)) break;
    ZmAssert(false);	// should not get here
    break;
  }
  if (val) args.push(ZuMv(val));
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4267)
#endif

int ZvCf::fromArgs(const ZvOpt *opts, int argc, char **argv)
{
  if (ZuUnlikely(argc < 0)) return 0;
  ZtArray<ZtString> args(argc);
  for (unsigned i = 0; i < (unsigned)argc; i++) args.push(argv[i]);
  return fromArgs(opts, args);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

int ZvCf::fromArgs(ZvCf *options, const ZtArray<ZtString> &args)
{
  int i, j, n, l, p;
  const auto &argShort = ZtStaticRegexUTF8("^-(\\w+)$");	// -a
  const auto &argLongFlag = ZtStaticRegexUTF8("^--([\\w:]+)$"); // --arg
  const auto &argLongValue = ZtStaticRegexUTF8("^--([\\w:]+)="); // --arg=val
  ZtRegex::Captures c;
  ZmRef<ZvCf> option;

  p = 0;
  l = args.length();
  for (i = 0; i < l; i = n) {
    n = i + 1;
    if (argShort.m(args[i], c)) {
      int m = c[2].length();
      for (j = 0; j < m; j++) {
	ZtString shortOpt(ZtString::Copy, c[2].data() + j, 1);
	ZuString longOpt;
	if (!options ||
	    !(longOpt = options->get(shortOpt)) ||
	    !(option = options->subset(longOpt, false)))
	  throw Usage(args[0], shortOpt);
	int type = option->getEnum<ZvOptTypes::Map>("type", true);
	if (type == ZvOptFlag) {
	  fromArg(longOpt, ZvOptFlag, "1");
	} else {
	  ZuString deflt = option->get("default");
	  if (deflt) {
	    if (n < l && args[n][0] != '-') {
	      fromArg(longOpt, type,
		      args[n].data() +
			(args[n][0] == '`' && args[n][1] == '-'));
	      n++;
	    } else {
	      fromArg(longOpt, type, deflt);
	    }
	  } else {
	    if (n == l) throw Usage(args[0], shortOpt);
	    fromArg(longOpt, type,
		    args[n].data() +
		      (args[n][0] == '`' && args[n][1] == '-'));
	    n++;
	  }
	}
      }
    } else if (argLongFlag.m(args[i], c)) {
      ZtString longOpt = c[2];
      if (!options ||
	  !(option = options->subset(longOpt, false)))
	throw Usage(args[0], longOpt);
      int type = option->getEnum<ZvOptTypes::Map>("type", true);
      if (type == ZvOptFlag) {
	fromArg(longOpt, ZvOptFlag, "1");
      } else {
	ZuString deflt = option->get("default");
	if (!deflt) throw Usage(args[0], longOpt);
	fromArg(longOpt, type, deflt);
      }
    } else if (argLongValue.m(args[i], c)) {
      ZtString longOpt = c[2];
      if (!options ||
	  !(option = options->subset(longOpt, false)))
	throw Usage(args[0], longOpt);
      fromArg(longOpt,
	  option->getEnum<ZvOptTypes::Map>("type", false, ZvOptScalar), c[3]);
    } else {
      fromArg(ZtString(ZuBox<int>(p++)), ZvOptScalar, args[i]);
    }
  }
  {
    NodeRef node = m_tree.findVal("#");
    if (!node) m_tree.add("#", node = new Node());
    node->m_values.null();
    node->m_values.push(ZuBox<int>(p));
  }
  return p;
}

int ZvCf::fromArgs(const ZvOpt *opts, const ZtArray<ZtString> &args)
{
  ZmRef<ZvCf> options = new ZvCf();

  for (int i = 0; opts[i].m_long; i++) {
    ZmRef<ZvCf> option = new ZvCf();
    static const char *types[] = { "flag", "scalar", "multi" };
    if (opts[i].m_type < 0 ||
	opts[i].m_type >= (int)(sizeof(types) / sizeof(types[0])))
      throw Usage(args[0], opts[i].m_long);
    option->set("type", types[opts[i].m_type]);
    if (opts[i].m_default) option->set("default", opts[i].m_default);
    options->subset(opts[i].m_long, option);
    if (opts[i].m_short) options->set(opts[i].m_short, opts[i].m_long);
  }

  return fromArgs(options, args);
}

static ZuString scope_(ZuString &key)
{
  if (!key) return key;
  unsigned i = 0, n = key.length();
  for (; i < n && key[i] != ':'; ++i);
  ZuString s{key.data(), i};
  key.offset(i + (i < n));
  return s;
}

ZmRef<ZvCf> ZvCf::scope(ZuString fullKey, ZuString &key, bool create)
{
  ZmRef<ZvCf> self = this;
  ZuString scope = scope_(fullKey);
  ZuString nscope;
  if (scope)
    for (;;) {
      if (!(nscope = scope_(fullKey))) break;
      NodeRef node = self->m_tree.findVal(scope);
      if (!node) {
	if (!create) return 0;
	self->m_tree.add(scope, node = new Node());
      }
      if (!node->m_cf) {
	if (!create) return 0;
	node->m_cf = new ZvCf(self);
      }
      self = node->m_cf;
      scope = nscope;
    }
  key = scope;
  return self;
}

static bool flagValue(const ZtString &s)
{
  return s == "1" || ZtICmp::equals(s, "y") || ZtICmp::equals(s, "yes");
}

void ZvCf::fromArg(ZuString fullKey, int type, ZuString argVal)
{
  ZuString key;
  ZmRef<ZvCf> self = scope(fullKey, key, 1);
  NodeRef node = self->m_tree.findVal(key);
  if (!node) self->m_tree.add(key, node = new Node());
  node->m_values.null();

  if (type == ZvOptFlag) {
    node->m_values.push(flagValue(argVal) ? "1" : "0");
    return;
  }

  // type == ZvOptScalar || ZvOptMulti

  const auto &argValue = ZtStaticRegexUTF8("\\G[^`]+");		// \G[^`]+
  const auto &argValueMulti = ZtStaticRegexUTF8("\\G[^`,]+");	// \G[^`,]+
  const auto &argValueQuoted = ZtStaticRegexUTF8("\\G`(.)");	// \G`(.)
  const auto &argValueComma = ZtStaticRegexUTF8("\\G,");	// \G,
  ZtRegex::Captures c;
  unsigned pos = 0;

val:
  ZtString val;
  bool multi = false;

append:
  if (type == ZvOptScalar ?
      argValue.m(argVal, c, pos) :
      argValueMulti.m(argVal, c, pos)) {
    pos += c[1].length();
    val += c[1];
  }
  if (argValueQuoted.m(argVal, c, pos)) {
    pos += c[1].length();
    val += c[2];
    goto append;
  }
  if (type == ZvOptMulti && argValueComma.m(argVal, c, pos)) {
    pos += c[1].length();
    multi = true;
  }

  node->m_values.push(ZuMv(val));
  if (multi) goto val;
}

void ZvCf::fromString(ZuString in,
    bool validate, ZuString fileName, ZmRef<Defines> defines)
{
  unsigned n = in.length();

  if (!n) return;

  ZmRef<ZvCf> self = this;

  const auto &fileSkip = ZtStaticRegexUTF8(
      "\\G(?:#[^\\n]*\\n|\\s+)"); // \G(?:#[^\n]*\n|\s+)
  const auto &fileEndScope = ZtStaticRegexUTF8("\\G\\}");	// \G\}
  const auto &fileKey = ZtStaticRegexUTF8(
      "\\G(?:[^$#%`\"{},:=\\s]+|[%=]\\w+)"); // \G(?:[^$#%`"{},:=\s]+|[%=]\w+)
  const auto &fileLine = ZtStaticRegexUTF8("\\G[^\\n]*\\n");	// \G[^\n]*\n1
  const auto &fileValue = ZtStaticRegexUTF8(
      "\\G[^$#`\"{},\\s]+"); // \G[^#`"{},\s]+
  const auto &fileValueQuoted = ZtStaticRegexUTF8("\\G`(.)");	// \G`(.)
  const auto &fileDblQuote = ZtStaticRegexUTF8("\\G\"");	// \G"
  const auto &fileValueDblQuoted = ZtStaticRegexUTF8(
      "\\G[^`\"]+"); // \G[^`"]+
  const auto &fileBeginScope = ZtStaticRegexUTF8("\\G\\{");	// \G\{
  const auto &fileComma = ZtStaticRegexUTF8("\\G,");		// \G,
  const auto &fileDefine = ZtStaticRegexUTF8(
      "([^$#%`\"{},:=\\s]+)=(.+)"); // ([^$#%`"{},:=\s]+)=(.+)
  const auto &fileValueVar = ZtStaticRegexUTF8(
      "\\G\\${([^$#%`\"{},:=\\s]+)}"); // \G\${([^$#%`"{},:=\s]+)}
  ZtRegex::Captures c;
  unsigned pos = 0;

key:
  while (fileSkip.m(in, c, pos))
    pos += c[1].length();
  if (self->m_parent && fileEndScope.m(in, c, pos)) {
    pos += c[1].length();
    self = self->m_parent;
    goto key;
  }
  if (!fileKey.m(in, c, pos)) {
    if (pos < n - 1) {
      unsigned lpos = 0, line = 1;
      while (lpos < pos &&
	     fileLine.m(in, c, lpos)) {
	lpos += c[1].length();
	line++;
      }
      if (line > 1) --line;
      throw Syntax(line, in[pos], fileName);
    }
    return;
  }
  pos += c[1].length();
  ZuString key = c[1];
  NodeRef node;
  if (key[0] != '%') {
    if (!(node = self->m_tree.findVal(key))) {
      if (validate) throw Invalid(self, key, fileName);
      self->m_tree.add(key, node = new Node());
    }
  }
  ZtArray<ZtString> values;

val:
  while (fileSkip.m(in, c, pos))
    pos += c[1].length();

  ZtString val;
  bool multi = false;

append:
  if (fileValue.m(in, c, pos)) {
    pos += c[1].length();
    val += c[1];
  }
  if (fileValueQuoted.m(in, c, pos)) {
    pos += c[1].length();
    val += c[2];
    goto append;
  }
  if (fileValueVar.m(in, c, pos)) {
    pos += c[1].length();
    ZuString d = defines->findVal(c[2]);
    if (!d) d = getenv(c[2]);
    if (!!d) val += d;
    goto append;
  }
  if (fileDblQuote.m(in, c, pos)) {
    pos += c[1].length();
quoted:
    if (fileValueDblQuoted.m(in, c, pos)) {
      pos += c[1].length();
      val += c[1];
    }
    if (fileValueQuoted.m(in, c, pos)) {
      pos += c[1].length();
      val += c[2];
      goto quoted;
    }
    if (pos < n - 1) {
      pos++;
      goto append;
    }
    values.push(ZuMv(val));
    node->m_values = ZuMv(values);
    return;
  }
  if (fileBeginScope.m(in, c, pos)) {
    pos += c[1].length();
    if (val) values.push(ZuMv(val));
    if (node && values.length()) {
      node->m_values = ZuMv(values);
      values.length(0);
    }
    if (node && !node->m_cf) {
      if (validate) throw Invalid(self, key, fileName);
      node->m_cf = new ZvCf(self);
    }
    self = node ? node->m_cf : self;
    goto key;
  }
  if (fileComma.m(in, c, pos)) {
    pos += c[1].length();
    multi = true;
  }

  values.push(ZuMv(val));
  if (multi) goto val;
  if (node) {
    node->m_values = ZuMv(values);
    values.length(0);
  } else {
    if (key == "%include") {
      unsigned n = values.length();
      for (unsigned i = 0; i < n; i++) {
	ZmRef<ZvCf> incCf = new ZvCf();
	incCf->fromFile(values[i], false);
	self->merge(incCf);
      }
    } else if (key == "%define") {
      unsigned n = values.length();
      for (unsigned i = 0; i < n; i++) {
	if (!fileDefine.m(values[i], c, 0))
	  throw BadDefine(values[i], fileName);
	defines->del(c[2]);
	defines->add(c[2], c[3]);
      }
    }
    // other % directives here
  }
  goto key;
}

// suppress security warning about getenv()
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif

void ZvCf::fromEnv(const char *name, bool validate)
{
  ZuString data = getenv(name);
  unsigned n = data.length();

  if (!data) return;

  ZmRef<ZvCf> self = this;
  bool first = true;

  unsigned pos = 0;
  const auto &envEndScope = ZtStaticRegexUTF8("\\G\\}");	// \G\}
  const auto &envColon = ZtStaticRegexUTF8("\\G:");		// \G:
  const auto &envKey = ZtStaticRegexUTF8(
      "\\G[^#`\"{},:=\\s]+"); // \G[^#`"{},:=\s]+
  const auto &envEquals = ZtStaticRegexUTF8("\\G=");		// \G=
  const auto &envValue = ZtStaticRegexUTF8(
      "\\G[^`\"{},:]+");	// \G[^`"{},:]+
  const auto &envValueQuoted = ZtStaticRegexUTF8(
      "\\G`([`\"{},:\\s])"); // \G\\([`"{},:\s])
  const auto &envDblQuote = ZtStaticRegexUTF8("\\G\"");	// \G"
  const auto &envValueDblQuoted = ZtStaticRegexUTF8(
      "\\G[^`\"]+");	// \G[^`"]+
  const auto &envBeginScope = ZtStaticRegexUTF8("\\G\\{");	// \G\{
  const auto &envComma = ZtStaticRegexUTF8("\\G,");		// \G,
  ZtRegex::Captures c;

key:
  if (self->m_parent &&
      envEndScope.m(data, c, pos)) {
    pos += c[1].length();
    self = self->m_parent;
    goto key;
  }
  if (!first) {
    if (!envColon.m(data, c, pos)) {
      if (pos < n - 1) throw EnvSyntax(pos, data[pos]);
      return;
    }
    pos += c[1].length();
  }
  if (!envKey.m(data, c, pos)) {
    if (pos < n - 1 || !first) throw EnvSyntax(pos, data[pos]);
    return;
  }
  pos += c[1].length();

  first = false;

  ZuString key = c[1];
  NodeRef node = self->m_tree.findVal(key);
  if (!node) {
    if (validate) throw Invalid(self, key, ZuString());
    self->m_tree.add(key, node = new Node());
  }
  ZtArray<ZtString> values;

val:
  if (!values.length()) {
    if (!envEquals.m(data, c, pos))
      throw EnvSyntax(pos, data[pos]);
    pos += c[1].length();
  }

  ZtString val;
  bool multi = false;

append:
  if (envValue.m(data, c, pos)) {
    pos += c[1].length();
    val += c[1];
  }
  if (envValueQuoted.m(data, c, pos)) {
    pos += c[1].length();
    val += c[2];
    goto append;
  }
  if (envDblQuote.m(data, c, pos)) {
    pos += c[1].length();
quoted:
    if (envValueDblQuoted.m(data, c, pos)) {
      pos += c[1].length();
      val += c[1];
    }
    if (envValueQuoted.m(data, c, pos)) {
      pos += c[1].length();
      val += c[2];
      goto quoted;
    }
    if (pos < n - 1) {
      pos++;
      goto append;
    }
    values.push(ZuMv(val));
    node->m_values = ZuMv(values);
    return;
  }
  if (envBeginScope.m(data, c, pos)) {
    pos += c[1].length();
    if (val) values.push(ZuMv(val));
    if (values.length()) {
      node->m_values = ZuMv(values);
      values.length(0);
    }
    if (!node->m_cf) {
      if (validate) throw Invalid(self, key, ZuString());
      node->m_cf = new ZvCf(self);
    }
    self = node->m_cf;
    first = true;
    goto key;
  }
  if (envComma.m(data, c, pos)) {
    pos += c[1].length();
    multi = true;
  }

  values.push(ZuMv(val));
  if (multi) goto val;
  node->m_values = ZuMv(values);
  values.length(0);
  goto key;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

void ZvCf::toArgs(int &argc, char **&argv)
{
  ZtArray<ZtString> args;

  toArgs(args, "");
  argc = args.length();
  argv = (char **)::malloc(argc * sizeof(char *));
  ZmAssert(argv);
  if (!argv) throw std::bad_alloc();
  for (int i = 0; i < argc; i++) argv[i] = args[i].data(true);
}

void ZvCf::freeArgs(int argc, char **argv)
{
  for (int i = 0; i < argc; i++) ::free(argv[i]);
  free(argv);
}

void ZvCf::toArgs(ZtArray<ZtString> &args, ZuString prefix)
{
  auto i = m_tree.readIterator();
  Tree::NodeRef node_;
  NodeRef node;

  while (node_ = i.iterate()) {
    node = node_->val();
    {
      int n = node->m_values.length();
      if (n) {
	ZtString arg;
	if (ZuBox<int>().scan(node_->key()) != node_->key().length())
	  arg << "--" << prefix << node_->key() << '=';
	for (int i = 0; i < n; i++) {
	  arg << quoteArgValue(node->m_values[i]);
	  if (i < n - 1) arg << ',';
	}
	args.push(ZuMv(arg));
      }
    }
    if (node->m_cf)
      node->m_cf->toArgs(args, ZtString() << prefix << node_->key() << ':');
  }
}

ZtString ZvCf::quoteArgValue(ZuString in)
{
  if (!in) return "\"\"";

  ZtString out = in;

  const auto &argQuote = ZtStaticRegexUTF8("[`,]");	// [`,]
  ZtRegex::Captures c;
  unsigned pos = 0;

  while (pos < out.length() && argQuote.m(out, c, pos)) {
    pos = c[0].length();
    out.splice(pos, 0, "`", 1);
    pos += c[1].length() + 1;
    pos++;
  }

  return out;
}

void ZvCf::print(ZmStream &s, ZtString prefix) const
{
  auto i = m_tree.readIterator();
  Tree::NodeRef node_;
  NodeRef node;

  while (node_ = i.iterate()) {
    node = node_->val();
    {
      int n = node->m_values.length();
      if (n) {
	s << prefix << node_->key() << ' ';
	for (int i = 0; i < n; i++) {
	  s << quoteValue(node->m_values[i]);
	  if (i < n - 1)
	    s << ", ";
	  else
	    s << '\n';
	}
      }
    }
    if (node->m_cf) {
      s << prefix << node_->key() << " {\n" <<
	node->m_cf->prefixed(ZtString() << "  " << prefix) <<
	prefix << "}\n";
    }
  }
}

ZtString ZvCf::quoteValue(ZuString in)
{
  if (!in) return "\"\"";

  ZtString out = in;

  const auto &quote1 = ZtStaticRegexUTF8("[#`\"{},\\s]");	// [#`"{},\s]
  const auto &quote2 = ZtStaticRegexUTF8("[#`\",\\s]");	// [#`",\s]
  const auto &quoteValueDbl = ZtStaticRegexUTF8("[`\"]");	// [`"]
  const auto &quoteValue = ZtStaticRegexUTF8("[#`\"{},\\s]"); // [#`"{},\s]
  ZtRegex::Captures c;
  bool doubleQuote = false;
  unsigned pos = 0;

  if (quote1.m(out, c, pos)) {
    pos = c[0].length() + c[1].length();
    if (quote2.m(out, c, pos))
      doubleQuote = true;
  }

  pos = 0;
  if (doubleQuote) {
    while (quoteValueDbl.m(out, c, pos)) {
      pos = c[0].length();
      out.splice(pos, 0, "`", 1);
      pos += c[1].length() + 1;
    }
    out.splice(0, 0, "\"", 1);
    out.splice(out.length(), 0, "\"", 1);
  } else {
    while (quoteValue.m(out, c, pos)) {
      pos = c[0].length();
      out.splice(pos, 0, "`", 1);
      pos += c[1].length() + 1;
    }
  }

  return out;
}

void ZvCf::toFile_(ZiFile &file)
{
  ZtString out;
  out << *this;
  ZeError e;
  if (file.write(out.data(), out.length(), &e) != Zi::OK) throw e;
}

ZuString ZvCf::get(ZuString fullKey, bool required, ZuString def)
{
  ZuString key;
  ZmRef<ZvCf> self = scope(fullKey, key, 1);
  if (!self) {
    if (required) throw Required(this, fullKey);
    return def;
  }
  NodeRef node = self->m_tree.findVal(key);
  if (!node || !node->m_values.length()) {
    if (required) throw Required(this, fullKey);
    return def;
  }
  return node->m_values[0];
}

const ZtArray<ZtString> *ZvCf::getMultiple(ZuString fullKey,
    unsigned minimum, unsigned maximum, bool required)
{
  ZuString key;
  ZmRef<ZvCf> self = scope(fullKey, key, 1);
  if (!self) {
    if (required) throw Required(this, fullKey);
    return 0;
  }
  NodeRef node = self->m_tree.findVal(key);
  if (!node) {
    if (required) throw Required(this, fullKey);
    return 0;
  }
  unsigned n = node->m_values.length();
  if (n < minimum || n > maximum)
    throw NValues(this, fullKey, minimum, maximum, n);
  return &node->m_values;
}

void ZvCf::set(ZuString fullKey, ZuString val)
{
  ZuString key;
  ZmRef<ZvCf> self = scope(fullKey, key, 1);
  NodeRef node = self->m_tree.findVal(key);
  if (!node) self->m_tree.add(key, node = new Node());
  node->m_values.length(1, true);
  node->m_values[0] = val;
}

ZtArray<ZtString> *ZvCf::setMultiple(ZuString fullKey)
{
  ZuString key;
  ZmRef<ZvCf> self = scope(fullKey, key, true);
  NodeRef node = self->m_tree.findVal(key);
  if (!node) self->m_tree.add(key, node = new Node());
  return &node->m_values;
}

void ZvCf::unset(ZuString fullKey)
{
  ZuString key;
  ZmRef<ZvCf> self = scope(fullKey, key, 1);
  self->m_tree.del(key);
}

ZmRef<ZvCf> ZvCf::subset(ZuString key, bool create, bool required)
{
  ZmRef<ZvCf> self = this;
  while (ZuString scope = scope_(key)) {
    NodeRef node = self->m_tree.findVal(scope);
    if (!node) {
      if (!create) {
	if (required) throw Required(this, key);
	return nullptr;
      }
      self->m_tree.add(scope, node = new Node());
    }
    if (!node->m_cf) {
      if (!create) {
	if (required) throw Required(this, key);
	return nullptr;
      }
      node->m_cf = new ZvCf(self);
    }
    self = node->m_cf;
  }
  return self;
}

void ZvCf::subset(ZuString key, ZvCf *cf)
{
  ZmRef<ZvCf> self = this;
  ZuString scope = scope_(key);
  if (scope) {
    NodeRef node;
    for (;;) {
      node = self->m_tree.findVal(scope);
      if (!node) self->m_tree.add(scope, node = new Node());
      if (!(scope = scope_(key))) break;
      if (!node->m_cf) node->m_cf = new ZvCf(self);
      self = node->m_cf;
    }
    if (cf) cf->m_parent = self;
    node->m_cf = cf;
  }
}

void ZvCf::merge(ZvCf *cf)
{
  auto i = cf->m_tree.readIterator();
  Tree::NodeRef srcNode_;
  NodeRef srcNode;

  while (srcNode_ = i.iterate()) {
    srcNode = srcNode_->val();
    NodeRef dstNode = m_tree.findVal(srcNode_->key());
    if (!dstNode) {
      m_tree.add(srcNode_->key(), srcNode);
      if (srcNode->m_cf) srcNode->m_cf->m_parent = this;
    } else {
      if (srcNode->m_cf) {
	if (dstNode->m_cf)
	  dstNode->m_cf->merge(srcNode->m_cf);	// recursive
	else {
	  dstNode->m_cf = srcNode->m_cf;
	  dstNode->m_cf->m_parent = this;
	}
      }
      if (srcNode->m_values.length() && !dstNode->m_values.length())
	dstNode->m_values += srcNode->m_values;
    }
  }
}

ZvCf::Iterator::~Iterator()
{
}

ZuString ZvCf::Iterator::get(ZuString &key)
{
  Tree::NodeRef node;

  do {
    node = m_iterator.iterate();
    if (!node) {
      key = ZuString();
      return ZuString();
    }
  } while (!node->val()->m_values);

  key = node->key();
  return node->val()->m_values[0];
}

const ZtArray<ZtString> *ZvCf::Iterator::getMultiple(
    ZuString &key, unsigned minimum, unsigned maximum)
{
  Tree::NodeRef node;

  do {
    node = m_iterator.iterate();
    if (!node) {
      key = ZuString();
      return 0;
    }
  } while (!node->val()->m_values);

  unsigned n = node->val()->m_values.length();
  if (n < minimum || n > maximum)
    throw NValues(m_cf, node->key(), minimum, maximum, n);
  key = node->key();
  return &node->val()->m_values;
}

ZmRef<ZvCf> ZvCf::Iterator::subset(ZuString &key)
{
  Tree::NodeRef node;

  do {
    node = m_iterator.iterate();
    if (!node) {
      key = ZuString();
      return 0;
    }
  } while (!node->val()->m_cf);

  key = node->key();
  return node->val()->m_cf;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
