/*
 * mavlink.h - MAVLink协议处理头文件
 */

#ifndef MAVLINK_H
#define MAVLINK_H

#include <stdint.h>
#include <stddef.h>

/* MAVLink消息ID */
#define MAVLINK_MSG_ID_HEARTBEAT 0
#define MAVLINK_MSG_ID_SYS_STATUS 1
#define MAVLINK_MSG_ID_PARAM_REQUEST_LIST 21
#define MAVLINK_MSG_ID_PARAM_REQUEST_READ 20
#define MAVLINK_MSG_ID_PARAM_VALUE 22
#define MAVLINK_MSG_ID_GPS_RAW_INT 24
#define MAVLINK_MSG_ID_ATTITUDE 30
#define MAVLINK_MSG_ID_GLOBAL_POSITION_INT 33

/* MAVLink协议常量 */
#define MAVLINK_STX 0xFE            // MAVLink v1 起始标志
#define MAVLINK_HEADER_LEN 6        // 消息头长度
#define MAVLINK_CHECKSUM_LEN 2      // 校验和长度
#define MAVLINK_MAX_PAYLOAD_LEN 255 // 最大载荷长度

/* MAVLink消息结构 */
typedef struct {
    uint8_t magic;                  // STX
    uint8_t len;                    // 载荷长度
    uint8_t seq;                    // 序列号
    uint8_t sysid;                  // 系统ID
    uint8_t compid;                 // 组件ID
    uint8_t msgid;                  // 消息ID
    uint8_t payload[MAVLINK_MAX_PAYLOAD_LEN]; // 载荷
    uint16_t checksum;              // 校验和
} mavlink_message_t;

/* 无人机状态 */
typedef struct {
    int32_t lat;                    // 纬度 (度 * 1e7)
    int32_t lon;                    // 经度 (度 * 1e7)
    int32_t alt;                    // 海拔 (mm)
    int16_t vx;                     // X速度 (cm/s)
    int16_t vy;                     // Y速度 (cm/s)
    int16_t vz;                     // Z速度 (cm/s)
    uint16_t hdg;                   // 航向 (度 * 100)
    float roll;                     // 横滚角 (rad)
    float pitch;                    // 俯仰角 (rad)
    float yaw;                      // 偏航角 (rad)
    uint8_t satellites_visible;     // 可见卫星数
    uint8_t fix_type;               // GPS定位类型
} drone_state_t;

/* 函数声明 */

/**
 * 初始化MAVLink处理器
 */
void mavlink_init(void);

/**
 * 解析MAVLink消息
 * @param data 接收到的数据
 * @param len 数据长度
 * @param msg 输出的消息结构
 * @return 1成功，0失败
 */
int mavlink_parse_message(const uint8_t *data, size_t len, mavlink_message_t *msg);

/**
 * 创建心跳消息
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 消息长度
 */
size_t mavlink_create_heartbeat(uint8_t *buffer, size_t buf_size);

/**
 * 创建GPS位置消息
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 消息长度
 */
size_t mavlink_create_gps_position(uint8_t *buffer, size_t buf_size);

/**
 * 创建姿态消息
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 消息长度
 */
size_t mavlink_create_attitude(uint8_t *buffer, size_t buf_size);

/**
 * 创建GPS原始数据消息
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 消息长度
 */
size_t mavlink_create_gps_raw(uint8_t *buffer, size_t buf_size);

/**
 * 创建系统状态消息
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 消息长度
 */
size_t mavlink_create_sys_status(uint8_t *buffer, size_t buf_size);

/**
 * 创建参数值消息
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param param_id 参数名称
 * @param param_value 参数值
 * @param param_index 参数索引
 * @param param_count 参数总数
 * @return 消息长度
 */
size_t mavlink_create_param_value(uint8_t *buffer, size_t buf_size,
                                   const char *param_id, float param_value,
                                   uint16_t param_index, uint16_t param_count);

/**
 * 获取当前无人机状态
 * @return 无人机状态指针
 */
drone_state_t* mavlink_get_state(void);

#endif /* MAVLINK_H */
