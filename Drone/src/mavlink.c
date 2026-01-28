/*
 * mavlink.c - MAVLink协议处理实现（精简版）
 * 只保留解析功能，生成消息由SITL负责
 */

#include "mavlink.h"
#include <string.h>

/**
 * 解析MAVLink消息
 */
int mavlink_parse_message(const uint8_t *data, size_t len, mavlink_message_t *msg) {
    // 检查最小长度
    if (len < MAVLINK_HEADER_LEN_V1 + MAVLINK_CHECKSUM_LEN) {
        return 0;
    }
    
    // 检查起始标志并确定协议版本
    int is_v2 = 0;
    int header_len;
    
    if (data[0] == MAVLINK_STX_V1) {
        is_v2 = 0;
        header_len = MAVLINK_HEADER_LEN_V1;
    } else if (data[0] == MAVLINK_STX_V2) {
        is_v2 = 1;
        header_len = MAVLINK_HEADER_LEN_V2;
    } else {
        return 0; // 无效的起始标志
    }
    
    // 获取payload长度
    uint8_t payload_len = data[1];
    
    // 检查是否有足够的数据
    size_t min_msg_len = header_len + payload_len + MAVLINK_CHECKSUM_LEN;
    if (len < min_msg_len) {
        return 0;
    }
    
    // 解析消息头
    msg->magic = data[0];
    msg->len = payload_len;
    
    if (is_v2) {
        // MAVLink v2格式
        // [0]=STX, [1]=len, [2]=incompat_flags, [3]=compat_flags,
        // [4]=seq, [5]=sysid, [6]=compid, [7-9]=msgid (24-bit)
        msg->seq = data[4];
        msg->sysid = data[5];
        msg->compid = data[6];
        // v2的msgid是24位，但我们只存储低8位以兼容v1结构
        msg->msgid = data[7];  // 只取低8位
    } else {
        // MAVLink v1格式
        // [0]=STX, [1]=len, [2]=seq, [3]=sysid, [4]=compid, [5]=msgid
        msg->seq = data[2];
        msg->sysid = data[3];
        msg->compid = data[4];
        msg->msgid = data[5];
    }
    
    // 复制payload
    if (payload_len > 0) {
        memcpy(msg->payload, &data[header_len], payload_len);
    }
    
    // 提取校验和
    msg->checksum = data[header_len + payload_len] | (data[header_len + payload_len + 1] << 8);
    
    return 1;
}

