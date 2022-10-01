#include "base/hello.h"

#include "toy-redis/server.hpp"
#include <iostream>

int main(int, char **)
{
    std::cout << HelloMessage() << std::endl;
    tr::ToyRedisServer server("");
}
