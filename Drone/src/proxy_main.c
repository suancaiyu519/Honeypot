/*
 * proxy_main.c - MAVLink代理主程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "proxy.h"
#include "mavlink.h"
#include "logger.h"

static volatile int g_running = 1;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n[系统] 接收到退出信号\n");
        proxy_stop();  // 停止代理运行
        g_running = 0;
    }
}

int main(int argc, char *argv[]) {
    printf("========================================\n");
    printf("     MAVLink代理蜜罐 v2.0\n");
    printf("========================================\n\n");
    
    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化MAVLink处理器
    printf("[系统] 初始化MAVLink处理器...\n");
    mavlink_init();
    
    // 初始化日志系统
    printf("[系统] 初始化日志系统...\n");
    if (logger_init() < 0) {
        fprintf(stderr, "[错误] 日志系统初始化失败\n");
        return 1;
    }
    
    // 初始化代理
    printf("[系统] 初始化MAVLink代理...\n");
    if (proxy_init() < 0) {
        fprintf(stderr, "[错误] 代理初始化失败\n");
        logger_close();
        return 1;
    }
    
    printf("\n");
    printf("┌────────────────────────────────────┐\n");
    printf("│  代理已启动！                       │\n");
    printf("│  QGroundControl → :14550           │\n");
    printf("│  代理监控中...                      │\n");
    printf("│  ArduPilot SITL ← :14551           │\n");
    printf("└────────────────────────────────────┘\n");
    printf("\n[提示] 确保ArduPilot SITL运行在端口14551\n");
    printf("[提示] 按Ctrl+C退出\n\n");
    
    // 运行代理
    proxy_run();
    
    // 清理资源
    proxy_close();
    logger_close();
    
    printf("[系统] 代理已安全退出\n");
    return 0;
}
