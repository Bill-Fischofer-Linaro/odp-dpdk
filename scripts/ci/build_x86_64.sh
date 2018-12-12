#!/bin/bash
set -e

export TARGET_ARCH=x86_64-linux-gnu
if [ "${CC#clang}" != "${CC}" ] ; then
	export CXX="clang++"
fi

exec "$(dirname "$0")"/build.sh
