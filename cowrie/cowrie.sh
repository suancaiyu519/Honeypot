#!/bin/bash
# Cowrie Docker 管理脚本
# 用途: 统一管理 Cowrie 蜜罐容器的部署、启动、停止、日志等操作

set -e

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COWRIE_DIR="${SCRIPT_DIR}"
DOCKER_DIR="${COWRIE_DIR}/docker"

# 显示使用帮助
show_help() {
    echo -e "${BLUE}Cowrie Docker 管理脚本${NC}\n"
    echo "用法: $0 <命令> [选项]"
    echo ""
    echo "可用命令:"
    echo -e "  ${GREEN}deploy${NC}     部署并启动 Cowrie"
    echo -e "  ${GREEN}start${NC}      启动 Cowrie 容器"
    echo -e "  ${GREEN}stop${NC}       停止 Cowrie 容器"
    echo -e "  ${GREEN}restart${NC}    重启 Cowrie 容器"
    echo -e "  ${GREEN}status${NC}     查看容器状态"
    echo -e "  ${GREEN}logs${NC}       查看实时日志"
    echo -e "  ${GREEN}logs-tail${NC}  查看最近日志"
    echo -e "  ${GREEN}jsons${NC}    查看JSON日志 (实时)"
    echo -e "  ${GREEN}jsons-tail${NC} 查看JSON日志 (最近N条)"
    echo -e "  ${GREEN}down${NC}       删除容器"
    echo -e "  ${GREEN}down-all${NC}   删除数据卷"
    echo -e "  ${GREEN}shell${NC}      进入容器 shell"
    echo -e "  ${GREEN}add-user${NC}   配置用户 (用法: add-user <username> <password>)"
    echo -e "  ${GREEN}help${NC}       显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0 deploy      # 首次部署"
    echo "  $0 logs        # 查看实时日志"
    echo "  $0 stop        # 停止服务"
    echo "  $0 add-user    # 设置默认用户"
    echo ""
}

# 检查 Docker 环境
check_docker() {
    if ! command -v docker &> /dev/null; then
        echo -e "${RED}✗ 错误: Docker 未安装${NC}"
        exit 1
    fi
    
    if ! docker compose version &> /dev/null; then
        echo -e "${RED}✗ 错误: Docker Compose 不可用${NC}"
        exit 1
    fi
}

# 检查目录是否存在
check_directory() {
    if [ ! -d "${DOCKER_DIR}" ]; then
        echo -e "${RED}✗ 错误: 找不到 Docker 目录: ${DOCKER_DIR}${NC}"
        echo -e "${YELLOW}提示: 请确保已克隆 Cowrie 仓库到 ${COWRIE_DIR}${NC}"
        exit 1
    fi
}

# 部署函数
deploy() {
    echo -e "${BLUE}=== Cowrie 部署 ===${NC}\n"
    
    check_docker
    check_directory
    
    echo -e "${GREEN}✓${NC} Docker 环境检查通过"
    echo -e "${YELLOW}→${NC} 切换到目录: ${DOCKER_DIR}\n"
    
    cd "${DOCKER_DIR}"
    
    echo -e "${YELLOW}→${NC} 开始构建和启动 Cowrie 容器..."
    echo -e "${YELLOW}  提示: 首次构建可能需要几分钟...${NC}\n"
    
    docker compose up -d --build
    
    echo -e "\n${YELLOW}→${NC} 等待容器启动..."
    sleep 3
    
    echo -e "\n${GREEN}=== 容器状态 ===${NC}"
    docker compose ps
    
    echo -e "\n${GREEN}=== 最近日志 ===${NC}"
    docker compose logs --tail=20
    
    echo -e "\n${GREEN}✓ 部署完成！${NC}"
    echo -e "\n${YELLOW}测试连接:${NC}"
    echo -e "  SSH:    ${GREEN}ssh -p 2222 root@localhost${NC}"
    echo -e "  Telnet: ${GREEN}telnet localhost 2223${NC}"
}

# 启动函数
start() {
    echo -e "${YELLOW}→${NC} 启动 Cowrie 容器..."
    check_directory
    cd "${DOCKER_DIR}"
    docker compose start
    echo -e "${GREEN}✓${NC} Cowrie 已启动"
    docker compose ps
}

# 停止函数
stop() {
    echo -e "${YELLOW}→${NC} 停止 Cowrie 容器..."
    check_directory
    cd "${DOCKER_DIR}"
    docker compose stop
    echo -e "${GREEN}✓${NC} Cowrie 已停止"
    docker compose ps
}

# 重启函数
restart() {
    echo -e "${YELLOW}→${NC} 重启 Cowrie 容器..."
    check_directory
    cd "${DOCKER_DIR}"
    docker compose restart
    echo -e "${GREEN}✓${NC} Cowrie 已重启"
    docker compose ps
}

# 查看状态
status() {
    echo -e "${BLUE}=== Cowrie 容器状态 ===${NC}"
    check_directory
    cd "${DOCKER_DIR}"
    docker compose ps
    
    echo -e "\n${BLUE}=== 资源使用情况 ===${NC}"
    docker stats --no-stream $(docker compose ps -q) 2>/dev/null || echo "容器未运行"
}

# 查看实时日志
logs() {
    echo -e "${YELLOW}→${NC} 实时查看 Cowrie 日志 (按 Ctrl+C 退出)..."
    check_directory
    cd "${DOCKER_DIR}"
    docker compose logs -f cowrie
}

# 查看最近日志
logs_tail() {
    local lines=${1:-50}
    echo -e "${BLUE}=== 最近 ${lines} 行日志 ===${NC}"
    check_directory
    cd "${DOCKER_DIR}"
    docker compose logs --tail=${lines} cowrie
}

# 查看中文JSON日志（实时）
jsons() {
    echo -e "${YELLOW}→${NC} 实时查看中文 JSON 日志 (按 Ctrl+C 退出)..."
    check_directory
    
    # 获取 volume 挂载路径
    VOLUME_PATH=$(docker volume inspect docker_cowrie-var --format '{{ .Mountpoint }}' 2>/dev/null)
    
    if [ -z "$VOLUME_PATH" ]; then
        echo -e "${RED}✗ 错误: 无法获取 volume 路径${NC}"
        exit 1
    fi
    
    LOG_FILE="$VOLUME_PATH/log/cowrie/cowrie_chinese.json"
    
    if ! sudo test -f "$LOG_FILE"; then
        echo -e "${RED}✗ 错误: 中文日志文件不存在${NC}"
        echo -e "${YELLOW}提示: 请先触发一些SSH连接以生成日志${NC}"
        exit 1
    fi
    
    # 检查是否安装了 jq
    if command -v jq &> /dev/null; then
        echo -e "${GREEN}使用 jq 格式化显示${NC}"
        sudo tail -f "$LOG_FILE" | while read -r line; do
            echo "$line" | jq -C .
        done
    else
        echo -e "${YELLOW}提示: 安装 jq 可以获得更好的显示效果 (sudo apt install jq)${NC}"
        sudo tail -f "$LOG_FILE"
    fi
}

# 查看中文JSON日志（最近N条）
jsons_tail() {
    local lines=${1:-10}
    echo -e "${BLUE}=== 最近 ${lines} 条中文 JSON 日志 ===${NC}"
    check_directory
    
    # 获取 volume 挂载路径
    VOLUME_PATH=$(docker volume inspect docker_cowrie-var --format '{{ .Mountpoint }}' 2>/dev/null)
    
    if [ -z "$VOLUME_PATH" ]; then
        echo -e "${RED}✗ 错误: 无法获取 volume 路径${NC}"
        exit 1
    fi
    
    LOG_FILE="$VOLUME_PATH/log/cowrie/cowrie_chinese.json"
    
    if ! sudo test -f "$LOG_FILE"; then
        echo -e "${RED}✗ 错误: 中文日志文件不存在${NC}"
        echo -e "${YELLOW}提示: 请先触发一些SSH连接以生成日志${NC}"
        exit 1
    fi
    
    # 检查是否安装了 jq
    if command -v jq &> /dev/null; then
        echo -e "${GREEN}使用 jq 格式化显示${NC}\n"
        sudo tail -${lines} "$LOG_FILE" | jq -C .
    else
        echo -e "${YELLOW}提示: 安装 jq 可以获得更好的显示效果 (sudo apt install jq)${NC}\n"
        sudo tail -${lines} "$LOG_FILE"
    fi
}

# 停止并删除容器
down() {
    echo -e "${YELLOW}→${NC} 停止并删除 Cowrie 容器（保留数据卷）..."
    check_directory
    cd "${DOCKER_DIR}"
    docker compose down
    echo -e "${GREEN}✓${NC} 容器已删除，数据卷已保留"
}

# 停止并删除容器和数据卷
down_all() {
    echo -e "${RED}警告: 此操作将删除所有容器和数据卷！${NC}"
    read -p "确定要继续吗？(yes/no): " confirm
    if [ "$confirm" = "yes" ]; then
        echo -e "${YELLOW}→${NC} 停止并删除 Cowrie 容器和数据卷..."
        check_directory
        cd "${DOCKER_DIR}"
        docker compose down -v
        echo -e "${GREEN}✓${NC} 容器和数据卷已完全删除"
    else
        echo -e "${YELLOW}操作已取消${NC}"
    fi
}

# 进入容器 shell
shell() {
    echo -e "${YELLOW}→${NC} 进入 Cowrie 容器 shell..."
    check_directory
    cd "${DOCKER_DIR}"
    docker compose exec cowrie sh
}

# 添加用户
add_user() {
    local username="$1"
    local password="$2"
    
    if [ -z "$username" ] || [ -z "$password" ]; then
        echo -e "${RED}✗ 用法: $0 add-user <username> <password>${NC}"
        echo -e "${YELLOW}示例: $0 add-user root 123456${NC}"
        exit 1
    fi
    
    check_directory
    cd "${DOCKER_DIR}"
    
    # 检查容器是否运行
    CONTAINER_ID=$(docker compose ps -q cowrie)
    if [ -z "$CONTAINER_ID" ]; then
        echo -e "${RED}✗ 错误: 容器未运行，请先启动容器${NC}"
        echo -e "${YELLOW}运行: sudo $0 start${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}→${NC} 设置用户: ${GREEN}$username${NC}"
    
    # 获取 volume 挂载路径
    VOLUME_PATH=$(docker volume inspect docker_cowrie-etc --format '{{ .Mountpoint }}' 2>/dev/null)
    
    if [ -z "$VOLUME_PATH" ]; then
        echo -e "${RED}✗ 错误: 无法获取 volume 路径${NC}"
        exit 1
    fi
    
    # 直接写入 volume（需要 sudo）
    echo "${username}:x:${password}" | sudo tee "$VOLUME_PATH/userdb.txt" > /dev/null
    
    # 设置正确的权限（让容器内的 cowrie 用户可读）
    sudo chmod 644 "$VOLUME_PATH/userdb.txt"
    
    echo -e "${GREEN}✓${NC} 用户已添加到数据库"
    echo -e "${YELLOW}→${NC} 重启容器以应用更改..."
    docker compose restart cowrie > /dev/null 2>&1
    
    sleep 2
    echo -e "${GREEN}✓ 完成！${NC}"
    echo -e "\n${YELLOW}测试连接:${NC}"
    echo -e "  ${GREEN}ssh -p 2222 ${username}@localhost${NC}"
    echo -e "  密码: ${password}"
}


# 主逻辑
case "${1:-help}" in
    deploy)
        deploy
        ;;
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
    logs-tail)
        logs_tail "${2:-50}"
        ;;
    jsons)
        jsons
        ;;
    jsons-tail)
        jsons_tail "${2:-10}"
        ;;
    down)
        down
        ;;
    down-all)
        down_all
        ;;
    shell)
        shell
        ;;
    add-user)
        add_user "$2" "$3"
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo -e "${RED}✗ 未知命令: $1${NC}\n"
        show_help
        exit 1
        ;;
esac
