#!/bin/bash
# setup_sitl.sh - ArduPilot SITL安装脚本

set -e

echo "========================================"
echo "  ArduPilot SITL 安装脚本"
echo "========================================"
echo ""

# 检查系统
if [ ! -f /etc/os-release ]; then
    echo "[错误] 无法检测操作系统"
    exit 1
fi

source /etc/os-release

echo "[1/5] 检测操作系统: $PRETTY_NAME"

# 安装依赖
echo "[2/5] 安装依赖包..."
if [ "$ID" = "ubuntu" ] || [ "$ID" = "debian" ]; then
    sudo apt-get update -qq
    sudo apt-get install -y \
        git \
        python3 \
        python3-pip \
        python3-dev \
        python3-opencv \
        python3-wxgtk4.0 \
        python3-matplotlib \
        python-is-python3 \
        libxml2-dev \
        libxslt1-dev \
        build-essential \
        ccache \
        gawk \
        libtool \
        libncurses5-dev \
        autoconf \
        automake
else
    echo "[警告] 未知系统，请手动安装依赖"
fi

# 配置Git以提高克隆稳定性
echo "[3/5] 配置Git..."
git config --global http.postBuffer 524288000
git config --global http.lowSpeedLimit 0
git config --global http.lowSpeedTime 999999

# 克隆ArduPilot
ARDUPILOT_DIR="$HOME/ardupilot"
if [ ! -d "$ARDUPILOT_DIR" ]; then
    echo "[4/5] 克隆ArduPilot仓库（使用浅克隆加速）..."
    echo "提示：使用浅克隆可以显著减少下载时间和大小"
    
    # 使用浅克隆，只获取最近的提交（使用 master 分支以支持 Python 3.13）
    git clone --depth 1 https://github.com/ArduPilot/ardupilot.git "$ARDUPILOT_DIR"
    
    cd "$ARDUPILOT_DIR"
    
    echo "正在更新子模块..."
    git submodule update --init --recursive --depth 1
else
    echo "[4/5] ArduPilot已存在，跳过克隆"
    cd "$ARDUPILOT_DIR"
fi

# 安装Python依赖
echo "[5/5] 安装Python依赖..."
# Ubuntu 25.10+ 使用外部管理环境，需要添加 --break-system-packages
pip3 install --user --break-system-packages pymavlink MAVProxy empy==3.3.4

echo ""
echo "========================================"
echo "  安装完成！"
echo "========================================"
echo ""
echo "ArduPilot安装路径: $ARDUPILOT_DIR"
echo ""
echo "使用方法："
echo "  1. 启动SITL:"
echo "     cd $ARDUPILOT_DIR/ArduCopter"
echo "     ../Tools/autotest/sim_vehicle.py -L 北京 --map --console -I 0"
echo ""
echo "  或使用提供的启动脚本:"
echo "     ./scripts/start_sitl.sh"
echo ""
