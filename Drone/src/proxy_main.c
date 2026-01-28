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
        printf("\n系统接收到退出信号\n");
        proxy_stop();  // 停止代理运行
        g_running = 0;
    }
}

int main(int argc, char *argv[]) {
    
    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // MAVLink解析器不需要初始化
    
    // 初始化日志系统
    if (logger_init() < 0) {
        fprintf(stderr, "[错误] 日志系统初始化失败\n");
        return 1;
    }
    
    // 初始化代理
    if (proxy_init() < 0) {
        fprintf(stderr, "[错误] 代理初始化失败\n");
        logger_close();
        return 1;
    }
    
    // 运行代理
    proxy_run();
    
    // 清理资源
    proxy_close();
    logger_close();
    
    printf("已安全退出\n");
    return 0;
}
