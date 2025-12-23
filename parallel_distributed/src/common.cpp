/*
    通用函数实现：数据初始化和洗牌打乱
*/

#include <iostream>
#include <random>
#include "common.hpp"

float rawFloatData[DATANUM];

// 数据初始化，值域 [start + 1, start + local_data_size]
void init_rawData(int start, size_t local_data_size)
{
    for (size_t i = 0; i < local_data_size; ++i) {
        rawFloatData[i] = static_cast<float>(start + i + 1);
    }
}

// 使用Fisher-Yates算法打乱数组
void shuffle_rawData(size_t local_data_size)
{
    std::random_device rd;
    std::mt19937 g(rd());
    
    for (size_t i = local_data_size - 1; i > 0; --i)
    {
        std::uniform_int_distribution<size_t> distrib(0, i);
        size_t j = distrib(g);
        std::swap(rawFloatData[i], rawFloatData[j]);
    }
}

// 初始化并打乱数据
void data_init_and_shuffle(int start, size_t local_data_size)
{
    init_rawData(start, local_data_size);
    shuffle_rawData(local_data_size);
    std::cout << "[数据已初始化并打乱完成]" << std::endl;
}