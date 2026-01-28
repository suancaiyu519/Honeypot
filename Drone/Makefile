# MAVLink代理蜜罐 Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm

# 目录
SRC_DIR = src
LIB_DIR = lib
BUILD_DIR = build

# 源文件
PROXY_SRCS = src/proxy_main.c src/proxy.c src/mavlink.c src/logger.c lib/cJSON.c

# 目标文件
PROXY_OBJS = $(BUILD_DIR)/proxy_main.o $(BUILD_DIR)/proxy.o $(BUILD_DIR)/mavlink.o $(BUILD_DIR)/logger.o $(BUILD_DIR)/cJSON.o

# 目标程序
TARGET = drone_proxy

.PHONY: all clean run debug install

# 默认目标
all: $(TARGET)

# 编译代理程序
$(TARGET): $(PROXY_OBJS)
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
	@rm -rf $(BUILD_DIR) $(TARGET)
	@echo "清理完成！"

run: $(TARGET)
	@echo "启动代理..."
	@./$(TARGET)

# 开发模式（带调试信息）
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# 安装
install: $(TARGET)
	@echo "安装代理到 /usr/local/bin..."
	@install -m 755 $(TARGET) /usr/local/bin/
	@echo "安装完成！"
