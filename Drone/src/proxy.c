/*
 * proxy.c - MAVLink代理实现
 * 功能：在QGroundControl和ArduPilot SITL之间转发流量并监控
 */

#include "proxy.h"
#include "mavlink.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <time.h>

/* 全局变量 */
static int g_external_sock = -1;    // 外部socket（监听客户端）
static int g_internal_sock = -1;    // 内部socket（连接SITL）
static struct sockaddr_in g_sitl_addr; // SITL地址
static proxy_client_t g_client;     // 客户端信息
static proxy_stats_t g_stats;       // 统计信息
static volatile int g_proxy_running = 1;

/**
 * 创建UDP socket
 */
static int create_udp_socket(uint16_t port, int reuse) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[代理] socket创建失败");
        return -1;
    }
    
    // 设置地址重用
    if (reuse) {
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("[代理] setsockopt(SO_REUSEADDR)失败");
        }
    }
    
    // 绑定端口
    if (port > 0) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("[代理] bind失败");
            close(sockfd);
            return -1;
        }
    }
    
    return sockfd;
}

/**
 * 转发数据到SITL
 */
static void forward_to_sitl(const uint8_t *data, size_t len) {
    ssize_t sent = sendto(g_internal_sock, data, len, 0,
                         (struct sockaddr*)&g_sitl_addr, sizeof(g_sitl_addr));
    if (sent < 0) {
        perror("[代理] 发送到SITL失败");
        return;
    }
    
    g_stats.bytes_to_sitl += sent;
    g_stats.messages_from_client++;
}

/**
 * 转发数据到客户端
 */
static void forward_to_client(const uint8_t *data, size_t len) {
    if (!g_client.active) {
        return; // 没有活跃的客户端
    }
    
    ssize_t sent = sendto(g_external_sock, data, len, 0,
                         (struct sockaddr*)&g_client.addr, g_client.addr_len);
    if (sent < 0) {
        perror("[代理] 发送到客户端失败");
        return;
    }
    
    g_stats.bytes_to_client += sent;
    g_stats.messages_to_client++;
}

/**
 * 处理来自客户端的数据
 */
static void handle_client_data(const uint8_t *data, size_t len,
                                const struct sockaddr_in *client_addr,
                                socklen_t addr_len) {
    // 更新客户端信息
    if (!g_client.active || 
        memcmp(&g_client.addr, client_addr, sizeof(struct sockaddr_in)) != 0) {
        memcpy(&g_client.addr, client_addr, sizeof(struct sockaddr_in));
        g_client.addr_len = addr_len;
        g_client.active = 1;
        
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr->sin_addr, ip_str, sizeof(ip_str));
        printf("[代理] 新客户端连接: %s:%d\n", ip_str, ntohs(client_addr->sin_port));
    }
    
    g_client.last_seen = time(NULL);
    g_stats.bytes_from_client += len;
    
    // 解析MAVLink消息（用于日志）
    mavlink_message_t msg;
    if (mavlink_parse_message(data, len, &msg)) {
        // 记录日志
        client_info_t log_client;
        memcpy(&log_client.addr, client_addr, sizeof(struct sockaddr_in));
        log_client.addr_len = addr_len;
        
        // 根据消息类型记录
        switch (msg.msgid) {
            case MAVLINK_MSG_ID_HEARTBEAT:
                logger_heartbeat(&log_client, &msg);
                break;
            case MAVLINK_MSG_ID_PARAM_REQUEST_LIST:
            case MAVLINK_MSG_ID_PARAM_REQUEST_READ:
                logger_request(&log_client, &msg);
                break;
            case 76: // COMMAND_LONG
            case 75: // COMMAND_INT
                logger_command(&log_client, &msg);
                break;
            default:
                // 记录其他消息
                break;
        }
    }
    
    // 转发到SITL
    forward_to_sitl(data, len);
}

/**
 * 处理来自SITL的数据
 */
static void handle_sitl_data(const uint8_t *data, size_t len) {
    g_stats.bytes_from_sitl += len;
    
    // 转发到客户端
    forward_to_client(data, len);
}

int proxy_init(void) {
    memset(&g_stats, 0, sizeof(g_stats));
    memset(&g_client, 0, sizeof(g_client));
    
    // 创建外部socket（监听客户端）
    printf("[代理] 创建外部socket (端口 %d)...\n", PROXY_EXTERNAL_PORT);
    g_external_sock = create_udp_socket(PROXY_EXTERNAL_PORT, 1);
    if (g_external_sock < 0) {
        return -1;
    }
    
    // 创建内部socket（连接SITL）
    printf("[代理] 创建内部socket...\n");
    g_internal_sock = create_udp_socket(0, 0);
    if (g_internal_sock < 0) {
        close(g_external_sock);
        return -1;
    }
    
    // 设置SITL地址
    memset(&g_sitl_addr, 0, sizeof(g_sitl_addr));
    g_sitl_addr.sin_family = AF_INET;
    g_sitl_addr.sin_port = htons(PROXY_INTERNAL_PORT);
    if (inet_pton(AF_INET, PROXY_SITL_HOST, &g_sitl_addr.sin_addr) <= 0) {
        fprintf(stderr, "[代理] 无效的SITL地址: %s\n", PROXY_SITL_HOST);
        close(g_external_sock);
        close(g_internal_sock);
        return -1;
    }
    
    printf("[代理] 初始化完成\n");
    printf("[代理] 外部端口: %d (QGroundControl)\n", PROXY_EXTERNAL_PORT);
    printf("[代理] 内部地址: %s:%d (SITL)\n", PROXY_SITL_HOST, PROXY_INTERNAL_PORT);
    
    return 0;
}

void proxy_run(void) {
    uint8_t buffer[PROXY_BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len;
    
    printf("[代理] 开始运行...\n\n");
    
    while (g_proxy_running) {
        fd_set readfds;
        struct timeval tv;
        int max_fd;
        
        FD_ZERO(&readfds);
        FD_SET(g_external_sock, &readfds);
        FD_SET(g_internal_sock, &readfds);
        
        max_fd = (g_external_sock > g_internal_sock) ? g_external_sock : g_internal_sock;
        
        // 设置超时时间（1秒）
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("[代理] select失败");
            break;
        }
        
        if (ret == 0) {
            // 超时，继续循环
            continue;
        }
        
        // 检查外部socket（来自客户端）
        if (FD_ISSET(g_external_sock, &readfds)) {
            from_len = sizeof(from_addr);
            ssize_t recv_len = recvfrom(g_external_sock, buffer, sizeof(buffer), 0,
                                       (struct sockaddr*)&from_addr, &from_len);
            if (recv_len > 0) {
                handle_client_data(buffer, recv_len, &from_addr, from_len);
            }
        }
        
        // 检查内部socket（来自SITL）
        if (FD_ISSET(g_internal_sock, &readfds)) {
            ssize_t recv_len = recvfrom(g_internal_sock, buffer, sizeof(buffer), 0,
                                       NULL, NULL);
            if (recv_len > 0) {
                handle_sitl_data(buffer, recv_len);
            }
        }
    }
}

void proxy_close(void) {
    printf("\n[代理] 正在关闭...\n");
    
    if (g_external_sock >= 0) {
        close(g_external_sock);
        g_external_sock = -1;
    }
    
    if (g_internal_sock >= 0) {
        close(g_internal_sock);
        g_internal_sock = -1;
    }
    
    printf("[代理] 关闭完成\n");
    printf("[代理] 统计信息:\n");
    printf("  客户端 → SITL: %lu字节, %lu消息\n", 
           g_stats.bytes_to_sitl, g_stats.messages_from_client);
    printf("  SITL → 客户端: %lu字节, %lu消息\n",
           g_stats.bytes_to_client, g_stats.messages_to_client);
}

proxy_stats_t* proxy_get_stats(void) {
    return &g_stats;
}
