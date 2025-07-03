#!/usr/bin/env python3
"""
CAN 信号模拟器 — 独立 Python 脚本
用于在 Linux SocketCAN 环境或测试环境中向虚拟 CAN 总线注入信号。

用法:
  # 使用虚拟 CAN（需先配置 vcan 接口）
  sudo modprobe vcan
  sudo ip link add dev vcan0 type vcan
  sudo ip link set up vcan0
  python3 can_simulator.py --channel vcan0

  # 或使用 python-can 的虚拟通道（无需 root）
  python3 can_simulator.py --channel virtual --interface virtual

依赖:
  pip install python-can
"""

import argparse
import math
import time
import struct
import random
import sys

try:
    import can
except ImportError:
    print("ERROR: python-can not installed. Run: pip install python-can")
    sys.exit(1)


class CanSimulator:
    """模拟车辆 ECU 发报"""

    def __init__(self, channel='virtual', interface='virtual', bitrate=500000):
        self.channel = channel
        self.interface = interface

        # 初始化 CAN 总线
        try:
            self.bus = can.interface.Bus(
                interface=interface,
                channel=channel,
                bitrate=bitrate
            )
        except Exception as e:
            print(f"Failed to open CAN bus: {e}")
            sys.exit(1)

        # 模拟状态
        self.tick = 0
        self.speed = 0.0
        self.rpm = 0.0
        self.coolant = 40.0
        self.fuel = 75.0
        self.gear = 0
        self.odometer = 12345
        self.left_turn = False
        self.right_turn = False
        self.high_beam = False
        self.handbrake = True
        self.door_open = False
        self.seatbelt = True  # 未系
        self.adas_state = 0
        self.running = True

    def update_physics(self):
        """更新模拟物理值"""
        cycle = self.tick % 800

        if cycle < 300:
            t = cycle / 300.0
            self.speed = 220.0 * (1.0 - math.cos(t * math.pi)) * 0.5
        elif cycle < 500:
            self.speed = 220.0 + math.sin(cycle * 0.1) * 2.0
        else:
            t = (cycle - 500) / 300.0
            self.speed = 220.0 * (1.0 + math.cos(t * math.pi)) * 0.5

        self.gear = min(8, int(self.speed / 30.0))
        if self.speed < 1.0:
            self.gear = 0
        gear_ratio = 30.0 * self.gear if self.gear > 0 else 10.0
        self.rpm = min(7500, (self.speed / gear_ratio) * 6000.0)
        if self.rpm < 800:
            self.rpm = 800 + math.sin(self.tick * 0.3) * 100

        self.coolant = min(105.0, 40.0 + self.tick * 0.02)
        if self.tick % 200 == 0:
            self.fuel -= 1.0
        if self.fuel < 0:
            self.fuel = 100.0

        self.left_turn = (100 < cycle < 150)
        self.right_turn = (450 < cycle < 500)
        self.high_beam = (600 < cycle < 650)
        self.handbrake = (self.speed < 5.0)
        self.door_open = (cycle == 0 or cycle == 400)

        if self.speed > 120:
            self.adas_state = 1  # FCW
        elif self.speed > 60:
            self.adas_state = 2  # LDW
        else:
            self.adas_state = 0

        self.odometer += int(self.speed * 0.014)

    def build_engine_frame(self):
        """ID 0x100: Engine ECU"""
        rpm_raw = int(self.rpm * 4)
        oil_raw = int(300 + math.sin(self.tick * 0.1) * 50)
        data = struct.pack('<HBHB', rpm_raw, int(self.coolant + 40),
                          int(self.fuel), oil_raw)
        data += bytes([0x01, 0x00])
        return can.Message(arbitration_id=0x100, data=data, is_extended_id=False)

    def build_transmission_frame(self):
        """ID 0x200: Transmission ECU"""
        speed_raw = int(self.speed * 100)
        data = struct.pack('<BHB', self.gear & 0xFF, speed_raw,
                          int(self.coolant - 10))
        shift = 0x01 if self.rpm > 6000 else 0x00
        data += bytes([shift, 0, 0, 0])
        return can.Message(arbitration_id=0x200, data=data, is_extended_id=False)

    def build_adas_frame(self):
        """ID 0x300: ADAS ECU"""
        adas_status = 0
        fcw_level = 0
        ldw_dir = 0
        dist = 8000

        if self.adas_state == 1:
            adas_status |= 0x01
            fcw_level = 3 if self.speed > 180 else 2
            dist = max(100, int(1500 - self.speed * 5))
        elif self.adas_state == 2:
            adas_status |= 0x02
            ldw_dir = 1 if (self.tick % 20 < 10) else 2
            dist = 4000

        speed_limit = 120 if self.speed > 100 else 0
        data = struct.pack('<BBBHB', adas_status, fcw_level, ldw_dir,
                          dist, speed_limit)
        data += bytes([0, 0])
        return can.Message(arbitration_id=0x300, data=data, is_extended_id=False)

    def build_body_frame(self):
        """ID 0x400: Body ECU"""
        body_status = 0
        if self.left_turn:   body_status |= 0x01
        if self.right_turn:  body_status |= 0x02
        if self.high_beam:    body_status |= 0x04
        if self.handbrake:    body_status |= 0x08
        if self.door_open:    body_status |= 0x10
        body_status |= 0x20  # seatbelt

        ambient_light = int(200 + math.sin(self.tick * 0.05) * 50)
        interior_temp = int(22 + 40)
        data = struct.pack('<BBB', body_status, ambient_light, interior_temp)
        data += bytes([0, 0, 0, 0, 0])
        return can.Message(arbitration_id=0x400, data=data, is_extended_id=False)

    def build_cluster_frame(self):
        """ID 0x500: Cluster ECU"""
        trip = self.tick * 5
        data = struct.pack('<IHHB', self.odometer, trip,
                           0, int(18 + 40))
        data += bytes([0])
        return can.Message(arbitration_id=0x500, data=data, is_extended_id=False)

    def send_all(self):
        """发送所有 ECU 帧"""
        for msg in [self.build_engine_frame(),
                    self.build_transmission_frame(),
                    self.build_adas_frame(),
                    self.build_body_frame(),
                    self.build_cluster_frame()]:
            try:
                self.bus.send(msg)
            except can.CanError as e:
                print(f"CAN send error: {e}")

    def run(self, interval=0.05):
        """主循环"""
        print(f"[CAN Simulator] Channel: {self.channel}, Interface: {self.interface}")
        print(f"[CAN Simulator] Sending frames every {interval*1000:.0f}ms...")
        print("[CAN Simulator] Press Ctrl+C to stop.")

        try:
            while self.running:
                self.tick += 1
                self.update_physics()
                self.send_all()

                if self.tick % 20 == 0:  # 每 1s 打印状态
                    print(f"[tick={self.tick}] speed={self.speed:.1f} km/h, "
                          f"rpm={self.rpm:.0f}, gear={self.gear}, "
                          f"coolant={self.coolant:.0f}°C, fuel={self.fuel:.0f}%")

                time.sleep(interval)
        except KeyboardInterrupt:
            print("\n[CAN Simulator] Stopped.")
        finally:
            self.bus.shutdown()


def main():
    parser = argparse.ArgumentParser(description='CAN Signal Simulator')
    parser.add_argument('--channel', default='virtual',
                       help='CAN channel (default: virtual)')
    parser.add_argument('--interface', default='virtual',
                       help='CAN interface type (virtual, socketcan, pcan, etc.)')
    parser.add_argument('--bitrate', type=int, default=500000,
                       help='CAN bitrate (default: 500000)')
    parser.add_argument('--interval', type=float, default=0.05,
                       help='Frame interval in seconds (default: 0.05)')
    args = parser.parse_args()

    sim = CanSimulator(
        channel=args.channel,
        interface=args.interface,
        bitrate=args.bitrate
    )
    sim.run(interval=args.interval)


if __name__ == '__main__':
    main()
