/*
 * main.c - 无人机蜜罐主程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include "config.h"
#include "mavlink.h"
#include "network.h"
#include "logger.h"

/* 全局变量 */
static volatile int g_running = 1;
static int g_sockfd = -1;

/* 信号处理函数 */
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n[系统] 接收到退出信号，正在关闭...\n");
        g_running = 0;
    }
}

/* 处理接收到的MAVLink消息 */
void handle_mavlink_message(int sockfd __attribute__((unused)), const client_info_t *client, 
                            const mavlink_message_t *msg) {
    static int first_contact = 1;
    
    // 记录首次连接
    if (first_contact) {
        logger_connection(client);
        first_contact = 0;
    }
    
    // 根据消息类型处理
    switch (msg->msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT:
            logger_heartbeat(client, msg);
            break;
        
        case 66: // REQUEST_DATA_STREAM
        case 126: // REQUEST_MESSAGE
            logger_request(client, msg);
            break;
        
        case 76: // COMMAND_LONG
        case 75: // COMMAND_INT
            logger_command(client, msg);
            break;
        
        default:
            logger_unknown(client, msg);
            break;
    }
}

/* 发送心跳和遥测数据 */
void send_telemetry(int sockfd, const client_info_t *client) {
    static time_t last_heartbeat = 0;
    static time_t last_gps = 0;
    static time_t last_attitude = 0;
    
    time_t now = time(NULL);
    uint8_t buffer[BUFFER_SIZE];
    size_t len;
    
    // 发送心跳（1秒间隔）
    if (now - last_heartbeat >= HEARTBEAT_INTERVAL) {
        len = mavlink_create_heartbeat(buffer, sizeof(buffer));
        if (len > 0) {
            network_send(sockfd, buffer, len, client);
        }
        last_heartbeat = now;
    }
    
    // 发送GPS位置（1秒间隔）
    if (now - last_gps >= 1) {
        len = mavlink_create_gps_position(buffer, sizeof(buffer));
        if (len > 0) {
            network_send(sockfd, buffer, len, client);
        }
        
        len = mavlink_create_gps_raw(buffer, sizeof(buffer));
        if (len > 0) {
            network_send(sockfd, buffer, len, client);
        }
        last_gps = now;
    }
    
    // 发送姿态（1秒间隔）
    if (now - last_attitude >= 1) {
        len = mavlink_create_attitude(buffer, sizeof(buffer));
        if (len > 0) {
            network_send(sockfd, buffer, len, client);
        }
        
        len = mavlink_create_sys_status(buffer, sizeof(buffer));
        if (len > 0) {
            network_send(sockfd, buffer, len, client);
        }
        last_attitude = now;
    }
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    printf("========================================\n");
    printf("       MAVLink无人机蜜罐 v2.0\n");
    printf("========================================\n\n");
    
    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化各模块
    printf("[系统] 初始化MAVLink处理器...\n");
    mavlink_init();
    
    printf("[系统] 初始化日志系统...\n");
    if (logger_init() < 0) {
        fprintf(stderr, "[错误] 日志系统初始化失败\n");
        return 1;
    }
    
    printf("[系统] 初始化网络服务...\n");
    g_sockfd = network_init(UDP_PORT);
    if (g_sockfd < 0) {
        fprintf(stderr, "[错误] 网络初始化失败\n");
        logger_close();
        return 1;
    }
    
    printf("\n[系统] 蜜罐启动成功！等待连接...\n");
    printf("[提示] 按Ctrl+C退出\n\n");
    
    // 主循环
    uint8_t recv_buffer[BUFFER_SIZE];
    client_info_t client;
    mavlink_message_t msg;
    
    while (g_running) {
        // 设置超时（100ms）
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(g_sockfd, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        
        int ret = select(g_sockfd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (g_running) {
                perror("[错误] select失败");
            }
            break;
        }
        
        if (ret == 0) {
            // 超时，继续循环
            continue;
        }
        
        // 接收数据
        ssize_t recv_len = network_receive(g_sockfd, recv_buffer, sizeof(recv_buffer), &client);
        if (recv_len <= 0) {
            continue;
        }
        
        // 解析MAVLink消息
        if (mavlink_parse_message(recv_buffer, recv_len, &msg)) {
            handle_mavlink_message(g_sockfd, &client, &msg);
            
            // 发送遥测数据响应
            send_telemetry(g_sockfd, &client);
        }
    }
    
    // 清理资源
    printf("\n[系统] 正在清理资源...\n");
    network_close(g_sockfd);
    logger_close();
    
    printf("[系统] 蜜罐已安全退出\n");
    return 0;
}
