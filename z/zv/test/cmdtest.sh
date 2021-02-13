#!/bin/sh
libtool exec valgrind ./CmdTest cmdtest.crt cmdtest.key user.db 127.0.0.1 9000
