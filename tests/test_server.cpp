//
// Created by innoyiya on 2022/10/2.
//
#include "gtest/gtest.h"
#include "toy-redis/server.hpp"

TEST(server, addClient){
    std::thread([&](){
        tr::ToyRedisServer server("");
    }).detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "client init" << std::endl;


    int clientId = socket(AF_INET, SOCK_STREAM, 0);
    std::cout << clientId << std::endl;
    assert(clientId != -1 && "Error connect");

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(6758);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    auto result = connect(clientId, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    std::cout << result << std::endl;
    assert( result >= 0 && "Error: connect");
    close(clientId);
}