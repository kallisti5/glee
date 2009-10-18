#!/bin/sh

rm -f ./test/*
cp -f ../../DATA/outputLinux/* ./test
cd test
./configure
make
