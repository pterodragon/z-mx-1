//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <iostream>

#include <ZtRegex.hpp>
#include <ZtArray.hpp>

#include <ZTLS.hpp>

const char *Content =
  "<html><head>\n"
  "<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\n"
  "<title>200 OK</title>\n"
  "</head><body>\n"
  "<h1>OK</h1>\n"
  "Test document\n"
  "</body></html>";

const char *Response =
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "Content-Length: ";
const char *Response2 = "\r\n"
  "Accept: */*\r\n"
  "\r\n";

struct App : public ZTLS::Server<App> {
  struct Link : public ZmPolymorph, public ZTLS::SrvLink<App, Link> {
    inline Link(App *app) : ZTLS::SrvLink<App, Link>(app) { }

    void connected(const char *hostname, const char *alpn) {
      if (!hostname) hostname = "(null)";
      std::cerr << (ZuStringN<100>()
	  << "TLS handshake completed (hostname: " << hostname
	  << " ALPN: " << alpn << ")\n")
	<< std::flush;
    }
    void disconnected() {
      std::cerr << "disconnected\n" << std::flush;
      app()->done();
    }

    int process(const char *data, unsigned len) {
      std::cout << ZuString{data, len} << std::flush;
      ZtString response;
      ZtString content = Content;
      response << Response << content.length() << Response2;
      send_(response.data(), response.length());
      send_(content.data(), content.length());
      return len;
    }
  };

  inline Link::TCP *accepted(const ZiCxnInfo &ci) {
    link = new Link(this);
    return new Link::TCP(link, ci);
  }

  void listenFailed(bool transient) {
    logError("listen() failed ", transient ? "(transient)" : "");
    done();
  }

  void done() { sem.post(); }

  ZmSemaphore	sem;
  ZmRef<Link>	link;
};

void usage()
{
  std::cerr << "usage: ZTLSServer server port cert key\n" << std::flush;
  ::exit(1);
}

int main(int argc, char **argv)
{
  if (argc != 5) usage();

  ZuString server = argv[1];
  unsigned port = ZuBox<unsigned>(argv[2]);

  if (!port) usage();

  ZeLog::init("ZTLSServer");
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

  if (!app.init(&mx, "3", "/etc/ssl/certs", alpn, argv[3], argv[4])) {
    std::cerr << "TLS server initialization failed\n" << std::flush;
    return 1;
  }

  app.listen(server, port);

  app.sem.wait();

  mx.stop(true);

  ZeLog::stop();

  return 0;
}
