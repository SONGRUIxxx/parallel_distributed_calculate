/*
    基础无加速版的函数实现
*/

#include "common.hpp"

#include <iostream>
#include <cmath>
#include <limits>

// 未加速的求和函数
float sumBasic(const float data[], const int len)
{
    const int UNROLL = 4096;
    float sums[UNROLL] = {0};  // 数组初始化为0
    
    // 主循环：每次处理UNROLL个元素
    for(int i = 0; i < len; i += UNROLL)
    {
        for(int j = 0; j < UNROLL && (i + j) < len; j++)
        {
            sums[j] += std::log(std::sqrt(data[i + j]));
        }
    }
    
    // 合并所有部分和
    float total_sum = 0.0f;
    for(int i = 0; i < UNROLL; i++)
    {
        total_sum += sums[i];
    }
    
    return total_sum;
}

// 未加速的最大值函数
float maxBasic(const float data[], const int len)
{
    float max_value = -std::numeric_limits<float>::infinity(); // 初始化为最小值
    for (int i = 0; i < len; i++)
    {
        float value = std::log(std::sqrt(data[i]));
        if (value > max_value)
        {
            max_value = value;
        }
    }
    return max_value;
}

// 归并排序的合并函数
void merge(const float data[], size_t* indices, size_t left, size_t mid, size_t right, size_t* temp)
{
    size_t i = left; // 左子数组起始索引
    size_t j = mid + 1; // 右子数组起始索引
    size_t k = left;    // 临时数组起始索引
    
    // 合并两个子数组
    while (i <= mid && j <= right)
    {
        if (std::log(std::sqrt(data[indices[i]])) <= std::log(std::sqrt(data[indices[j]])))
        {
            temp[k++] = indices[i++];
        }
        else
        {
            temp[k++] = indices[j++];
        }
    }
    
    // 复制剩余元素
    while (i <= mid)
    {
        temp[k++] = indices[i++];
    }
    while (j <= right)
    {
        temp[k++] = indices[j++];
    }
    
    // 将排序结果复制回原数组
    for (size_t idx = left; idx <= right; idx++)
    {
        indices[idx] = temp[idx];
    }
}

// 归并排序递归函数
void mergeSort(const float data[], size_t* indices, size_t left, size_t right, size_t* temp)
{
    if (left < right)
    {
        size_t mid = left + (right - left) / 2; 
        mergeSort(data, indices, left, mid, temp); // 排序左半部分
        mergeSort(data, indices, mid + 1, right, temp); // 排序右半部分
        merge(data, indices, left, mid, right, temp); // 合并两部分
    }
}

// 未加速的排序函数
void sortBasic(const float data[], const int len, float* result)
{
    // 创建索引数组
    size_t* sortIndices = new size_t[len];
    for (int i = 0; i < len; i++)
    {
        sortIndices[i] = i;
    }
    
    // 创建临时数组用于归并排序
    size_t* tempIndices = new size_t[len];
    
    // 进行归并排序
    mergeSort(data, sortIndices, 0, len - 1, tempIndices);
    
    // 根据排序后的索引填充结果数组
    for (int i = 0; i < len; i++)
    {
        result[i] = std::log(std::sqrt(data[sortIndices[i]]));
    }
    
    // 释放内存
    delete[] tempIndices;
    delete[] sortIndices;
}