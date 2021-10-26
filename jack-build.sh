#!/bin/bash
export EXTRA=/opt/msprod/sysroot/osx
export PKG_CONFIG_PATH=/opt/msprod/sysroot/osx/lib/pkgconfig
make -j4 DEBUG=1  
