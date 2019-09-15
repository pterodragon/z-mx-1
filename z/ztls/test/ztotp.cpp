#include <iostream>

#include <ZuBox.hpp>

#include <ZtlsBase32.hpp>
#include <ZtlsTOTP.hpp>


static void usage() {
  std::cout
    << "usage: ztotp BASE32\n\n"
    << "app URI is otpauth://totp/ID@DOMAIN?secret=BASE32&issuer=ISSUER\n"
    << std::flush;
  ::exit(1);
}

int main(int argc, char **argv)
{
  char secret[20];
  if (argc != 2) usage();
  int n = strlen(argv[1]);
  if (n > 32) usage();
  auto code =
    Ztls::TOTP::google(secret, Ztls::Base32::decode(secret, 20, argv[1], n));
  std::cout << ZuBoxed(code).fmt(ZuFmt::Right<6>()) << '\n';
}
