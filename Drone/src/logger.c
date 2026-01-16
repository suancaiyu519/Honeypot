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
    
    printf("[日志] 日志系统初始化成功，文件: %s\n", log_filename);
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
    cJSON_AddStringToObject(msg_info, "消息类型", "COMMAND");
    cJSON_AddNumberToObject(msg_info, "消息ID", msg->msgid);
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
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "时间", time_str);
    cJSON_AddStringToObject(json, "事件类型", "未知消息");
    cJSON_AddStringToObject(json, "来源IP", client->ip_str);
    cJSON_AddNumberToObject(json, "来源端口", client->port);
    
    cJSON *msg_info = cJSON_CreateObject();
    cJSON_AddNumberToObject(msg_info, "消息ID", msg->msgid);
    cJSON_AddNumberToObject(msg_info, "数据长度", msg->len);
    cJSON_AddItemToObject(json, "消息信息", msg_info);
    
    cJSON_AddStringToObject(json, "警告", "收到未识别的MAVLink消息");
    
    write_json_log(json);
    cJSON_Delete(json);
    
    printf("[未知] %s:%d | 消息ID=%d 长度=%d\n", 
           client->ip_str, client->port, msg->msgid, msg->len);
}

void logger_close(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
        printf("[日志] 日志系统已关闭\n");
    }
}
