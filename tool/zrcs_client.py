#!/usr/bin/env python3
"""
ZRCS ZMQ Client - Python implementation
用于上位机/GUI 与 NRT 进程通信
"""

import zmq
import sys
import time
from pathlib import Path

# 添加 protobuf 生成文件路径
proto_path = Path(__file__).parent.parent / "build"
sys.path.insert(0, str(proto_path))

try:
    from message_pb2 import MotionCommand
except ImportError:
    print("Error: message_pb2 not found. Please compile protobuf first:")
    print("  protoc --python_out=. message.proto")
    sys.exit(1)


class ZRCSClient:
    """ZRCS NRT 进程客户端"""
    
    def __init__(self, host="localhost", port=5555, timeout=5000):
        """
        初始化客户端
        
        Args:
            host: NRT 进程地址
            port: ZMQ 端口
            timeout: 接收超时 (ms)
        """
        self.endpoint = f"tcp://{host}:{port}"
        self.timeout = timeout
        self.context = None
        self.socket = None
    
    def connect(self):
        """连接到 NRT 进程"""
        try:
            self.context = zmq.Context()
            self.socket = self.context.socket(zmq.REQ)
            self.socket.setsockopt(zmq.RCVTIMEO, self.timeout)
            self.socket.connect(self.endpoint)
            print(f"[Client] Connected to {self.endpoint}")
            return True
        except zmq.error.ZMQError as e:
            print(f"[Client] Connection error: {e}")
            return False
    
    def disconnect(self):
        """断开连接"""
        if self.socket:
            self.socket.close()
        if self.context:
            self.context.term()
        print("[Client] Disconnected")
    
    def send_command(self, command, args=None):
        """
        发送命令
        
        Args:
            command: 命令名称 (str)
            args: 参数列表 (list of float)
        
        Returns:
            bool: 是否成功
        """
        if not self.socket:
            print("[Client] Not connected")
            return False
        
        try:
            # 创建 Protobuf 消息
            cmd = MotionCommand()
            cmd.command = command
            if args:
                cmd.args.extend(args)
            
            # 序列化并发送
            self.socket.send(cmd.SerializeToString())
            print(f"[Client] Sent: {command} with args {args}")
            
            # 接收回复
            reply = self.socket.recv()
            response = reply.decode('utf-8', errors='ignore')
            print(f"[Client] Response: {response}")
            
            return response == "OK"
        
        except zmq.error.Again:
            print("[Client] Timeout: No response from server")
            return False
        except Exception as e:
            print(f"[Client] Error: {e}")
            return False
    
    def move_j(self, j1, j2, j3, j4, j5, j6):
        """关节运动"""
        return self.send_command("MoveJ", [j1, j2, j3, j4, j5, j6])
    
    def move_l(self, x, y, z, rx, ry, rz):
        """直线运动"""
        return self.send_command("MoveL", [x, y, z, rx, ry, rz])
    
    def move_c(self, x1, y1, z1, x2, y2, z2):
        """圆弧运动"""
        return self.send_command("MoveC", [x1, y1, z1, x2, y2, z2])
    
    def stop(self):
        """停止运动"""
        return self.send_command("Stop")
    
    def enable(self):
        """使能系统"""
        return self.send_command("Enable")
    
    def disable(self):
        """禁用系统"""
        return self.send_command("Disable")
    
    def jog(self, axis, direction, speed):
        """点动"""
        return self.send_command("Jog", [axis, direction, speed])


def main():
    """测试程序"""
    client = ZRCSClient()
    
    if not client.connect():
        sys.exit(1)
    
    try:
        # 等待服务器启动
        time.sleep(0.5)
        
        # 测试 1: 使能
        print("\n=== Test 1: Enable ===")
        client.enable()
        time.sleep(0.5)
        
        # 测试 2: 关节运动
        print("\n=== Test 2: MoveJ ===")
        client.move_j(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)
        time.sleep(0.5)
        
        # 测试 3: 直线运动
        print("\n=== Test 3: MoveL ===")
        client.move_l(100.0, 200.0, 300.0, 0.0, 0.0, 0.0)
        time.sleep(0.5)
        
        # 测试 4: 点动
        print("\n=== Test 4: Jog ===")
        client.jog(0, 1, 10.0)
        time.sleep(0.5)
        
        # 测试 5: 停止
        print("\n=== Test 5: Stop ===")
        client.stop()
        time.sleep(0.5)
        
        # 测试 6: 禁用
        print("\n=== Test 6: Disable ===")
        client.disable()
        
        print("\n=== All tests completed ===")
    
    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
