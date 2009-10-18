#!/bin/sh

rm -f ../../DATA/outputLinux/*
cp -f linuxfiles/* ../../DATA/outputLinux
cp -f ../../DATA/output/* ../../DATA/outputLinux
rm -f ../../DATA/outputLinux/*.zip
rm -f ../../DATA/outputLinux/*.lib

cd ../../DATA/outputLinux
tar czvf GLee-5.4.0-src.tar.gz *

#TODO: add binary and redist versions too
