#!/bin/bash
# ShellGuard 系统依赖安装脚本
# 用途: 一键安装运行 ShellGuard 所需的所有系统依赖

set -e

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== ShellGuard 系统依赖安装脚本 ===${NC}\n"

# 检查是否为 root 或有 sudo 权限
if [ "$EUID" -ne 0 ] && ! command -v sudo &> /dev/null; then
    echo -e "${RED}✗ 错误: 需要 root 权限或 sudo 命令${NC}"
    exit 1
fi

SUDO=""
if [ "$EUID" -ne 0 ]; then
    SUDO="sudo"
fi

# 1. 更新软件包列表
echo -e "${YELLOW}→${NC} 更新软件包列表..."
$SUDO apt update

# 2. 安装 Docker
if ! command -v docker &> /dev/null; then
    echo -e "${YELLOW}→${NC} 安装 Docker..."
    $SUDO apt install -y docker.io
    echo -e "${GREEN}✓${NC} Docker 已安装"
else
    echo -e "${GREEN}✓${NC} Docker 已经安装"
    docker --version
fi

# 3. 安装 Docker Buildx
if ! docker buildx version &> /dev/null; then
    echo -e "${YELLOW}→${NC} 安装 Docker Buildx..."
    $SUDO apt install -y docker-buildx
    echo -e "${GREEN}✓${NC} Docker Buildx 已安装"
else
    echo -e "${GREEN}✓${NC} Docker Buildx 已经安装"
    docker buildx version
fi

# 4. 安装 Docker Compose (如果未安装)
if ! docker compose version &> /dev/null; then
    echo -e "${YELLOW}→${NC} 安装 Docker Compose..."
    $SUDO apt install -y docker-compose-v2
    echo -e "${GREEN}✓${NC} Docker Compose 已安装"
else
    echo -e "${GREEN}✓${NC} Docker Compose 已经安装"
    docker compose version
fi

# 5. 安装 jq (用于格式化 JSON 日志，可选)
if ! command -v jq &> /dev/null; then
    echo -e "${YELLOW}→${NC} 安装 jq (JSON 格式化工具)..."
    $SUDO apt install -y jq
    echo -e "${GREEN}✓${NC} jq 已安装"
else
    echo -e "${GREEN}✓${NC} jq 已经安装"
fi

# 6. 启动 Docker 服务
echo -e "${YELLOW}→${NC} 确保 Docker 服务已启动..."
$SUDO systemctl start docker
$SUDO systemctl enable docker
echo -e "${GREEN}✓${NC} Docker 服务已启动并设置为开机自启"

# 7. 将当前用户添加到 docker 组（避免每次都用 sudo）
CURRENT_USER="${SUDO_USER:-$USER}"
if ! groups "$CURRENT_USER" | grep -q docker; then
    echo -e "${YELLOW}→${NC} 将用户 ${CURRENT_USER} 添加到 docker 组..."
    $SUDO usermod -aG docker "$CURRENT_USER"
    echo -e "${GREEN}✓${NC} 用户已添加到 docker 组"
    echo -e "${YELLOW}  注意: 需要重新登录或运行 'newgrp docker' 使权限生效${NC}"
else
    echo -e "${GREEN}✓${NC} 用户已在 docker 组中"
fi

echo -e "\n${GREEN}=== 所有依赖安装完成！===${NC}"
echo -e "\n${BLUE}已安装的组件:${NC}"
echo -e "  • Docker:         $(docker --version)"
echo -e "  • Docker Buildx:  $(docker buildx version)"
echo -e "  • Docker Compose: $(docker compose version)"
echo -e "  • jq:             $(jq --version)"

echo -e "\n${YELLOW}下一步:${NC}"
echo -e "  1. 如果是首次配置，运行: ${GREEN}newgrp docker${NC} 或重新登录"
echo -e "  2. 部署 ShellGuard: ${GREEN}./shellguard.sh deploy${NC}"
echo -e "  3. 查看帮助信息:   ${GREEN}./shellguard.sh help${NC}"
