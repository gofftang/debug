#/bin/bash
CURDIR=`pwd`
TOOLEAN_DIR=${CURDIR}/../../../US536_Linux_SDK/tools/toolchain/unione/arm-none-linux-gnueabi-4.6.4_linux-3.3/bin

export PATH=${PATH}:${TOOLEAN_DIR}
echo ${PATH}

make clean && make && make install
