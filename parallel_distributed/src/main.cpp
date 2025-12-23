/*
    主函数入口，从此进入客户端或服务器端测试
*/

#include <iostream>
#include "common.hpp"
#include "network_config.h"

int main(int argc, char* argv[])
{
    printf("=== ParDist - Parallel Distributed Computing Tool ===\n");
    printf("Configuration:\n");
    printf("  Server IP: %s\n", SERVER_IP);
    printf("  Server Port: %d\n", SERVER_PORT);
    printf("\n请选择模式（客户端或服务器端）:\n");
    printf("  1. Server\n");
    printf("  2. Client\n");
    printf("输入选择 (1 或 2): ");
    
    int choice;  
    std::cin >> choice;
    
    if (choice == 1)
    {
        printf("\nStarting in SERVER mode...\n");
        run_server();
    }
    else if (choice == 2)
    {
        printf("\nStarting in CLIENT mode...\n");
        run_client();
    }
    else
    {
        printf("Error: Invalid choice. Please enter 1 or 2.\n");
        return 1;
    }

    return 0;
}


