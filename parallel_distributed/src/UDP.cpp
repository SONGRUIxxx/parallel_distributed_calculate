/*
    UDP通信服务器端和客户端函数实现
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <cstdio>
#include <pthread.h>
#include <fcntl.h>
#include <chrono>
#include <cmath>
#include <limits>
#include "network_config.h"
#include "common.hpp"

using namespace std;

const size_t local_data_size_basic = DATANUM; // 基础版本处理全部数据
const size_t local_data_size_speedup_server = DATANUM * portion_server; // 加速版本服务器端处理数据量
const size_t local_data_size_speedup_client = DATANUM * (1 - portion_server); // 加速版本客户端处理数据量

// Global socket and address info
int g_socket;
struct sockaddr_in g_peerAddr;
socklen_t g_peerLen;
bool g_isServer = false;
int g_run_times = 0; // Number of test rounds
bool g_basicDone = false; // Whether basic version is completed
bool g_peerBasicDone = false; // Whether peer's basic version is completed

// Client results for speedup version
bool g_clientResultsReady = false;
float g_clientSum = 0.0f;
float g_clientMax = 0.0f;
float* g_clientSortedData = nullptr; // Will store Client's sorted 64M data

// Receive message thread function
void* receive_thread(void* arg)
{
    char buffer[256];
    
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int recv_len = recvfrom(g_socket, buffer, sizeof(buffer) - 1, 0, 
                               (struct sockaddr*)&g_peerAddr, &g_peerLen);
        if (recv_len > 0)
        {
            buffer[recv_len] = '\0';
            
            // Check if it's a signal message
            if (strcmp(buffer, "BASIC_DONE") == 0)
            {
                printf("[Received peer basic version done signal]\n");
                g_peerBasicDone = true;
            }
            else if (strncmp(buffer, "RUN_TIMES:", 10) == 0)
            {
                // Received run times signal
                g_run_times = atoi(buffer + 10);
                printf("[Received run_times: %d]\n", g_run_times);
            }
            else if (strncmp(buffer, "RESULT_SUM:", 11) == 0)
            {
                // Received Client's sum result
                g_clientSum = atof(buffer + 11);
                printf("[Received Client sum: %f]\n", g_clientSum);
            }
            else if (strncmp(buffer, "RESULT_MAX:", 11) == 0)
            {
                // Received Client's max result
                g_clientMax = atof(buffer + 11);
                printf("[Received Client max: %f]\n", g_clientMax);
            }
            else if (strcmp(buffer, "RESULTS_READY") == 0)
            {
                // All Client results received
                g_clientResultsReady = true;
                printf("[Client results ready]\n");
            }
            else
            {
                printf("\n[Peer]: %s\n", buffer);
            }
        }
    }
    return NULL;
}



void run_server()
{
    g_isServer = true;
    g_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_socket < 0)
    {
        printf("Can't create a socket! Quitting\n");
        return;
    }   
    printf("Socket Created.\n");

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(SERVER_PORT);
    local.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_socket, (struct sockaddr*)&local, sizeof(local)) < 0)
    {
        printf("bind failed with error: %s\n", strerror(errno));
        close(g_socket);
        return;
    }

    printf("Server listening on port %d...\n", SERVER_PORT);
    printf("Waiting for Client to send run times...\n\n");
    
    g_peerLen = sizeof(g_peerAddr);
    
    // Create receive thread
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_thread, NULL);
    
    // Wait for run times
    while (g_run_times == 0)
    {
        usleep(100000); // Wait for run times
    }
    
    printf("\n========================================\n");
    printf("Ready! 开始 %d 轮测试\n", g_run_times);
    printf("========================================\n\n");
    
    // Track total times for averaging
    double total_basic_time = 0.0;
    double total_speedup_time = 0.0;
    
    // 每一轮测试都进行一次基础版和加速版
    for (int round = 1; round <= g_run_times; round++)
    {
        // 第 round 轮测试卡斯
        printf("\n========== Round %d/%d ==========\n", round, g_run_times);
        
        // ===== 1. BASIC VERSION =====
        printf("\n[Basic版本 - 只用Server端处理]\n");

        data_init_and_shuffle(0, DATANUM); // 初始化数据，全部客户端处理

        // 开始计时，基础版本处理全部数据
        struct timespec start, end;

        // 1. Sum计算和计时
        clock_gettime(CLOCK_MONOTONIC, &start);
        float basic_sum = sumBasic(rawFloatData, local_data_size_basic);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double basic_time_1= (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
        std::cout << "Basic Sum用时：" << basic_time_1 << " ms，结果：" << basic_sum << std::endl;
        
        // 2. Max计算和计时
        clock_gettime(CLOCK_MONOTONIC, &start);
        float basic_max = maxBasic(rawFloatData, local_data_size_basic);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double basic_time_2= (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
        std::cout << "Basic Max用时：" << basic_time_2 << " ms，结果：" << basic_max << std::endl;

        // 3. Sort计算和计时
        float* basic_sorted = new float[local_data_size_basic];
        clock_gettime(CLOCK_MONOTONIC, &start);
        sortBasic(rawFloatData, local_data_size_basic, basic_sorted);
        clock_gettime(CLOCK_MONOTONIC, &end);
        delete[] basic_sorted;
        double basic_time_3= (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
        std::cout << "Basic Sort用时：" << basic_time_3 << " ms" << std::endl;

        double basic_time = basic_time_1 + basic_time_2 + basic_time_3;
        total_basic_time += basic_time;
        printf("***本轮Basic版本用时: %.2f ms***\n\n", basic_time);

        printf("[Server] Basic版本完成，通知Client...\n");
        const char* basic_done_msg = "BASIC_DONE";
        sendto(g_socket, basic_done_msg, strlen(basic_done_msg), 0, 
              (struct sockaddr*)&g_peerAddr, g_peerLen);
        g_basicDone = true;
        
        // Wait for Client confirmation
        while (!g_peerBasicDone)
        {
            usleep(10000); // 10ms
        }
        printf("[Server] Client ready, 开始本轮加速版本...\n");
        
        // Reset flags for next round
        g_basicDone = false;
        g_peerBasicDone = false;
        
        // ===== 2. SPEEDUP VERSION =====
        printf("\n[SpeedUp版本 - Server和Client同时处理]\n");
        
        // Reset Client results flag
        g_clientResultsReady = false;
        
        data_init_and_shuffle(0, local_data_size_speedup_server); // 初始化数据，Server处理前一部分

        // Wait for Client to finish data generation
        usleep(100000); // Wait 100ms for Client data generation
        
        // 开始计时
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // 1. 求和
        float server_sum = sumSpeedUp(rawFloatData, local_data_size_speedup_server);

        // 2. 求最大值
        float server_max = maxSpeedUp(rawFloatData, local_data_size_speedup_server);

        // 3. 排序
        float* server_sorted = new float[local_data_size_speedup_server];
        sortSpeedUp(rawFloatData, local_data_size_speedup_server, server_sorted);

        printf("[Server] Server端已完成，Sum结果: %f, Max结果: %f\n", server_sum, server_max);
        
        // Wait for Client results
        printf("[Server] 等待Client结果...\n");
        while (!g_clientResultsReady)
        {
            usleep(10000); // 10ms
        }
        
        // Merge results
        printf("[Server] Merging results...\n");
        float final_sum = server_sum + g_clientSum;
        float final_max = (server_max > g_clientMax) ? server_max : g_clientMax;

        // 合并排序结果

        clock_gettime(CLOCK_MONOTONIC, &end);
        
        printf("[Server] 最终加速的Sum结果: %f, Max结果: %f\n", final_sum, final_max);
        printf("[Server] 排序合并完成\n");
        
        delete[] server_sorted;
        
        
        double speedup_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
        total_speedup_time += speedup_time;
        printf("***本轮SpeedUp版总共用时: %.2f ms（未单独统计各部分时间）***\n", speedup_time);
    }
    
    printf("\n========================================\n");
    printf("测试完成！统计信息:\n");
    printf("========================================\n");
    printf("Basic版本平均用时: %.2f ms\n", total_basic_time / g_run_times);
    printf("SpeedUp版本平均用时: %.2f ms\n", total_speedup_time / g_run_times);
    printf("加速比: %.2fx\n", total_basic_time / total_speedup_time);
    printf("========================================\n");
    
    close(g_socket);
}

void run_client()
{
    g_isServer = false;
    g_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_socket < 0)
    {
        printf("Can't create a socket! Quitting\n");
        return;
    }
    printf("Socket Created.\n");
    
    g_peerAddr.sin_family = AF_INET;
    g_peerAddr.sin_port = htons(SERVER_PORT);
    g_peerAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    g_peerLen = sizeof(g_peerAddr);

    printf("Connecting to server %s:%d...\n", SERVER_IP, SERVER_PORT);
    
    // 创建接收线程
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_thread, NULL);
    
    // 让用户输入运行次数
    int run_times;
    printf("\n请输入测试的轮数（每轮包括basic版本和speedup版本）: ");
    while(true)
    {
        cin >> run_times;
        if(cin.good() && run_times > 0)
        {
            g_run_times = run_times;
            break;
        }
        else
        {
            printf("Please enter a valid positive integer: ");
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
    
    // 发送运行次数信号给 Server
    char times_msg[32];
    sprintf(times_msg, "RUN_TIMES:%d", g_run_times);
    sendto(g_socket, times_msg, strlen(times_msg), 0, 
          (struct sockaddr*)&g_peerAddr, g_peerLen);
    printf("[Client] Sent run_times: %d\n", g_run_times);
    
    // Wait a moment for Server to receive
    usleep(100000);
    
    printf("\n========================================\n");
    printf("Ready! 开始 %d 轮测试\n", g_run_times);
    printf("========================================\n\n");
    
    // Loop for specified number of rounds, each round runs basic + speedup
    for (int round = 1; round <= g_run_times; round++)
    {
        printf("\n========== Round %d/%d ==========\n", round, g_run_times);
        
        // ===== 1. BASIC VERSION =====
        // Client doesn't participate, wait for Server to complete
        printf("\n[Basic Version - Client waiting for Server...]\n");
        
        // Wait for Server to send BASIC_DONE signal
        while (!g_peerBasicDone)
        {
            usleep(10000); // 10ms
        }
        printf("[Client] Received Server basic version done signal\n");
        
        // Send confirmation signal
        const char* confirm_msg = "BASIC_DONE";
        sendto(g_socket, confirm_msg, strlen(confirm_msg), 0, 
              (struct sockaddr*)&g_peerAddr, g_peerLen);
        printf("[Client] Confirmed, ready for speedup version...\n");
        
        // Reset flag
        g_peerBasicDone = false;
        
        // ===== 2. SPEEDUP VERSION =====
        printf("\n[SpeedUp版本 - Client处理后半部分]\n");

        // 数据初始化：Client生成自己的数据（从逻辑上对应服务器的后半部分数据）
        data_init_and_shuffle(local_data_size_speedup_server, local_data_size_speedup_client);

        // Wait for Server ready signal
        usleep(100000);
        
        // Process data (Client处理自己生成的数据，从数组开头开始)
        float client_sum = sumSpeedUp(rawFloatData, local_data_size_speedup_client);
        float client_max = maxSpeedUp(rawFloatData, local_data_size_speedup_client);
        float* client_sorted = new float[local_data_size_speedup_client];
        sortSpeedUp(rawFloatData, local_data_size_speedup_client, client_sorted);
        
        printf("[Client] Sum: %f, Max: %f\n", client_sum, client_max);
        
        // Send results to Server
        printf("[Client] Sending results to Server...\n");
        char result_msg[64];
        
        sprintf(result_msg, "RESULT_SUM:%f", client_sum);
        sendto(g_socket, result_msg, strlen(result_msg), 0, 
              (struct sockaddr*)&g_peerAddr, g_peerLen);
        usleep(1000);
        
        sprintf(result_msg, "RESULT_MAX:%f", client_max);
        sendto(g_socket, result_msg, strlen(result_msg), 0, 
              (struct sockaddr*)&g_peerAddr, g_peerLen);
        usleep(1000);
        
        // Signal that all results are ready
        const char* ready_msg = "RESULTS_READY";
        sendto(g_socket, ready_msg, strlen(ready_msg), 0, 
              (struct sockaddr*)&g_peerAddr, g_peerLen);
        
        delete[] client_sorted;
        
        printf("[Client] Results sent\n");
    }
    
    printf("\n========================================\n");
    printf("[Client] 测试完成！\n");
    printf("========================================\n");
    
    close(g_socket);
}


