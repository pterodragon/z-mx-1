#include <ZeLog.hpp>

#include <ZvCf.hpp>

int main(int argc, char **argv)
{
  if (argc < 2) {
    fputs("You must specify a input config file name.\n", stderr);
    return 1;
  }

  ZmRef<ZvCf> cf = new ZvCf();
  try {
    cf->fromFile(argv[1], false);
  } catch (ZvError &error) {
    fputs(error.message().data(), stderr);
    fputs("\n", stderr);
    return 1;
  }
  ZtZString output = cf->toString();
  puts(output.data());

  return 0;
}

