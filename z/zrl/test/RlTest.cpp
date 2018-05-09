//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stdio.h>

#include <ZvCf.hpp>
#include <Zrl.hpp>

static const char *syntax_ =
"help { }\n"
"foo { bar { type flag } b bar }\n"
"bah { booze { type scalar default flooze } }\n"
"baz { one { } two { default zab } }\n"
"exit { }\n";

int main()
{
  ZmRef<ZvCf> syntax = new ZvCf();
  syntax->fromString(syntax_, false);
  Zrl::syntax(syntax);
  for (;;) {
    try {
      ZmRef<ZvCf> line = Zrl::readline("prompt> ");
      if (!line) exit(0);
      std::cout << *line << std::flush;
      if (line->get("0") == "exit") exit(0);
    } catch (const Zrl::EndOfFile &) {
      exit(0);
    } catch (ZvError &e) {
      std::cerr << e << '\n' << std::flush;
    }
  }
}
