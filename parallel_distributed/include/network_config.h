#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

// 网络配置宏定义
// 如果需要本机自己和自己通信，使用 127.0.0.1
// 如果需要两台主机通信，修改 SERVER_IP 为目标主机的实际IP

// 服务器IP地址 - 默认本机回环地址(自己和自己通信)
#define SERVER_IP "127.0.0.1"  // **可修改为实际服务器IP**

// 如果需要两台主机通信，取消下面这行的注释并注释掉上面的定义
// #define SERVER_IP "192.168.137.217"

// 端口配置
#define SERVER_PORT 9999  // **两个设备可修改为同一端口，建议9999**
#define CLIENT_PORT 8080  // 客户端连接端口(实际未使用)

#endif // NETWORK_CONFIG_H
