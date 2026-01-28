/* 
 * config.h - 蜜罐配置定义
 */

#ifndef CONFIG_H
#define CONFIG_H

/* 网络配置 */
#define UDP_PORT 14550              // 对外监听端口 (QGC连接)
#define SITL_PORT 5760              // SITL 连接端口
#define BUFFER_SIZE 2048            // 接收缓冲区大小

/* 日志配置 */
#define LOG_DIR "./logs"            // 日志目录
#define LOG_FILE_PREFIX "drone_honeypot" // 日志文件前缀

#endif /* CONFIG_H */

