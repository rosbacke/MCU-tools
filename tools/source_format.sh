#!/bin/sh

# Absolute path to this script. /home/user/bin/foo.sh
SCRIPT=$(readlink -f $0)

# Absolute path this script is in. /home/user/bin
SCRIPTPATH=`dirname $SCRIPT`

cd ${SCRIPTPATH}/../src
SRC_CPP=`find -iname '*.cpp'`
SRC_H=`find -iname '*.h'`
clang-format-5.0 -i ${SRC_CPP} ${SRC_H}
