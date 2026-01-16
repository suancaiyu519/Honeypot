/*
 * network.h - UDP网络服务头文件
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stddef.h>
#include <netinet/in.h>

/* 客户端信息 */
typedef struct {
    struct sockaddr_in addr;
    socklen_t addr_len;
    char ip_str[INET_ADDRSTRLEN];
    uint16_t port;
} client_info_t;

/**
 * 初始化UDP服务器
 * @param port 监听端口
 * @return socket描述符，失败返回-1
 */
int network_init(uint16_t port);

/**
 * 接收数据
 * @param sockfd socket描述符
 * @param buffer 接收缓冲区
 * @param buf_size 缓冲区大小
 * @param client 客户端信息（输出）
 * @return 接收到的字节数，失败返回-1
 */
ssize_t network_receive(int sockfd, uint8_t *buffer, size_t buf_size, client_info_t *client);

/**
 * 发送数据
 * @param sockfd socket描述符
 * @param data 发送数据
 * @param len 数据长度
 * @param client 客户端信息
 * @return 发送的字节数，失败返回-1
 */
ssize_t network_send(int sockfd, const uint8_t *data, size_t len, const client_info_t *client);

/**
 * 关闭socket
 * @param sockfd socket描述符
 */
void network_close(int sockfd);

#endif /* NETWORK_H */
