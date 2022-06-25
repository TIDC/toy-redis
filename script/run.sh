if [ ! -d "./build" ]; then
    rm -fr ./build
fi

mkdir ./build
cd build
cmake ..
make -j

./toy_redis