#!/usr/bin/env bash
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

get_cpu_count(){
    cpu=2
    if [ "$(uname)" == 'Darwin' ]; then
        cpu=$(sysctl -n machdep.cpu.thread_count)
    elif [ "$(expr substr $(uname -s) 1 5)"=="Linux" ]; then   
        cpu=$(cat /proc/cpuinfo | grep processor | wc -l)
    fi
    return $cpu
}

if [ ! -d "./build" ]; then
    mkdir ./build
fi

cd build
error_check
cmake ..
error_check
get_cpu_count
cpu=$?

make -j ${cpu}
error_check
ctest --verbose
error_check
