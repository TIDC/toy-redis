error_check() {
    if [ $? -ne 0 ]; then
        echo "!!!!!!!!!!!!!!!!!! 出错 !!!!!!!!!!!!!!!!!!"
        echo "!!!!!!!!!!!!!!!!!! 出错 !!!!!!!!!!!!!!!!!!"
        echo "!!!!!!!!!!!!!!!!!! 出错 !!!!!!!!!!!!!!!!!!"
        echo "!!!!!!!!!!!!!!!!!! 出错 !!!!!!!!!!!!!!!!!!"
        echo "!!!!!!!!!!!!!!!!!! 出错 !!!!!!!!!!!!!!!!!!"
        echo "!!!!!!!!!!!!!!!!!! 出错 !!!!!!!!!!!!!!!!!!"
        echo "!!!!!!!!!!!!!!!!!! 出错 !!!!!!!!!!!!!!!!!!"
        exit $?
    fi
}
if [ ! -d "./build" ]; then
    mkdir ./build
fi

cd build
error_check
cmake ..
error_check
cpu=$(cat /proc/cpuinfo | grep processor | wc -l)
make -j ${cpu}
error_check
ctest --verbose
error_check
