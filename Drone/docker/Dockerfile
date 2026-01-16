FROM alpine:latest

# 安装编译工具和运行时依赖
RUN apk add --no-cache \
    gcc \
    g++ \
    make \
    libc-dev

# 创建工作目录
WORKDIR /app

# 复制源代码
COPY src/ /app/src/
COPY lib/ /app/lib/
COPY Makefile /app/

# 编译
RUN make

# 创建日志目录
RUN mkdir -p /app/logs

# 暴露端口
EXPOSE 14550/udp

# 运行蜜罐
CMD ["./drone_honeypot"]
