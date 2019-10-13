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

#include <stdio.h>
#include <stdlib.h>

#include <zlib/ZuPolymorph.hpp>
#include <zlib/ZuByteSwap.hpp>

#include <zlib/ZmPlatform.hpp>
#include <zlib/ZmTrap.hpp>

#include <zlib/ZiMultiplex.hpp>
#include <zlib/ZiModule.hpp>

#include <zlib/ZvCf.hpp>
#include <zlib/ZvCmdClient.hpp>
#include <zlib/ZvCmdHost.hpp>
#include <zlib/ZvMultiplexCf.hpp>

#include <zlib/ZtlsBase32.hpp>
#include <zlib/ZtlsBase64.hpp>

#include <zlib/Zrl.hpp>
#include <zlib/ZCmd.hpp>

#ifdef _WIN32
#include <io.h>		// for _isatty
#ifndef isatty
#define isatty _isatty
#endif
#ifndef fileno
#define fileno _fileno
#endif
#else
#include <termios.h>
#include <unistd.h>	// for isatty
#include <fcntl.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable:4800)
#endif

static void usage()
{
  static const char *usage =
    "usage: zcmd USER@[HOST:]PORT [CMD [ARGS]]\n"
    "\tUSER\t- user\n"
    "\tHOST\t- target host (default localhost)\n"
    "\tPORT\t- target port\n"
    "\tCMD\t- command to send to target\n"
    "\t\t  (reads commands from standard input if none specified)\n"
    "\tARGS\t- command arguments\n\n"
    "Environment Variables:\n"
    "\tZCMD_KEY_ID\tAPI key ID\n"
    "\tZCMD_KEY_SECRET\tAPI key secret\n"
    "\tZCMD_PLUGIN\tzcmd plugin module\n";
  std::cerr << usage << std::flush;
  ZeLog::stop();
  ZmPlatform::exit(1);
}

static ZtString getpass_(const ZtString &prompt, unsigned passLen)
{
  ZtString passwd;
  passwd.size(passLen + 4); // allow for \r\n and null terminator
#ifdef _WIN32
  HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
  DWORD omode, nmode;
  GetConsoleMode(h, &omode);
  nmode = (omode | ENABLE_LINE_INPUT) & ~ENABLE_ECHO_INPUT;
  SetConsoleMode(h, nmode);
  DWORD n = 0;
  WriteConsole(h, prompt.data(), prompt.length(), &n, nullptr);
  n = 0;
  ReadConsole(h, passwd.data(), passwd.size() - 1, &n, nullptr);
  if (n < passwd.size()) passwd.length(n);
  WriteConsole(h, "\r\n", 2, &n, nullptr);
  mode |= ENABLE_ECHO_INPUT;
  SetConsoleMode(h, omode);
#else
  int fd = ::open("/dev/tty", O_RDWR, 0);
  if (fd < 0) return passwd;
  struct termios oflags, nflags;
  memset(&oflags, 0, sizeof(termios));
  tcgetattr(fd, &oflags);
  memcpy(&nflags, &oflags, sizeof(termios));
  nflags.c_lflag = (nflags.c_lflag & ~ECHO) | ECHONL;
  if (tcsetattr(fd, TCSANOW, &nflags)) return passwd;
  ::write(fd, prompt.data(), prompt.length());
  FILE *in = fdopen(fd, "r");
  if (!fgets(passwd, passwd.size() - 1, in)) return passwd;
  passwd[passwd.size() - 1] = 0;
  passwd.calcLength();
  tcsetattr(fd, TCSANOW, &oflags);
  fclose(in);
  ::close(fd);
#endif
  passwd.chomp();
  return passwd;
}

class ZCmd : public ZmPolymorph, public ZvCmdClient<ZCmd>, public ZCmdHost {
public:
  using Base = ZvCmdClient<ZCmd>;

  struct Link : public ZvCmdCliLink<ZCmd, Link> {
  using Base = ZvCmdCliLink<ZCmd, Link>;
    Link(ZCmd *app) : Base(app) { }
    void loggedIn() {
      this->app()->loggedIn();
    }
    void disconnected() {
      this->app()->disconnected();
      Base::disconnected();
    }
    void connectFailed(bool transient) {
      this->app()->connectFailed();
      // Base::connectFailed(transient);
    }
    int processApp(ZuArray<const uint8_t> data) {
      return this->app()->processApp(data);
    }
  };

  void init(ZiMultiplex *mx, ZvCf *cf, bool interactive) {
    Base::init(mx, cf);
    m_interactive = interactive;
    ZvCmdHost::init();
    initCmds();
  }
  void final() {
    m_link = nullptr;
    ZvCmdHost::final();
    Base::final();
  }

  bool interactive() const { return m_interactive; }

  void solo(ZtString s) {
    m_solo = true;
    m_soloMsg = ZuMv(s);
  }
  void target(ZuString s) {
    m_prompt.null();
    m_prompt << s << "] ";
  }

  template <typename ...Args>
  void login(Args &&... args) {
    m_link = new Link(this);
    m_link->login(ZuFwd<Args>(args)...);
  }
  template <typename ...Args>
  void access(Args &&... args) {
    m_link = new Link(this);
    m_link->access(ZuFwd<Args>(args)...);
  }

  void disconnect() { m_link->disconnect(); }

  void wait() { m_done.wait(); }
  void post() { m_done.post(); }

  int code() const { return m_code; }

  void exiting() { m_exiting = true; }

  void sendApp(Zfb::IOBuilder &fbb) { return m_link->sendApp(fbb); }

private:
  void loggedIn() {
    if (auto plugin = ::getenv("ZCMD_PLUGIN")) {
      ZvCf *args = new ZvCf();
      args->set("1", plugin);
      args->set("#", "2");
      ZtString out;
      loadModCmd(stdout, args, out);
      fwrite(out.data(), 1, out.length(), stdout);
    }
    if (m_solo) {
      send(ZuMv(m_soloMsg));
    } else {
      if (m_interactive)
	std::cout <<
	  "For a list of valid commands: help\n"
	  "For help on a particular command: COMMAND --help\n" << std::flush;
      prompt();
    }
  }

  int processApp(ZuArray<const uint8_t> data) {
    if (!processAppFn) return -1;
    return processAppFn(data);
  }

  void disconnected() {
    if (m_exiting) return;
    if (m_interactive) {
      Zrl::stop();
      std::cerr << "server disconnected\n" << std::flush;
    }
    ZmPlatform::exit(1);
  }
  void connectFailed() {
    if (m_interactive) {
      Zrl::stop();
      std::cerr << "connect failed\n" << std::flush;
    }
    ZmPlatform::exit(1);
  }

  void prompt() {
    mx()->add(ZmFn<>{this, [](ZCmd *app) { app->prompt_(); }});
  }
  void prompt_() {
    ZtString cmd;
next:
    if (m_interactive) {
      try {
	cmd = Zrl::readline_(m_prompt);
      } catch (const Zrl::EndOfFile &) {
	post();
	return;
      }
    } else {
      cmd.size(4096);
      if (!fgets(cmd, cmd.size() - 1, stdin)) {
	post();
	return;
      }
      cmd[cmd.size() - 1] = 0;
      cmd.calcLength();
      cmd.chomp();
    }
    if (!cmd) goto next;
    send(cmd);
  }

  void send(ZtString cmd) {
    FILE *file = stdout;
    ZtString cmd_ = ZuMv(cmd);
    {
      ZtRegex::Captures c;
      const auto &append = ZtStaticRegexUTF8("\\s*>>\\s*");
      const auto &output = ZtStaticRegexUTF8("\\s*>\\s*");
      unsigned pos = 0, n = 0;
      if (n = append.m(cmd, c, pos)) {
	if (!(file = fopen(c[2], "a"))) {
	  logError(ZtString{c[2]}, ": ", ZeLastError);
	  return;
	}
	cmd = c[0];
      } else if (n = output.m(cmd, c, pos)) {
	if (!(file = fopen(c[2], "w"))) {
	  logError(ZtString{c[2]}, ": ", ZeLastError);
	  return;
	}
	cmd = c[0];
      } else {
	cmd = ZuMv(cmd_);
      }
    }
    ZtArray<ZtString> args;
    ZvCf::parseCLI(cmd, args);
    if (args[0] == "help") {
      if (args.length() == 1) {
	ZtString out;
	out << "Local ";
	processCmd(file, args, out);
	out << "\nRemote ";
	fwrite(out.data(), 1, out.length(), file);
      } else if (args.length() == 2 && hasCmd(args[1])) {
	ZtString out;
	int code = processCmd(file, args, out);
	if (code || out) executed(code, file, out);
	return;
      }
    } else if (hasCmd(args[0])) {
      ZtString out;
      int code = processCmd(file, args, out);
      if (code || out) executed(code, file, out);
      return;
    }
    auto seqNo = m_seqNo++;
    m_fbb.Clear();
    m_fbb.Finish(ZvCmd::fbs::CreateRequest(m_fbb, seqNo,
	  Zfb::Save::strVecIter(m_fbb, args.length(),
	    [&args](unsigned i) { return args[i]; })));
    m_link->sendCmd(m_fbb, seqNo, [this, file](const ZvCmd::fbs::ReqAck *ack) {
      using namespace Zfb::Load;
      executed(ack->code(), file, str(ack->out()));
    });
  }

  int executed(int code, FILE *file, ZuString out) {
    m_code = code;
    if (out) fwrite(out.data(), 1, out.length(), file);
    if (file != stdout) fclose(file);
    if (m_solo)
      post();
    else
      prompt();
    return code;
  }

private:
  // built-in commands

  int filterAck(
      FILE *file, const ZvUserDB::fbs::ReqAck *ack,
      int ackType1, int ackType2,
      const char *op, ZtString &out) {
    using namespace ZvUserDB;
    if (ack->rejCode()) {
      out << '[' << ZuBox<unsigned>(ack->rejCode()) << "] "
	<< Zfb::Load::str(ack->rejText()) << '\n';
      return 1;
    }
    auto ackType = ack->data_type();
    if ((int)ackType != ackType1 &&
	ackType2 >= fbs::ReqAckData_MIN && (int)ackType != ackType2) {
      logError("mismatched ack from server: ",
	  fbs::EnumNameReqAckData(ackType));
      out << op << " failed\n";
      return 1;
    }
    return 0;
  }

  void initCmds() {
    addCmd("passwd", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->passwdCmd(static_cast<FILE *>(file), args, out);
	}},  "change passwd", "usage: passwd");

    addCmd("users", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->usersCmd(static_cast<FILE *>(file), args, out);
	}},  "list users", "usage: users");
    addCmd("useradd",
	"e enabled enabled { type flag } "
	"i immutable immutable { type flag }",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->userAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add user",
	"usage: useradd ID NAME ROLE[,ROLE,...] [OPTIONS...]\n\n"
	"Options:\n"
	"  -e, --enabled\t\tset Enabled flag\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("resetpass", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->resetPassCmd(static_cast<FILE *>(file), args, out);
	}},  "reset password", "usage: resetpass USERID");
    addCmd("usermod",
	"e enabled enabled { type flag } "
	"i immutable immutable { type flag }",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->userModCmd(static_cast<FILE *>(file), args, out);
	}},  "modify user",
	"usage: usermod ID NAME ROLE[,ROLE,...] [OPTIONS...]\n\n"
	"Options:\n"
	"  -e, --enabled\t\tset Enabled flag\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("userdel", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->userDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete user", "usage: userdel ID");

    addCmd("roles", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->rolesCmd(static_cast<FILE *>(file), args, out);
	}},  "list roles", "usage: roles");
    addCmd("roleadd", "i immutable immutable { type flag }",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->roleAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add role",
	"usage: roleadd NAME PERMS APIPERMS [OPTIONS...]\n\n"
	"Options:\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("rolemod", "i immutable immutable { type scalar }",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->roleModCmd(static_cast<FILE *>(file), args, out);
	}},  "modify role",
	"usage: rolemod NAME PERMS APIPERMS [OPTIONS...]\n\n"
	"Options:\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("roledel", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->roleDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete role",
	"usage: roledel NAME");

    addCmd("perms", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permsCmd(static_cast<FILE *>(file), args, out);
	}},  "list permissions", "usage: perms");
    addCmd("permadd", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add permission", "usage: permadd NAME");
    addCmd("permmod", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permModCmd(static_cast<FILE *>(file), args, out);
	}},  "modify permission", "usage: permmod ID NAME");
    addCmd("permdel", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete permission", "usage: permdel ID");

    addCmd("keys", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keysCmd(static_cast<FILE *>(file), args, out);
	}},  "list keys", "usage: keys [USERID]");
    addCmd("keyadd", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keyAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add key", "usage: keyadd [USERID]");
    addCmd("keydel", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keyDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete key", "usage: keydel ID");
    addCmd("keyclr", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keyClrCmd(static_cast<FILE *>(file), args, out);
	}},  "clear all keys", "usage: keyclr [USERID]");

    addCmd("loadmod", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->loadModCmd(static_cast<FILE *>(file), args, out);
	}},  "load application-specific module", "usage: loadmod MODULE");
  }

  int passwdCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 1) throw ZvCmdUsage();
    ZtString oldpw = getpass_("Current password: ", 100);
    ZtString newpw = getpass_("New password: ", 100);
    ZtString checkpw = getpass_("Retype new password: ", 100);
    if (checkpw != newpw) {
      out << "passwords do not match\npassword unchanged!\n";
      return 1;
    }
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_ChPass,
	    fbs::CreateUserChPass(m_fbb,
	      str(m_fbb, oldpw),
	      str(m_fbb, newpw)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_ChPass, -1, "password change", out))
	return executed(code, file, out);
      auto userAck = static_cast<const fbs::UserAck *>(ack->data());
      if (!userAck->ok()) {
	out << "password change rejected\n";
	return executed(1, file, out);
      }
      out << "password changed\n";
      return executed(0, file, out);
    });
    return 0;
  }

  static void printUser(ZtString &out, const ZvUserDB::fbs::User *user_) {
    using namespace ZvUserDB;
    using namespace Zfb::Load;
    auto hmac_ = bytes(user_->hmac());
    ZtString hmac;
    hmac.length(Ztls::Base64::enclen(hmac_.length()));
    Ztls::Base64::encode(
	hmac.data(), hmac.length(), hmac_.data(), hmac_.length());
    auto secret_ = bytes(user_->secret());
    ZtString secret;
    secret.length(Ztls::Base32::enclen(secret_.length()));
    Ztls::Base32::encode(
	secret.data(), secret.length(), secret_.data(), secret_.length());
    out << user_->id() << ' ' << str(user_->name()) << " roles=[";
    all(user_->roles(), [&out](unsigned i, auto role_) {
      if (i) out << ',';
      out << str(role_);
    });
    out << "] hmac=" << hmac << " secret=" << secret << " flags=";
    bool pipe = false;
    if (user_->flags() & User::Enabled) {
      out << "Enabled";
      pipe = true;
    }
    if (user_->flags() & User::Immutable) {
      if (pipe) out << '|';
      out << "Immutable";
      pipe = true;
    }
    if (user_->flags() & User::ChPass) {
      if (pipe) out << '|';
      out << "ChPass";
      pipe = true;
    }
  }

  int usersCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      fbs::UserIDBuilder fbb_(m_fbb);
      if (argc == 2) fbb_.add_id(args->getInt64("1", 0, LLONG_MAX, true));
      auto userID = fbb_.Finish();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_UserGet, userID.Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_UserGet, -1,
	    "user get", out))
	return executed(code, file, out);
      auto userList = static_cast<const fbs::UserList *>(ack->data());
      using namespace Zfb::Load;
      all(userList->list(), [&out](unsigned, auto user_) {
	printUser(out, user_); out << '\n';
      });
      return executed(0, file, out);
    });
    return 0;
  }
  int userAddCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 4) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      uint8_t flags = 0;
      if (args->get("enabled")) flags |= User::Enabled;
      if (args->get("immutable")) flags |= User::Immutable;
      const auto &comma = ZtStaticRegexUTF8(",");
      ZtRegex::Captures roles;
      comma.split(args->get("3"), roles);
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_UserAdd,
	    fbs::CreateUser(m_fbb,
	      args->getInt64("1", 0, LLONG_MAX, true),
	      str(m_fbb, args->get("2")), 0, 0,
	      strVecIter(m_fbb, roles.length(),
		[&roles](unsigned i) { return roles[i]; }),
	      flags).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_UserAdd, -1,
	    "user add", out))
	return executed(code, file, out);
      auto userPass = static_cast<const fbs::UserPass *>(ack->data());
      if (!userPass->ok()) {
	out << "user add rejected\n";
	return executed(1, file, out);
      }
      printUser(out, userPass->user()); out << '\n';
      using namespace Zfb::Load;
      out << "passwd=" << str(userPass->passwd()) << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int resetPassCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_ResetPass,
	    fbs::CreateUserID(m_fbb,
	      args->getInt64("1", 0, LLONG_MAX, true)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_ResetPass, -1,
	    "reset password", out))
	return executed(code, file, out);
      auto userPass = static_cast<const fbs::UserPass *>(ack->data());
      if (!userPass->ok()) {
	out << "reset password rejected\n";
	return executed(1, file, out);
      }
      printUser(out, userPass->user()); out << '\n';
      using namespace Zfb::Load;
      out << "passwd=" << str(userPass->passwd()) << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int userModCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 4) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      uint8_t flags = 0;
      if (args->get("enabled")) flags |= User::Enabled;
      if (args->get("immutable")) flags |= User::Immutable;
      const auto &comma = ZtStaticRegexUTF8(",");
      ZtRegex::Captures roles;
      comma.split(args->get("3"), roles);
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_UserMod,
	    fbs::CreateUser(m_fbb,
	      args->getInt64("1", 0, LLONG_MAX, true),
	      str(m_fbb, args->get("2")), 0, 0,
	      strVecIter(m_fbb, roles.length(),
		[&roles](unsigned i) { return roles[i]; }),
	      flags).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_UserMod, -1,
	    "user modify", out))
	return executed(code, file, out);
      auto userUpdAck = static_cast<const fbs::UserUpdAck *>(ack->data());
      if (!userUpdAck->ok()) {
	out << "user modify rejected\n";
	return executed(1, file, out);
      }
      printUser(out, userUpdAck->user()); out << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int userDelCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_UserDel,
	    fbs::CreateUserID(m_fbb,
	      args->getInt64("1", 0, LLONG_MAX, true)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_UserDel, -1,
	    "user delete", out))
	return executed(code, file, out);
      auto userUpdAck = static_cast<const fbs::UserUpdAck *>(ack->data());
      if (!userUpdAck->ok()) {
	out << "user delete rejected\n";
	return executed(1, file, out);
      }
      printUser(out, userUpdAck->user()); out << '\n';
      out << "user deleted\n";
      return executed(0, file, out);
    });
    return 0;
  }

  static void printRole(ZtString &out, const ZvUserDB::fbs::Role *role_) {
    using namespace ZvUserDB;
    using namespace Zfb::Load;
    Bitmap perms, apiperms;
    all(role_->perms(), [&perms](unsigned i, uint64_t w) {
      if (ZuLikely(i < Bitmap::Words)) perms.data[i] = w;
    });
    all(role_->apiperms(), [&apiperms](unsigned i, uint64_t w) {
      if (ZuLikely(i < Bitmap::Words)) apiperms.data[i] = w;
    });
    out << str(role_->name())
      << " perms=[" << perms
      << "] apiperms=[" << apiperms
      << "] flags=";
    if (role_->flags() & Role::Immutable) out << "Immutable";
  }

  int rolesCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      Zfb::Offset<Zfb::String> name_;
      if (argc == 2) name_ = str(m_fbb, args->get("1"));
      fbs::RoleIDBuilder fbb_(m_fbb);
      if (argc == 2) fbb_.add_name(name_);
      auto roleID = fbb_.Finish();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_RoleGet, roleID.Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_RoleGet, -1,
	    "role get", out))
	return executed(code, file, out);
      auto roleList = static_cast<const fbs::RoleList *>(ack->data());
      using namespace Zfb::Load;
      all(roleList->list(), [&out](unsigned, auto role_) {
	printRole(out, role_); out << '\n';
      });
      return executed(0, file, out);
    });
    return 0;
  }
  int roleAddCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 4) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      Bitmap perms{args->get("2")};
      Bitmap apiperms{args->get("3")};
      uint8_t flags = 0;
      if (args->get("immutable")) flags |= Role::Immutable;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_RoleAdd,
	    fbs::CreateRole(m_fbb,
	      str(m_fbb, args->get("1")),
	      m_fbb.CreateVector(perms.data, Bitmap::Words),
	      m_fbb.CreateVector(apiperms.data, Bitmap::Words),
	      flags).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_RoleAdd, -1,
	    "role add", out))
	return executed(code, file, out);
      auto roleUpdAck = static_cast<const fbs::RoleUpdAck *>(ack->data());
      if (!roleUpdAck->ok()) {
	out << "role add rejected\n";
	return executed(1, file, out);
      }
      printRole(out, roleUpdAck->role()); out << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int roleModCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 4) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      Bitmap perms{args->get("2")};
      Bitmap apiperms{args->get("3")};
      uint8_t flags = 0;
      if (args->get("immutable")) flags |= Role::Immutable;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_RoleMod,
	    fbs::CreateRole(m_fbb,
	      str(m_fbb, args->get("1")),
	      m_fbb.CreateVector(perms.data, Bitmap::Words),
	      m_fbb.CreateVector(apiperms.data, Bitmap::Words),
	      flags).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_RoleMod, -1,
	    "role modify", out))
	return executed(code, file, out);
      auto roleUpdAck = static_cast<const fbs::RoleUpdAck *>(ack->data());
      if (!roleUpdAck->ok()) {
	out << "role modify rejected\n";
	return executed(1, file, out);
      }
      printRole(out, roleUpdAck->role()); out << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int roleDelCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_RoleDel,
	    fbs::CreateRoleID(m_fbb, str(m_fbb, args->get("1"))).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_RoleMod, -1,
	    "role delete", out))
	return executed(code, file, out);
      auto roleUpdAck = static_cast<const fbs::RoleUpdAck *>(ack->data());
      if (!roleUpdAck->ok()) {
	out << "role delete rejected\n";
	return executed(1, file, out);
      }
      printRole(out, roleUpdAck->role()); out << '\n';
      out << "role deleted\n";
      return executed(0, file, out);
    });
    return 0;
  }

  int permsCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      fbs::PermIDBuilder fbb_(m_fbb);
      if (argc == 2) fbb_.add_id(args->getInt("1", 0, Bitmap::Bits, true));
      auto permID = fbb_.Finish();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_PermGet, permID.Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_PermGet, -1,
	    "perm get", out))
	return executed(code, file, out);
      auto permList = static_cast<const fbs::PermList *>(ack->data());
      using namespace Zfb::Load;
      all(permList->list(), [&out](unsigned, auto perm_) {
	out << ZuBoxed(perm_->id()).fmt(ZuFmt::Right<3, ' '>()) << ' '
	  << str(perm_->name()) << '\n';
      });
      return executed(0, file, out);
    });
    return 0;
  }
  int permAddCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      auto name = args->get("1");
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_PermAdd,
	    fbs::CreatePermAdd(m_fbb, str(m_fbb, name)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_PermAdd, -1,
	    "permission add", out))
	return executed(code, file, out);
      using namespace Zfb::Load;
      auto permUpdAck = static_cast<const fbs::PermUpdAck *>(ack->data());
      if (!permUpdAck->ok()) {
	out << "permission add rejected\n";
	return executed(1, file, out);
      }
      auto perm = permUpdAck->perm();
      out << "added " << perm->id() << ' ' << str(perm->name()) << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int permModCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 3) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      auto permID = args->getInt("1", 0, Bitmap::Bits, true);
      auto permName = args->get("2");
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_PermMod,
	    fbs::CreatePerm(m_fbb, permID, str(m_fbb, permName)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_PermMod, -1,
	    "permission modify", out))
	return executed(code, file, out);
      using namespace Zfb::Load;
      auto permUpdAck = static_cast<const fbs::PermUpdAck *>(ack->data());
      if (!permUpdAck->ok()) {
	out << "permission modify rejected\n";
	return executed(1, file, out);
      }
      auto perm = permUpdAck->perm();
      out << "modified " << perm->id() << ' ' << str(perm->name()) << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int permDelCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      auto permID = args->getInt("1", 0, Bitmap::Bits, true);
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_PermDel,
	    fbs::CreatePermID(m_fbb, permID).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_PermDel, -1,
	    "permission delete", out))
	return executed(code, file, out);
      using namespace Zfb::Load;
      auto permUpdAck = static_cast<const fbs::PermUpdAck *>(ack->data());
      if (!permUpdAck->ok()) {
	out << "permission delete rejected\n";
	return executed(1, file, out);
      }
      auto perm = permUpdAck->perm();
      out << "deleted " << perm->id() << ' ' << str(perm->name()) << '\n';
      return executed(0, file, out);
    });
    return 0;
  }

  int keysCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      if (argc == 1)
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_OwnKeyGet,
	      fbs::CreateUserID(m_fbb, m_link->userID()).Union()));
      else {
	auto userID = ZuBox<uint64_t>(args->get("1"));
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_KeyGet,
	      fbs::CreateUserID(m_fbb, userID).Union()));
      }
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_OwnKeyGet, fbs::ReqAckData_KeyGet,
	    "key get", out))
	return executed(code, file, out);
      auto keyIDList = static_cast<const fbs::KeyIDList *>(ack->data());
      using namespace Zfb::Load;
      all(keyIDList->list(), [&out](unsigned, auto key_) {
	out << str(key_) << '\n';
      });
      return executed(0, file, out);
    });
    return 0;
  }

  int keyAddCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      if (argc == 1)
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_OwnKeyAdd,
	      fbs::CreateUserID(m_fbb, m_link->userID()).Union()));
      else {
	auto userID = ZuBox<uint64_t>(args->get("1"));
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_KeyAdd,
	      fbs::CreateUserID(m_fbb, userID).Union()));
      }
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_OwnKeyAdd, fbs::ReqAckData_KeyAdd,
	    "key add", out))
	return executed(code, file, out);
      using namespace Zfb::Load;
      auto keyUpdAck = static_cast<const fbs::KeyUpdAck *>(ack->data());
      if (!keyUpdAck->ok()) {
	out << "key add rejected\n";
	return executed(1, file, out);
      }
      auto secret_ = bytes(keyUpdAck->key()->secret());
      ZtString secret;
      secret.length(Ztls::Base64::enclen(secret_.length()));
      Ztls::Base64::encode(
	  secret.data(), secret.length(), secret_.data(), secret_.length());
      out << "id: " << str(keyUpdAck->key()->id())
	<< "\nsecret: " << secret << '\n';
      return executed(0, file, out);
    });
    return 0;
  }

  int keyDelCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      auto keyID = args->get("1");
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_KeyDel,
	    fbs::CreateKeyID(m_fbb, str(m_fbb, keyID)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_OwnKeyDel, fbs::ReqAckData_KeyDel,
	    "key delete", out))
	return executed(code, file, out);
      using namespace Zfb::Load;
      auto userAck = static_cast<const fbs::UserAck *>(ack->data());
      if (!userAck->ok()) {
	out << "key delete rejected\n";
	return executed(1, file, out);
      }
      out << "key deleted\n";
      return executed(0, file, out);
    });
    return 0;
  }

  int keyClrCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage();
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      if (argc == 1)
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_OwnKeyClr,
	      fbs::CreateUserID(m_fbb, m_link->userID()).Union()));
      else {
	auto userID = ZuBox<uint64_t>(args->get("1"));
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_KeyClr,
	      fbs::CreateUserID(m_fbb, userID).Union()));
      }
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_OwnKeyGet, fbs::ReqAckData_KeyGet,
	    "key clear", out))
	return executed(code, file, out);
      auto userAck = static_cast<const fbs::UserAck *>(ack->data());
      if (!userAck->ok()) {
	out << "key clear rejected\n";
	return executed(1, file, out);
      }
      out << "keys cleared\n";
      return executed(0, file, out);
    });
    return 0;
  }

  int loadModCmd(FILE *, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage();
    ZiModule module;
    ZiModule::Path name = args->get("1", true);
    ZtString e;
    if (module.load(name, false, &e) < 0) {
      out << "failed to load \"" << name << "\": " << ZuMv(e) << '\n';
      return 1;
    }
    ZCmdInitFn initFn = (ZCmdInitFn)module.resolve("ZCmd_plugin", &e);
    if (!initFn) {
      module.unload();
      out << "failed to resolve \"ZCmd_plugin\" in \"" <<
	name << "\": " << ZuMv(e) << '\n';
      return 1;
    }
    (*initFn)(static_cast<ZCmdHost *>(this));
    out << "module \"" << name << "\" loaded\n";
    return 0;
  }

private:
  ZmSemaphore		m_done;
  ZtString		m_prompt;
  ZtString		m_soloMsg;
  ZmRef<Link>		m_link;
  ZvCmdSeqNo		m_seqNo = 0;
  Zfb::IOBuilder	m_fbb;
  int			m_code = 0;
  bool			m_interactive = true;
  bool			m_solo = false;
  bool			m_exiting = false;
};

int main(int argc, char **argv)
{
  if (argc < 2) usage();

  ZeLog::init("zcmd");
  ZeLog::level(0);
  ZeLog::sink(ZeLog::lambdaSink(
	[](ZeEvent *e) { std::cerr << e->message() << '\n' << std::flush; }));
  ZeLog::start();

  bool interactive = isatty(fileno(stdin));
  auto keyID = ::getenv("ZCMD_KEY_ID");
  auto secret = ::getenv("ZCMD_KEY_SECRET");
  ZtString user, passwd, server;
  ZuBox<unsigned> totp;
  ZuBox<unsigned> port;

  try {
    {
      const auto &remote = ZtStaticRegexUTF8("^([^@]+)@([^:]+):(\\d+)$");
      ZtRegex::Captures c;
      if (remote.m(argv[1], c) == 4) {
	user = c[2];
	server = c[3];
	port = c[4];
      }
    }
    if (!user) {
      const auto &local = ZtStaticRegexUTF8("^([^@]+)@(\\d+)$");
      ZtRegex::Captures c;
      if (local.m(argv[1], c) == 3) {
	user = c[2];
	server = "localhost";
	port = c[3];
      }
    }
    if (!user) {
      const auto &remote = ZtStaticRegexUTF8("^([^:]+):(\\d+)$");
      ZtRegex::Captures c;
      if (remote.m(argv[1], c) == 3) {
	server = c[2];
	port = c[3];
      }
    }
    if (!server) {
      const auto &local = ZtStaticRegexUTF8("^(\\d+)$");
      ZtRegex::Captures c;
      if (local.m(argv[1], c) == 2) {
	server = "localhost";
	port = c[2];
      }
    }
  } catch (const ZtRegex::Error &) {
    usage();
  }
  if (!server || !*port || !port) usage();
  if (user)
    keyID = secret = nullptr;
  else if (!keyID) {
    std::cerr << "set ZCMD_KEY_ID and ZCMD_KEY_SECRET "
      "to use without username\n" << std::flush;
    ::exit(1);
  }
  if (keyID) {
    if (!secret) {
      std::cerr << "set ZCMD_KEY_SECRET "
	"to use with ZCMD_KEY_ID\n" << std::flush;
      ::exit(1);
    }
  } else {
    if (!interactive || argc > 2) {
      std::cerr << "set ZCMD_KEY_ID and ZCMD_KEY_SECRET "
	"to use non-interactively\n" << std::flush;
      ::exit(1);
    }
    passwd = getpass_("password: ", 100);
    totp = getpass_("totp: ", 6);
  }

  ZiMultiplex *mx = new ZiMultiplex(
      ZiMxParams()
	.scheduler([](auto &s) {
	  s.nThreads(4)
	  .thread(1, [](auto &t) { t.isolated(1); })
	  .thread(2, [](auto &t) { t.isolated(1); })
	  .thread(3, [](auto &t) { t.isolated(1); }); })
	.rxThread(1).txThread(2));

  mx->start();

  ZmRef<ZCmd> client = new ZCmd();

  ZmTrap::sigintFn(ZmFn<>{client, [](ZCmd *client) { client->post(); }});
  ZmTrap::trap();

  {
    ZmRef<ZvCf> cf = new ZvCf();
    cf->set("thread", "3");
    if (auto caPath = ::getenv("ZCMD_CAPATH"))
      cf->set("caPath", caPath);
    else
      cf->set("caPath", "/etc/ssl/certs");
    try {
      client->init(mx, cf, interactive);
    } catch (const ZvError &e) {
      std::cerr << e << '\n' << std::flush;
      ::exit(1);
    } catch (const ZtString &e) {
      std::cerr << e << '\n' << std::flush;
      ::exit(1);
    } catch (...) {
      std::cerr << "unknown exception\n" << std::flush;
      ::exit(1);
    }
  }

  if (argc > 2) {
    ZtString solo;
    for (int i = 2; i < argc; i++) {
      solo << argv[i];
      if (ZuLikely(i < argc - 1)) solo << ' ';
    }
    client->solo(ZuMv(solo));
  } else
    client->target(argv[1]);

  if (keyID)
    client->access(server, port, keyID, secret);
  else
    client->login(server, port, user, passwd, totp);

  client->wait();

  if (client->interactive()) {
    Zrl::stop();
    std::cout << std::flush;
  }

  client->exiting();
  client->disconnect();

  mx->stop(true);

  ZeLog::stop();

  client->final();

  return client->code();
}
