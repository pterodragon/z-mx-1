//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <iostream>

#include <ZtRegex.hpp>
#include <ZtArray.hpp>

#include <ZTLS.hpp>

const char *Request =
  "GET / HTTP/1.1\r\n"
  "Host: ";
const char *Request2 = "\r\n"
  "User-Agent: ZTLSClient/1.0\r\n"
  "Accept: */*\r\n"
  "\r\n";

struct App : public ZTLS::Client<App> {
  struct Link : public ZTLS::CliLink<App, Link> {
    inline Link(App *app) : ZTLS::CliLink<App, Link>(app) { }

    void connected(const char *hostname, const char *alpn) {
      if (!hostname) hostname = "(null)";
      std::cerr << (ZuStringN<100>()
	  << "TLS handshake completed (hostname: " << hostname
	  << " ALPN: " << alpn << ")\n")
	<< std::flush;
      ZtString request;
      request << Request << hostname << Request2;
      send(request.data(), request.length());
    }
    void disconnected() {
      std::cerr << "disconnected\n" << std::flush;
      app()->done();
    }

    void connectFailed(bool transient) {
      if (transient)
	std::cerr << "failed to connect (transient)\n" << std::flush;
      else
	std::cerr << "failed to connect\n" << std::flush;
    }

    int process(const char *data, unsigned len) {
      std::cout << ZuString{data, len} << std::flush;
      // disconnect_();
      // return -1; // would return len to continue
      return len;
    }
  };

  void done() { sem.post(); }

  ZmSemaphore sem;
};

void usage()
{
  std::cerr << "usage: ZTLSClient server port\n" << std::flush;
  ::exit(1);
}

int main(int argc, char **argv)
{
  if (argc != 3) usage();

  ZuString server = argv[1];
  unsigned port = ZuBox<unsigned>(argv[2]);

  if (!port) usage();

  ZeLog::init("ZTLSClient");
  ZeLog::level(0);
  ZeLog::sink(ZeLog::fileSink("&2"));
  ZeLog::start();

  static const char *alpn[] = { "http/1.1", 0 };

  App app;

  ZiMultiplex mx(
      ZiMxParams()
	.scheduler([](auto &s) {
	  s.nThreads(4)
	  .thread(1, [](auto &t) { t.isolated(1); })
	  .thread(2, [](auto &t) { t.isolated(1); })
	  .thread(3, [](auto &t) { t.isolated(1); }); })
	.rxThread(1).txThread(2));

  if (mx.start() != Zi::OK) {
    std::cerr << "ZiMultiplex start failed\n" << std::flush;
    return 1;
  }

  if (!app.init(&mx, "3", "/etc/ssl/certs", alpn)) {
    std::cerr << "TLS client initialization failed\n" << std::flush;
    return 1;
  }

  App::Link link(&app);

  if (!link.connect(server, port)) {
    std::cerr << "failed to connect to "
      << server << ':' << port << '\n' << std::flush;
    return 1;
  }

  app.sem.wait();

  mx.stop(true);

  ZeLog::stop();

  return 0;
}
