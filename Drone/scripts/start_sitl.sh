#!/bin/bash
# start_sitl.sh - 简易 SITL 启动器 (固定翼)

SITL_BIN="$HOME/ardupilot/build/sitl/bin/arduplane"

# 检查是否安装
if [ ! -f "$SITL_BIN" ]; then
    echo "❌ 错误: 未找到 ArduPlane 执行文件"
    echo "请先编译: cd ~/ardupilot && ./waf configure --board sitl && ./waf plane"
    exit 1
fi

echo "========================================"
echo "  启动 SITL - 固定翼模式"
echo "========================================"
echo "提示: 直接启动 ArduPlane 并等待 QGC 连接。"
echo ""

cd "$HOME/ardupilot/ArduPlane"

# 启动参数说明:
# --model plane     : 固定翼飞机
# --home ...        : 起始位置 (北京郊区)
# --speedup 1       : 1倍速
# --defaults ...    : 加载固定翼默认参数
# -I0               : 实例0

# 关键：SITL 默认监听 TCP 5760
# 最简单方案：直接运行，它会在 TCP 5760 等待连接

"$SITL_BIN" \
    --model plane \
    --home "39.5098,116.4105,30,0" \
    --speedup 1 \
    --defaults "$HOME/ardupilot/Tools/autotest/default_params/plane.parm,$HOME/Honeypot/Drone/sitl/plane.parm" \
    -I0

# 注意：这个命令会占用终端
