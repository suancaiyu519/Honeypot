# Copyright (c) 2015 Michel Oosterhof <michel@oosterhof.net>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The names of the author(s) may not be used to endorse or promote
#    products derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

from __future__ import annotations

import json
import os
from pathlib import Path

from twisted.python import log

import shellguard.core.output
import shellguard.python.logfile
from shellguard.core.config import ShellGuardConfig


class Output(shellguard.core.output.Output):
    """
    Chinese JSON log output
    输出中文 JSON 格式的日志
    """

    # 事件ID中英文映射
    EVENT_TRANSLATIONS = {
        "shellguard.client.fingerprint": "客户端指纹",
        "shellguard.client.kex": "密钥交换",
        "shellguard.client.size": "客户端尺寸",
        "shellguard.client.var": "客户端变量",
        "shellguard.client.version": "客户端版本",
        "shellguard.command.failed": "命令失败",
        "shellguard.command.input": "命令输入",
        "shellguard.command.success": "命令成功",
        "shellguard.direct-tcpip.data": "TCP-IP转发数据",
        "shellguard.direct-tcpip.request": "TCP-IP转发请求",
        "shellguard.log.closed": "日志关闭",
        "shellguard.log.open": "日志打开",
        "shellguard.login.failed": "登录失败",
        "shellguard.login.success": "登录成功",
        "shellguard.session.closed": "会话关闭",
        "shellguard.session.connect": "会话连接",
        "shellguard.session.file_download": "文件下载",
        "shellguard.session.file_upload": "文件上传",
        "shellguard.session.input": "会话输入",
        "shellguard.session.params": "会话参数",
    }

    # 字段名中英文映射
    FIELD_TRANSLATIONS = {
        "eventid": "事件ID",
        "timestamp": "时间戳",
        "session": "会话",
        "message": "消息",
        "src_ip": "来源IP",
        "src_port": "来源端口",
        "dst_ip": "目标IP",
        "dst_port": "目标端口",
        "username": "用户名",
        "password": "密码",
        "input": "输入",
        "command": "命令",
        "duration": "持续时间",
        "size": "大小",
        "sensor": "蜜罐",
        "version": "版本",
        "protocol": "协议",
        "fingerprint": "指纹",
        "hassh": "HASSH",
        "kexAlgs": "密钥交换算法",
        "keyAlgs": "密钥算法",
        "encCS": "加密算法(客户端到服务器)",
        "macCS": "MAC算法(客户端到服务器)",
        "compCS": "压缩算法(客户端到服务器)",
        "langCS": "语言(客户端到服务器)",
        "encSC": "加密算法(服务器到客户端)",
        "macSC": "MAC算法(服务器到客户端)",
        "compSC": "压缩算法(服务器到客户端)",
        "langSC": "语言(服务器到客户端)",
        "width": "宽度",
        "height": "高度",
        "shasum": "SHA校验和",
        "url": "URL",
        "outfile": "输出文件",
        "duplicate": "重复",
        "destfile": "目标文件",
        "arch": "架构",
        "machinetype": "机器类型",
        "ttylog": "TTY日志",
        "isError": "错误",
    }

    def start(self):
        self.epoch_timestamp = ShellGuardConfig.getboolean(
            "output_chinesejson", "epoch_timestamp", fallback=False
        )
        fn = ShellGuardConfig.get(
            "output_chinesejson", "logfile", fallback="shellguard_chinese.json"
        )
        dirs = os.path.dirname(fn)
        base = os.path.basename(fn)

        logtype = ShellGuardConfig.get("honeypot", "logtype", fallback="plain")
        if logtype == "rotating":
            self.outfile = shellguard.python.logfile.ShellGuardDailyLogFile(
                base, dirs, defaultMode=0o664
            )
        elif logtype == "plain":
            self.outfile = open(Path(dirs, base), "w", encoding="utf-8")
        else:
            raise ValueError

    def stop(self):
        if self.outfile:
            self.outfile.flush()

    def translate_field_name(self, key: str) -> str:
        """翻译字段名为中文"""
        return self.FIELD_TRANSLATIONS.get(key, key)

    def translate_event_id(self, eventid: str) -> str:
        """翻译事件ID为中文"""
        return self.EVENT_TRANSLATIONS.get(eventid, eventid)

    def translate_message(self, message: str) -> str:
        """翻译消息内容为中文"""
        translations = {
            "New connection": "新连接",
            "Remote SSH version": "远程SSH版本",
            "SSH client hassh fingerprint": "SSH客户端HASSH指纹",
            "login attempt": "登录尝试",
            "login return": "登录返回",
            "CMD": "命令",
            "Command found": "找到命令",
            "Command not found": "未找到命令",
            "Opening TTY log": "打开TTY日志",
            "Closing TTY log": "关闭TTY日志",
            "Terminal size": "终端尺寸",
            "Session closed": "会话关闭",
            "Connection lost": "连接断开",
            "direct-tcpip": "直接TCP-IP",
            "Downloaded": "已下载",
        }

        # 简单的关键词替换
        for eng, chn in translations.items():
            if eng in message:
                message = message.replace(eng, chn)

        return message

    def write(self, event):
        if self.epoch_timestamp:
            event["epoch"] = int(event["time"] * 1000000 / 1000)

        # 创建中文版本的事件字典
        chinese_event = {}

        for key, value in list(event.items()):
            # 移除 twisted 15 遗留键
            if key.startswith("log_") or key == "time" or key == "system":
                continue

            # 翻译字段名
            chinese_key = self.translate_field_name(key)

            # 特殊处理部分字段
            if key == "eventid":
                value = self.translate_event_id(value)
            elif key == "message":
                value = self.translate_message(value)

            chinese_event[chinese_key] = value

        try:
            json.dump(chinese_event, self.outfile, ensure_ascii=False, separators=(",", ":"))
            self.outfile.write("\n")
            self.outfile.flush()
        except TypeError:
            log.err("jsonlog_chinese: Can't serialize: '" + repr(event) + "'")
