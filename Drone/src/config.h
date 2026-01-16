/* 
 * config.h - 蜜罐配置定义
 */

#ifndef CONFIG_H
#define CONFIG_H

/* 网络配置 */
#define UDP_PORT 14550              // MAVLink默认端口
#define BUFFER_SIZE 2048            // 接收缓冲区大小

/* MAVLink配置 */
#define SYSTEM_ID 1                 // 系统ID
#define COMPONENT_ID 1              // 组件ID
#define AUTOPILOT_TYPE 3            // MAV_AUTOPILOT_ARDUPILOTMEGA
#define VEHICLE_TYPE 2              // MAV_TYPE_QUADROTOR (四旋翼)

/* 无人机模拟位置（北京坐标） */
#define HOME_LAT 39.9042            // 纬度
#define HOME_LON 116.4074           // 经度
#define HOME_ALT 100.0              // 高度（米）

/* 心跳配置 */
#define HEARTBEAT_INTERVAL 1        // 心跳间隔（秒）

/* 日志配置 */
#define LOG_DIR "./logs"            // 日志目录
#define LOG_FILE_PREFIX "drone_honeypot" // 日志文件前缀

#endif /* CONFIG_H */
