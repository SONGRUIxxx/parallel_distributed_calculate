#pragma once

#include <cstddef>

// 常量定义
#define MAX_THREADS 64
#define SUBDATANUM 2000000
#define DATANUM (SUBDATANUM * MAX_THREADS)

// 数据分配比例：加速版Server端处理的数据比例（0-1之间）
/* 
    由于本组为 ubuntu系统 + ubuntu虚拟机环境，性能差距较大
    故将服务器端处理比例调高以适应环境
    测试时推荐修改为 0.5 
*/
#define portion_server 0.85 // **可修改比例以适配设备**

// 全局数据
extern float rawFloatData[DATANUM];

// 数据初始化函数
void init_rawData(int start, size_t local_data_size);
void shuffle_rawData(size_t local_data_size);
void data_init_and_shuffle(int start, size_t local_data_size);

// 基础版本函数
float sumBasic(const float data[], const int len);
float maxBasic(const float data[], const int len);
void sortBasic(const float data[], const int len, float* result);

// 归并排序辅助函数
void merge(const float data[], size_t* indices, size_t left, size_t mid, size_t right, size_t* temp);
void mergeSort(const float data[], size_t* indices, size_t left, size_t right, size_t* temp);

// 加速版本函数
float sumSpeedUp(const float data[], const int len);
float maxSpeedUp(const float data[], const int len);
void sortSpeedUp(const float data[], const int len, float* result);

// 加速版本归并排序辅助函数
void insertionSort(const float data[], size_t* indices, size_t left, size_t right);
void mergeParallel(const float data[], size_t* indices, size_t left, size_t mid, size_t right, size_t* temp, int depth);
void mergeSortParallel(const float data[], size_t* indices, size_t left, size_t right, size_t* temp, int depth);

// UDP 通信函数
void run_server();
void run_client();
