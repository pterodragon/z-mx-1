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

struct Values {
  ZtEnumValues(High, Low, Normal);
  ZtEnumNames("High", "Low", "Normal");
};

int main()
{
  ZeLog::init("CfTest");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink());
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
      out = cf->toString();
      ZiFile::remove("out.cf");
      bool caught = false;
      try {
	cf->fromFile("out.cf", false);
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
      if (out != cf->toString()) ZeLOG(Error, "out.cf and out2.cf differ");
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
	{ 0 }
      };

      {
	ZmRef<ZvCf> cf = new ZvCf();

	cf->fromArgs(opts, sizeof(argv) / sizeof(argv[0]) - 1, (char **)argv);
	cf->toFile("out3.cf");
	if (out != cf->toString()) ZeLOG(Error, "out.cf and out3.cf differ");
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
	  "Arg1");
	cf->toFile("out4.cf");
	if (out != cf->toString()) ZeLOG(Error, "out.cf and out4.cf differ");
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
	"key7={foo=bar{bah=1}}";

#ifdef _WIN32
      _putenv(env);
#else
      putenv((char *)env);
#endif

      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromEnv("CFTEST", false);
      cf->toFile("out5.cf");
      if (out != cf->toString()) ZeLOG(Error, "out.cf and out5.cf differ");
    }

    try {
      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromString("i 101", false);
      if (cf->getInt("j", 1, 100, false, 42) != 42)
	ZeLOG(Error, "getInt() default failed");
      try {
	cf->getInt("j", 1, 100, true);
      }
      catch (const ZvCf::Required &e) {
	printf("OK: %s\n", e.message().data());
      }
      cf->getInt("i", 1, 100, false, 42);
      ZeLOG(Error, "getInt() range failed");
    } catch (const ZvCf::RangeInt &e) {
      printf("OK: %s\n", e.message().data());
    }

    try {
      ZmRef<ZvCf> cf = new ZvCf();

      cf->fromString("i 100.01", false);
      if (cf->getDbl("j", .1, 100, false, .42) != .42)
	ZeLOG(Error, "getDbl() default failed");
      try {
	cf->getDbl("j", .1, 100, true);
      }
      catch (const ZvCf::Required &e) {
	printf("OK: %s\n", e.message().data());
      }
      cf->getDbl("i", .1, 100, false, .42);
      ZeLOG(Error, "getDbl() range failed");
    } catch (const ZvCf::RangeDbl &e) {
      printf("OK: %s\n", e.message().data());
    }

    try {
      ZmRef<ZvCf> cf = new ZvCf();
      cf->fromString("i FooHigh", false);
      if (cf->getEnum<Values::Map>("j", false, -1) >= 0)
	ZeLOG(Error, "getEnum() default failed");
      cf->getEnum<Values::Map>("i", false, 0);
      ZeLOG(Error, "getEnum() invalid failed");
    } catch (const ZvInvalidEnum &e) {
      printf("OK: %s\n", e.message().data());
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
      if (cf3->toString() != cf4->toString())
	ZeLOG(Error, "merge() results inconsistent");
    }

    {
      ZmRef<ZvCf> cf = new ZvCf();
      cf->fromString("=A value", false);
      printf("OK: %s\n", cf->get("=A", true).data());
    }

  } catch (const ZvError &e) {
    fputs(e.message().data(), stderr);
    fputc('\n', stderr);
    exit(1);
  } catch (const ZeError &e) {
    fputs(e.message(), stderr);
    fputc('\n', stderr);
    exit(1);
  } catch (...) {
    fputs("unknown exception\n", stderr);
    exit(1);
  }

  ZeLog::stop();
  return 0;
}
