#include "base/simple_dynamic_string.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>

int main(int argc, const char *args[])
{
    std::string_view hello = "Hello World";
    base::SimpleDynamicString sds(hello);

    assert(sds.Length() == hello.length());
    std::cout << sds.Length() << std::endl;

    return 0;
}