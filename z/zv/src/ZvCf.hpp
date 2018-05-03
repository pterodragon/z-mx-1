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

// application configuration

#ifndef ZvCf_HPP
#define ZvCf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <ZuBox.hpp>

#include <ZmObject.hpp>
#include <ZmRef.hpp>
#include <ZmList.hpp>
#include <ZmRBTree.hpp>
#include <ZmNoLock.hpp>

#include <ZtArray.hpp>
#include <ZtString.hpp>
#include <ZtRegex.hpp>

#include <ZiFile.hpp>

#include <ZvError.hpp>
#include <ZvRegexError.hpp>
#include <ZvEnum.hpp>

#define ZvCfMaxFileSize	(1<<20)	// 1Mb

#define ZvOptFlag	0
#define ZvOptScalar	1
#define ZvOptMulti	2

extern "C" {
  struct ZvOpt {
    const char	*m_long;
    const char	*m_short;
    int		m_type;		// ZvOptFlag, ZvOptScalar or ZvOptMulti
    const char	*m_default;
  };
};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

class ZvAPI ZvCf : public ZuObject {
  ZvCf(const ZvCf &);
  ZvCf &operator =(const ZvCf &);	// prevent mis-use

public:
  ZvCf();
private:
  ZvCf(ZvCf *parent);
public:
  ~ZvCf();

  typedef ZuBox<int32_t> Int;
  typedef ZuBox<int64_t> Int64;
  typedef ZuBox<double> Double;
  typedef ZtEnum Enum;
  typedef ZuBox<uint32_t> Flags;
  typedef ZuBox<uint64_t> Flags64;

  // fromArgs option types
  struct OptTypes {
    ZtEnumValues(Flag = ZvOptFlag, Scalar = ZvOptScalar, Multi = ZvOptMulti);
    ZtEnumNames("flag", "scalar", "multi");
  };

  // thrown by fromArgs() on error
  class ZvAPI Usage : public ZvError {
  public:
    template <typename Option>
    inline Usage(const Option &option) : m_option(option) { }
    inline Usage(const Usage &u) : m_option(u.m_option) { }
    inline Usage &operator =(const Usage &u) {
      if (this != &u) m_option = u.m_option;
      return *this;
    }
    virtual ~Usage() { }
    ZtZString message() const {
      return ZtZString() << "invalid option \"" << m_option << '"';
    }
    inline ZtZString option() const { return m_option; }
  private:
    ZtString	m_option;
  };

  static ZtZArray<ZtString> parseCLI(ZuString line);
  // fromCLI() and fromArgs() return the number of positional arguments
  int fromCLI(ZvCf *syntax, ZuString line);
  int fromArgs(ZvCf *options, const ZtZArray<ZtString> &args);
  int fromArgs(const ZvOpt *opts, int argc, char **argv);
  int fromCLI(const ZvOpt *opts, ZuString line); // deprecated
  int fromArgs(const ZvOpt *opts, const ZtZArray<ZtString> &args); // deprecated

  // thrown by fromString() and fromFile() for an invalid key
  class ZvAPI Invalid : public ZvError {
  public:
    template <typename Key, typename FileName>
    inline Invalid(const Key &key, const FileName &fileName) :
	m_key(key), m_fileName(fileName) { }
    inline Invalid(const Invalid &i) :
	m_key(i.m_key), m_fileName(i.m_fileName) { }
    inline Invalid &operator =(const Invalid &i) {
      if (this != &i) m_key = i.m_key, m_fileName = i.m_fileName;
      return *this;
    }
    virtual ~Invalid() { }
    ZtZString message() const {
      ZtZString s;
      if (m_fileName) s << '"' << m_fileName << "\" ";
      s << "invalid key \"" << m_key << '"';
      return s;
    }
    inline ZtZString key() const { return m_key; }
    inline ZtZString fileName() const { return m_fileName; }
  private:
    ZtString	m_key;
    ZtString	m_fileName;
  };

  // thrown by fromString() and fromFile() on error
  class ZvAPI Syntax : public ZvError {
  public:
    template <typename FileName>
    inline Syntax(int line, char ch, const FileName &fileName) :
      m_line(line), m_ch(ch), m_fileName(fileName) { }
    inline Syntax(const Syntax &s) :
      m_line(s.m_line), m_ch(s.m_ch), m_fileName(s.m_fileName) { }
    inline Syntax &operator =(const Syntax &s) {
      if (this != &s)
	m_line = s.m_line, m_ch = s.m_ch, m_fileName = s.m_fileName;
      return *this;
    }
    virtual ~Syntax() { }
    ZtZString message() const {
      ZtZString s;
      if (m_fileName)
	s << '"' << m_fileName << "\":" << ZuBoxed(m_line) << " syntax error";
      else
	s << "syntax error at line " << ZuBoxed(m_line);
      s << " near '";
      if (m_ch >= 0x20 && m_ch < 0x7f)
	s << m_ch;
      else
	s << '\\' << ZuBoxed((unsigned)m_ch & 0xff).fmt(
	    ZuFmt::Hex<0, ZuFmt::Alt<ZuFmt::Right<2> > >());
      s << '\'';
      return s;
    }
    inline int line() const { return m_line; }
    inline int ch() const { return m_ch; }
    inline ZtZString fileName() const { return m_fileName; }
  private:
    int		m_line;
    char	m_ch;
    ZtString	m_fileName;
  };

  class ZvAPI BadDefine : public ZvError {
  public:
    template <typename Define, typename FileName>
    inline BadDefine(const Define &define, const FileName &fileName) :
      m_define(define), m_fileName(fileName) { }
    inline BadDefine(const BadDefine &d) :
      m_define(d.m_define), m_fileName(d.m_fileName) { }
    inline BadDefine &operator =(const BadDefine &d) {
      if (this != &d) m_define = d.m_define, m_fileName = d.m_fileName;
      return *this;
    }
    virtual ~BadDefine() { }
    ZtZString message() const {
      ZtZString s;
      if (m_fileName) s << '"' << m_fileName << "\": ";
      s << "bad %%define \"" << m_define << '"';
      return s;
    }
    inline ZtZString define() const { return m_define; }
    inline ZtZString fileName() const { return m_fileName; }
  private:
    ZtString	m_define;
    ZtString	m_fileName;
  };

  typedef ZmRBTree<ZtString,
	    ZmRBTreeVal<ZtString,
	      ZmRBTreeBase<ZuObject,
		ZmRBTreeLock<ZmNoLock> > > > Defines;

  inline void fromString(
      ZuString in, bool validate,
      ZmRef<Defines> defines = new Defines()) {
    fromString(in, validate, ZtZString(), defines);
  }

  // thrown by fromFile() on error
  class ZvAPI File2Big : public ZvError {
  public:
    template <typename FileName>
    inline File2Big(const FileName &fileName) : m_fileName(fileName) { }
    inline File2Big(const File2Big &f) : m_fileName(f.m_fileName) { }
    inline File2Big &operator =(const File2Big &f) {
      if (this != &f) m_fileName = f.m_fileName;
      return *this;
    }
    virtual ~File2Big() { }
    ZtZString message() const {
      return ZtZString() << '"' << m_fileName << " file too big";
    };
    inline ZtZString fileName() const { return m_fileName; }
  private:
    ZtString	m_fileName;
  };

  class ZvAPI FileOpenError : public ZvError {
    public:
      template <typename FileName>
      FileOpenError(const FileName &fileName, ZeError e) :
	m_fileName(fileName), m_err(e) { }
      ZtZString message() const {
	return ZtZString() << '"' << m_fileName << "\" " << m_err;
      }
    private:
      ZtString	m_fileName;
      ZeError   m_err;
  };

  template <typename FileName>
  inline void fromFile(
      const FileName &fileName, bool validate,
      ZmRef<Defines> defines = new Defines()) {
    ZtString in;
    {
      ZiFile file;
      ZeError e;

      if (file.open(fileName, ZiFile::ReadOnly, 0, &e) < 0)
	throw FileOpenError(fileName, e);

      int n = (int)file.size();

      if (n >= ZvCfMaxFileSize) throw File2Big(fileName);
      in.length(n);
      if (file.read(in.data(), n, &e) < 0) throw e;

      file.close();
    }
    ZtString dir = ZiFile::dirname(fileName);
    if (!defines->find("TOPDIR")) defines->add("TOPDIR", dir);
    defines->del("CURDIR"); defines->add("CURDIR", ZuMv(dir));
    fromString(in, validate, fileName, defines);
  }

  // thrown by fromEnv() on error
  class ZvAPI EnvSyntax : public ZvError {
  public:
    inline EnvSyntax(int pos, char ch) : m_pos(pos), m_ch(ch) { }
    inline EnvSyntax(const EnvSyntax &s) : m_pos(s.m_pos), m_ch(s.m_ch) { }
    inline EnvSyntax &operator =(const EnvSyntax &s) {
      if (this != &s) m_pos = s.m_pos, m_ch = s.m_ch;
      return *this;
    }
    virtual ~EnvSyntax() { }
    ZtZString message() const {
      ZtZString s;
      s << "syntax error at position " << ZuBoxed(m_pos) << " near '";
      if (m_ch >= 0x20 && m_ch < 0x7f)
	s << m_ch;
      else
	s << '\\' << ZuBoxed((unsigned)m_ch & 0xff).fmt(
	    ZuFmt::Hex<0, ZuFmt::Alt<ZuFmt::Right<2> > >());
      s << '\'';
      return s;
    }
    inline int pos() const { return m_pos; }
    inline int ch() const { return m_ch; }
  private:
    int		m_pos;
    char	m_ch;
  };

  void fromEnv(const char *name, bool validate);

  // caller must call freeArgs() after toArgs()
  void toArgs(int &argc, char **&argv);
  static void freeArgs(int argc, char **argv);

  ZtZString toString();

  // toFile() will throw ZeError on I/O error
  template <typename FileName>
  inline void toFile(const FileName &fileName) {
    ZiFile file;
    ZeError e;
    if (file.open(fileName, ZiFile::Create | ZiFile::Truncate, 0777, &e) < 0)
      throw e;
    toFile_(file);
  }
private:
  void toFile_(ZiFile &file);

  static void unstrtok(ZtString &key) {
    int i, n = key.length();

    for (i = 0; i < n; i++) if (!key[i]) key[i] = ':';
  }

public:
  // thrown by all get methods for missing values when required is true
  class ZvAPI Required : public ZvError {
  public:
    template <typename Key>
    inline Required(const Key &key) : m_key(key) { unstrtok(m_key); }
    inline Required(const Required &r) : m_key(r.m_key) { }
    inline Required &operator =(const Required &r) {
      if (this != &r) m_key = r.m_key;
      return *this;
    }
    ZtZString message() const {
      return ZtZString() << m_key << " missing";
    }
    inline ZtZString key() const { return m_key; }
  private:
    ZtString	m_key;
  };

  // template base class for NValues / RangeInt / RangeDbl exceptions
  template <typename T> class Range_ : public ZvError {
  public:
    template <typename Key>
    inline Range_(T minimum, T maximum, T value, const Key &key) :
	m_minimum(minimum), m_maximum(maximum), m_value(value), m_key(key) {
      unstrtok(m_key);
    }
    inline Range_(const Range_ &r) :
	m_minimum(r.m_minimum), m_maximum(r.m_maximum),
	m_value(r.m_value), m_key(r.m_key) { }
    inline Range_ &operator =(const Range_ &r) {
      if (this != &r)
	m_minimum = r.m_minimum, m_maximum = r.m_maximum,
	m_value = r.m_value, m_key = r.m_key;
      return *this;
    }
    virtual ~Range_() { }
    inline T minimum() const { return m_minimum; }
    inline T maximum() const { return m_maximum; }
    inline T value() const { return m_value; }
    inline ZtZString key() const { return m_key; }
  protected:
    T		m_minimum;
    T		m_maximum;
    T		m_value;
    ZtString	m_key;
  };
  // thrown by getMultiple() on number of values error
  class ZvAPI NValues : public Range_<int> {
  public:
    template <typename Key>
    inline NValues(int minimum, int maximum, int value, const Key &key) :
	Range_<int>(minimum, maximum, value, key) { }
    inline NValues(const NValues &r) : Range_<int>(r) { }
    inline NValues &operator =(const NValues &r) {
      Range_<int>::operator =(r);
      return *this;
    }
    virtual ~NValues() { }
    ZtZString message() const {
      return ZtZString() << m_key << " invalid number of values "
	"min(" << ZuBoxed(m_minimum) << ") <= " <<
	ZuBoxed(m_value) <<
	" <= max(" << ZuBoxed(m_maximum) << ")";
    }
  };
  // thrown by getInt() on range error
  class ZvAPI RangeInt : public Range_<int> {
  public:
    template <typename Key>
    inline RangeInt(int minimum, int maximum, int value, const Key &key) :
	Range_<int>(minimum, maximum, value, key) { }
    inline RangeInt(const RangeInt &r) : Range_<int>(r) { }
    inline RangeInt &operator =(const RangeInt &r) {
      Range_<int>::operator =(r);
      return *this;
    }
    virtual ~RangeInt() { }
    ZtZString message() const {
      return ZtZString() << m_key <<
	" out of range min(" << m_minimum <<
	") <= value(" << m_value <<
	") <= max(" << m_maximum << ")";
    }
  };
  // thrown by getInt64() on range error
  class ZvAPI RangeInt64 : public Range_<Int64> {
  public:
    template <typename Key>
    inline RangeInt64(
	Int64 minimum, Int64 maximum, Int64 value, const Key &key) :
	Range_<Int64>(minimum, maximum, value, key) { }
    inline RangeInt64(const RangeInt64 &r) : Range_<Int64>(r) { }
    inline RangeInt64 &operator =(const RangeInt64 &r) {
      Range_<Int64>::operator =(r);
      return *this;
    }
    virtual ~RangeInt64() { }
    ZtZString message() const {
      return ZtZString() << m_key <<
	" out of range min(" << m_minimum <<
	") <= value(" << m_value <<
	") <= max(" << m_maximum << ")";
    }
  };
  // thrown by getDbl() on range error
  class ZvAPI RangeDbl : public Range_<Double> {
  public:
    template <typename Key>
    inline RangeDbl(
	Double minimum, Double maximum, Double value, const Key &key) :
      Range_<Double>(minimum, maximum, value, key) { }
    inline RangeDbl(const RangeDbl &r) : Range_<Double>(r) { }
    inline RangeDbl &operator =(const RangeDbl &r) {
      Range_<Double>::operator =(r);
      return *this;
    }
    virtual ~RangeDbl() { }
    ZtZString message() const {
      return ZtZString() << m_key <<
	" out of range min(" << m_minimum <<
	") <= value(" << m_value <<
	") <= max(" << m_maximum << ")";
    }
  };

  class ZvAPI BadFmt : public ZvError {
  public:
    template <typename Value, typename Fmt>
    inline BadFmt(Value &&value, Fmt &&fmt) :
	m_value(ZuFwd<Value>(value)), m_fmt(ZuFwd<Fmt>(fmt)) { }
    inline BadFmt &operator =(const BadFmt &e) {
      if (this != &e) m_value = e.m_value, m_fmt = e.m_fmt;
      return *this;
    }
    virtual ~BadFmt() { }
    ZtZString message() const {
      return ZtZString() << '"' << m_value << "\" not format " << m_fmt;
    }
    inline ZtZString value() const { return m_value; }
    inline ZtZString fmt() const { return m_fmt; }
  private:
    ZtString	m_value;
    ZtString	m_fmt;
  };

  ZtZString get(ZuString key, bool required, ZuString def);
  inline ZtZString get(ZuString key, bool required = false)
    { return get(key, required, ZuString()); }
  typedef const ZtArray<ZtString> Multiple;
  const ZtArray<ZtString> *getMultiple(ZuString key,
      unsigned minimum, unsigned maximum, bool required = false);
  void set(ZuString key, ZuString val);
  ZtArray<ZtString> *setMultiple(ZuString key);
  ZmRef<ZvCf> subset(ZuString key, bool create, bool required = false);
  void subset(ZuString key, ZvCf *cf);

  void merge(ZvCf *cf);

  template <typename Key>
  inline static Int toInt(const Key &key, const char *value,
      Int minimum, Int maximum, Int def = Int()) {
    if (!value) return def;
    ZuBox<int> i; i.scan(value);
    if (i < minimum || i > maximum) throw RangeInt(minimum, maximum, i, key);
    return i;
  }

  template <typename Key>
  inline static Int64 toInt64(const Key &key, const char *value,
      Int64 minimum, Int64 maximum, Int64 def = Int64()) {
    if (!value) return def;
    ZuBox<Int64> i; i.scan(value);
    if (i < minimum || i > maximum) throw RangeInt64(minimum, maximum, i, key);
    return i;
  }

  template <typename Key>
  inline static Double toDbl(const Key &key, const char *value,
      Double minimum, Double maximum, Double def = Double()) {
    if (!value) return def;
    ZuBox<Double> d; d.scan(value);
    if (d < minimum || d > maximum) throw RangeDbl(minimum, maximum, d, key);
    return d;
  }

  template <typename Key>
  Int getInt(const Key &key, Int minimum, Int maximum, bool required,
      Int def = Int()) {
    return toInt(key, get(key, required), minimum, maximum, def);
  }

  template <typename Key>
  Int64 getInt64(const Key &key, Int64 minimum, Int64 maximum,
      bool required, Int64 def = Int64()) {
    return toInt64(key, get(key, required), minimum, maximum, def);
  }

  template <typename Key>
  Double getDbl(const Key &key, Double minimum, Double maximum,
      bool required, Double def = Double()) {
    return toDbl(key, get(key, required), minimum, maximum, def);
  }

  template <typename Map, typename Key, typename Value>
  inline static Enum toEnum(
      const Key &key, const Value &value, Enum def = Enum()) {
    return ZvEnum<Map>::instance()->s2v(key, value, def);
  }

  template <typename Map, typename Key>
  inline Enum getEnum(const Key &key, bool required, Enum def = Enum()) {
    return toEnum<Map>(key, get(key, required), def);
  }

  template <typename Map, typename Key, typename Value>
  inline static Flags toFlags(
      const Key &key, const Value &value, Flags def = 0) {
    if (!value) return def;
    return ZvFlags<Map>::instance()->template scan<Flags>(value);
  }

  template <typename Map, typename Key, typename Value>
  inline static Flags64 toFlags64(
      const Key &key, const Value &value, Flags64 def = 0) {
    if (!value) return def;
    return ZvFlags<Map>::instance()->template scan<Flags64>(value);
  }

  template <typename Flags_, typename Key>
  inline Flags getFlags(
      const Key &key, bool required = false, Flags def = 0) {
    return toFlags<Flags_>(key, get(key, required), def);
  }

  template <typename Flags_, typename Key>
  inline Flags64 getFlags64(
      const Key &key, bool required = false, Flags64 def = 0) {
    return toFlags64<Flags_>(key, get(key, required), def);
  }

private:
  struct Node : public ZuObject {
    ZtArray<ZtString>	m_values;
    ZmRef<ZvCf>		m_cf;
  };

  typedef ZmRef<Node> NodeRef;

  struct HeapID { inline static const char *id() { return "ZvCf"; } };

  typedef ZmRBTree<ZtString,
	    ZmRBTreeVal<NodeRef,
	      ZmRBTreeHeapID<HeapID> > > Tree;

public:
  inline int count() const { return m_tree.count(); }
  inline ZmRef<ZvCf> parent() const { return m_parent; }

  class Iterator;
friend class Iterator;
  class ZvAPI Iterator {
  public:
    inline Iterator(ZvCf *cf) : m_iterator(cf->m_tree) { }
    inline Iterator(ZvCf *cf, ZuString prefix) :
	m_iterator(cf->m_tree, prefix) { }
    ~Iterator();

    ZtZString get(ZtZString &key);
    const ZtArray<ZtString> *getMultiple(ZtZString &key,
	unsigned minimum, unsigned maximum);
    ZmRef<ZvCf> subset(ZtZString &key);
    inline Int getInt(ZtZString &key, Int minimum, Int maximum,
	Int def = Int()) {
      return toInt(key, get(key), minimum, maximum, def);
    }
    inline Int64 getInt64(ZtZString &key, Int64 minimum, Int64 maximum,
	Int64 def = Int64()) {
      return toInt64(key, get(key), minimum, maximum, def);
    }
    inline Double getDbl(ZtZString &key, Double minimum, Double maximum,
	Double def = Double()) {
      return toDbl(key, get(key), minimum, maximum, def);
    }
    template <typename Map>
    inline Enum getEnum(ZtZString &key, Enum def = Enum()) {
      return toEnum<Map>(key, get(key), def);
    }

  private:
    Tree::ReadIterator	m_iterator;
  };

private:
  ZmRef<ZvCf> scope(ZtString &fullKey, ZtZString &key, bool create);

  void fromArg(ZuString fullKey, int type, ZuString argVal);
  void fromString(ZuString in, bool validate,
      ZuString fileName, ZmRef<Defines> defines);

  void toArgs(ZtArray<ZtString> &args, ZuString prefix);
  void toString(ZtString &out, ZuString prefix);

  static ZtZString quoteArgValue(ZuString value);
  static ZtZString quoteValue(ZuString value);

  Tree		m_tree;
  ZvCf		*m_parent;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZvCf_HPP */
