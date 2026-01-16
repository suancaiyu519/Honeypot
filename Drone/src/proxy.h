/*
 * proxy.h - MAVLink代理头文件
 */

#ifndef PROXY_H
#define PROXY_H

#include <stdint.h>
#include <netinet/in.h>

/* 代理配置 */
#define PROXY_EXTERNAL_PORT 14550  // 外部端口（QGroundControl连接）
#define PROXY_INTERNAL_PORT 14551  // 内部端口（SITL连接）
#define PROXY_SITL_HOST "127.0.0.1" // SITL地址
#define PROXY_BUFFER_SIZE 2048      // 缓冲区大小

/* 客户端连接信息 */
typedef struct {
    struct sockaddr_in addr;
    socklen_t addr_len;
    time_t last_seen;
    int active;
} proxy_client_t;

/* 代理统计信息 */
typedef struct {
    uint64_t bytes_from_client;     // 来自客户端的字节数
    uint64_t bytes_to_client;       // 发往客户端的字节数
    uint64_t bytes_from_sitl;       // 来自SITL的字节数
    uint64_t bytes_to_sitl;         // 发往SITL的字节数
    uint64_t messages_from_client;  // 来自客户端的消息数
    uint64_t messages_to_client;    // 发往客户端的消息数
} proxy_stats_t;

/**
 * 初始化代理
 * @return 成功返回0，失败返回-1
 */
int proxy_init(void);

/**
 * 运行代理主循环
 */
void proxy_run(void);

/**
 * 关闭代理
 */
void proxy_close(void);

/**
 * 获取代理统计信息
 * @return 统计信息指针
 */
proxy_stats_t* proxy_get_stats(void);

#endif /* PROXY_H */
