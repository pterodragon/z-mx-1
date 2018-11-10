//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stdio.h>

#include <ZtEnum.hpp>

#include <ZeLog.hpp>

#include <ZiFile.hpp>

#include <ZvCf.hpp>

static const char *testdata =
"#\n"
"  #\n"
"     key4 # kick kick\n"
"\n"
"\n"
"     `#` value4\n"
"key2 ok` \n"
"key3 ok2``\n"
"\n"
"# `grok this sucker\n"
"\n"
"	key1		\n"
"			\"ok `\"this is val1``\"		# comment !!\n"
"  0 \"\" 1 Arg1\n"
"key6 { a b c d`} }\n"
"\n"
"key5 `#` k51, \"k5``2\", k` 53`,,\n"
"k54` , k55\n"
"\n"
"%define FAT=artma\n"
"key7 { foo bar{ bah 1 } } key8 C${FAT}n\n";

namespace Values {
  ZtEnumValues(High, Low, Normal);
  ZtEnumNames("High", "Low", "Normal");
}

int main()
{
  ZeLog::init("CfTest");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink("&2"));
  ZeLog::start();

  try {
    {
      ZiFile file;
      ZeError e;
      if (file.open("in.cf", ZiFile::Create | ZiFile::Truncate,
		    0777, &e) != Zi::OK)
	throw e;
      if (file.write((void *)testdata, (int)strlen(testdata), &e) != Zi::OK)
	throw e;
    }
    {
      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromFile("in.cf", false);
      cf->toFile("out.cf");
    }
    ZtString out;
    {
      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromFile("out.cf", false);
      out << *cf;
      bool caught = false;
      try {
	cf->fromFile("out_.cf", false);
      } catch (const ZvError &) {
	caught = true;
      } catch (const ZeError &) {
	caught = true;
      }
      if (!caught) ZeLOG(Error, "Nonexistent file not detected");
    }
    {
      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromString(out, false);
      cf->fromFile("in.cf", true);
      cf->toFile("out2.cf");
    }
    {
      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromFile("out2.cf", false);
      ZtString out2;
      out2 << *cf;
      if (out != out2) ZeLOG(Error, "out.cf and out2.cf differ");
    }
    {
      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromString(out, false);
      bool caught = false;
      try {
	cf->fromString("fubar snafu", true);
      } catch (const ZvCf::Invalid &e) {
	if (e.key() != "fubar") ZeLOG(Error, "Invalid key not passed");
	caught = true;
      }
      if (!caught) ZeLOG(Error, "Invalid key not detected");
    }
    {
      int argc;
      char **argv;
      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromFile("in.cf", false);
      cf->toArgs(argc, argv);
      for (int i = 0; i < argc; i++)
	printf("%d: %s\n", i, argv[i]);
      ZvCf::freeArgs(argc, argv);
    }
    {
      static const char *argv[] = {
	"",
	"--key1=ok \"this is val1``",
	"-AB", "ok ", "ok2``",
	"--key5=# k51,k5``2,k` 53`,,k54` ,k55",
	"-C", "b",
	"--key6:c=d}",
	"--key4",
	"-D",
	"Arg1",
	"--key7:foo=bar",
	"--key8=Cartman",
	0
      };
      static ZvOpt opts[] = {
	{ "key1", 0, ZvOptScalar, 0 },
	{ "key2", "A", ZvOptScalar, 0 },
	{ "key3", "B", ZvOptScalar, 0 },
	{ "key4", 0, ZvOptScalar, "# value4" },
	{ "key5", 0, ZvOptMulti, 0 },
	{ "key6:a", "C", ZvOptScalar, 0 },
	{ "key6:c", 0, ZvOptScalar, 0 },
	{ "key7:foo", 0, ZvOptScalar, 0 },
	{ "key7:foo:bah", "D", ZvOptFlag, 0 },
	{ "key8", 0, ZvOptScalar, 0 },
	{ 0 }
      };

      {
	ZmRef<ZvCf> cf = new ZvCf();

	cf->fromArgs(opts, sizeof(argv) / sizeof(argv[0]) - 1, (char **)argv);
	cf->unset("#");
	cf->toFile("out3.cf");
	ZtString out3;
	out3 << *cf;
	if (out != out3) ZeLOG(Error, "out.cf and out3.cf differ");
      }
      {
	ZmRef<ZvCf> cf = new ZvCf();

	cf->fromCLI(opts,
	  " "
	  "--key1=\'ok \"this is val1``\' "
	  "-AB \"ok \" ok2`` "
	  "--key5=\"# k51,k5``2,k 53`,,k54 ,k55\" "
	  "-C b "
	  "--key6:c=d} "
	  "--key4 "
	  "-D "
	  "--key7:foo=bar "
	  "--key8=Cartman "
	  "Arg1");
	cf->unset("#");
	cf->toFile("out4.cf");
	ZtString out4;
	out4 << *cf;
	if (out != out4) ZeLOG(Error, "out.cf and out4.cf differ");
      }
    }
    {
      static const char *env = "CFTEST="
	"0=:"
	"1=Arg1:"
	"key1=\"ok `\"this is val1``\":"
	"key2=ok` :"
	"key3=ok2``:"
	"key4=\"# value4\":"
	"key5=\"# k51\",k5``2,\"k 53,\",k54` ,k55:"
	"key6={a=b:c=d`}}:"
	"key7={foo=bar{bah=1}}:"
	"key8=Cartman";

#ifdef _WIN32
      _putenv(env);
#else
      putenv((char *)env);
#endif

      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromEnv("CFTEST", false);
      cf->toFile("out5.cf");
      ZtString out5;
      out5 << *cf;
      if (out != out5) ZeLOG(Error, "out.cf and out5.cf differ");
    }

    try {
      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromString("i 101", false);
      if (cf->getInt("j", 1, 100, false, 42) != 42)
	ZeLOG(Error, "getInt() default failed");
      try {
	cf->getInt("j", 1, 100, true);
      } catch (const ZvError &e) {
	std::cout << "OK: " << e << '\n';
      }
      cf->getInt("i", 1, 100, false, 42);
      ZeLOG(Error, "getInt() range failed");
    } catch (const ZvError &e) {
      std::cout << "OK: " << e << '\n';
    }

    try {
      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromString("i 100.01", false);
      if (cf->getDbl("j", .1, 100, false, .42) != .42)
	ZeLOG(Error, "getDbl() default failed");
      try {
	cf->getDbl("j", .1, 100, true);
      } catch (const ZvError &e) {
	std::cout << "OK: " << e << '\n';
      }
      cf->getDbl("i", .1, 100, false, .42);
      ZeLOG(Error, "getDbl() range failed");
    } catch (const ZvError &e) {
      std::cout << "OK: " << e << '\n';
    }

    try {
      ZmRef<ZvCf> cf = new ZvCf();
      cf->fromString("i FooHigh", false);
      if (cf->getEnum<Values::Map>("j", false, -1) >= 0)
	ZeLOG(Error, "getEnum() default failed");
      cf->getEnum<Values::Map>("i", false, 0);
      ZeLOG(Error, "getEnum() invalid failed");
    } catch (const ZvError &e) {
      std::cout << "OK: " << e << '\n';
    }

    {
      ZmRef<ZvCf> cf1 = new ZvCf(), cf2 = new ZvCf(),
		  cf3 = new ZvCf(), cf4 = new ZvCf();

      cf1->fromString("i foo l { m baz }", false);
      cf2->fromString("j { k bar } n bah", false);
      cf3->merge(cf1);
      cf3->merge(cf2);
      cf4->merge(cf2);
      cf4->merge(cf1);
      ZtString out3, out4;
      out3 << *cf3;
      out4 << *cf4;
      if (out3 != out4)
	ZeLOG(Error, "merge() results inconsistent");
    }

    {
      ZmRef<ZvCf> cf = new ZvCf();
      cf->fromString("=A value", false);
      std::cout << "OK: " << cf->get("=A", 1) << '\n';
    }

  } catch (const ZvError &e) {
    std::cerr << e << '\n';
    ZmPlatform::exit(1);
  } catch (const ZeError &e) {
    fputs(e.message(), stderr);
    fputc('\n', stderr);
    ZmPlatform::exit(1);
  } catch (...) {
    fputs("unknown exception\n", stderr);
    ZmPlatform::exit(1);
  }

  ZeLog::stop();
  return 0;
}
