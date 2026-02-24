#!/usr/bin/env python3
"""
前端一体化服务器
  - 静态文件服务（HTTP）：将 src/web/ 托管在 HTTP_PORT
  - WebSocket ↔ TCP 代理：转发到 C++ TCP 后端（4字节小端长度头协议）
只需运行一条命令：python src/web/ws_proxy.py
"""

import asyncio
import struct
import json
import sys
import threading
import os
from http.server import HTTPServer, SimpleHTTPRequestHandler
from functools import partial

try:
    import websockets
except ImportError:
    print("请先安装 websockets: pip install websockets")
    sys.exit(1)

# ===== 配置 =====
WS_HOST   = "0.0.0.0"
WS_PORT   = 8765          # WebSocket 代理端口
HTTP_HOST = "0.0.0.0"
HTTP_PORT = 8080          # 静态页面端口
TCP_HOST  = "127.0.0.1"
TCP_PORT  = 5085          # C++ 后端 TCP 端口

# 静态文件根目录（ws_proxy.py 所在目录，即 src/web/）
WEB_ROOT = os.path.dirname(os.path.abspath(__file__))


async def tcp_send_recv(message: str) -> str:
    """
    连接 TCP 后端，发送一条消息并接收响应。
    协议：4字节小端整数(body长度) + body(JSON字符串)
    """
    reader, writer = await asyncio.open_connection(TCP_HOST, TCP_PORT)

    try:
        # 编码并发送
        payload = message.encode("utf-8")
        header = struct.pack("<I", len(payload))  # 4字节小端
        writer.write(header + payload)
        await writer.drain()

        # 接收响应头（4字节长度）
        resp_header = await reader.readexactly(4)
        resp_len = struct.unpack("<I", resp_header)[0]

        # 接收响应体
        resp_body = await reader.readexactly(resp_len)
        return resp_body.decode("utf-8")

    finally:
        writer.close()
        await writer.wait_closed()


async def handle_client(websocket, path=None):
    """处理单个 WebSocket 客户端连接"""
    remote = websocket.remote_address
    print(f"[WS] 新连接: {remote}")

    try:
        async for raw_message in websocket:
            try:
                # 验证是合法 JSON
                json.loads(raw_message)
            except json.JSONDecodeError:
                await websocket.send(json.dumps({
                    "code": -1,
                    "message": "无效的JSON格式"
                }))
                continue

            try:
                # 转发到 TCP 后端
                response = await asyncio.wait_for(
                    tcp_send_recv(raw_message),
                    timeout=30.0  # 30秒超时（AI回复可能较慢）
                )
                await websocket.send(response)

            except asyncio.TimeoutError:
                await websocket.send(json.dumps({
                    "code": -1,
                    "message": "后端响应超时"
                }))
            except ConnectionRefusedError:
                await websocket.send(json.dumps({
                    "code": -1,
                    "message": "无法连接到后端服务器，请确认服务已启动"
                }))
            except Exception as e:
                await websocket.send(json.dumps({
                    "code": -1,
                    "message": f"代理错误: {str(e)}"
                }))

    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        print(f"[WS] 断开连接: {remote}")


def start_http_server():
    """在子线程中启动静态文件 HTTP 服务器"""
    handler = partial(SimpleHTTPRequestHandler, directory=WEB_ROOT)
    # 关闭访问日志噪音
    handler.log_message = lambda *a: None
    httpd = HTTPServer((HTTP_HOST, HTTP_PORT), handler)
    httpd.serve_forever()


async def main():
    # 启动静态文件服务（后台线程，daemon=True 随主进程一起退出）
    t = threading.Thread(target=start_http_server, daemon=True)
    t.start()

    print(f"[服务启动]")
    print(f"  页面地址   →  http://localhost:{HTTP_PORT}")
    print(f"  WebSocket  →  ws://localhost:{WS_PORT}  （代理至 TCP {TCP_HOST}:{TCP_PORT}）")
    print(f"  按 Ctrl+C 停止\n")

    async with websockets.serve(handle_client, WS_HOST, WS_PORT):
        await asyncio.get_event_loop().create_future()  # 永久运行


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[服务] 已停止")
