#!/bin/sh
BFDDIR="$MINGW_PREFIX"/lib/binutils
[ ! -d "$BFDDIR" ] && pacman -S mingw-w64-x86_64-binutils
[ ! -f "$BFDDIR"/libbfd.a ] && {
  echo "$BFDDIR""/libbfd.a does not exist!";
  exit 1;
}
cd $BFDDIR
dlltool --export-all-symbols -e libbfd.o -l libbfd.dll.a -D libbfd.dll libbfd.a
gcc -shared -o libbfd.dll libbfd.o libbfd.a -L . -liberty -lintl -lz
rm libbfd.o
echo "built/installed $BFDDIR/libbfd.dll"
