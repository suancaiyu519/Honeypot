# 混合架构MAVLink代理蜜罐使用指南

## 架构概览

```
客户端 (QGroundControl)
         ↓
    UDP :14550
         ↓
┌─────────────────────┐
│   MAVLink代理        │ ← 监控、记录、分析
│   (C语言实现)        │
└──────────┬──────────┘
           ↓
    UDP :14551 (内部)
           ↓
┌──────────────────────┐
│  ArduPilot SITL      │ ← 真实飞控模拟
│  (Copter 4.3)        │
└──────────────────────┘
```

## 快速开始

### 方案1：Docker部署（推荐）

```bash
# 1. 构建并启动服务
cd docker
docker-compose up -d

# 2. 查看日志
docker-compose logs -f

# 3. 停止服务
docker-compose down
```

### 方案2：本地部署

#### 步骤1：安装ArduPilot SITL

```bash
# 运行安装脚本
./scripts/setup_sitl.sh
```

#### 步骤2：编译代理程序

```bash
# 编译
make

# 这将生成两个程序：
# - drone_honeypot：原始蜜罐（独立模式）
# - drone_proxy：代理模式
```

#### 步骤3：启动服务

**终端1 - 启动SITL：**
```bash
./scripts/start_sitl.sh
```

**终端2 - 启动代理：**
```bash
./drone_proxy
```

## 连接QGroundControl

1. 启动QGroundControl
2. 配置连接：
   - 类型：UDP
   - 端口：14550
3. 点击连接

## 查看日志

### 中文JSON日志
```bash
# 查看最新日志
tail -f logs/interactions_$(date +%Y%m%d).json

# 格式化查看
cat logs/interactions_*.json | jq '.'
```

### 流量统计
代理会实时显示流量统计：
- 客户端 → SITL：字节数、消息数
- SITL → 客户端：字节数、消息数

## 配置选项

### 修改Home位置

编辑 `scripts/start_sitl.sh`：
```bash
HOME_LAT="39.9042"  # 纬度
HOME_LON="116.4074" # 经度
HOME_ALT="100"      # 海拔(米)
HOME_HDG="0"        # 航向(度)
```

### 修改端口

编辑 `src/proxy.h`：
```c
#define PROXY_EXTERNAL_PORT 14550  // 外部端口
#define PROXY_INTERNAL_PORT 14551  // 内部端口
```

## 优势

✅ **完全真实** - ArduPilot SITL提供真实的MAVLink协议支持
✅ **所有参数** - 支持数百个ArduPilot参数，不再有"缺少参数"错误
✅ **完整监控** - 代理层记录所有双向流量
✅ **中文日志** - 保留原有的中文日志系统
✅ **易于部署** - Docker一键启动

## 故障排除

### SITL无法启动
```bash
# 检查Python依赖
pip3 install pymavlink MAVProxy

# 手动启动测试
cd ~/ardupilot/ArduCopter
../Tools/autotest/sim_vehicle.py --help
```

### 代理无法连接SITL
```bash
# 检查SITL是否运行在正确端口
netstat -an | grep 14551

# 检查防火墙
sudo ufw allow 14550/udp
sudo ufw allow 14551/udp
```

### QGroundControl无法连接
```bash
# 确保代理正在运行
ps aux | grep drone_proxy

# 测试端口
nc -u -l 14550
```

## 文件说明

- `src/proxy.c` - 代理核心实现
- `src/proxy.h` - 代理接口定义
- `src/proxy_main.c` - 代理主程序
- `scripts/setup_sitl.sh` - SITL安装脚本
- `scripts/start_sitl.sh` - SITL启动脚本
- `docker/Dockerfile.sitl` - SITL Docker镜像
- `docker/Dockerfile.proxy` - 代理Docker镜像
- `docker/docker-compose.yml` - Docker编排配置
