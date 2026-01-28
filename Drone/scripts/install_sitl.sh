#!/bin/bash
# install_sitl.sh - 适用于 Ubuntu 25.10 的 ArduPilot 安装脚本

set -e

ARDUPILOT_DIR="$HOME/ardupilot"

echo "========================================"
echo "  ArduPilot SITL 安装程序"
echo "  目标系统: Ubuntu 25.10"
echo "========================================"
echo ""

# 1. 清理旧环境
if [ -d "$ARDUPILOT_DIR" ]; then
    echo "[1/4] 检测到旧的 ardupilot 目录，正在清理..."
    rm -rf "$ARDUPILOT_DIR"
    echo "✓ 已清理"
fi

# 2. 安装系统依赖
echo ""
echo "[2/4] 安装系统依赖 (需要sudo密码)..."
echo "使用系统包代替pip编译，以确安装速度和稳定性。"

sudo apt-get update -qq
sudo apt-get install -y \
    git \
    build-essential \
    python3-dev \
    python3-pip \
    python3-venv \
    python3-numpy \
    python3-opencv \
    python3-matplotlib \
    python3-wxgtk4.0 \
    libxml2-dev \
    libxslt1-dev

echo "✓ 依赖安装完成"

# 3. 克隆代码
echo ""
echo "[3/4] 下载 ArduPilot 代码..."
# 使用 --depth 1 进行浅克隆，速度最快
git clone --depth 1 --recurse-submodules https://github.com/ArduPilot/ardupilot.git "$ARDUPILOT_DIR"

echo "✓ 下载完成"

# 4. 配置与编译
echo ""
echo "[4/4] 编译 ArduCopter SITL..."
cd "$ARDUPILOT_DIR"

# 更新子模块（防止浅克隆遗漏）
git submodule update --init --recursive

# 安装必要的Python工具 (使用 --break-system-packages 因为这是专用环境)
# MAVProxy 使用 pip 安装，但因为我们已经安装了 python3-wxgtk4.0，它应该能复用或者跳过长时间编译
# 如果 pip 安装 MAVProxy 失败，我们将仅安装 empy 用于编译
pip3 install --user --break-system-packages empy==3.3.4 pexpect

# 配置
./waf configure --board sitl

# 编译
./waf copter

echo ""
echo "========================================"
echo "✅ 安装成功！"
echo "========================================"
echo "ArduPilot 位置: $HOME/ardupilot"
echo "编译好的文件: $HOME/ardupilot/build/sitl/bin/arducopter"
