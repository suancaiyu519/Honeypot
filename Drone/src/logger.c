/*
 * logger.c - 中文日志记录实现
 */

#include "logger.h"
#include "config.h"
#include "../lib/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

static FILE *log_file = NULL;

/* 获取当前时间字符串 */
static void get_current_time(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* 获取日志文件名 */
static void get_log_filename(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%Y%m%d", tm_info);
    snprintf(buffer, size, "%s/%s_%s.json", LOG_DIR, LOG_FILE_PREFIX, date_str);
}

/* 写入JSON日志 */
static void write_json_log(cJSON *json_obj) {
    if (!log_file) {
        return;
    }
    
    char *json_str = cJSON_PrintUnformatted(json_obj);
    if (json_str) {
        fprintf(log_file, "%s\n", json_str);
        fflush(log_file);
        free(json_str);
    }
}

int logger_init(void) {
    // 创建日志目录
    mkdir(LOG_DIR, 0755);
    
    // 打开日志文件
    char log_filename[256];
    get_log_filename(log_filename, sizeof(log_filename));
    
    log_file = fopen(log_filename, "a");
    if (!log_file) {
        perror("无法打开日志文件");
        return -1;
    }
    
    printf("日志系统初始化成功，文件: %s\n", log_filename);
    return 0;
}

void logger_connection(const client_info_t *client) {
    char time_str[64];
    get_current_time(time_str, sizeof(time_str));
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "时间", time_str);
    cJSON_AddStringToObject(json, "事件类型", "新建连接");
    cJSON_AddStringToObject(json, "来源IP", client->ip_str);
    cJSON_AddNumberToObject(json, "来源端口", client->port);
    cJSON_AddStringToObject(json, "描述", "检测到新的客户端连接");
    
    write_json_log(json);
    cJSON_Delete(json);
    
    printf("[连接] %s:%d\n", client->ip_str, client->port);
}

void logger_heartbeat(const client_info_t *client, const mavlink_message_t *msg) {
    char time_str[64];
    get_current_time(time_str, sizeof(time_str));
    
    // 解析心跳载荷
    uint8_t vehicle_type = msg->payload[4];
    uint8_t autopilot = msg->payload[5];
    
    const char *vehicle_type_str = "未知";
    switch (vehicle_type) {
        case 0: vehicle_type_str = "通用型"; break;
        case 1: vehicle_type_str = "固定翼"; break;
        case 2: vehicle_type_str = "四旋翼"; break;
        case 3: vehicle_type_str = "共轴helicopter"; break;
        case 4: vehicle_type_str = "直升机"; break;
        case 5: vehicle_type_str = "天线追踪器"; break;
        case 6: vehicle_type_str = "地面站"; break;
        case 7: vehicle_type_str = "飞艇"; break;
    }
    
    const char *autopilot_str = "未知";
    switch (autopilot) {
        case 0: autopilot_str = "通用型"; break;
        case 3: autopilot_str = "ArduPilot"; break;
        case 4: autopilot_str = "OpenPilot"; break;
        case 12: autopilot_str = "PX4"; break;
    }
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "时间", time_str);
    cJSON_AddStringToObject(json, "事件类型", "心跳消息");
    cJSON_AddStringToObject(json, "来源IP", client->ip_str);
    cJSON_AddNumberToObject(json, "来源端口", client->port);
    
    cJSON *msg_info = cJSON_CreateObject();
    cJSON_AddStringToObject(msg_info, "消息类型", "HEARTBEAT");
    cJSON_AddStringToObject(msg_info, "飞行器类型", vehicle_type_str);
    cJSON_AddStringToObject(msg_info, "自驾仪类型", autopilot_str);
    cJSON_AddItemToObject(json, "消息信息", msg_info);
    
    write_json_log(json);
    cJSON_Delete(json);
    
    printf("[心跳] %s:%d | %s | %s\n", client->ip_str, client->port, 
           vehicle_type_str, autopilot_str);
}

void logger_command(const client_info_t *client, const mavlink_message_t *msg) {
    char time_str[64];
    get_current_time(time_str, sizeof(time_str));
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "时间", time_str);
    cJSON_AddStringToObject(json, "事件类型", "命令接收");
    cJSON_AddStringToObject(json, "来源IP", client->ip_str);
    cJSON_AddNumberToObject(json, "来源端口", client->port);
    
    cJSON *msg_info = cJSON_CreateObject();
    
    // 解析命令payload
    if (msg->msgid == 76) { // COMMAND_LONG
        cJSON_AddStringToObject(msg_info, "消息类型", "COMMAND_LONG");
        
        // COMMAND_LONG payload布局（MAVLink字段按大小排序）：
        // 0-3: param1 (float)
        // 4-7: param2 (float)
        // 8-11: param3 (float)
        // 12-15: param4 (float)
        // 16-19: param5 (float)
        // 20-23: param6 (float)
        // 24-27: param7 (float)
        // 28-29: command (uint16)
        // 30: target_system (uint8)
        // 31: target_component (uint8)
        // 32: confirmation (uint8)
        
        // 提取命令ID (字节28-29, little-endian uint16)
        uint16_t command = msg->payload[28] | (msg->payload[29] << 8);
        cJSON_AddNumberToObject(msg_info, "命令ID", command);
        
        // 解析命令类型
        const char *cmd_name = "未知命令";
        switch (command) {
            case 16: cmd_name = "导航至航点"; break;
            case 20: cmd_name = "返回起飞点(RTL)"; break;
            case 21: cmd_name = "降落"; break;
            case 22: cmd_name = "起飞"; break;
            case 84: cmd_name = "执行任务"; break;
            case 176: cmd_name = "设置飞行模式"; break;
            case 179: cmd_name = "设置引导位置(旧版)"; break;
            case 192: cmd_name = "设置引导位置"; break;
            case 241: cmd_name = "传感器校准"; break;
            case 400: cmd_name = "解锁/锁定"; break;
            case 410: cmd_name = "获取Home位置"; break;
            case 500: cmd_name = "请求自动驾驶仪能力"; break;
            case 511: cmd_name = "请求消息"; break;
            case 512: cmd_name = "请求自动驾驶仪能力"; break;
            case 519: cmd_name = "请求飞行信息"; break;
            case 520: cmd_name = "开始接收消息"; break;
            case 521: cmd_name = "请求特定消息"; break;
            case 2500: cmd_name = "请求相机信息"; break;
            case 2510: cmd_name = "请求相机设置"; break;
            case 2800: cmd_name = "全景拍照"; break;
        }
        cJSON_AddStringToObject(msg_info, "命令名称", cmd_name);
        
        // 提取参数 (param1-7, 每个4字节float)
        cJSON *params = cJSON_CreateObject();
        float param1, param2, param3, param4, param5, param6, param7;
        memcpy(&param1, &msg->payload[0], 4);
        memcpy(&param2, &msg->payload[4], 4);
        memcpy(&param3, &msg->payload[8], 4);
        memcpy(&param4, &msg->payload[12], 4);
        memcpy(&param5, &msg->payload[16], 4);
        memcpy(&param6, &msg->payload[20], 4);
        memcpy(&param7, &msg->payload[24], 4);
        
        // 根据命令类型解析参数
        if (command == 400) { // ARM/DISARM
            cJSON_AddStringToObject(params, "动作", param1 == 1.0 ? "解锁" : "锁定");
        } else if (command == 176) { // SET_MODE
            int custom_mode = (int)param2;
            cJSON_AddNumberToObject(params, "自定义模式值", custom_mode);
            
            // 解析ArduPilot自定义飞行模式
            const char *mode_name = "未知模式";
            switch (custom_mode) {
                case 0: mode_name = "稳定模式(Stabilize)"; break;
                case 1: mode_name = "特技模式(Acro)"; break;
                case 2: mode_name = "高度保持(AltHold)"; break;
                case 3: mode_name = "自动模式(Auto)"; break;
                case 4: mode_name = "引导模式(Guided)"; break;
                case 5: mode_name = "留待模式(Loiter)"; break;
                case 6: mode_name = "返航模式(RTL)"; break;
                case 7: mode_name = "绕圈模式(Circle)"; break;
                case 9: mode_name = "降落模式(Land)"; break;
                case 11: mode_name = "漂移模式(Drift)"; break;
                case 13: mode_name = "运动模式(Sport)"; break;
                case 14: mode_name = "翻转模式(Flip)"; break;
                case 15: mode_name = "自动调参(AutoTune)"; break;
                case 16: mode_name = "位置保持(PosHold)"; break;
                case 17: mode_name = "刹车模式(Brake)"; break;
                case 18: mode_name = "抛投模式(Throw)"; break;
                case 19: mode_name = "避障模式(AvoidADSB)"; break;
                case 20: mode_name = "引导无GPS(GuidedNoGPS)"; break;
                case 21: mode_name = "智能RTL(SmartRTL)"; break;
                case 22: mode_name = "流体保持(FlowHold)"; break;
                case 23: mode_name = "跟随模式(Follow)"; break;
                case 24: mode_name = "Z字形自动(ZigZag)"; break;
            }
            cJSON_AddStringToObject(params, "飞行模式", mode_name);
        } else if (command == 22) { // TAKEOFF
            if (param7 != 0.0f) {
                cJSON_AddNumberToObject(params, "目标高度(米)", param7);
            }
        } else if (command == 21) { // LAND
            cJSON_AddStringToObject(params, "动作", "执行降落");
        } else if (command == 20) { // RTL
            cJSON_AddStringToObject(params, "动作", "返回起飞点");
        } else if (command == 511 || command == 521) { // REQUEST_MESSAGE
            int msg_id = (int)param1;
            cJSON_AddNumberToObject(params, "请求消息ID", msg_id);
            
            // 解析常见消息ID - 完整映射表
            const char *msg_name = NULL;
            switch (msg_id) {
                // 系统信息类
                case 0: msg_name = "心跳消息"; break;
                case 1: msg_name = "系统状态"; break;
                case 2: msg_name = "系统时间"; break;
                case 148: msg_name = "自动驾驶仪版本"; break;
                case 245: msg_name = "扩展系统状态"; break;
                case 259: msg_name = "自动驾驶仪版本(扩展)"; break;
                
                // GPS和位置类
                case 24: msg_name = "GPS原始数据"; break;
                case 25: msg_name = "GPS卫星状态"; break;
                case 33: msg_name = "全局位置(经纬度)"; break;
                case 32: msg_name = "本地位置(NED坐标)"; break;
                case 242: msg_name = "返航位置"; break;
                
                // 姿态和运动类
                case 30: msg_name = "姿态信息(欧拉角)"; break;
                case 31: msg_name = "姿态四元数"; break;
                case 74: msg_name = "VFR HUD数据"; break;
                
                // 电池和动力类
                case 147: msg_name = "电池状态"; break;
                case 125: msg_name = "动力状态"; break;
                
                // 传感器类
                case 27: msg_name = "原始IMU数据"; break;
                case 28: msg_name = "缩放IMU数据"; break;
                case 29: msg_name = "原始压力数据"; break;
                case 65: msg_name = "遥控器通道"; break;
                case 134: msg_name = "地形数据"; break;
                
                // 任务和导航类
                case 42: msg_name = "任务当前项"; break;
                case 62: msg_name = "导航控制器输出"; break;
                
                // 参数类
                case 22: msg_name = "参数值"; break;
                case 23: msg_name = "参数设置"; break;
                
                // 相机和云台类
                case 260: msg_name = "相机图像捕获"; break;
                case 261: msg_name = "相机设置"; break;
                case 262: msg_name = "存储信息"; break;
                
                // 其他
                case 241: msg_name = "振动信息"; break;
                case 253: msg_name = "状态文本"; break;
                
                default: msg_name = "其他消息"; break;
            }
            
            cJSON_AddStringToObject(params, "消息名称", msg_name);
            
            // 添加消息类别
            const char *category = "其他";
            if (msg_id == 0) category = "连接管理";
            else if (msg_id >= 1 && msg_id <= 2) category = "系统状态";
            else if (msg_id == 245) category = "系统状态";
            else if (msg_id >= 24 && msg_id <= 33) category = "位置导航";
            else if (msg_id == 242) category = "位置导航";
            else if (msg_id >= 27 && msg_id <= 31) category = "姿态运动";
            else if (msg_id == 147 || msg_id == 125) category = "动力系统";
            else if (msg_id >= 260 && msg_id <= 262) category = "载荷设备";
            else if (msg_id == 148 || msg_id == 259) category = "版本信息";
            else if (msg_id == 134) category = "地形数据";
            
            cJSON_AddStringToObject(params, "消息类别", category);
        } else {
            // 只显示非零参数
            if (param1 != 0.0f) cJSON_AddNumberToObject(params, "参数1", param1);
            if (param2 != 0.0f) cJSON_AddNumberToObject(params, "参数2", param2);
            if (param3 != 0.0f) cJSON_AddNumberToObject(params, "参数3", param3);
            if (param4 != 0.0f) cJSON_AddNumberToObject(params, "参数4", param4);
            if (param5 != 0.0f) cJSON_AddNumberToObject(params, "参数5", param5);
            if (param6 != 0.0f) cJSON_AddNumberToObject(params, "参数6", param6);
            if (param7 != 0.0f) cJSON_AddNumberToObject(params, "参数7", param7);
        }
        cJSON_AddItemToObject(msg_info, "参数", params);
        
    } else if (msg->msgid == 75) { // COMMAND_INT
        cJSON_AddStringToObject(msg_info, "消息类型", "COMMAND_INT");
        
        // COMMAND_INT payload布局：
        // 0-3: param1 (float)
        // 4-7: param2 (float)
        // 8-11: param3 (float)
        // 12-15: param4 (float)
        // 16-19: x (int32) - 纬度
        // 20-23: y (int32) - 经度
        // 24-27: z (float) - 高度
        // 28-29: command (uint16)
        // 30: target_system (uint8)
        // 31: target_component (uint8)
        // 32: frame (uint8)
        // 33: current (uint8)
        // 34: autocontinue (uint8)
        
        // 提取命令ID
        uint16_t command = msg->payload[28] | (msg->payload[29] << 8);
        cJSON_AddNumberToObject(msg_info, "命令ID", command);
        
        const char *cmd_name = "未知命令";
        switch (command) {
            case 16: cmd_name = "导航至航点"; break;
            case 80: cmd_name = "设置ROI位置"; break;
            case 81: cmd_name = "ROI跟随"; break;
            case 192: cmd_name = "设置引导位置"; break;
            case 195: cmd_name = "设置引导位置(扩展)"; break;
        }
        cJSON_AddStringToObject(msg_info, "命令名称", cmd_name);
        
        // 提取坐标参数
        int32_t x, y;
        float z;
        memcpy(&x, &msg->payload[16], 4);
        memcpy(&y, &msg->payload[20], 4);
        memcpy(&z, &msg->payload[24], 4);
        
        cJSON *params = cJSON_CreateObject();
        cJSON_AddNumberToObject(params, "纬度", x / 1e7);
        cJSON_AddNumberToObject(params, "经度", y / 1e7);
        cJSON_AddNumberToObject(params, "高度", z);
        cJSON_AddItemToObject(msg_info, "参数", params);
    }
    
    cJSON_AddItemToObject(json, "消息信息", msg_info);
    cJSON_AddStringToObject(json, "警告", "检测到控制命令尝试");
    
    write_json_log(json);
    cJSON_Delete(json);
    
    printf("[命令] %s:%d | 消息ID=%d\n", client->ip_str, client->port, msg->msgid);
}

void logger_request(const client_info_t *client, const mavlink_message_t *msg) {
    char time_str[64];
    get_current_time(time_str, sizeof(time_str));
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "时间", time_str);
    cJSON_AddStringToObject(json, "事件类型", "数据请求");
    cJSON_AddStringToObject(json, "来源IP", client->ip_str);
    cJSON_AddNumberToObject(json, "来源端口", client->port);
    
    cJSON *msg_info = cJSON_CreateObject();
    cJSON_AddStringToObject(msg_info, "消息类型", "REQUEST_DATA");
    cJSON_AddNumberToObject(msg_info, "消息ID", msg->msgid);
    cJSON_AddItemToObject(json, "消息信息", msg_info);
    
    write_json_log(json);
    cJSON_Delete(json);
    
    printf("[请求] %s:%d | 消息ID=%d\n", client->ip_str, client->port, msg->msgid);
}

void logger_unknown(const client_info_t *client, const mavlink_message_t *msg) {
    char time_str[64];
    get_current_time(time_str, sizeof(time_str));
    
    // 获取消息名称
    const char *msg_name = "未知消息";
    const char *event_type = "其他消息";
    
    switch (msg->msgid) {
        case 2: msg_name = "系统时间"; event_type = "系统信息"; break;
        case 39: msg_name = "任务项"; event_type = "任务操作"; break;
        case 43: msg_name = "任务请求"; event_type = "任务操作"; break;
        case 44: msg_name = "任务设置当前"; event_type = "任务操作"; break;
        case 47: msg_name = "任务计数"; event_type = "任务操作"; break;
        case 51: msg_name = "任务开始"; event_type = "任务操作"; break;
        case 66: msg_name = "数据流请求"; event_type = "数据请求"; break;
        case 84: msg_name = "设置本地位置目标"; event_type = "位置控制"; break;
        case 86: msg_name = "设置全局位置目标"; event_type = "位置控制"; break;
        case 110: msg_name = "时间同步"; event_type = "系统信息"; break;
        case 111: msg_name = "时间同步响应"; event_type = "系统信息"; break;
        case 134: msg_name = "地形数据"; event_type = "地形信息"; break;
        case 148: msg_name = "协议版本"; event_type = "系统信息"; break;
        default:
            msg_name = "未识别消息";
            event_type = "未知消息";
            break;
    }
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "时间", time_str);
    cJSON_AddStringToObject(json, "事件类型", event_type);
    cJSON_AddStringToObject(json, "来源IP", client->ip_str);
    cJSON_AddNumberToObject(json, "来源端口", client->port);
    
    cJSON *msg_info = cJSON_CreateObject();
    cJSON_AddNumberToObject(msg_info, "消息ID", msg->msgid);
    cJSON_AddStringToObject(msg_info, "消息名称", msg_name);
    cJSON_AddNumberToObject(msg_info, "数据长度", msg->len);
    cJSON_AddItemToObject(json, "消息信息", msg_info);
    
    write_json_log(json);
    cJSON_Delete(json);
    
    printf("[%s] %s:%d | %s (ID=%d)\n", 
           event_type, client->ip_str, client->port, msg_name, msg->msgid);
}

void logger_close(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
        printf("日志系统已关闭\n");
    }
}
