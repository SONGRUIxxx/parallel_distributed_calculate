/*
    加速版函数实现
*/
#include "common.hpp"

#include <iostream>
#include <cmath>
#include <limits>
#include <immintrin.h>  // SSE/AVX 指令集
#include <omp.h>        // OpenMP


// 加速的求和函数 - 使用 SSE + OpenMP（修复版）
float sumSpeedUp(const float data[], const int len)
{
    float total_sum = 0.0f;
    
    #pragma omp parallel reduction(+:total_sum)
    {
        const int limit4 = len & ~3; //

        // 处理4个一组的主循环
        #pragma omp for nowait // 并行分配循环迭代
        for (int i = 0; i < limit4; i += 4)
        {
            // 加载并防止非正数
            __m128 vec_data = _mm_loadu_ps(&data[i]);
            __m128 min_val = _mm_set1_ps(1e-37f);
            vec_data = _mm_max_ps(vec_data, min_val);
            
            __m128 v = _mm_sqrt_ps(vec_data);
            float t[4];
            _mm_storeu_ps(t, v);
            
            // 直接累加到total_sum（reduction会自动处理）
            total_sum += std::log(t[0]) + std::log(t[1]) +
                         std::log(t[2]) + std::log(t[3]);
        }
        
        // 串行处理尾部剩余元素
        #pragma omp for nowait
        for (int i = limit4; i < len; ++i)
        {
            total_sum += std::log(std::sqrt(data[i]));
        }
    }
    
    return total_sum;
}

// 加速的最大值函数 - 使用 SSE + OpenMP（修复版）
float maxSpeedUp(const float data[], const int len)
{
    float global_max = -std::numeric_limits<float>::infinity();
    
    #pragma omp parallel reduction(max:global_max)
    {
        const int limit4 = len & ~3;
        
        #pragma omp for nowait
        for (int i = 0; i < limit4; i += 4)
        {
            // 加载并防止非正数
            __m128 vec_data = _mm_loadu_ps(&data[i]);
            __m128 min_val = _mm_set1_ps(1e-37f);
            vec_data = _mm_max_ps(vec_data, min_val);
            
            __m128 v = _mm_sqrt_ps(vec_data);
            float temp[4];
            _mm_storeu_ps(temp, v);
            
            float v0 = std::log(temp[0]);
            float v1 = std::log(temp[1]);
            float v2 = std::log(temp[2]);
            float v3 = std::log(temp[3]);
            
            // 直接更新global_max（reduction会自动处理）
            if (v0 > global_max) global_max = v0;
            if (v1 > global_max) global_max = v1;
            if (v2 > global_max) global_max = v2;
            if (v3 > global_max) global_max = v3;
        }
        
        // 串行处理尾部
        #pragma omp for nowait
        for (int i = limit4; i < len; ++i)
        {
            float value = std::log(std::sqrt(data[i]));
            if (value > global_max)
                global_max = value;
        }
    }
    
    return global_max;
}

// 插入排序（小数组优化）
void insertionSort(const float data[], size_t* indices, size_t left, size_t right)
{
    for (size_t i = left + 1; i <= right; ++i)
    {
        size_t key = indices[i];
        float key_value = std::log(std::sqrt(data[key]));
        size_t j = i;
        
        while (j > left && std::log(std::sqrt(data[indices[j-1]])) > key_value)
        {
            indices[j] = indices[j-1];
            --j;
        }
        indices[j] = key;
    }
}

// OpenMP 并行归并排序的合并函数（并行版本）
void mergeParallel(const float data[], size_t* indices, size_t left, size_t mid, size_t right, size_t* temp, int depth)
{
    const size_t len = right - left + 1;
    const int MAX_MERGE_DEPTH = 3;  // 合并的并行深度限制
    
    // 小区间或深度过深，直接串行合并
    if (len < 8192 || depth >= MAX_MERGE_DEPTH)
    {
        size_t i = left;
        size_t j = mid + 1;
        size_t k = left;
        
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
        
        while (i <= mid)
        {
            temp[k++] = indices[i++];
        }
        
        while (j <= right)
        {
            temp[k++] = indices[j++];
        }
        
        for (size_t idx = left; idx <= right; idx++)
        {
            indices[idx] = temp[idx];
        }
        return;
    }
    
    // 并行合并：分割成两段
    size_t left_mid = left + (mid - left) / 2;
    float pivot = std::log(std::sqrt(data[indices[left_mid]]));
    
    // 在右半段找第一个 >= pivot 的位置
    size_t lo = mid + 1, hi = right + 1;
    while (lo < hi)
    {
        size_t m = lo + (hi - lo) / 2;
        if (std::log(std::sqrt(data[indices[m]])) < pivot)
            lo = m + 1;
        else
            hi = m;
    }
    size_t right_mid = (lo > mid + 1) ? lo - 1 : mid;
    
    // 计算pivot在temp中的最终位置
    size_t k_mid = left + (left_mid - left) + (right_mid - mid);
    temp[k_mid] = indices[left_mid];
    
    // 并行合并两段（不含pivot）
    const int next_depth = depth + 1;
    if (left < left_mid)
    {
        #pragma omp task shared(data, indices, temp) if(next_depth < MAX_MERGE_DEPTH)
        mergeParallel(data, indices, left, left_mid - 1, right_mid, temp, next_depth);
    }
    
    if (left_mid + 1 <= mid)
    {
        #pragma omp task shared(data, indices, temp) if(next_depth < MAX_MERGE_DEPTH)
        mergeParallel(data, indices, left_mid + 1, mid, right, temp, next_depth);
    }
    
    #pragma omp taskwait
    
    // 拷贝回indices
    for (size_t idx = left; idx <= right; idx++)
    {
        indices[idx] = temp[idx];
    }
}

// OpenMP 并行归并排序递归函数（修复版）
void mergeSortParallel(const float data[], size_t* indices, size_t left, size_t right, size_t* temp, int depth)
{
    if (left >= right) return;
    
    const size_t len = right - left + 1;
    
    // 小数组用插入排序
    if (len <= 32)
    {
        insertionSort(data, indices, left, right);
        return;
    }
    
    size_t mid = left + (right - left) / 2;
    
    // 动态阈值：根据线程数和深度判断（优化后）
    const int max_threads = omp_get_max_threads();
    const size_t grainsize = (max_threads > 1) ? (max_threads * 2048) : 32768;
    const int MAX_DEPTH = 10;  // 增大深度限制，让更多task并行
    
    // 只在数据足够大且深度合理时才并行
    if (len > grainsize && depth < MAX_DEPTH)
    {
        // 使用 firstprivate 传递参数，temp 共享
        #pragma omp task shared(data, indices, temp) firstprivate(left, mid, depth)
        mergeSortParallel(data, indices, left, mid, temp, depth + 1);
        
        #pragma omp task shared(data, indices, temp) firstprivate(mid, right, depth)
        mergeSortParallel(data, indices, mid + 1, right, temp, depth + 1);
        
        #pragma omp taskwait  // 确保两个子任务完成
    }
    else
    {
        // 串行处理
        mergeSortParallel(data, indices, left, mid, temp, depth + 1);
        mergeSortParallel(data, indices, mid + 1, right, temp, depth + 1);
    }
    
    // 合并阶段（并行merge）
    mergeParallel(data, indices, left, mid, right, temp, 0);
}

// 加速的排序函数 - 使用 OpenMP Task 并行（优化版）
void sortSpeedUp(const float data[], const int len, float* result)
{
    // 创建索引数组（并行初始化）
    size_t* sortIndices = new size_t[len];
    #pragma omp parallel for
    for (int i = 0; i < len; i++)
    {
        sortIndices[i] = i;
    }
    
    // 创建临时数组
    size_t* tempIndices = new size_t[len];
    
    // 使用 OpenMP 并行归并排序
    #pragma omp parallel
    {
        #pragma omp single
        {
            mergeSortParallel(data, sortIndices, 0, len - 1, tempIndices, 0);
        }
    }
    
    // 填充结果数组 - 轻量级并行（分块策略）
    const int block = (len + 3) / 4;
    #pragma omp parallel for schedule(static, block)
    for (int i = 0; i < len; i++)
    {
        result[i] = std::log(std::sqrt(data[sortIndices[i]]));
    }
    
    delete[] tempIndices;
    delete[] sortIndices;
}