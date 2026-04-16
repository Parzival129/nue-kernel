#!/bin/sh
set -e
. ./config.sh

mkdir -p "$SYSROOT"

# Install Newlib headers and libraries into the sysroot.
# In Docker, newlib is pre-installed under the cross-toolchain prefix.
# Locally, it may be built at $HOME/Dev/newlib-build.
NEWLIB_BUILD="${NEWLIB_BUILD:-$HOME/Dev/newlib-build}"
CROSS_NEWLIB="/opt/cross/$HOST"
if [ -d "$CROSS_NEWLIB/include" ]; then
  mkdir -p "$SYSROOT/usr/$HOST/include" "$SYSROOT/usr/$HOST/lib"
  cp -R "$CROSS_NEWLIB/include/." "$SYSROOT/usr/$HOST/include/."
  cp -R "$CROSS_NEWLIB/lib/." "$SYSROOT/usr/$HOST/lib/."
elif [ -d "$NEWLIB_BUILD" ]; then
  make -C "$NEWLIB_BUILD" DESTDIR="$SYSROOT" install-target-newlib
fi

for PROJECT in $SYSTEM_HEADER_PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install-headers)
done
