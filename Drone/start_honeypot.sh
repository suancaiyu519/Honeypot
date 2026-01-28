#!/bin/bash
# start_honeypot.sh - 无人机蜜罐统一启动脚本
# 一键启动 ArduPlane SITL + MAVLink 代理

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 加载配置
if [ -f "honeypot.conf" ]; then
    source honeypot.conf
else
    echo "❌ 错误: 未找到 honeypot.conf 配置文件"
    exit 1
fi

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 清理函数
cleanup() {
    echo ""
    echo -e "${YELLOW}正在停止蜜罐...${NC}"
    pkill -f "arduplane.*--home" 2>/dev/null || true
    pkill -f "drone_proxy" 2>/dev/null || true
    rm -f "$SCRIPT_DIR/.sitl.pid" "$SCRIPT_DIR/.proxy.pid"
    echo -e "${GREEN}蜜罐已停止${NC}"
    exit 0
}

trap cleanup SIGINT SIGTERM

echo -e "${BLUE}"
echo "╔════════════════════════════════════════════╗"
echo "║     🛩️  无人机蜜罐系统 v2.0                 ║"
echo "║     ArduPlane SITL + MAVLink Proxy         ║"
echo "╚════════════════════════════════════════════╝"
echo -e "${NC}"

# 检查组件
SITL_BIN="$SCRIPT_DIR/sitl/arduplane"
PROXY_BIN="$SCRIPT_DIR/drone_proxy"

if [ ! -f "$SITL_BIN" ]; then
    echo -e "${RED}❌ 错误: 未找到 SITL 可执行文件: $SITL_BIN${NC}"
    echo "请先复制 arduplane 到 sitl/ 目录"
    exit 1
fi

if [ ! -f "$PROXY_BIN" ]; then
    echo -e "${YELLOW}⚠️  代理程序未编译，正在编译...${NC}"
    make
fi

# 创建日志目录
mkdir -p "$LOG_DIR"
mkdir -p "$SCRIPT_DIR/sitl/logs"

# 清理旧进程
echo -e "${YELLOW}[1/4] 清理旧进程...${NC}"
pkill -f "arduplane.*--home" 2>/dev/null || true
pkill -f "drone_proxy" 2>/dev/null || true
sleep 1

# 构建SITL参数
SITL_ARGS="--model plane --home $HOME_LOCATION --speedup $SPEEDUP"
SITL_ARGS="$SITL_ARGS --defaults $SCRIPT_DIR/sitl/plane.parm"
SITL_ARGS="$SITL_ARGS -I0"

# 禁用日志
if [ "$DISABLE_SITL_LOG" = true ]; then
    echo "LOG_BACKEND_TYPE 0" > "$SCRIPT_DIR/sitl/no_log.parm"
    SITL_ARGS="$SITL_ARGS --defaults $SCRIPT_DIR/sitl/no_log.parm"
fi

# 启动 SITL (后台运行，输出到日志文件)
echo -e "${YELLOW}[2/4] 启动 ArduPlane SITL...${NC}"
echo "      位置: $HOME_LOCATION"
cd "$SCRIPT_DIR/sitl"
$SITL_BIN $SITL_ARGS > "$SCRIPT_DIR/sitl/sitl.log" 2>&1 &
SITL_PID=$!
echo "      PID: $SITL_PID"

# 等待 SITL 就绪
echo -e "${YELLOW}[3/4] 等待 SITL 就绪...${NC}"
sleep 3

# 检查 SITL 是否运行
if ! kill -0 $SITL_PID 2>/dev/null; then
    echo -e "${RED}❌ SITL 启动失败，查看日志: $SCRIPT_DIR/sitl/sitl.log${NC}"
    cat "$SCRIPT_DIR/sitl/sitl.log" | tail -20
    exit 1
fi
echo -e "${GREEN}      ✓ SITL 已就绪 (TCP:5760)${NC}"

# 保存PID
echo "$SITL_PID" > "$SCRIPT_DIR/.sitl.pid"

echo ""
echo -e "${GREEN}╔════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ 蜜罐系统已启动！                        ║${NC}"
echo -e "${GREEN}╠════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  SITL PID:  $SITL_PID                              ║${NC}"
echo -e "${GREEN}╠════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  🎯 QGC 连接地址: UDP localhost:14555      ║${NC}"
echo -e "${GREEN}║  📁 日志目录: $LOG_DIR                      ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${BLUE}提示: 按 Ctrl+C 停止蜜罐${NC}"
echo -e "${BLUE}提示: 查看日志: tail -f $LOG_DIR/drone_honeypot_*.json${NC}"
echo ""

# 启动代理 (前台运行，脚本会阻塞在这里)
echo -e "${YELLOW}[4/4] 启动 MAVLink 代理...${NC}"
cd "$SCRIPT_DIR"
exec $PROXY_BIN
