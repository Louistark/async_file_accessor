#! /bin/sh

# if [-d "./temp/"]; then
#     mkdir "./temp/"
# fi

cp -rf "./CMakeLists.txt" "./temp/"
cd "./temp/"
cmake .
make