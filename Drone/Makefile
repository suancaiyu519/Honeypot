# MAVLink无人机蜜罐 Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm

# 目录
SRC_DIR = src
LIB_DIR = lib
BUILD_DIR = build

# 源文件
SRCS = src/main.c src/network.c src/mavlink.c src/logger.c lib/cJSON.c
PROXY_SRCS = src/proxy_main.c src/proxy.c src/network.c src/mavlink.c src/logger.c lib/cJSON.c

# 目标文件
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/network.o $(BUILD_DIR)/mavlink.o $(BUILD_DIR)/logger.o $(BUILD_DIR)/cJSON.o
PROXY_OBJS = $(BUILD_DIR)/proxy_main.o $(BUILD_DIR)/proxy.o $(BUILD_DIR)/network.o $(BUILD_DIR)/mavlink.o $(BUILD_DIR)/logger.o $(BUILD_DIR)/cJSON.o

# 目标程序
TARGET = drone_honeypot
PROXY_TARGET = drone_proxy

.PHONY: all clean run proxy

# 默认目标
all: $(TARGET) $(PROXY_TARGET)

# 编译主程序
$(TARGET): $(OBJS)
	@echo "链接 $@..."
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "✓ 构建完成: $@"

# 编译代理程序
$(PROXY_TARGET): $(PROXY_OBJS)
	@echo "链接 $@..."
	$(CC) $(PROXY_OBJS) -o $@ $(LDFLAGS)
	@echo "✓ 构建完成: $@"

# 编译规则
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@echo "编译 $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@echo "编译 $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "清理编译文件..."
	@rm -rf $(BUILD_DIR) $(TARGET) $(PROXY_TARGET)
	@echo "清理完成！"

run: $(TARGET)
	@echo "启动蜜罐..."
	@./$(TARGET)

# 开发模式（带调试信息）
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# 安装
install: $(TARGET)
	@echo "安装蜜罐到 /usr/local/bin..."
	@install -m 755 $(TARGET) /usr/local/bin/
	@echo "安装完成！"
