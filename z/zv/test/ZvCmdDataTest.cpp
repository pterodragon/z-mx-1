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

#include <ZvCmd.hpp>

struct TestCmd1 : public ZvCmdObject {
  TestCmd1() : ZvCmdObject("test1") {
    data().option("a", "all", "", "flag", "do it all");
    data().option("", "long-option", "LIST", "multi", 
		  "a long option for something");
    data().option("b", "boi", "RANGE", "scalar", 
		  "a long option for something else");
    data().option("z", "", "", "scalar", "an option with a really long "
		  "description that will break the line");
    data().option("q", "superawesomeactionforce", "", "scalar", 
		  "this message will be doubly indented because "
		  "of long opt name");
    data().brief("Does something with stuff");
    data().manuscript("this is a really long description of doing that may "
		      "flow to more than one line but who knows");
    compile();
  }
};

struct TestCmd2 : public ZvCmdObject {
  TestCmd2() : ZvCmdObject("test2") {
    data().usage("[OPTIONS]... [FILES]...");
    data().usage("[OPTIONS]... secondary usage");
    data().usage("[OPTIONS]... omg a third!");
    data().manuscript("there are no options or brief!");
    compile();
  }
};

struct TestCmd3 : public ZvCmdObject {
  TestCmd3() : ZvCmdObject("test3") {
    data().brief("a command to do someting");
    data().manuscript("there are no options");
    compile();
  }
};

struct TestCmd4 : public ZvCmdObject {
  TestCmd4() : ZvCmdObject("test4") {
    data().brief("a command to do someting");
    option("a", "all", "", "flag", "an option");
    group("Special");
    option("b", "bark", "", "flag", "a special option");
    group("Not Special");
    option("c", "herp", "", "flag", "a not special option");
    option("", "derp", "", "flag", "a not special option");
    option("b", "bark", "", "flag", "repeat option");
    data().manuscript("a command with groups");
    compile();
  }
};

struct TestCmdEx : public ZvCmdObject {
  TestCmdEx() : ZvCmdObject("exceptions") {
    try {
      data().option("", "work", "", "BREAK", "");
    } catch (const ZvError &err) {
      printf ("OK %s\n", err.message().data());
    } catch (...) {
      printf ("ERROR unhandled 0\n");
    }

    try {
      data().option("a", "work", "", "", "");
    } catch (const ZvError &err) {
      printf ("ERROR %s\n", err.message().data());
    } catch (ZtZString &err2) {
      printf ("OK %s\n", err2.data());
    } catch (...) {
      printf ("ERROR unhandled 1\n");
    }

    try {
      data().option("", "long", "", "flag", "");
    } catch (ZtZString &err) {
      printf ("OK %s\n", err.data());    
    } catch (...) {
      printf ("ERROR unhandled 2\n");
    }

    try {
      data().option("", "", "", "flag", "");
    } catch (ZtZString &err) {
      printf ("OK %s\n", err.data());    
    } catch (...) {
      printf ("ERROR unhandled 3\n");
    }

    try {
      data().option("ab", "", "", "flag", "");
    } catch (ZtZString &err) {
      printf ("OK %s\n", err.data());    
    } catch (...) {
      printf ("ERROR unhandled 4\n");
    }
    compile();
  }
};

int main(int argc, char **argv)
{
  try {
    ZmRef<TestCmd1> cmd = new TestCmd1();
    printf("%s\n", cmd->usage().data());
    printf("--------------------------\n");

    ZmRef<TestCmd2> cmd2 = new TestCmd2();
    printf("%s\n", cmd2->usage().data());
    printf("--------------------------\n");

    ZmRef<TestCmd3> cmd3 = new TestCmd3();
    printf("%s\n", cmd3->usage().data());
    printf("--------------------------\n");

    ZmRef<TestCmd4> cmd4 = new TestCmd4();
    printf("%s\n", cmd4->usage().data());
    printf("--------------------------\n");
    printf("%s\n", cmd4->syntax().data());

  } catch (const ZvError &err) {
    printf ("ERROR %s\n", err.message().data());
  } catch (ZtZString &err2) {
    printf ("ERROR %s\n", err2.data());    
  } catch (...) {
    printf ("ERROR unhandled 0\n");
  }

  ZmRef<TestCmdEx> cmd4 = new TestCmdEx();

  return 0;
}
