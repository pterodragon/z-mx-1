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
    inline Usage(Option &&option) : m_option(ZuFwd<Option>(option)) { }
    void print_(ZmStream &s) const {
      s << "invalid option \"" << m_option << '"';
    }
  private:
    ZtString	m_option;
  };

  void parseCLI(ZuString line, ZtArray<ZtString> &args);
  // fromCLI() and fromArgs() return the number of positional arguments
  int fromCLI(ZvCf *syntax, ZuString line);
  int fromArgs(ZvCf *options, const ZtArray<ZtString> &args);
  int fromArgs(const ZvOpt *opts, int argc, char **argv);
  int fromCLI(const ZvOpt *opts, ZuString line); // deprecated
  int fromArgs(const ZvOpt *opts, const ZtArray<ZtString> &args); // deprecated

  // thrown by fromString() and fromFile() for an invalid key
  class ZvAPI Invalid : public ZvError {
  public:
    template <typename Key, typename FileName>
    inline Invalid(Key &&key, FileName &&fileName) :
	m_key(ZuFwd<Key>(key)), m_fileName(ZuFwd<FileName>(fileName)) { }
    void print_(ZmStream &s) const {
      if (m_fileName) s << '"' << m_fileName << "\": ";
      s << "invalid key \"" << m_key << '"';
    }
    ZuInline const ZtString &key() const { return m_key; }
  private:
    ZtString	m_key;
    ZtString	m_fileName;
  };

  // thrown by fromString() and fromFile() on error
  class ZvAPI Syntax : public ZvError {
  public:
    template <typename FileName>
    inline Syntax(int line, char ch, FileName &&fileName) :
      m_line(line), m_ch(ch), m_fileName(ZuFwd<FileName>(fileName)) { }
    void print_(ZmStream &s) const {
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
    }
  private:
    int		m_line;
    char	m_ch;
    ZtString	m_fileName;
  };

  class ZvAPI BadDefine : public ZvError {
  public:
    template <typename Define, typename FileName>
    inline BadDefine(Define &&define, FileName &&fileName) :
      m_define(ZuFwd<Define>(define)), m_fileName(ZuFwd<FileName>(fileName)) { }
    void print_(ZmStream &s) const {
      if (m_fileName) s << '"' << m_fileName << "\": ";
      s << "bad %%define \"" << m_define << '"';
    }
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
    fromString(in, validate, ZuString(), defines);
  }

  // thrown by fromFile() on error
  class ZvAPI File2Big : public ZvError {
  public:
    template <typename FileName>
    inline File2Big(FileName &&fileName) :
      m_fileName(ZuFwd<FileName>(fileName)) { }
    void print_(ZmStream &s) const {
      s << '"' << m_fileName << " file too big";
    };
  private:
    ZtString	m_fileName;
  };

  class ZvAPI FileOpenError : public ZvError {
  public:
    template <typename FileName>
    FileOpenError(FileName &&fileName, ZeError e) :
      m_fileName(ZuFwd<FileName>(fileName)), m_err(e) { }

    void print_(ZmStream &s) const {
      s << '"' << m_fileName << "\" " << m_err;
    }

  private:
    ZtString	m_fileName;
    ZeError   m_err;
  };

  template <typename FileName>
  inline void fromFile(const FileName &fileName, bool validate,
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
    void print_(ZmStream &s) const {
      s << "syntax error at position " << ZuBoxed(m_pos) << " near '";
      if (m_ch >= 0x20 && m_ch < 0x7f)
	s << m_ch;
      else
	s << '\\' << ZuBoxed((unsigned)m_ch & 0xff).fmt(
	    ZuFmt::Hex<0, ZuFmt::Alt<ZuFmt::Right<2> > >());
      s << '\'';
    }
  private:
    int		m_pos;
    char	m_ch;
  };

  void fromEnv(const char *name, bool validate);

  // caller must call freeArgs() after toArgs()
  void toArgs(int &argc, char **&argv);
  static void freeArgs(int argc, char **argv);

  void print(ZmStream &s, ZtString prefix) const;

  inline void print(ZmStream &s) const { print(s, ""); }
  template <typename S> inline void print(S &s_) const {
    ZmStream s(s_);
    print(s, "");
  }

  struct Prefixed {
    inline void print(ZmStream &s) const {
      cf.print(s, ZuMv(prefix));
    }
    template <typename S> void print(S &s_) const {
      ZmStream s(s_);
      print(s);
    }
    const ZvCf		&cf;
    mutable ZtString	prefix;
  };
  template <typename Prefix>
  inline Prefixed prefixed(Prefix &&prefix) {
    return Prefixed{*this, ZuFwd<Prefix>(prefix)};
  }

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

public:
  // thrown by all get methods for missing values when required is true
  class ZvAPI Required : public ZvError {
  public:
    template <typename Key>
    inline Required(const Key &key) : m_key(key) { }
    void print_(ZmStream &s) const {
      s << m_key << " missing";
    }
  private:
    ZtString	m_key;
  };

  // template base class for NValues / RangeInt / RangeDbl exceptions
  template <typename T> class Range_ : public ZvError {
  public:
    template <typename Key>
    inline Range_(T minimum, T maximum, T value, const Key &key) :
	m_minimum(minimum), m_maximum(maximum), m_value(value), m_key(key) { }
    inline T minimum() const { return m_minimum; }
    inline T maximum() const { return m_maximum; }
    inline T value() const { return m_value; }
    inline const ZtString &key() const { return m_key; }
  protected:
    T		m_minimum;
    T		m_maximum;
    T		m_value;
    ZtString	m_key;
  };
  // thrown by getMultiple() on number of values error
  class ZvAPI NValues : public Range_<Int> {
  public:
    template <typename Key>
    inline NValues(Int minimum, Int maximum, Int value, Key &&key) :
	Range_<Int>(minimum, maximum, value, ZuFwd<Key>(key)) { }
    void print_(ZmStream &s) const {
      s << m_key << " invalid number of values "
	"min(" << m_minimum << ") <= " << m_value <<
	" <= max(" << m_maximum << ")";
    }
  };
  // thrown by getInt() on range error
  class ZvAPI RangeInt : public Range_<Int> {
  public:
    template <typename Key>
    inline RangeInt(Int minimum, Int maximum, Int value, Key &&key) :
	Range_<Int>(minimum, maximum, value, ZuFwd<Key>(key)) { }
    void print_(ZmStream &s) const {
      s << m_key << " out of range "
	"min(" << m_minimum << ") <= " << m_value <<
	" <= max(" << m_maximum << ")";
    }
  };
  // thrown by getInt64() on range error
  class ZvAPI RangeInt64 : public Range_<Int64> {
  public:
    template <typename Key>
    inline RangeInt64(
	Int64 minimum, Int64 maximum, Int64 value, Key &&key) :
	Range_<Int64>(minimum, maximum, value, ZuFwd<Key>(key)) { }
    void print_(ZmStream &s) const {
      s << m_key << " out of range "
	"min(" << m_minimum << ") <= " << m_value <<
	" <= max(" << m_maximum << ")";
    }
  };
  // thrown by getDbl() on range error
  class ZvAPI RangeDbl : public Range_<Double> {
  public:
    template <typename Key>
    inline RangeDbl(
	Double minimum, Double maximum, Double value, Key &&key) :
      Range_<Double>(minimum, maximum, value, ZuFwd<Key>(key)) { }
    void print_(ZmStream &s) const {
      s << m_key << " out of range "
	"min(" << m_minimum << ") <= " << m_value <<
	" <= max(" << m_maximum << ")";
    }
  };

  class ZvAPI BadFmt : public ZvError {
  public:
    template <typename Value, typename Fmt>
    inline BadFmt(Value &&value, Fmt &&fmt) :
	m_value(ZuFwd<Value>(value)), m_fmt(ZuFwd<Fmt>(fmt)) { }
    void print_(ZmStream &s) const {
      s << '"' << m_value << "\" not format " << m_fmt;
    }
  private:
    ZtString		m_value;
    ZtString		m_fmt;
  };

  ZuString get(ZuString key, bool required, ZuString def);
  inline ZuString get(ZuString key, bool required = false)
    { return get(key, required, ZuString()); }
  const ZtArray<ZtString> *getMultiple(ZuString key,
      unsigned minimum, unsigned maximum, bool required = false);
  void set(ZuString key, ZuString val);
  ZtArray<ZtString> *setMultiple(ZuString key);
  ZmRef<ZvCf> subset(ZuString key, bool create, bool required = false);
  void subset(ZuString key, ZvCf *cf);

  void merge(ZvCf *cf);

  template <typename Key>
  inline static Int toInt(const Key &key, ZuString value,
      Int minimum, Int maximum, Int def = Int()) {
    if (!value) return def;
    Int i(value);
    if (i < minimum || i > maximum) throw RangeInt(minimum, maximum, i, key);
    return i;
  }

  template <typename Key>
  inline static Int64 toInt64(const Key &key, ZuString value,
      Int64 minimum, Int64 maximum, Int64 def = Int64()) {
    if (!value) return def;
    Int64 i(value);
    if (i < minimum || i > maximum) throw RangeInt64(minimum, maximum, i, key);
    return i;
  }

  template <typename Key>
  inline static Double toDbl(const Key &key, ZuString value,
      Double minimum, Double maximum, Double def = Double()) {
    if (!value) return def;
    ZuBox<Double> d(value);
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
    return ZvFlags<Map>::instance()->template scan<Flags>(key, value);
  }

  template <typename Map, typename Key, typename Value>
  inline static Flags64 toFlags64(
      const Key &key, const Value &value, Flags64 def = 0) {
    if (!value) return def;
    return ZvFlags<Map>::instance()->template scan<Flags64>(key, value);
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

    ZuString get(ZuString &key);
    const ZtArray<ZtString> *getMultiple(ZuString &key,
	unsigned minimum, unsigned maximum);
    ZmRef<ZvCf> subset(ZuString &key);
    inline Int getInt(ZuString &key, Int minimum, Int maximum,
	Int def = Int()) {
      return toInt(key, get(key), minimum, maximum, def);
    }
    inline Int64 getInt64(ZuString &key, Int64 minimum, Int64 maximum,
	Int64 def = Int64()) {
      return toInt64(key, get(key), minimum, maximum, def);
    }
    inline Double getDbl(ZuString &key, Double minimum, Double maximum,
	Double def = Double()) {
      return toDbl(key, get(key), minimum, maximum, def);
    }
    template <typename Map>
    inline Enum getEnum(ZuString &key, Enum def = Enum()) {
      return toEnum<Map>(key, get(key), def);
    }

  private:
    Tree::ReadIterator<>	m_iterator;
  };

private:
  ZmRef<ZvCf> scope(ZuString fullKey, ZuString &key, bool create);

  void fromArg(ZuString fullKey, int type, ZuString argVal);
  void fromString(ZuString in, bool validate,
      ZuString fileName, ZmRef<Defines> defines);

  void toArgs(ZtArray<ZtString> &args, ZuString prefix);

  static ZtString quoteArgValue(ZuString value);
  static ZtString quoteValue(ZuString value);

  Tree		m_tree;
  ZvCf		*m_parent;
};
template <> struct ZuPrint<ZvCf> : public ZuPrintFn { };
template <> struct ZuPrint<ZvCf::Prefixed> : public ZuPrintFn { };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZvCf_HPP */
