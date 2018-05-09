#!/bin/sh
#libtool execute gdb --args
./ZdbTest -f orders2 -h 2 --electionTimeout=5 $*
