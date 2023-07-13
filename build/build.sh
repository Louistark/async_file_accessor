#! /bin/sh

cp -rf "./CMakeLists.txt" "./temp/"
cd "./temp/"
cmake .
make