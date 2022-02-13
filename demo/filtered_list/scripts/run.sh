#!/bin/bash

CW=cw
HUMID=humid


if [ "$1" != "-q" ]; then
  if [ ! -r lib/system_exec.so.1.0 ]; then
    echo "Warning: lib/system_exec.so.1.0 not found"
    echo "If you have managed this your own way, avoid this warning with -q"
    echo "otherwise, please build system exec in the clocwork plugin directory"
    echo "and copy the file system_exec.so.1.0 to the lib directory"
    exit 1
  fi
fi

system=`uname -s`
if [ "$system" = "Darwin" ]; then
  export DYLD_LIBRARY_PATH=`pwd`/lib:${DYLD_LIBRARY_PATH}
else
  export LD_LIBRARY_PATH=./lib:${LD_LIBRARY_PATH}
fi

${HUMID} humid/ &
${CW} cw/

