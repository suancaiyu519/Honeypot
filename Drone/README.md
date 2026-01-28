# MAVLink代理蜜罐

基于C语言实现的MAVLink协议代理蜜罐，连接ArduPilot SITL，转发消息并记录所有外来交互到中文JSON日志。

## 功能特性

- ✅ **消息代理** - 透明转发SITL与客户端之间的MAVLink消息
- ✅ **中文日志** - 所有交互记录到标准中文JSON格式日志
- ✅ **低资源占用** - C语言实现，性能卓越
- ✅ **Docker部署** - 容器化部署，SITL和代理统一启动

## 系统架构

```
┌─────────────┐
│ QGC/客户端  │
└──────┬──────┘
       │ UDP:14550
       ▼
┌──────────────────┐
│   代理蜜罐        │
│  - 转发消息       │
│  - 记录日志       │
└──────┬───────────┘
       │ UDP:14551
       ▼
┌──────────────────┐
│  ArduPilot SITL  │
└──────────────────┘
```

## Docker部署（推荐）

### 启动服务

```bash
cd docker
docker-compose up -d
```

这将同时启动：
- ArduPilot SITL容器（内网端口14551）
- 代理蜜罐容器（对外端口14550）

### 查看日志

```bash
# 查看代理日志
docker-compose logs -f mavlink-proxy

# 查看SITL日志
docker-compose logs -f ardupilot-sitl

# 查看JSON日志
tail -f logs/drone_*.json
```

### 停止服务

```bash
docker-compose down
```

## 本地开发部署

### 1. 安装SITL

```bash
./scripts/setup_sitl.sh
```

### 2. 编译代理

```bash
make
```

### 3. 启动SITL

在一个终端中：
```bash
./scripts/start_sitl.sh
```

### 4. 启动代理

在另一个终端中：
```bash
./drone_proxy
```

## 测试连接

### 使用QGroundControl

1. 打开QGroundControl
2. 设置连接：UDP端口14550
3. 连接后可看到SITL的无人机状态

### 使用pymavlink测试

```python
from pymavlink import mavutil

# 连接代理
master = mavutil.mavlink_connection('udp:127.0.0.1:14550')

# 等待心跳
master.wait_heartbeat()
print("连接成功！")

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
  "时间": "2026-01-17 13:30:45",
  "事件类型": "新建连接",
  "来源IP": "192.168.1.100",
  "来源端口": 54321,
  "描述": "检测到新的客户端连接"
}
```

```json
{
  "时间": "2026-01-17 13:30:46",
  "事件类型": "心跳消息",
  "来源IP": "192.168.1.100",
  "来源端口": 54321,
  "消息信息": {
    "消息类型": "HEARTBEAT",
    "系统ID": 255,
    "组件ID": 0
  }
}
```

## 项目结构

```
Drone/
├── src/                    # C语言源代码
│   ├── proxy_main.c        # 代理主程序
│   ├── proxy.c             # 代理实现
│   ├── proxy.h             # 代理头文件
│   ├── mavlink.c           # MAVLink处理
│   ├── mavlink.h           # MAVLink头文件
│   ├── logger.c            # 日志记录
│   ├── logger.h            # 日志头文件
│   └── config.h            # 配置
├── lib/                    # 第三方库
│   ├── cJSON.c             # JSON库
│   └── cJSON.h             
├── docker/                 # Docker部署
│   ├── Dockerfile.proxy    # 代理容器
│   ├── Dockerfile.sitl     # SITL容器
│   └── docker-compose.yml  # 统一编排
├── scripts/                # 本地开发脚本
│   ├── setup_sitl.sh       # SITL安装
│   └── start_sitl.sh       # SITL启动
├── logs/                   # 日志目录
├── Makefile                # 编译配置
└── README.md               # 本文件
```

## 配置说明

### 代理配置（src/config.h）

- `PROXY_PORT` - 对外监听端口（默认14550）
- `SITL_PORT` - SITL连接端口（默认14551）
- `SITL_HOST` - SITL主机（默认127.0.0.1）

### SITL配置（docker-compose.yml）

- `SITL_LAT/LON/ALT` - 模拟位置坐标
- 默认位置：北京天安门（39.9042, 116.4074）

## 依赖项

### Docker部署
- Docker
- Docker Compose

### 本地开发
- GCC编译器
- Make工具
- ArduPilot SITL（通过setup_sitl.sh安装）

## 许可证

MIT License

## 作者

Drone Honeypot Project
