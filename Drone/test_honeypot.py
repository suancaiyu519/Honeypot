#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MAVLink蜜罐测试脚本
用于测试蜜罐是否正常工作
"""

import socket
import struct
import time

def calculate_crc(data, msgid):
    """计算MAVLink CRC"""
    crc_extra = {0: 50}  # HEARTBEAT
    crc = 0xFFFF
    
    for byte in data:
        tmp = byte ^ (crc & 0xFF)
        tmp ^= (tmp << 4) & 0xFF
        crc = ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF
    
    # 添加CRC_EXTRA
    extra = crc_extra.get(msgid, 0)
    tmp = extra ^ (crc & 0xFF)
    tmp ^= (tmp << 4) & 0xFF
    crc = ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF
    
    return crc

def create_heartbeat():
    """创建心跳消息"""
    # 载荷
    payload = struct.pack('<IBBBBB',
        0,      # custom_mode
        6,      # type (地面站)
        8,      # autopilot (通用)
        0,      # base_mode
        4,      # system_status
        3       # mavlink_version
    )
    
    # 消息头
    header = struct.pack('<BBBBBB',
        0xFE,   # STX
        len(payload),  # len
        0,      # seq
        255,    # sysid (地面站)
        0,      # compid
        0       # msgid (HEARTBEAT)
    )
    
    # 计算CRC
    crc = calculate_crc(header[1:] + payload, 0)
    checksum = struct.pack('<H', crc)
    
    return header + payload + checksum

def test_honeypot(host='127.0.0.1', port=14550):
    """测试蜜罐"""
    print("=" * 50)
    print("MAVLink蜜罐测试脚本")
    print("=" * 50)
    
    # 创建UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)
    
    print(f"\n[1] 连接蜜罐: {host}:{port}")
    
    try:
        # 发送心跳
        heartbeat = create_heartbeat()
        print(f"[2] 发送心跳消息 ({len(heartbeat)} 字节)")
        sock.sendto(heartbeat, (host, port))
        
        # 接收响应
        print("[3] 等待响应...")
        received_count = 0
        
        for i in range(5):  # 接收5个消息
            try:
                data, addr = sock.recvfrom(1024)
                if data[0] == 0xFE:  # MAVLink消息
                    msgid = data[5]
                    received_count += 1
                    
                    msg_types = {
                        0: "HEARTBEAT",
                        1: "SYS_STATUS",
                        24: "GPS_RAW_INT",
                        30: "ATTITUDE",
                        33: "GLOBAL_POSITION_INT"
                    }
                    
                    msg_name = msg_types.get(msgid, f"未知({msgid})")
                    print(f"    ✓ 收到消息 #{received_count}: {msg_name} ({len(data)} 字节)")
                    
            except socket.timeout:
                break
        
        print(f"\n[4] 测试结果:")
        if received_count > 0:
            print(f"    ✅ 成功！共收到 {received_count} 个响应消息")
            print(f"    ✅ 蜜罐工作正常")
        else:
            print(f"    ❌ 失败：未收到响应")
            
    except Exception as e:
        print(f"\n    ❌ 错误: {e}")
    finally:
        sock.close()
    
    print("\n" + "=" * 50)

if __name__ == "__main__":
    import sys
    
    host = sys.argv[1] if len(sys.argv) > 1 else '127.0.0.1'
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 14550
    
    test_honeypot(host, port)
