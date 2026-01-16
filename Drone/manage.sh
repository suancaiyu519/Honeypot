#!/bin/bash

# MAVLink无人机蜜罐管理脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

HONEYPOT_BIN="./drone_honeypot"
PID_FILE="/tmp/drone_honeypot.pid"
LOG_DIR="./logs"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${GREEN}[信息]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[警告]${NC} $1"
}

print_error() {
    echo -e "${RED}[错误]${NC} $1"
}

# 检查是否正在运行
is_running() {
    if [ -f "$PID_FILE" ]; then
        pid=$(cat "$PID_FILE")
        if ps -p "$pid" > /dev/null 2>&1; then
            return 0
        else
            rm -f "$PID_FILE"
            return 1
        fi
    fi
    return 1
}

# 启动蜜罐
start() {
    if is_running; then
        print_warn "蜜罐已经在运行中"
        return 1
    fi
    
    if [ ! -f "$HONEYPOT_BIN" ]; then
        print_error "找不到可执行文件，请先编译: make"
        return 1
    fi
    
    print_info "启动无人机蜜罐..."
    mkdir -p "$LOG_DIR"
    
    nohup "$HONEYPOT_BIN" > /dev/null 2>&1 &
    echo $! > "$PID_FILE"
    
    sleep 1
    if is_running; then
        print_info "蜜罐启动成功！PID: $(cat $PID_FILE)"
        print_info "监听端口: 14550"
        return 0
    else
        print_error "蜜罐启动失败"
        rm -f "$PID_FILE"
        return 1
    fi
}

# 停止蜜罐
stop() {
    if ! is_running; then
        print_warn "蜜罐未在运行"
        return 1
    fi
    
    pid=$(cat "$PID_FILE")
    print_info "停止蜜罐 (PID: $pid)..."
    
    kill "$pid" 2>/dev/null
    sleep 1
    
    if is_running; then
        print_warn "进程未响应，强制终止..."
        kill -9 "$pid" 2>/dev/null
        sleep 1
    fi
    
    rm -f "$PID_FILE"
    print_info "蜜罐已停止"
    return 0
}

# 重启蜜罐
restart() {
    print_info "重启蜜罐..."
    stop
    sleep 1
    start
}

# 查看状态
status() {
    if is_running; then
        pid=$(cat "$PID_FILE")
        print_info "蜜罐正在运行"
        echo "  PID: $pid"
        echo "  端口: 14550"
        echo "  日志目录: $LOG_DIR"
        
        # 显示最新日志文件
        latest_log=$(ls -t "$LOG_DIR"/*.json 2>/dev/null | head -1)
        if [ -n "$latest_log" ]; then
            echo "  最新日志: $latest_log"
            echo "  日志行数: $(wc -l < "$latest_log")"
        fi
    else
        print_warn "蜜罐未在运行"
    fi
}

# 查看日志
logs() {
    if [ ! -d "$LOG_DIR" ]; then
        print_error "日志目录不存在"
        return 1
    fi
    
    latest_log=$(ls -t "$LOG_DIR"/*.json 2>/dev/null | head -1)
    if [ -z "$latest_log" ]; then
        print_warn "没有找到日志文件"
        return 1
    fi
    
    print_info "显示最新日志: $latest_log"
    echo "-----------------------------------"
    
    if command -v jq &> /dev/null; then
        # 如果安装了jq，格式化显示
        tail -20 "$latest_log" | jq -C '.'
    else
        # 否则直接显示
        tail -20 "$latest_log"
    fi
}

# 编译
build() {
    print_info "编译蜜罐..."
    make clean
    make
    
    if [ $? -eq 0 ]; then
        print_info "编译成功！"
        return 0
    else
        print_error "编译失败！"
        return 1
    fi
}

# 显示帮助
show_help() {
    cat << EOF
MAVLink无人机蜜罐管理脚本

用法: $0 {start|stop|restart|status|logs|build}

命令:
  start     - 启动蜜罐
  stop      - 停止蜜罐
  restart   - 重启蜜罐
  status    - 查看运行状态
  logs      - 查看最新日志
  build     - 编译蜜罐

EOF
}

# 主逻辑
case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        restart
        ;;
    status)
        status
        ;;
    logs)
        logs
        ;;
    build)
        build
        ;;
    *)
        show_help
        exit 1
        ;;
esac

exit $?
