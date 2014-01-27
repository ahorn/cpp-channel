#!/bin/sh

# This script generates the configure script and other files required in
# building the C++11 cpp-channel library.

GTEST="gtest-1.7.0"

set -e

# Check that we're being run from the right directory.
if test ! -f src/channel.cpp; then
  cat >&2 << __EOF__
Could not find source code. Make sure you are running this script from the
root of the distribution tree.
__EOF__
  exit 1
fi

# Check that gtest is present.
if test ! -e gtest; then
  echo "Google Test not present. Fetching ${GTEST} from the web..."
  curl http://googletest.googlecode.com/files/${GTEST}.zip > ${GTEST}.zip
  unzip ${GTEST}.zip
  mv ${GTEST} gtest
fi

set -ex

autoreconf -f -i -Wall,no-obsolete

rm -rf autom4te.cache config.h.in~
exit 0
