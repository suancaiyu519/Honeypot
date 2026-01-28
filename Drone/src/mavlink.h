/*
 * mavlink.h - MAVLink协议处理头文件（精简版）
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
#define MAVLINK_STX_V1 0xFE         // MAVLink v1 起始标志
#define MAVLINK_STX_V2 0xFD         // MAVLink v2 起始标志
#define MAVLINK_HEADER_LEN_V1 6     // v1消息头长度
#define MAVLINK_HEADER_LEN_V2 10    // v2消息头长度
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

/**
 * 解析MAVLink消息
 * @param data 接收到的数据
 * @param len 数据长度
 * @param msg 输出的消息结构
 * @return 1成功，0失败
 */
int mavlink_parse_message(const uint8_t *data, size_t len, mavlink_message_t *msg);

#endif /* MAVLINK_H */
