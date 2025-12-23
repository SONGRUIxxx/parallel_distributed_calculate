# parallel_distributed_calculate
# 分布式并行计算实验

**同济大学 算法课程大作业**

## 项目简介

本项目是一个高性能的分布式并行计算系统，使用C++实现，结合SSE指令集、OpenMP和UDP网络通信技术，在双机协作下高速处理千万至亿级浮点数据的三个核心运算：
- **数组求和** (Sum)
- **求最大值** (Max)  
- **排序** (Sort)

通过合理的任务划分和多层次并行优化，实现了 **9.46倍** 的性能加速。

## 技术栈

- **编程语言**: C++
- **并行技术**: SSE指令集、OpenMP多线程
- **通信协议**: UDP Socket
- **操作系统**: Ubuntu Linux
- **构建工具**: CMake

## 项目架构

### 文件结构

```
parallel_distributed/
├── CMakeLists.txt          # CMake构建配置
├── README.md               # 项目说明文档
├── include/                # 头文件目录
│   ├── common.hpp          # 公共头文件，链接各模块
│   └── network_config.h    # 网络配置（IP、端口）
├── src/                    # 源代码目录
│   ├── main.cpp            # 主函数入口（客户端/服务器）
│   ├── basic.cpp           # 基础版本算法实现
│   ├── speed_up.cpp        # 加速版本算法实现
│   ├── UDP.cpp             # UDP通信模块
│   └── common.cpp          # 公共函数（数据初始化、洗牌等）
└── build/                  # 编译输出目录
```

### 系统架构

```
┌─────────────────────┐         UDP          ┌─────────────────────┐
│   Server (Ubuntu)   │◄─────────────────────►│  Client (Ubuntu VM) │
│  处理 85% 数据       │      协议通信         │  处理 15% 数据       │
│  - OpenMP多线程     │                       │  - OpenMP多线程      │
│  - SSE向量化        │                       │  - SSE向量化         │
└─────────────────────┘                       └─────────────────────┘
         │                                              │
         └──────────────── 结果合并 ────────────────────┘
```

## 核心功能

### 1. 基础版本 (Basic)
- 单机串行处理全部数据
- 用于性能对比基准

### 2. 加速版本 (SpeedUp)
采用三层加速策略：

#### 🚀 **多机并行**
- Server和Client按配置比例分别处理数据
- 默认Server处理85%，Client处理15%（可根据硬件性能调整）
- UDP协议实现任务分发与结果汇总

#### 🧵 **OpenMP多线程**
- 自动检测CPU核心数，创建线程池
- `#pragma omp parallel` 并行区域
- `reduction` 操作优化求和/最大值
- 任务窃取式递归归并排序

#### ⚡ **SSE向量化**
- `__m128` 一次处理4个浮点数
- `_mm_sqrt_ps` 并行计算平方根
- 显著减少循环次数和指令数

## 性能测试结果

### 测试环境
- **数据规模**: 千万至亿级浮点数
- **测试轮数**: 5轮取平均
- **Server**: Ubuntu系统（高性能）
- **Client**: Ubuntu虚拟机

### 性能数据

| 版本 | 平均耗时 | 加速比 |
|------|---------|--------|
| Basic版本 | 149,243 ms | 1.00x (基准) |
| SpeedUp版本 | 15,778 ms | **9.46x** |

### 详细测试结果

```
========================================
测试完成！统计信息:
========================================
Basic版本平均用时: 149243.25 ms
Speedup版本平均用时: 15777.60 ms
加速比: 9.46x
========================================
```

## 快速开始

### 环境要求

- **操作系统**: Ubuntu Linux (或其他Linux发行版)
- **编译器**: GCC/G++ (支持C++11及以上)
- **工具**: CMake 3.10+
- **网络**: 两台计算机需在同一局域网

### 配置步骤

#### 1. 配置网络参数

编辑 [include/network_config.h](include/network_config.h)：

```cpp
// 第9行：修改为实际服务器IP地址
#define SERVER_IP "192.168.1.100"  // ← 改为Server的实际IP

// 第15行：修改端口号（可选）
#define SERVER_PORT 9999  // 建议使用9999
```

#### 2. 配置任务分配比例

编辑 [include/common.hpp](include/common.hpp)：

```cpp
// 第16行：根据两台电脑性能调整数据分配比例
#define portion_server 0.85  // Server处理85%，Client处理15%
```

**调整建议**:
- 性能相近的两台机器：设置为 `0.5`（各处理一半）
- Server性能更强：设置为 `0.7~0.85`
- 性能差异极大：根据实际测试调整

### 编译步骤

```bash
# 1. 进入项目根目录
cd parallel_distributed

# 2. 进入build目录
cd build

# 3. 生成构建文件
cmake ..

# 4. 编译项目
make

# 编译成功后会生成可执行文件 pardist
```

### 运行步骤

#### 在Server端（性能较强的机器）

```bash
./pardist
# 根据提示选择运行模式：输入 's' 或 'server'
# 等待Client连接...
```

#### 在Client端（另一台机器）

```bash
./pardist
# 根据提示选择运行模式：输入 'c' 或 'client'
# 输入测试轮数（建议5轮）
# 自动开始测试...
```

### 运行流程

1. **启动Server**: Server端进入监听状态
2. **启动Client**: Client连接Server并发送测试轮数
3. **基础测试**: Server单独运行Basic版本（性能基准）
4. **加速测试**: Server和Client协同运行SpeedUp版本
5. **结果输出**: Server端显示详细统计和加速比

## 技术细节

### 1. 求和加速 (`sumSpeedUp`)

```cpp
float sumSpeedUp(const float data[], const int len) {
    float total_sum = 0.0f;
    
    #pragma omp parallel reduction(+:total_sum)
    {
        #pragma omp for nowait
        for (int i = 0; i <= len - 4; i += 4) {
            __m128 vec_data = _mm_loadu_ps(&data[i]);  // 加载4个float
            __m128 v = _mm_sqrt_ps(vec_data);          // 并行开方
            
            float temp[4];
            _mm_storeu_ps(temp, v);
            
            total_sum += std::log(temp[0]) + std::log(temp[1]) 
                       + std::log(temp[2]) + std::log(temp[3]);
        }
    }
    return total_sum;
}
```

**优化点**:
- OpenMP自动线程分配
- SSE一次处理4个浮点数
- Reduction自动规约求和

### 2. 最大值加速 (`maxSpeedUp`)

```cpp
float maxSpeedUp(const float data[], const int len) {
    float global_max = -std::numeric_limits<float>::infinity();
    
    #pragma omp parallel reduction(max:global_max)
    {
        #pragma omp for nowait
        for (int i = 0; i <= len - 4; i += 4) {
            __m128 vec_data = _mm_loadu_ps(&data[i]);
            __m128 v = _mm_sqrt_ps(vec_data);
            
            float temp[4];
            _mm_storeu_ps(temp, v);
            
            for (int j = 0; j < 4; ++j) {
                global_max = std::max(global_max, std::log(temp[j]));
            }
        }
    }
    return global_max;
}
```

**优化点**:
- 初始值设为负无穷保证正确性
- Reduction自动规约取最大值

### 3. 排序加速 (`sortSpeedUp`)

```cpp
void sortSpeedUp(const float data[], const int len, float* result) {
    size_t* sortIndices = new size_t[len];
    
    // 并行初始化索引
    #pragma omp parallel for
    for (int i = 0; i < len; i++) {
        sortIndices[i] = i;
    }
    
    // 任务并行归并排序
    #pragma omp parallel
    {
        #pragma omp single
        mergeSortParallel(sortIndices, 0, len - 1, data, 0);
    }
    
    // 并行填充结果
    #pragma omp parallel for schedule(static, block)
    for (int i = 0; i < len; i++) {
        result[i] = std::log(std::sqrt(data[sortIndices[i]]));
    }
}
```

**优化点**:
- 并行初始化索引数组
- 任务窃取式递归归并
- 并行归并阶段（大区间二分）
- 小数组回退插入排序
- 并行结果填充

### 4. UDP通信模块

**核心函数**:
- `run_server()`: 服务器主循环，处理多轮测试
- `run_client()`: 客户端主循环，协同计算
- `receive_thread()`: 独立接收线程，异步处理消息

**通信协议**:
```
RUN_TIMES       -> 测试轮数通知
BASIC_DONE      -> Basic版本完成信号
RESULT_SUM      -> 求和结果传输
RESULT_MAX      -> 最大值结果传输
RESULTS_READY   -> Client处理完成信号
```

**可靠性保障**:
- 超时重发机制
- 消息类型识别
- 状态同步控制

## 常见问题

### Q1: 编译时找不到头文件？
**A**: 确保在 `build/` 目录下执行 `cmake ..` 和 `make`

### Q2: 运行时连接失败？
**A**: 检查以下项：
- 两台电脑是否在同一局域网
- 防火墙是否开放指定端口
- `network_config.h` 中的IP地址是否正确
- Server是否已启动并监听

### Q3: 性能不佳/加速比低？
**A**: 尝试调整：
- `portion_server` 比例（根据两台机器性能）
- 数据规模（太小无法体现并行优势）
- 关闭系统其他占用资源的程序

### Q4: 如何在本地单机测试？
**A**: 
1. 将 `SERVER_IP` 设置为 `"127.0.0.1"`
2. 打开两个终端，分别运行Server和Client
3. 注意：单机测试无法体现多机并行优势

## 贡献者

- **徐意**: 加速模块开发 + 报告编写（1,2,3,5部分）
- **宋睿轩**: 通信模块开发 + 报告编写（4,6,7部分）

## 许可证

本项目为同济大学算法课程大作业，仅供学习交流使用。

---
