#include "base/hello.h"

#include <iostream>
#include "toy-redis/toy_redis_server.hpp"

int main(int, char **)
{
    std::cout << HelloMessage() << std::endl;
    tr::ToyRedisServer server("");
}
