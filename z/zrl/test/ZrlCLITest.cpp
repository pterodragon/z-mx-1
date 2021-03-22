#include <zlib/ZrlCLI.hpp>
#include <zlib/ZrlHistory.hpp>
#include <zlib/ZrlGlobber.hpp>

int main()
{
  Zrl::Globber globber;
  Zrl::History history{100};
  Zrl::CLI cli;
  cli.init(Zrl::App{
    .error = [](ZuString s) { std::cerr << s << '\n'; },
    .prompt = [](ZtArray<uint8_t> &s) { if (!s) s = "-->] "; },
    .enter = [](ZuString s) -> bool {
      std::cout << s << '\n';
      return s == "quit";
    },
    .sig = [](int sig) -> bool {
      switch (sig) {
	case SIGINT: std::cout << "SIGINT\n"; break;
	case SIGQUIT: std::cout << "SIGQUIT\n"; break;
	case SIGTSTP: std::cout << "SIGTSTP\n"; break;
      }
      return false;
    },
    .compInit = globber.initFn(),
    .compNext = globber.nextFn(),
    .histSave = history.saveFn(),
    .histLoad = history.loadFn()
  });
  if (!cli.open()) {
    std::cerr << "failed to open terminal\n";
    ::exit(1);
  }
  std::cout << cli.dumpVKeys();
  std::cout << cli.dumpMaps();
  cli.join();
  cli.stop();
  cli.close();
  cli.final();
}
