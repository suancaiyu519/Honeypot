# MAVLink无人机蜜罐 (C语言版)

基于C语言实现的高性能MAVLink协议无人机蜜罐系统，用于检测和记录针对无人机通信的攻击行为。

## 功能特性

- ✅ **MAVLink协议模拟** - 完整实现MAVLink v1.0协议，模拟四旋翼无人机
- ✅ **多消息类型支持** - 心跳、GPS位置、姿态、系统状态等
- ✅ **中文JSON日志** - 所有交互记录生成标准中文JSON格式日志
- ✅ **低资源占用** - C语言实现，性能卓越，资源占用极低
- ✅ **攻击检测** - 检测端口扫描、数据请求、命令注入等攻击行为
- ✅ **Docker部署** - 支持容器化部署

## 系统架构

```
┌─────────────┐
│ QGC/攻击者  │
└──────┬──────┘
       │ MAVLink (UDP:14550)
       ▼
┌──────────────────┐
│   蜜罐服务器       │
│  ┌────────────┐  │
│  │ 协议处理器  │  │
│  ├────────────┤  │
│  │ 命令响应器  │  │
│  ├────────────┤  │
│  │ 日志记录器  │  │
│  └────────────┘  │
└──────────────────┘
       │
       ▼
┌──────────────────┐
│  中文JSON日志     │
└──────────────────┘
```

## 快速开始

### 编译

```bash
# 编译蜜罐
make

# 或使用管理脚本
./manage.sh build
```

### 运行

```bash
# 直接运行
./drone_honeypot

# 或使用管理脚本（后台运行）
./manage.sh start
```

### 管理

```bash
# 查看状态
./manage.sh status

# 查看日志
./manage.sh logs

# 停止蜜罐
./manage.sh stop

# 重启蜜罐
./manage.sh restart
```

## 测试

### 使用QGroundControl连接

1. 启动蜜罐
2. 打开QGroundControl
3. 配置连接：UDP端口14550
4. 连接后可看到模拟的无人机状态

### 使用Python脚本测试

```python
from pymavlink import mavutil

# 连接蜜罐
master = mavutil.mavlink_connection('udp:127.0.0.1:14550')

# 等待心跳
master.wait_heartbeat()
print("收到心跳！")

# 请求数据流
master.mav.request_data_stream_send(
    master.target_system,
    master.target_component,
    mavutil.mavlink.MAV_DATA_STREAM_ALL,
    4,
    1
)
```

## 日志示例

```json
{
  "时间": "2026-01-16 19:30:45",
  "事件类型": "新建连接",
  "来源IP": "192.168.1.100",
  "来源端口": 54321,
  "描述": "检测到新的客户端连接"
}
```

```json
{
  "时间": "2026-01-16 19:30:46",
  "事件类型": "心跳消息",
  "来源IP": "192.168.1.100",
  "来源端口": 54321,
  "消息信息": {
    "消息类型": "HEARTBEAT",
    "飞行器类型": "四旋翼",
    "自驾仪类型": "ArduPilot"
  }
}
```

## 项目结构

```
Drone-v2/
├── src/
│   ├── main.c           # 主程序
│   ├── mavlink.c/h      # MAVLink协议处理
│   ├── network.c/h      # UDP网络服务
│   ├── logger.c/h       # 日志记录
│   └── config.h         # 配置定义
├── lib/
│   └── cJSON.c/h        # JSON库
├── logs/                # 日志目录
├── Makefile            # 编译配置
├── manage.sh           # 管理脚本
└── README.md           # 本文件
```

## 配置说明

在`src/config.h`中可修改以下配置：

- `UDP_PORT` - 监听端口（默认14550）
- `SYSTEM_ID` - 系统ID
- `VEHICLE_TYPE` - 飞行器类型
- `HOME_LAT/LON/ALT` - 模拟位置坐标

## 依赖项

- GCC编译器
- Make工具
- cJSON库（已包含）
- 标准C库

## Docker部署

```bash
# 构建镜像
docker build -t drone-honeypot -f docker/Dockerfile .

# 运行容器
docker run -d -p 14550:14550/udp --name drone-honeypot drone-honeypot
```

## 防御能力

本蜜罐可以检测和记录以下攻击：

1. **端口扫描** - 记录所有连接尝试
2. **数据侦察** - 记录数据流请求
3. **命令注入** - 记录控制命令尝试
4. **协议探测** - 记录异常MAVLink消息

## 许可证

MIT License

## 作者

Drone Honeypot Project
