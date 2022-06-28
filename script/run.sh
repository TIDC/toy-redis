if [ ! -d "./build" ]; then
    rm -fr ./build
fi

mkdir ./build
cd build
cmake ..
cpu=$(cat /proc/cpuinfo | grep processor | wc -l)
make -j ${cpu}

./toy_redis
