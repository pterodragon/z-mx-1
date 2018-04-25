#ifndef RUMP_HPP
#define RUMP_HPP

#include <stdint.h>
#include <errno.h>

#include <sys/types.h>

extern "C" {
#include <rump/rump.h>
#include <rump/rump_syscalls.h>
#include <rump/netconfig.h>
};

extern int rump_errno(int e);

#endif /* RUMP_HPP */
