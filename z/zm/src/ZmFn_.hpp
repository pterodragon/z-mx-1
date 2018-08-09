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

// function delegates for multithreading and deferred execution (callbacks)

#ifndef ZmFn__HPP
#define ZmFn__HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuTraits.hpp>
#include <ZuConversion.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuNull.hpp>

#include <ZmRef.hpp>
#include <ZmPolymorph.hpp>

template <typename ...Args> class ZmFn;

// ZmFn traits
template <typename ...Args>
struct ZuTraits<ZmFn<Args...> > : public ZuGenericTraits<ZmFn<Args...> > {
  enum { IsHashable = 1, IsComparable = 1 };
};

// ZmFn base class
class ZmAnyFn {
  struct Pass { };

  ZuInline static bool ref(uintptr_t o) {
    return o & (uintptr_t)1;
  }
  ZuInline static ZmPolymorph *ptr(uintptr_t o) {
    return (ZmPolymorph *)(o & ~(uintptr_t)1);
  }

public:
  ZuInline ZmAnyFn() : m_invoker(0), m_object(0) { }

  ZuInline ~ZmAnyFn() {
    if (ZuUnlikely(ref(m_object))) ZmDEREF(ptr(m_object));
  }

  ZuInline ZmAnyFn(const ZmAnyFn &fn) :
      m_invoker(fn.m_invoker), m_object(fn.m_object) {
    if (ZuUnlikely(ref(m_object))) ZmREF(ptr(m_object));
  }

  ZuInline ZmAnyFn(ZmAnyFn &&fn) noexcept :
      m_invoker(fn.m_invoker), m_object(fn.m_object) {
    fn.m_object &= ~(uintptr_t)1;
#ifdef ZmObject_DEBUG
    if (ZuUnlikely(ref(m_object))) ZmMVREF(ptr(m_object), &fn, this);
#endif
  }

  inline ZmAnyFn &operator =(const ZmAnyFn &fn) {
    if (this == &fn) return *this;
    if (ZuUnlikely(ref(fn.m_object))) ZmREF(ptr(fn.m_object));
    if (ZuUnlikely(ref(m_object))) ZmDEREF(ptr(m_object));
    m_invoker = fn.m_invoker;
    m_object = fn.m_object;
    return *this;
  }

  inline ZmAnyFn &operator =(ZmAnyFn &&fn) noexcept {
#ifdef ZmObject_DEBUG
    if (ZuUnlikely(ref(m_object))) ZmDEREF(ptr(m_object));
#endif
    m_invoker = fn.m_invoker;
    m_object = fn.m_object;
    fn.m_object &= ~(uintptr_t)1;
#ifdef ZmObject_DEBUG
    if (ZuUnlikely(ref(m_object))) ZmMVREF(ptr(m_object), &fn, this);
#endif
    return *this;
  }

  // accessed from ZmFn<...>
protected:
  template <typename Invoker, typename O>
  ZuInline ZmAnyFn(const Invoker &invoker, O *o) :
      m_invoker((uintptr_t)invoker), m_object((uintptr_t)o) { }
  template <typename Invoker, typename O>
  ZuInline ZmAnyFn(const Invoker &invoker, ZmRef<O> o,
      typename ZuIsBase<ZmPolymorph, O, Pass>::T *_ = 0) :
	m_invoker((uintptr_t)invoker) {
    new (&m_object) ZmRef<O>(ZuMv(o));
    m_object |= (uintptr_t)1;
  }

public:
  // downcast to ZmFn<...>
  template <typename Fn> ZuInline const Fn &as() const {
    return *static_cast<const Fn *>(this);
  }
  template <typename Fn> ZuInline Fn &as() {
    return *static_cast<Fn *>(this);
  }

  ZuInline bool operator ==(const ZmAnyFn &fn) const {
    return m_invoker == fn.m_invoker && m_object == fn.m_object;
  }

  ZuInline int cmp(const ZmAnyFn &fn) const {
    if (m_invoker < fn.m_invoker) return -1;
    if (m_invoker > fn.m_invoker) return 1;
    return ZuCmp<uintptr_t>::cmp(m_object, fn.m_object);
  }

  ZuInline bool operator !() const { return !m_invoker; }
  ZuOpBool

  ZuInline uint32_t hash() const {
    return
      ZuHash<uintptr_t>::hash(m_invoker) ^ ZuHash<uintptr_t>::hash(m_object);
  }

protected:
  uintptr_t	m_invoker;
  uintptr_t	m_object;
};

struct ZmLambda_HeapID {
  inline static const char *id() { return "ZmLambda"; }
};
template <typename L, class HeapID> struct ZmLambda;

template <typename ...Args> class ZmFn : public ZmAnyFn {
  typedef uintptr_t (*Invoker)(uintptr_t, Args...);
  template <bool VoidRet, auto> struct FnInvoker;
  template <typename, bool VoidRet, auto> struct BoundInvoker;
  template <typename, bool VoidRet, auto> struct MemberInvoker;
  template <typename, class HeapID> struct LambdaInvoker;
  template <typename, class HeapID> struct LBoundInvoker;

  template <typename T> struct IsFunctor_ { enum { OK = 0 }; };
  template <typename L, typename R, typename ...Args_>
    struct IsFunctor_<R (L::*)(Args_...) const> { enum { OK = 1 }; };
  template <typename L, typename R, typename ...Args_>
    struct IsFunctor_<R (L::*)(Args_...)> { enum { OK = 1 }; };
  template <typename T, typename F = decltype(&T::operator ())>
  struct IsFunctor {
    enum { OK = !ZuConversion<ZmAnyFn, T>::Is && IsFunctor_<F>::OK };
  };
  template <typename T, typename R = void, bool OK = IsFunctor<T>::OK>
  struct MatchFunctor;
  template <typename T_, typename R>
  struct MatchFunctor<T_, R, 1> { typedef R T; };

public:
  inline ZmFn() : ZmAnyFn() { }
  inline ZmFn(const ZmFn &fn) : ZmAnyFn(fn) { }
  inline ZmFn(ZmFn &&fn) noexcept : ZmAnyFn(static_cast<ZmAnyFn &&>(fn)) { }

private:
  class Pass {
    friend class ZmFn;
    ZuInline Pass() { }
    ZuInline Pass(const Pass &) { }
    Pass &operator =(const Pass &) = delete;
  };
  template <typename ...Args_>
  ZuInline ZmFn(Pass, Args_ &&...args) : ZmAnyFn(ZuFwd<Args_>(args)...) { }

public:
  // syntactic sugar for lambdas
  template <typename L>
  ZuInline ZmFn(L &&l, typename MatchFunctor<L>::T *_ = 0) :
    ZmAnyFn(lambdaFn(ZuFwd<L>(l))) { }
  template <typename L, typename O>
  ZuInline ZmFn(L &&l, O &&o, typename MatchFunctor<L>::T *_ = 0) :
    ZmAnyFn(lambdaFn(ZuFwd<L>(l), ZuFwd<O>(o))) { }

  inline ZmFn &operator =(const ZmFn &fn) {
    ZmAnyFn::operator =(fn);
    return *this;
  }
  inline ZmFn &operator =(ZmFn &&fn) noexcept {
    ZmAnyFn::operator =(static_cast<ZmAnyFn &&>(fn));
    return *this;
  }

private:
  inline ZmFn &operator =(const ZmAnyFn &fn) {
    ZmAnyFn::operator =(fn);
    return *this;
  }
  inline ZmFn &operator =(ZmAnyFn &&fn) noexcept {
    ZmAnyFn::operator =(static_cast<ZmAnyFn &&>(fn));
    return *this;
  }

public:
  template <typename ...Args_>
  ZuInline uintptr_t operator ()(Args_ &&... args) const {
    if (ZmAnyFn::operator !()) return 0;
    return (*(Invoker)m_invoker)(
	m_object & ~(uintptr_t)1, ZuFwd<Args_>(args)...);
  }

  // plain function pointer
  template <auto Fn> struct Ptr;
  template <typename R, R (*Fn)(Args...)> struct Ptr<Fn> {
    ZuInline static ZmFn fn() {
      return ZmFn(ZmFn::Pass(),
	  &FnInvoker<ZuConversion<void, R>::Same, Fn>::invoke,
	  (void *)0);
    }
  };

  // bound function pointer
  template <auto Fn> struct Bound;
  template <typename C, typename R, R (*Fn)(C *, Args...)>
  struct Bound<Fn> {
    template <typename O> ZuInline static ZmFn fn(O *o) {
      return ZmFn(ZmFn::Pass(),
	  &BoundInvoker<O *, ZuConversion<void, R>::Same, Fn>::invoke, o);
    }
    template <typename O> ZuInline static ZmFn fn(ZmRef<O> o) {
      return ZmFn(ZmFn::Pass(),
	  &BoundInvoker<ZmRef<O>, ZuConversion<void, R>::Same, Fn>::invoke,
	  ZuMv(o));
    }
  };

  // member function
  template <auto Fn> struct Member;
  template <typename C, typename R, R (C::*Fn)(Args...)>
  struct Member<Fn> {
    template <typename O> ZuInline static ZmFn fn(O *o) {
      return ZmFn(ZmFn::Pass(),
	  &MemberInvoker<O *, ZuConversion<void, R>::Same, Fn>::invoke, o);
    }
    template <typename O> ZuInline static ZmFn fn(ZmRef<O> o) {
      return ZmFn(ZmFn::Pass(),
	  &MemberInvoker<O *, ZuConversion<void, R>::Same, Fn>::invoke,
	  ZuMv(o));
    }
  };
  template <typename C, typename R, R (C::*Fn)(Args...) const>
  struct Member<Fn> {
    template <typename O> ZuInline static ZmFn fn(O *o) {
      return ZmFn(ZmFn::Pass(),
	  &MemberInvoker<const O *, ZuConversion<void, R>::Same, Fn>::invoke,
	  o);
    }
    template <typename O> ZuInline static ZmFn fn(ZmRef<O> o) {
      return ZmFn(ZmFn::Pass(),
	  &MemberInvoker<const O *, ZuConversion<void, R>::Same, Fn>::invoke,
	  ZuMv(o));
    }
  };

  // lambdas
  template <typename ...Args_>
  ZuInline static ZmFn lambdaFn(Args_ &&... args) {
    return Lambda<ZmLambda_HeapID>::fn(ZuFwd<Args_>(args)...);
  }
  // lambdas (specifying heap ID)
  template <class HeapID> struct Lambda {
    template <typename L>
    ZuInline static ZmFn fn(L &&l) {
      return LambdaInvoker<
	decltype(&L::operator ()), HeapID>::fn(ZuFwd<L>(l));
    }
    template <typename L, typename O>
    ZuInline static ZmFn fn(L &&l, O *o) {
      return LBoundInvoker<
	decltype(&L::operator ()), HeapID>::fn(ZuFwd<L>(l), o);
    }
    template <typename L, typename O>
    ZuInline static ZmFn fn(L &&l, ZmRef<O> o) {
      return LBoundInvoker<
	decltype(&L::operator ()), HeapID>::fn(ZuFwd<L>(l), ZuMv(o));
    }
  };

private:
  // unbound functions
  template <typename R, R (*Fn)(Args...)> struct FnInvoker<0, Fn> {
    static uintptr_t invoke(uintptr_t, Args... args) {
      return (uintptr_t)(*Fn)(ZuFwd<Args>(args)...);
    }
  };
  template <void (*Fn)(Args...)> struct FnInvoker<1, Fn> {
    static uintptr_t invoke(uintptr_t, Args... args) {
      (*Fn)(ZuFwd<Args>(args)...);
      return 0;
    }
  };

  // bound functions
  template <typename O, typename C, typename R, R (*Fn)(C *, Args...)>
  struct BoundInvoker<O *, 0, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      return (uintptr_t)(*Fn)(static_cast<C *>((O *)o), ZuFwd<Args>(args)...);
    }
  };
  template <typename O, typename C, void (*Fn)(C *, Args...)>
  struct BoundInvoker<O *, 1, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      (*Fn)(static_cast<C *>((O *)o), ZuFwd<Args>(args)...);
      return 0;
    }
  };
  template <typename O, typename C, typename R, R (*Fn)(C *, Args...)>
  struct BoundInvoker<ZmRef<O>, 0, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      return (uintptr_t)(*Fn)(static_cast<C *>((O *)o), ZuFwd<Args>(args)...);
    }
  };
  template <typename O, typename C, void (*Fn)(C *, Args...)>
  struct BoundInvoker<ZmRef<O>, 1, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      (*Fn)(static_cast<C *>((O *)o), ZuFwd<Args>(args)...);
      return 0;
    }
  };

  // member functions
  template <typename O, typename C, typename R, R (C::*Fn)(Args...)>
  struct MemberInvoker<O *, 0, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      return (uintptr_t)(static_cast<C *>((O *)o)->*Fn)(ZuFwd<Args>(args)...);
    }
  };
  template <typename O, typename C, void (C::*Fn)(Args...)>
  struct MemberInvoker<O *, 1, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      (static_cast<C *>((O *)o)->*Fn)(ZuFwd<Args>(args)...);
      return 0;
    }
  };
  template <typename O, typename C, typename R, R (C::*Fn)(Args...)>
  struct MemberInvoker<ZmRef<O>, 0, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      return (uintptr_t)(static_cast<C *>((O *)o)->*Fn)(ZuFwd<Args>(args)...);
    }
  };
  template <typename O, typename C, void (C::*Fn)(Args...)>
  struct MemberInvoker<ZmRef<O>, 1, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      (static_cast<C *>((O *)o)->*Fn)(ZuFwd<Args>(args)...);
      return 0;
    }
  };
  template <typename O, typename C, typename R, R (C::*Fn)(Args...) const>
  struct MemberInvoker<O *, 0, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      return (uintptr_t)
	(static_cast<const C *>((O *)o)->*Fn)(ZuFwd<Args>(args)...);
    }
  };
  template <typename O, typename C, void (C::*Fn)(Args...) const>
  struct MemberInvoker<O *, 1, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      (static_cast<const C *>((O *)o)->*Fn)(ZuFwd<Args>(args)...);
      return 0;
    }
  };
  template <typename O, typename C, typename R, R (C::*Fn)(Args...) const>
  struct MemberInvoker<ZmRef<O>, 0, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      return (uintptr_t)
	(static_cast<const C *>((O *)o)->*Fn)(ZuFwd<Args>(args)...);
    }
  };
  template <typename O, typename C, void (C::*Fn)(Args...) const>
  struct MemberInvoker<ZmRef<O>, 1, Fn> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      (static_cast<const C *>((O *)o)->*Fn)(ZuFwd<Args>(args)...);
      return 0;
    }
  };

  // lambdas (unbound)
  template <typename L, typename R, typename ...Args_> struct LambdaStateless {
    typedef R (*Fn)(Args_...);
    enum { OK = ZuConversion<L, Fn>::Exists };
  };
  template <typename L, typename R, class HeapID, bool, typename ...Args_>
  struct LambdaInvoker_;
  template <typename, typename, class, bool, typename ...Args_>
  friend struct LambdaInvoker_;

  // lambdas with captures (heap-allocated)
  template <typename L, typename R, class HeapID, typename ...Args_>
  struct LambdaInvoker_<L, R, HeapID, 0, Args_...> {
    template <typename L_> ZuInline static ZmFn fn(L_ &&l) {
      typedef typename ZmLambda<L, HeapID>::T O;
      return Member<&L::operator ()>::fn(
	    ZmRef<O>(new typename ZmLambda<L, HeapID>::T(ZuFwd<L_>(l))));
    }
  };
  template <typename L, typename R, class HeapID, typename ...Args_>
  struct LambdaInvoker_<const L, R, HeapID, 0, Args_...> {
    template <typename L_> ZuInline static ZmFn fn(L_ &&l) {
      typedef typename ZmLambda<L, HeapID>::T O;
      return Member<&L::operator ()>::fn(
	    ZmRef<const O>(new typename ZmLambda<L, HeapID>::T(ZuFwd<L_>(l))));
    }
  };

  // fast stateless lambdas (i.e. empty class without captures)
  template <typename L, typename R, class HeapID, typename ...Args_>
  struct LambdaInvoker_<L, R, HeapID, 1, Args_...> {
    static uintptr_t invoke(uintptr_t, Args... args) {
      return (uintptr_t)(*(L *)(void *)0)(ZuFwd<Args>(args)...);
    }
    template <typename L_> ZuInline static ZmFn fn(L_ &&) {
      return ZmFn(ZmFn::Pass(), &LambdaInvoker_::invoke, (void *)0);
    }
  };
  template <typename L, class HeapID, typename ...Args_>
  struct LambdaInvoker_<L, void, HeapID, 1, Args_...> {
    static uintptr_t invoke(uintptr_t, Args... args) {
      (*(L *)(void *)0)(ZuFwd<Args>(args)...);
      return 0;
    }
    template <typename L_> ZuInline static ZmFn fn(L_ &&) {
      return ZmFn(ZmFn::Pass(), &LambdaInvoker_::invoke, (void *)0);
    }
  };

  template <typename R, typename L, typename ...Args_, class HeapID>
  struct LambdaInvoker<R (L::*)(Args_...), HeapID> :
    public LambdaInvoker_<L, R, HeapID, 0, Args_...> { };
  template <typename R, typename L, typename ...Args_, class HeapID>
  struct LambdaInvoker<R (L::*)(Args_...) const, HeapID> :
    public LambdaInvoker_<const L, R, HeapID,
	     LambdaStateless<L, R, Args_...>::OK, Args_...> { };

  // bound lambdas
  template <typename, typename L, typename R, class HeapID, bool Stateless>
  struct LBoundInvoker_;
  template <typename, typename, typename, class, bool>
  friend struct LBoundInvoker_;

  template <typename O, typename L, typename R, class HeapID>
  struct LBoundInvoker_<O *, L, R, HeapID, 0> {
    template <typename L_>
    ZuInline static typename ZuNotMutable<L_, ZmFn>::T fn(L_ l, O *o) {
      return Lambda<HeapID>::fn([l = ZuMv(l), o](Args... args) {
	l(o, ZuFwd<Args>(args)...); });
    }
    template <typename L_>
    ZuInline static typename ZuIsMutable<L_, ZmFn>::T fn(L_ l, O *o) {
      return Lambda<HeapID>::fn([l = ZuMv(l), o](Args... args) mutable {
	l(o, ZuFwd<Args>(args)...); });
    }
    template <typename L_>
    ZuInline static typename ZuNotMutable<L_, ZmFn>::T fn(L_ l, ZmRef<O> o) {
      return Lambda<HeapID>::fn([l = ZuMv(l), o = ZuMv(o)](Args... args) {
	l(o, ZuFwd<Args>(args)...); });
    }
    template <typename L_>
    ZuInline static typename ZuIsMutable<L_, ZmFn>::T fn(L_ l, ZmRef<O> o) {
      return Lambda<HeapID>::fn(
	  [l = ZuMv(l), o = ZuMv(o)](Args... args) mutable {
	l(o, ZuFwd<Args>(args)...); });
    }
  };

  template <typename O, typename L, typename R, class HeapID>
  struct LBoundInvoker_<O *, L, R, HeapID, 1> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      return (uintptr_t)(*(L *)(void *)0)((O *)o, ZuFwd<Args>(args)...);
    }
    ZuInline static ZmFn fn(L, O *o) {
      return ZmFn(ZmFn::Pass(), &LBoundInvoker_::invoke, o);
    }
    ZuInline static ZmFn fn(L, ZmRef<O> o) {
      return ZmFn(ZmFn::Pass(), &LBoundInvoker_::invoke, ZuMv(o));
    }
  };
  template <typename O, typename L, class HeapID>
  struct LBoundInvoker_<O *, L, void, HeapID, 1> {
    static uintptr_t invoke(uintptr_t o, Args... args) {
      (*(L *)(void *)0)((O *)o, ZuFwd<Args>(args)...);
      return 0;
    }
    ZuInline static ZmFn fn(L, O *o) {
      return ZmFn(ZmFn::Pass(), &LBoundInvoker_::invoke, o);
    }
    ZuInline static ZmFn fn(L, ZmRef<O> o) {
      return ZmFn(ZmFn::Pass(), &LBoundInvoker_::invoke, ZuMv(o));
    }
  };

  template <typename O, typename R, typename L, typename ...Args_,
	   class HeapID>
  struct LBoundInvoker<R (L::*)(O *, Args_...), HeapID> :
    public LBoundInvoker_<O *, L, R, HeapID,
	   LambdaStateless<L, R, O *, Args_...>::OK> { };
  template <typename O, typename L, typename ...Args_, class HeapID>
  struct LBoundInvoker<void (L::*)(O *, Args_...), HeapID> :
    public LBoundInvoker_<O *, L, void, HeapID,
	   LambdaStateless<L, void, O *, Args_...>::OK> { };
  template <typename O, typename R, typename L, typename ...Args_,
	   class HeapID>
  struct LBoundInvoker<R (L::*)(O *, Args_...) const, HeapID> :
    public LBoundInvoker_<O *, const L, R, HeapID,
	   LambdaStateless<L, R, O *, Args_...>::OK> { };
  template <typename O, typename L, typename ...Args_, class HeapID>
  struct LBoundInvoker<void (L::*)(O *, Args_...) const, HeapID> :
    public LBoundInvoker_<O *, const L, void, HeapID,
	   LambdaStateless<L, void, O *, Args_...>::OK> { };
};

#endif /* ZmFn__HPP */
