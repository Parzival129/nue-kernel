#!/bin/sh
set -e
. ./config.sh

mkdir -p "$SYSROOT"

# Install Newlib headers and libraries into the sysroot
NEWLIB_BUILD="$HOME/Dev/newlib-build"
if [ -d "$NEWLIB_BUILD" ]; then
  make -C "$NEWLIB_BUILD" DESTDIR="$SYSROOT" install-target-newlib
fi

for PROJECT in $SYSTEM_HEADER_PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install-headers)
done
