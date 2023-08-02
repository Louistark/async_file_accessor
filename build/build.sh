#! /bin/sh

if [ ! -d "./temp/" ]; then
    mkdir "./temp/"
fi

if [ ! -d "./out/new_files/" ]; then
    mkdir -p "./out/new_files/"
fi

cd ..
export ROOT_DIR=`pwd`
export SRC_DIR="$ROOT_DIR/src"
export INC_DIR="$ROOT_DIR/inc"
export LIB_DIR="$ROOT_DIR/lib"
export BUILD_DIR="$ROOT_DIR/build"
export TEMP_DIR="$BUILD_DIR/temp"
export OUT_DIR="$BUILD_DIR/out"
export DATA_SET="$ROOT_DIR/data_set"

echo "set(ROOT_DIR      $ROOT_DIR)"     >  $TEMP_DIR/env.cmake
echo "set(SRC_DIR       $SRC_DIR)"      >> $TEMP_DIR/env.cmake
echo "set(INC_DIR       $INC_DIR)"      >> $TEMP_DIR/env.cmake
echo "set(LIB_DIR       $LIB_DIR)"      >> $TEMP_DIR/env.cmake
echo "set(TEMP_DIR      $TEMP_DIR)"     >> $TEMP_DIR/env.cmake
echo "set(OUT_DIR       $OUT_DIR)"      >> $TEMP_DIR/env.cmake
echo "set(DATA_SET      $DATA_SET)"     >> $TEMP_DIR/env.cmake

cp -rf "$BUILD_DIR/CMakeLists.txt" "$TEMP_DIR/"
cd $TEMP_DIR/
cmake .

make

make install