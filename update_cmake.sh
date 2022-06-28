#!/bin/bash
url=""
dir_name=""

get_arch=$(arch)
if [[ $get_arch =~ "x86_64" ]]; then
    echo "this is x86_64"
    url="https://cmake.org/files/v3.23/cmake-3.23.0-linux-x86_64.tar.gz"
    dir_name="cmake-3.23.0-linux-x86_64"
elif [[ $get_arch =~ "aarch64" ]]; then
    echo "this is arm64"
    url="https://cmake.org/files/v3.23/cmake-3.23.0-linux-aarch64.tar.gz"
    dir_name="cmake-3.23.0-linux-aarch64"
else
    echo "未知平台"
    exit 0
fi

mkdir ~/tool
mkdir ~/tool/download
cd ~/tool/download

echo "下载 cmake $version"
rm cmake.tar.gz
wget $url -O cmake.tar.gz
tar -zxvf cmake.tar.gz -C ../

cd ~/tool
echo "下载的 cmake 版本:"
./$dir_name/bin/cmake --version

# 替换原有的 cmake
sudo mv /usr/bin/cmake /usr/bin/cmake_backup
sudo update-alternatives --install /usr/bin/cmake cmake ~/tool/$dir_name/bin/cmake 100

sudo mv /usr/bin/ctest /usr/bin/ctest_backup
sudo update-alternatives --install /usr/bin/ctest ctest ~/tool/$dir_name/bin/ctest 100
