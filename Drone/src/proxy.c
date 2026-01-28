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
        perror("[代理] UDP socket创建失败");
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
            perror("[代理] UDP bind失败");
            close(sockfd);
            return -1;
        }
    }
    
    return sockfd;
}

/**
 * 创建TCP socket并连接到SITL
 */
static int create_tcp_connection(const char *host, uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[代理] TCP socket创建失败");
        return -1;
    }
    
    // 设置地址
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        fprintf(stderr, "[代理] 无效的地址: %s\n", host);
        close(sockfd);
        return -1;
    }
    
    // 连接
    printf("[代理] 正在连接 SITL (%s:%d)...\n", host, port);
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[代理] TCP连接失败");
        close(sockfd);
        return -1;
    }
    
    printf("[代理] TCP连接成功！\n");
    return sockfd;
}

static int g_sitl_connected = 0;    // SITL连接状态

/**
 * 转发数据到SITL（通过TCP）
 */
static void forward_to_sitl(const uint8_t *data, size_t len) {
    if (!g_sitl_connected || g_internal_sock < 0) {
        return;
    }
    
    ssize_t sent = send(g_internal_sock, data, len, 0);
    if (sent < 0) {
        perror("[代理] 发送到SITL失败");
        g_sitl_connected = 0;
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
        printf("[代理] 客户端连接: %s:%d\n", ip_str, ntohs(client_addr->sin_port));
        
        // 记录连接日志
        client_info_t log_client;
        memcpy(&log_client.addr, client_addr, sizeof(struct sockaddr_in));
        log_client.addr_len = addr_len;
        strcpy(log_client.ip_str, ip_str);
        log_client.port = ntohs(client_addr->sin_port);
        logger_connection(&log_client);
    }
    
    g_client.last_seen = time(NULL);
    g_stats.bytes_from_client += len;
    
    // 调试：显示接收到的数据
    printf("[调试] 收到客户端数据: %zu 字节, 起始字节: 0x%02x\n", len, data[0]);
    
    // 解析MAVLink消息（用于日志）- 支持多个消息
    client_info_t log_client;
    memcpy(&log_client.addr, client_addr, sizeof(struct sockaddr_in));
    log_client.addr_len = addr_len;
    inet_ntop(AF_INET, &client_addr->sin_addr, log_client.ip_str, sizeof(log_client.ip_str));
    log_client.port = ntohs(client_addr->sin_port);
    
    size_t offset = 0;
    int msg_count = 0;
    static uint16_t last_cmd_id = 0;  // 跟踪上一条命令ID，用于过滤
    
    while (offset < len) {
        mavlink_message_t msg;
        if (mavlink_parse_message(data + offset, len - offset, &msg)) {
            // 计算消息总长度（根据协议版本）
            int header_len = (msg.magic == 0xFD) ? MAVLINK_HEADER_LEN_V2 : MAVLINK_HEADER_LEN_V1;
            size_t msg_len = header_len + msg.len + MAVLINK_CHECKSUM_LEN;
            msg_count++;
            
            printf("[调试] 消息 #%d: ID=%d, 长度=%d, payload=%d\n", 
                   msg_count, msg.msgid, (int)msg_len, msg.len);
            
            // 根据消息类型记录
            switch (msg.msgid) {
                case MAVLINK_MSG_ID_HEARTBEAT:
                    // 心跳消息太频繁，跳过不记录
                    break;
                case 2:   // SYSTEM_TIME - 系统时间同步
                case 66:  // REQUEST_DATA_STREAM - 数据流请求
                case 110: // TIMESYNC - 时间同步
                case 134: // TERRAIN_DATA - 地形数据
                    // 这些消息太频繁，跳过不记录
                    break;
                case 43:  // MISSION_REQUEST - 任务请求
                case 47:  // MISSION_COUNT - 任务计数
                case 51:  // MISSION_SET_CURRENT - 设置当前任务
                    // 任务初始化消息，跳过不记录
                    break;
                case MAVLINK_MSG_ID_PARAM_REQUEST_LIST:
                case MAVLINK_MSG_ID_PARAM_REQUEST_READ:
                    logger_request(&log_client, &msg);
                    break;
                case 76: // COMMAND_LONG
                    // 过滤频繁的状态轮询命令，只保留真正的控制命令
                    {
                        uint16_t command = msg.payload[28] | (msg.payload[29] << 8);
                        float param1;
                        memcpy(&param1, &msg.payload[0], 4);
                        int req_msg_id = (int)param1;
                        
                        // 过滤状态轮询命令
                        if (command == 512) {
                            // 命令512：请求自动驾驶仪能力，跳过
                            break;
                        }
                        if (command == 511 && (req_msg_id == 242 || req_msg_id == 245)) {
                            // 命令511请求消息242(返航位置)/245(扩展状态)，跳过
                            break;
                        }
                        if (command == 521 && req_msg_id == 1) {
                            // 命令521请求消息1(系统状态)，跳过
                            break;
                        }
                        
                        // 过滤紧随位置命令后的模式确认命令
                        if (command == 176 && last_cmd_id == 192) {
                            // 命令176(设置模式)紧随命令192(设置位置)后，跳过
                            last_cmd_id = command;
                            break;
                        }
                        
                        last_cmd_id = command;
                        logger_command(&log_client, &msg);
                    }
                    break;
                case 75: // COMMAND_INT
                    {
                        // 获取COMMAND_INT中的命令ID
                        uint16_t cmd_int = msg.payload[28] | (msg.payload[29] << 8);
                        last_cmd_id = cmd_int;
                        logger_command(&log_client, &msg);
                    }
                    break;
                default:
                    logger_unknown(&log_client, &msg);
                    break;
            }
            
            offset += msg_len;
        } else {
            // 无法解析，跳出循环
            if (offset < len) {
                printf("[调试] 无法解析剩余数据 (offset=%zu, len=%zu, 剩余=%zu)\n", 
                       offset, len, len - offset);
            }
            break;
        }
    }
    
    printf("[调试] 本次共解析 %d 条消息\n", msg_count);
    
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
    
    // 创建外部UDP socket（监听客户端）
    printf("[代理] 创建外部UDP socket (端口 %d)...\n", PROXY_EXTERNAL_PORT);
    g_external_sock = create_udp_socket(PROXY_EXTERNAL_PORT, 1);
    if (g_external_sock < 0) {
        return -1;
    }
    
    // 创建TCP连接到SITL
    g_internal_sock = create_tcp_connection(PROXY_SITL_HOST, PROXY_INTERNAL_PORT);
    if (g_internal_sock < 0) {
        close(g_external_sock);
        return -1;
    }
    
    g_sitl_connected = 1;
    
    printf("[代理] 初始化完成\n");
    printf("[代理] 外部端口: UDP %d (等待QGroundControl连接)\n", PROXY_EXTERNAL_PORT);
    printf("[代理] 内部连接: TCP %s:%d (已连接SITL)\n", PROXY_SITL_HOST, PROXY_INTERNAL_PORT);
    
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
        
        if (g_sitl_connected && g_internal_sock >= 0) {
            FD_SET(g_internal_sock, &readfds);
        }
        
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
        
        // 检查外部UDP socket（来自客户端）
        if (FD_ISSET(g_external_sock, &readfds)) {
            from_len = sizeof(from_addr);
            ssize_t recv_len = recvfrom(g_external_sock, buffer, sizeof(buffer), 0,
                                       (struct sockaddr*)&from_addr, &from_len);
            if (recv_len > 0) {
                handle_client_data(buffer, recv_len, &from_addr, from_len);
            }
        }
        
        // 检查内部TCP socket（来自SITL）
        if (g_sitl_connected && FD_ISSET(g_internal_sock, &readfds)) {
            ssize_t recv_len = recv(g_internal_sock, buffer, sizeof(buffer), 0);
            if (recv_len > 0) {
                handle_sitl_data(buffer, recv_len);
            } else if (recv_len == 0) {
                printf("[代理] SITL连接断开\n");
                g_sitl_connected = 0;
                close(g_internal_sock);
                g_internal_sock = -1;
                break;
            } else {
                perror("[代理] 接收SITL数据失败");
                g_sitl_connected = 0;
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

void proxy_stop(void) {
    g_proxy_running = 0;
}
