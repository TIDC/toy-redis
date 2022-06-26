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
make -j
error_check
ctest --verbose
error_check

