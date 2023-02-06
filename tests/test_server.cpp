//
// Created by innoyiya on 2022/10/2.
//
#include "toy-redis/server.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>
#include <sys/socket.h>

TEST(server, addClient)
{
    // std::thread([&]() {
    //     tr::ToyRedisServer server("");
    // }).detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "client init" << std::endl;

    int clientId = socket(AF_INET, SOCK_STREAM, 0);
    std::cout << "clientId is " << clientId << std::endl;
    assert(clientId != -1 && "Error connect");

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(6758);
    serverAddr.sin_addr.s_addr = inet_addr("172.17.0.2");
    auto result = connect(clientId, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    std::cout << "result is " << result << std::endl;
    assert(result >= 0 && "Error: connect");
    char buf[1024];
    // buf[0] = '*';
    // buf[1] = 't';
    auto commond = "set k1 v1";
    auto send_length = send(clientId, commond, 9, 0);
    std::cout << "send is " << send_length << std::endl;
    assert(send_length > 0 && "Error: send");

    auto set_response_size = recv(clientId, buf, 1024, 0);
    std::cout << "set_response_size is " << set_response_size << std::endl;
    std::cout << "set_response is " << buf << std::endl;

    std::cout << "send get k1 commond" << std::endl;
    auto get_commond = "get k1";
    send_length = send(clientId, get_commond, 6, 0);

    auto recv_length = recv(clientId, buf, 1024, 0);
    std::cout << "recv_length is " << recv_length << std::endl;
    // std::string resu{buf};
    std::cout << "recv response is " << buf << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    close(clientId);
}