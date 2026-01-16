/*
 * network.c - UDP网络服务实现
 */

#include "network.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int network_init(uint16_t port) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    // 创建UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket创建失败");
        return -1;
    }
    
    // 设置socket选项（允许地址重用）
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt失败");
        close(sockfd);
        return -1;
    }
    
    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // 绑定端口
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind失败");
        close(sockfd);
        return -1;
    }
    
    printf("[网络] UDP服务器初始化成功，监听端口: %d\n", port);
    return sockfd;
}

ssize_t network_receive(int sockfd, uint8_t *buffer, size_t buf_size, client_info_t *client) {
    client->addr_len = sizeof(client->addr);
    
    ssize_t recv_len = recvfrom(sockfd, buffer, buf_size, 0,
                                (struct sockaddr*)&client->addr, &client->addr_len);
    
    if (recv_len > 0) {
        // 提取客户端IP和端口
        inet_ntop(AF_INET, &client->addr.sin_addr, client->ip_str, INET_ADDRSTRLEN);
        client->port = ntohs(client->addr.sin_port);
    }
    
    return recv_len;
}

ssize_t network_send(int sockfd, const uint8_t *data, size_t len, const client_info_t *client) {
    return sendto(sockfd, data, len, 0,
                  (struct sockaddr*)&client->addr, client->addr_len);
}

void network_close(int sockfd) {
    if (sockfd >= 0) {
        close(sockfd);
        printf("[网络] UDP服务器已关闭\n");
    }
}
