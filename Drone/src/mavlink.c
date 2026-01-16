/*
 * mavlink.c - MAVLink协议处理实现
 */

#include "mavlink.h"
#include "config.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

/* 全局变量 */
static uint8_t g_sequence = 0;
static drone_state_t g_drone_state;
static struct timeval g_start_time;

/* CRC_EXTRA表（MAVLink消息类型的CRC种子） */
static const uint8_t crc_extra[] = {
    [MAVLINK_MSG_ID_HEARTBEAT] = 50,
    [MAVLINK_MSG_ID_SYS_STATUS] = 124,
    [MAVLINK_MSG_ID_GPS_RAW_INT] = 24,
    [MAVLINK_MSG_ID_ATTITUDE] = 39,
    [MAVLINK_MSG_ID_GLOBAL_POSITION_INT] = 104,
};

/**
 * 计算CRC16
 */
static uint16_t crc_calculate(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        uint8_t tmp = data[i] ^ (crc & 0xFF);
        tmp ^= (tmp << 4);
        crc = (crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4);
    }
    return crc;
}

/**
 * 计算MAVLink CRC（包含CRC_EXTRA）
 */
static uint16_t mavlink_crc(const uint8_t *data, size_t len, uint8_t msgid) {
    uint16_t crc = crc_calculate(data, len);
    
    // 添加CRC_EXTRA
    if (msgid < sizeof(crc_extra)) {
        uint8_t extra = crc_extra[msgid];
        uint8_t tmp = extra ^ (crc & 0xFF);
        tmp ^= (tmp << 4);
        crc = (crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4);
    }
    
    return crc;
}

/**
 * 打包MAVLink消息
 */
static size_t mavlink_pack_message(uint8_t msgid, const uint8_t *payload, 
                                   uint8_t payload_len, uint8_t *buffer, size_t buf_size) {
    if (buf_size < MAVLINK_HEADER_LEN + payload_len + MAVLINK_CHECKSUM_LEN) {
        return 0;
    }
    
    // 构建消息头
    buffer[0] = MAVLINK_STX;
    buffer[1] = payload_len;
    buffer[2] = g_sequence++;
    buffer[3] = SYSTEM_ID;
    buffer[4] = COMPONENT_ID;
    buffer[5] = msgid;
    
    // 复制载荷
    if (payload_len > 0) {
        memcpy(&buffer[6], payload, payload_len);
    }
    
    // 计算CRC
    uint16_t crc = mavlink_crc(&buffer[1], 5 + payload_len, msgid);
    buffer[6 + payload_len] = crc & 0xFF;
    buffer[6 + payload_len + 1] = (crc >> 8) & 0xFF;
    
    return MAVLINK_HEADER_LEN + payload_len + MAVLINK_CHECKSUM_LEN;
}

/**
 * 获取启动时间戳（毫秒）
 */
static uint32_t get_time_boot_ms(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    
    uint64_t now_ms = (uint64_t)now.tv_sec * 1000 + now.tv_usec / 1000;
    uint64_t start_ms = (uint64_t)g_start_time.tv_sec * 1000 + g_start_time.tv_usec / 1000;
    
    return (uint32_t)(now_ms - start_ms);
}

/**
 * 获取当前时间戳（微秒）
 */
static uint64_t get_time_usec(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (uint64_t)now.tv_sec * 1000000 + now.tv_usec;
}

/* 公共函数实现 */

void mavlink_init(void) {
    // 记录启动时间
    gettimeofday(&g_start_time, NULL);
    
    // 初始化无人机状态
    g_drone_state.lat = (int32_t)(HOME_LAT * 1e7);
    g_drone_state.lon = (int32_t)(HOME_LON * 1e7);
    g_drone_state.alt = (int32_t)(HOME_ALT * 1000);
    g_drone_state.vx = 0;
    g_drone_state.vy = 0;
    g_drone_state.vz = 0;
    g_drone_state.hdg = 0;
    g_drone_state.roll = 0.0;
    g_drone_state.pitch = 0.0;
    g_drone_state.yaw = 0.0;
    g_drone_state.satellites_visible = 12;
    g_drone_state.fix_type = 3; // 3D fix
}

int mavlink_parse_message(const uint8_t *data, size_t len, mavlink_message_t *msg) {
    if (len < MAVLINK_HEADER_LEN + MAVLINK_CHECKSUM_LEN) {
        return 0;
    }
    
    if (data[0] != MAVLINK_STX) {
        return 0;
    }
    
    uint8_t payload_len = data[1];
    if (len < MAVLINK_HEADER_LEN + payload_len + MAVLINK_CHECKSUM_LEN) {
        return 0;
    }
    
    msg->magic = data[0];
    msg->len = payload_len;
    msg->seq = data[2];
    msg->sysid = data[3];
    msg->compid = data[4];
    msg->msgid = data[5];
    
    if (payload_len > 0) {
        memcpy(msg->payload, &data[6], payload_len);
    }
    
    msg->checksum = data[6 + payload_len] | (data[6 + payload_len + 1] << 8);
    
    return 1;
}

size_t mavlink_create_heartbeat(uint8_t *buffer, size_t buf_size) {
    uint8_t payload[9];
    
    // custom_mode (4 bytes)
    payload[0] = 0;
    payload[1] = 0;
    payload[2] = 0;
    payload[3] = 0;
    
    // type (1 byte)
    payload[4] = VEHICLE_TYPE;
    
    // autopilot (1 byte)
    payload[5] = AUTOPILOT_TYPE;
    
    // base_mode (1 byte) - MAV_MODE_FLAG_CUSTOM_MODE_ENABLED
    payload[6] = 0x80;
    
    // system_status (1 byte) - MAV_STATE_ACTIVE
    payload[7] = 4;
    
    // mavlink_version (1 byte)
    payload[8] = 3;
    
    return mavlink_pack_message(MAVLINK_MSG_ID_HEARTBEAT, payload, 9, buffer, buf_size);
}

size_t mavlink_create_gps_position(uint8_t *buffer, size_t buf_size) {
    uint8_t payload[28];
    uint32_t time_boot_ms = get_time_boot_ms();
    
    // time_boot_ms (4 bytes)
    memcpy(&payload[0], &time_boot_ms, 4);
    
    // lat (4 bytes)
    memcpy(&payload[4], &g_drone_state.lat, 4);
    
    // lon (4 bytes)
    memcpy(&payload[8], &g_drone_state.lon, 4);
    
    // alt (4 bytes)
    memcpy(&payload[12], &g_drone_state.alt, 4);
    
    // relative_alt (4 bytes)
    int32_t relative_alt = 100000; // 100m
    memcpy(&payload[16], &relative_alt, 4);
    
    // vx, vy, vz (2 bytes each)
    memcpy(&payload[20], &g_drone_state.vx, 2);
    memcpy(&payload[22], &g_drone_state.vy, 2);
    memcpy(&payload[24], &g_drone_state.vz, 2);
    
    // hdg (2 bytes)
    memcpy(&payload[26], &g_drone_state.hdg, 2);
    
    return mavlink_pack_message(MAVLINK_MSG_ID_GLOBAL_POSITION_INT, payload, 28, buffer, buf_size);
}

size_t mavlink_create_attitude(uint8_t *buffer, size_t buf_size) {
    uint8_t payload[28];
    uint32_t time_boot_ms = get_time_boot_ms();
    float rollspeed = 0.0f;
    float pitchspeed = 0.0f;
    float yawspeed = 0.0f;
    
    // time_boot_ms (4 bytes)
    memcpy(&payload[0], &time_boot_ms, 4);
    
    // roll, pitch, yaw (4 bytes each)
    memcpy(&payload[4], &g_drone_state.roll, 4);
    memcpy(&payload[8], &g_drone_state.pitch, 4);
    memcpy(&payload[12], &g_drone_state.yaw, 4);
    
    // rollspeed, pitchspeed, yawspeed (4 bytes each)
    memcpy(&payload[16], &rollspeed, 4);
    memcpy(&payload[20], &pitchspeed, 4);
    memcpy(&payload[24], &yawspeed, 4);
    
    return mavlink_pack_message(MAVLINK_MSG_ID_ATTITUDE, payload, 28, buffer, buf_size);
}

size_t mavlink_create_gps_raw(uint8_t *buffer, size_t buf_size) {
    uint8_t payload[30];
    uint64_t time_usec = get_time_usec();
    uint16_t eph = 65535;
    uint16_t epv = 65535;
    uint16_t vel = 65535;
    uint16_t cog = 65535;
    uint32_t alt_ellipsoid = 0;
    
    // time_usec (8 bytes)
    memcpy(&payload[0], &time_usec, 8);
    
    // lat, lon, alt (4 bytes each)
    memcpy(&payload[8], &g_drone_state.lat, 4);
    memcpy(&payload[12], &g_drone_state.lon, 4);
    memcpy(&payload[16], &g_drone_state.alt, 4);
    
    // eph, epv, vel, cog (2 bytes each)
    memcpy(&payload[20], &eph, 2);
    memcpy(&payload[22], &epv, 2);
    memcpy(&payload[24], &vel, 2);
    memcpy(&payload[26], &cog, 2);
    
    // fix_type (1 byte)
    payload[28] = g_drone_state.fix_type;
    
    // satellites_visible (1 byte)
    payload[29] = g_drone_state.satellites_visible;
    
    return mavlink_pack_message(MAVLINK_MSG_ID_GPS_RAW_INT, payload, 30, buffer, buf_size);
}

size_t mavlink_create_sys_status(uint8_t *buffer, size_t buf_size) {
    uint8_t payload[31];
    uint32_t onboard_control_sensors = 0x3FFFFFFF;
    uint16_t load = 500;
    uint16_t voltage_battery = 12600; // 12.6V
    int16_t current_battery = -1;
    int8_t battery_remaining = 80;
    uint16_t drop_rate_comm = 0;
    uint16_t errors_comm = 0;
    uint16_t errors_count1 = 0;
    uint16_t errors_count2 = 0;
    uint16_t errors_count3 = 0;
    uint16_t errors_count4 = 0;
    
    // onboard_control_sensors_present (4 bytes)
    memcpy(&payload[0], &onboard_control_sensors, 4);
    
    // onboard_control_sensors_enabled (4 bytes)
    memcpy(&payload[4], &onboard_control_sensors, 4);
    
    // onboard_control_sensors_health (4 bytes)
    memcpy(&payload[8], &onboard_control_sensors, 4);
    
    // load (2 bytes)
    memcpy(&payload[12], &load, 2);
    
    // voltage_battery (2 bytes)
    memcpy(&payload[14], &voltage_battery, 2);
    
    // current_battery (2 bytes)
    memcpy(&payload[16], &current_battery, 2);
    
    // battery_remaining (1 byte)
    payload[18] = battery_remaining;
    
    // drop_rate_comm (2 bytes)
    memcpy(&payload[19], &drop_rate_comm, 2);
    
    // errors_comm (2 bytes)
    memcpy(&payload[21], &errors_comm, 2);
    
    // errors_count1-4 (2 bytes each)
    memcpy(&payload[23], &errors_count1, 2);
    memcpy(&payload[25], &errors_count2, 2);
    memcpy(&payload[27], &errors_count3, 2);
    memcpy(&payload[29], &errors_count4, 2);
    
    return mavlink_pack_message(MAVLINK_MSG_ID_SYS_STATUS, payload, 31, buffer, buf_size);
}

drone_state_t* mavlink_get_state(void) {
    return &g_drone_state;
}
