#!/bin/bash
# start_sitl.sh - ArduPilot SITL启动脚本

ARDUPILOT_DIR="${ARDUPILOT_DIR:-$HOME/ardupilot}"

# 检查ArduPilot是否已安装
if [ ! -d "$ARDUPILOT_DIR" ]; then
    echo "[错误] ArduPilot未安装在: $ARDUPILOT_DIR"
    echo "请先运行: ./scripts/setup_sitl.sh"
    exit 1
fi

# 默认参数
HOME_LAT="${SITL_LAT:-39.9042}"   # 北京天安门
HOME_LON="${SITL_LON:-116.4074}"
HOME_ALT="${SITL_ALT:-100}"
HOME_HDG="${SITL_HDG:-0}"

# SITL端口
SITL_PORT="${SITL_PORT:-14551}"

echo "========================================"
echo "  启动ArduPilot SITL"
echo "========================================"
echo ""
echo "Home位置: $HOME_LAT,$HOME_LON,$HOME_ALT,$HOME_HDG"
echo "监听端口: $SITL_PORT"
echo ""

cd "$ARDUPILOT_DIR/ArduCopter" || exit 1

# 启动SITL
# -I 0: 实例0
# --console: 启用控制台
# --map: 启用地图
# --out: 设置MAVLink输出端口
../Tools/autotest/sim_vehicle.py \
    --vehicle=ArduCopter \
    --frame=quad \
    -L "$HOME_LAT,$HOME_LON,$HOME_ALT,$HOME_HDG" \
    -I 0 \
    --console \
    --map \
    --speedup=1 \
    --out=udp:127.0.0.1:$SITL_PORT

echo ""
echo "SITL已退出"
