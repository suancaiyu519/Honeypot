/*
 * logger.h - 中文日志记录头文件
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "mavlink.h"

/* 客户端信息结构 */
typedef struct {
    struct sockaddr_in addr;
    socklen_t addr_len;
    char ip_str[INET_ADDRSTRLEN];
    uint16_t port;
} client_info_t;

/**
 * 初始化日志系统
 * @return 0成功，-1失败
 */
int logger_init(void);

/**
 * 记录新连接事件
 * @param client 客户端信息
 */
void logger_connection(const client_info_t *client);

/**
 * 记录心跳消息
 * @param client 客户端信息
 * @param msg MAVLink消息
 */
void logger_heartbeat(const client_info_t *client, const mavlink_message_t *msg);

/**
 * 记录命令消息
 * @param client 客户端信息
 * @param msg MAVLink消息
 */
void logger_command(const client_info_t *client, const mavlink_message_t *msg);

/**
 * 记录数据请求
 * @param client 客户端信息
 * @param msg MAVLink消息
 */
void logger_request(const client_info_t *client, const mavlink_message_t *msg);

/**
 * 记录未知消息
 * @param client 客户端信息
 * @param msg MAVLink消息
 */
void logger_unknown(const client_info_t *client, const mavlink_message_t *msg);

/**
 * 关闭日志系统
 */
void logger_close(void);

#endif /* LOGGER_H */
