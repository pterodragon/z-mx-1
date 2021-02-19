#include <zlib/ZmBackTrace.hpp>

typedef ZmBackTrace (*Fn)();

ZmBackTrace baz(Fn fn) { return fn(); }

ZmBackTrace bar(Fn fn) { return baz(fn); }

extern
#ifdef _WIN32
ZuExport_API
#endif
ZmBackTrace xfoo(Fn fn) { return bar(fn); }
