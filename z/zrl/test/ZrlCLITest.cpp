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
    .enter = [](ZuString s) -> bool {
      std::cout << s << '\n';
      return s == "quit";
    },
    .sig = [](int sig) {
      switch (sig) {
	case SIGINT: std::cout << "SIGINT\n"; break;
	case SIGQUIT: std::cout << "SIGQUIT\n"; break;
	case SIGTSTP: std::cout << "SIGTSTP\n"; break;
      }
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
  cli.start("-->] ");
  cli.join();
  cli.stop();
  cli.close();
  cli.final();
}
