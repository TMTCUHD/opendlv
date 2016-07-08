#!/bin/bash

cd /opt/opendlv-sources

if [ "$1" == "FRESH" ]; then
    rm -fr build.arm.from.docker
    mkdir build.arm.from.docker
    cd build.arm.from.docker
    mkdir build
    mkdir install

    cd build
    cmake -DDISABLE_CAN=YES -D CMAKE_INSTALL_PREFIX=../install ../..
    make
    cd ..

    mkdir build.system
    cd build.system
    cmake -DDISABLE_CAN=YES -DEIGEN3_INCLUDE_DIR=../../thirdparty/eigen3 -DKALMAN_INCLUDE_DIR=../../thirdparty/kalman/include -DTINYCNN_INCLUDE_DIR=../../thirdparty/tiny-cnn -DCMAKE_INSTALL_PREFIX=../install ../../code/system
    make
    make install
    cd ..
fi

if [ "$1" == "INCREMENTAL" ]; then
    if [ -d build.arm.from.docker -a -d build.arm.from.docker/build -a -d build.arm.from.docker/install -a -d build.arm.from.docker/build.system ]; then
        cd build.arm.from.docker
        cd build.system
        make
        make install
    else
        exit 1
    fi
fi

