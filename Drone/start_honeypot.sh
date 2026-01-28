#!/bin/bash
# start_honeypot.sh - 无人机蜜罐统一启动脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 加载配置
if [ -f "honeypot.conf" ]; then
    source honeypot.conf
else
    echo "错误: 未找到 honeypot.conf"
    exit 1
fi

# 清理函数
cleanup() {
    echo ""
    pkill -f "arduplane.*--home" 2>/dev/null || true
    pkill -f "drone_proxy" 2>/dev/null || true
    rm -f "$SCRIPT_DIR/.sitl.pid" "$SCRIPT_DIR/.proxy.pid"
    exit 0
}

trap cleanup SIGINT SIGTERM

# 检查组件
SITL_BIN="$SCRIPT_DIR/sitl/arduplane"
PROXY_BIN="$SCRIPT_DIR/drone_proxy"

if [ ! -f "$SITL_BIN" ]; then
    echo "错误: 未找到 SITL: $SITL_BIN"
    exit 1
fi

if [ ! -f "$PROXY_BIN" ]; then
    make -s
fi

# 创建目录
mkdir -p "$LOG_DIR" "$SCRIPT_DIR/sitl/logs"

# 清理旧进程
pkill -f "arduplane.*--home" 2>/dev/null || true
pkill -f "drone_proxy" 2>/dev/null || true
sleep 1

# 构建SITL参数
SITL_ARGS="--model plane --home $HOME_LOCATION --speedup $SPEEDUP"
SITL_ARGS="$SITL_ARGS --defaults $SCRIPT_DIR/sitl/plane.parm"
SITL_ARGS="$SITL_ARGS -I0"

if [ "$DISABLE_SITL_LOG" = true ]; then
    echo "LOG_BACKEND_TYPE 0" > "$SCRIPT_DIR/sitl/no_log.parm"
    SITL_ARGS="$SITL_ARGS --defaults $SCRIPT_DIR/sitl/no_log.parm"
fi

# 启动 SITL
cd "$SCRIPT_DIR/sitl"
$SITL_BIN $SITL_ARGS > "$SCRIPT_DIR/sitl/sitl.log" 2>&1 &
SITL_PID=$!

# 等待 SITL 就绪
sleep 3

if ! kill -0 $SITL_PID 2>/dev/null; then
    echo "SITL 启动失败"
    exit 1
fi

echo "$SITL_PID" > "$SCRIPT_DIR/.sitl.pid"

# 启动代理
cd "$SCRIPT_DIR"
exec $PROXY_BIN
