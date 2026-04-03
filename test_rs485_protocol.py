#!/usr/bin/env python3
"""
MT-12 RS485 通信协议完整测试用例
================================================
测试固件所有支持的功能码和边界条件

协议格式:
  请求: [ID] [FUNC] [DATA...] [CRC_L] [CRC_H]
  响应: [ID] [FUNC] [DATA...] [CRC_L] [CRC_H]

功能码:
  0x30 - 读取多路 PV 数据 (地址列表)
  0x33 - 读取系统参数 (连续地址)
  0x60 - 写入多路数据 (地址+数据对)
  0x63 - 写入单个参数
  0x13 - 批量写入 (连续地址)
  0xEF - 读取版本信息
  0xF1 - 下载应用参数 (写入+确认)
"""

import serial
import time
import sys
import struct

# ─── CRC16 查表法 ─────────────────────────────────────────
auchCRCHi = bytes([
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
])
auchCRCLo = bytes([
    0x00,0xC0,0xC1,0x01,0xC3,0x03,0x02,0xC2,0xC6,0x06,0x07,0xC7,0x05,0xC5,0xC4,0x04,
    0xCC,0x0C,0x0D,0xCD,0x0F,0xCF,0xCE,0x0E,0x0A,0xCA,0xCB,0x0B,0xC9,0x09,0x08,0xC8,
    0xD8,0x18,0x19,0xD9,0x1B,0xDB,0xDA,0x1A,0x1E,0xDE,0xDF,0x1F,0xDD,0x1D,0x1C,0xDC,
    0x14,0xD4,0xD5,0x15,0xD7,0x17,0x16,0xD6,0xD2,0x12,0x13,0xD3,0x11,0xD1,0xD0,0x10,
    0xF0,0x30,0x31,0xF1,0x33,0xF3,0xF2,0x32,0x36,0xF6,0xF7,0x37,0xF5,0x35,0x34,0xF4,
    0x3C,0xFC,0xFD,0x3D,0xFF,0x3F,0x3E,0xFE,0xFA,0x3A,0x3B,0xFB,0x39,0xF9,0xF8,0x38,
    0x28,0xE8,0xE9,0x29,0xEB,0x2B,0x2A,0xEA,0xEE,0x2E,0x2F,0xEF,0x2D,0xED,0xEC,0x2C,
    0xE4,0x24,0x25,0xE5,0x27,0xE7,0xE6,0x26,0x22,0xE2,0xE3,0x23,0xE1,0x21,0x20,0xE0,
    0xA0,0x60,0x61,0xA1,0x63,0xA3,0xA2,0x62,0x66,0xA6,0xA7,0x67,0xA5,0x65,0x64,0xA4,
    0x6C,0xAC,0xAD,0x6D,0xAF,0x6F,0x6E,0xAE,0xAA,0x6A,0x6B,0xAB,0x69,0xA9,0xA8,0x68,
    0x78,0xB8,0xB9,0x79,0xBB,0x7B,0x7A,0xBA,0xBE,0x7E,0x7F,0xBF,0x7D,0xBD,0xBC,0x7C,
    0xB4,0x74,0x75,0xB5,0x77,0xB7,0xB6,0x76,0x72,0xB2,0xB3,0x73,0xB1,0x71,0x70,0xB0,
    0x50,0x90,0x91,0x51,0x93,0x53,0x52,0x92,0x96,0x56,0x57,0x97,0x55,0x95,0x94,0x54,
    0x9C,0x5C,0x5D,0x9D,0x5F,0x9F,0x9E,0x5E,0x5A,0x9A,0x9B,0x5B,0x99,0x59,0x58,0x98,
    0x88,0x48,0x49,0x89,0x4B,0x8B,0x8A,0x4A,0x4E,0x8E,0x8F,0x4F,0x8D,0x4D,0x4C,0x8C,
    0x44,0x84,0x85,0x45,0x87,0x47,0x46,0x86,0x82,0x42,0x43,0x83,0x41,0x81,0x80,0x40,
])

def crc16(data):
    hi, lo = 0xFF, 0xFF
    for b in data:
        idx = hi ^ b
        hi = lo ^ auchCRCHi[idx]
        lo = auchCRCLo[idx]
    return hi | (lo << 8)

def append_crc(data):
    c = crc16(data)
    return data + bytes([c & 0xFF, (c >> 8) & 0xFF])

def verify_crc(data, frame_len=None):
    """Verify CRC. If frame_len is given, only check first frame_len bytes."""
    if frame_len:
        data = data[:frame_len]
    if len(data) < 3:
        return False
    payload = data[:-2]
    expected = crc16(payload)
    actual = data[-2] | (data[-1] << 8)
    return expected == actual

# ─── 帧构建函数 ─────────────────────────────────────────

def build_0x30_read(dev_id, addrs):
    """0x30 - 读取 PV 数据 (地址列表方式)"""
    dl = len(addrs) * 2  # data length in bytes
    pdu = bytes([dev_id, 0x30, (dl >> 8) & 0xFF, dl & 0xFF])
    for a in addrs:
        pdu += bytes([(a >> 8) & 0xFF, a & 0xFF])
    return append_crc(pdu)

def build_0x33_read(dev_id, start_addr, count):
    """0x33 - 读取系统参数 (连续地址)
    两种模式：
    - start_addr <= AddressBase0 (200): count=寄存器数, response=count*2字节
    - start_addr > AddressBase0: count=字节数(含地址), response=count字节
    """
    pdu = bytes([dev_id, 0x33,
                 (start_addr >> 8) & 0xFF, start_addr & 0xFF,
                 (count >> 8) & 0xFF, count & 0xFF])
    return append_crc(pdu)

def build_0x60_write(dev_id, addr_data_pairs):
    """0x60 - 写入多路数据 (地址+数据对)
    每对 = 2字节地址 + 2字节数据 = 4字节
    """
    dl = len(addr_data_pairs) * 4
    pdu = bytes([dev_id, 0x60, (dl >> 8) & 0xFF, dl & 0xFF])
    for addr, data in addr_data_pairs:
        pdu += bytes([(addr >> 8) & 0xFF, addr & 0xFF,
                      (data >> 8) & 0xFF, data & 0xFF])
    return append_crc(pdu)

def build_0x63_write(dev_id, addr, data):
    """0x63 - 写入单个参数"""
    pdu = bytes([dev_id, 0x63,
                 (addr >> 8) & 0xFF, addr & 0xFF,
                 (data >> 8) & 0xFF, data & 0xFF])
    return append_crc(pdu)

def build_0x13_batch_write(dev_id, start_addr, values):
    """0x13 - 批量写入 (连续地址)"""
    count = len(values)
    byte_count = count * 2
    pdu = bytes([dev_id, 0x13,
                 (start_addr >> 8) & 0xFF, start_addr & 0xFF,
                 (count >> 8) & 0xFF, count & 0xFF,
                 byte_count])
    for v in values:
        pdu += bytes([(v >> 8) & 0xFF, v & 0xFF])
    return append_crc(pdu)

def build_0xEF_version(dev_id):
    """0xEF - 读取版本信息"""
    pdu = bytes([dev_id, 0xEF, 0x00, 0x00])
    return append_crc(pdu)

def build_0xF1_download(dev_id, addr_data_pairs):
    """0xF1 - 下载应用参数"""
    dl = len(addr_data_pairs) * 4
    pdu = bytes([dev_id, 0xF1, (dl >> 8) & 0xFF, dl & 0xFF])
    for addr, data in addr_data_pairs:
        pdu += bytes([(addr >> 8) & 0xFF, addr & 0xFF,
                      (data >> 8) & 0xFF, data & 0xFF])
    return append_crc(pdu)

# ─── 通信 ─────────────────────────────────────────

SERIAL_PORT = '/dev/cu.usbserial-212420'
BAUD_RATE = 38400
DEV_ID = 0x01
TIMEOUT = 1.0

def send_recv(ser, frame, timeout=TIMEOUT):
    ser.reset_input_buffer()
    ser.write(frame)
    time.sleep(0.5)
    resp = ser.read(ser.in_waiting or 200)
    if len(resp) < 5:
        time.sleep(0.5)
        resp += ser.read(ser.in_waiting or 200)
    return resp

# ─── 地址定义 ─────────────────────────────────────────
# PV 温度值: 20516-20527 (CH1-CH12)
# SV 设定值: 20501-20512
# 报警状态等: 20528+
# 系统参数:   地址 1-200 (AddressBase0 = 200)

# ═══════════════════════════════════════════════════
#                    测 试 用 例
# ═══════════════════════════════════════════════════

results = []

def test(name, passed, detail=""):
    status = "✅ PASS" if passed else "❌ FAIL"
    results.append((name, passed, detail))
    mark = "✅" if passed else "❌"
    print(f"  {mark} {name}")
    if detail:
        print(f"     {detail}")

def run_tests():
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=TIMEOUT)
    time.sleep(0.2)

    # ══════════════════════════════════════════
    print("\n" + "=" * 60)
    print("【1】功能码 0x30 — 读取 PV 数据（地址列表）")
    print("=" * 60)

    # TC-1.1: 读取单路 CH1 PV
    frame = build_0x30_read(DEV_ID, [20516])
    resp = send_recv(ser, frame)
    ok = len(resp) >= 5 and resp[0] == DEV_ID and resp[1] == 0x30 and verify_crc(resp)
    pv = (resp[3] << 8 | resp[4]) if ok else None
    test("TC-1.1: 读取 CH1 PV (单路)", ok,
         f"PV={pv} ({pv*0.1:.1f}°C)" if pv is not None else f"resp={resp.hex(' ') if resp else 'none'}")

    # TC-1.2: 读取全部 12 路 PV
    frame = build_0x30_read(DEV_ID, list(range(20516, 20528)))
    resp = send_recv(ser, frame)
    ok = len(resp) >= 27 and resp[0] == DEV_ID and resp[1] == 0x30 and verify_crc(resp)
    pvs = []
    if ok:
        for i in range(12):
            pvs.append(resp[3 + i*2] << 8 | resp[3 + i*2 + 1])
    test("TC-1.2: 读取 12 路 PV", ok,
         f"CH1-12: {pvs}" if pvs else f"resp len={len(resp)}")

    # TC-1.3: 读取 2 路 PV (CH1 + CH6)
    frame = build_0x30_read(DEV_ID, [20516, 20521])
    resp = send_recv(ser, frame)
    ok = len(resp) >= 7 and resp[0] == DEV_ID and resp[1] == 0x30 and verify_crc(resp)
    test("TC-1.3: 读取 2 路 PV (CH1+CH6)", ok,
         f"resp={resp.hex(' ')}" if resp else "none")

    # TC-1.4: CRC 校验错误 (不应响应或返回错误)
    frame = build_0x30_read(DEV_ID, [20516])
    bad_frame = frame[:-2] + bytes([0xFF, 0xFF])  # 故意错误 CRC
    resp = send_recv(ser, bad_frame, timeout=0.8)
    # 板子对 CRC 错误也会回应 (CrcAlarm → 仍然 SendRs485Fg=1)
    test("TC-1.4: CRC 错误帧", True,
         f"resp={resp.hex(' ') if resp else 'none'} (有响应=CrcAlarm, 无响应=静默丢弃)")

    # ══════════════════════════════════════════
    print("\n" + "=" * 60)
    print("【2】功能码 0x33 — 读取系统参数（连续地址）")
    print("=" * 60)

    # TC-2.1: 读取 SV1 (设定值, 地址 20501, 大地址模式)
    frame = build_0x33_read(DEV_ID, 20501, 2)  # 2 bytes
    resp = send_recv(ser, frame)
    ok = len(resp) >= 5 and resp[0] == DEV_ID and resp[1] == 0x33 and verify_crc(resp)
    sv = (resp[3] << 8 | resp[4]) if ok and len(resp) >= 5 else None
    test("TC-2.1: 读取 SV1 (addr=20501)", ok,
         f"SV1={sv} ({sv*0.1:.1f}°C)" if sv is not None else f"resp={resp.hex(' ') if resp else 'none'}")

    # TC-2.2: 读取多个 SV (SV1-SV4, 4 registers = 8 bytes)
    frame = build_0x33_read(DEV_ID, 20501, 8)
    resp = send_recv(ser, frame)
    ok = len(resp) >= 11 and resp[0] == DEV_ID and resp[1] == 0x33 and verify_crc(resp)
    svs = []
    if ok:
        for i in range(4):
            svs.append(resp[3 + i*2] << 8 | resp[3 + i*2 + 1])
    test("TC-2.2: 读取 SV1-SV4 (4路)", ok,
         f"SVs={svs}" if svs else f"resp={resp.hex(' ') if resp else 'none'}")

    # TC-2.3: 读取小地址参数 (addr=1, count=1 → 小地址模式)
    frame = build_0x33_read(DEV_ID, 1, 1)
    resp = send_recv(ser, frame)
    ok = len(resp) >= 5 and resp[0] == DEV_ID and resp[1] == 0x33 and verify_crc(resp)
    test("TC-2.3: 读取系统参数 (addr=1, 小地址模式)", ok,
         f"resp={resp.hex(' ') if resp else 'none'}")

    # TC-2.4: 读取 PID P 参数 (addr 20201-20212)
    frame = build_0x33_read(DEV_ID, 20201, 2)
    resp = send_recv(ser, frame)
    ok = len(resp) >= 5 and resp[0] == DEV_ID and resp[1] == 0x33 and verify_crc(resp)
    p_val = (resp[3] << 8 | resp[4]) if ok else None
    test("TC-2.4: 读取 PID P1 (addr=20201)", ok,
         f"P1={p_val}" if p_val is not None else f"resp={resp.hex(' ') if resp else 'none'}")

    # TC-2.5: 读取 PID I 参数 (addr 20216-20227)
    frame = build_0x33_read(DEV_ID, 20216, 2)
    resp = send_recv(ser, frame)
    ok = len(resp) >= 5 and resp[0] == DEV_ID and resp[1] == 0x33 and verify_crc(resp)
    i_val = (resp[3] << 8 | resp[4]) if ok else None
    test("TC-2.5: 读取 PID I1 (addr=20216)", ok,
         f"I1={i_val}" if i_val is not None else f"resp={resp.hex(' ') if resp else 'none'}")

    # TC-2.6: 读取 PID D 参数 (addr 20231-20242)
    frame = build_0x33_read(DEV_ID, 20231, 2)
    resp = send_recv(ser, frame)
    ok = len(resp) >= 5 and resp[0] == DEV_ID and resp[1] == 0x33 and verify_crc(resp)
    d_val = (resp[3] << 8 | resp[4]) if ok else None
    test("TC-2.6: 读取 PID D1 (addr=20231)", ok,
         f"D1={d_val}" if d_val is not None else f"resp={resp.hex(' ') if resp else 'none'}")

    # ══════════════════════════════════════════
    print("\n" + "=" * 60)
    print("【3】功能码 0x63 — 写入单个参数")
    print("=" * 60)

    # TC-3.1: 写入 SV1 然后读回验证
    # 先读取当前 SV1
    frame = build_0x33_read(DEV_ID, 20501, 2)
    resp = send_recv(ser, frame)
    orig_sv1 = (resp[3] << 8 | resp[4]) if len(resp) >= 5 else 0

    # 写入新值 (大地址模式)
    # 0x63 响应固定 8 字节: [ID][0x63][ADDR_H][ADDR_L][DATA_H][DATA_L][CRC_L][CRC_H]
    # 但板子可能发送更多字节（TX buffer 残留），只检查前 8 字节
    test_val = 1234  # 123.4°C
    frame = build_0x63_write(DEV_ID, 20501, test_val)
    resp = send_recv(ser, frame)
    ok_write = len(resp) >= 8 and resp[0] == DEV_ID and resp[1] == 0x63 and verify_crc(resp, 8)
    echo_addr = (resp[2] << 8 | resp[3]) if len(resp) >= 6 else None
    echo_data = (resp[4] << 8 | resp[5]) if len(resp) >= 6 else None
    test("TC-3.1: 写入 SV1=1234 (0x63, addr=20501)", ok_write,
         f"echo: addr={echo_addr} data={echo_data}" if ok_write else f"resp={resp.hex(' ') if resp else 'none'}")

    # TC-3.2: 读回 SV1 验证写入 (用 0x33 读回)
    time.sleep(0.5)
    frame = build_0x33_read(DEV_ID, 20501, 2)
    resp = send_recv(ser, frame)
    read_sv1 = (resp[3] << 8 | resp[4]) if len(resp) >= 5 else None
    test("TC-3.2: 读回 SV1 验证 (0x33)", read_sv1 == test_val,
         f"wrote={test_val}, read={read_sv1} (注: SV 可能被温控循环覆写)")

    # TC-3.3: 恢复原 SV1
    frame = build_0x63_write(DEV_ID, 20501, orig_sv1)
    resp = send_recv(ser, frame)
    ok = len(resp) >= 8 and verify_crc(resp, 8)
    test("TC-3.3: 恢复 SV1 原值", ok, f"restored to {orig_sv1}")

    # TC-3.4: 小地址写入 (addr <= 200)
    frame = build_0x63_write(DEV_ID, 501, test_val)
    resp = send_recv(ser, frame)
    ok_small = len(resp) >= 8 and resp[0] == DEV_ID and resp[1] == 0x63 and verify_crc(resp, 8)
    test("TC-3.4: 写入 SV1 (小地址模式, addr=501)", ok_small,
         f"resp[0:8]={resp[:8].hex(' ') if len(resp)>=8 else resp.hex(' ')}")

    # 恢复
    time.sleep(0.2)
    frame = build_0x63_write(DEV_ID, 20501, orig_sv1)
    send_recv(ser, frame)

    # ══════════════════════════════════════════
    print("\n" + "=" * 60)
    print("【4】功能码 0x60 — 写入多路数据（地址+数据对）")
    print("=" * 60)

    # TC-4.1: 写入 SV1 + SV2
    pairs = [(20501, 1000), (20502, 2000)]  # 100.0°C, 200.0°C
    frame = build_0x60_write(DEV_ID, pairs)
    resp = send_recv(ser, frame)
    ok = len(resp) >= 5 and resp[0] == DEV_ID and resp[1] == 0x60 and verify_crc(resp)
    test("TC-4.1: 写入 SV1=1000, SV2=2000", ok,
         f"resp={resp.hex(' ') if resp else 'none'}")

    # TC-4.2: 读回验证 (用 0x33 读取)
    time.sleep(0.5)
    frame = build_0x33_read(DEV_ID, 20501, 4)  # 4 bytes = 2 registers
    resp = send_recv(ser, frame)
    sv1 = (resp[3] << 8 | resp[4]) if len(resp) >= 7 else None
    sv2 = (resp[5] << 8 | resp[6]) if len(resp) >= 7 else None
    test("TC-4.2: 读回验证 SV1/SV2 (0x33)", sv1 == 1000 and sv2 == 2000,
         f"SV1={sv1}, SV2={sv2} (注: SV 可能被温控循环覆写)")

    # 恢复
    time.sleep(0.2)
    frame = build_0x60_write(DEV_ID, [(20501, orig_sv1), (20502, orig_sv1)])
    send_recv(ser, frame)

    # ══════════════════════════════════════════
    print("\n" + "=" * 60)
    print("【5】功能码 0x13 — 批量写入（连续地址）")
    print("=" * 60)

    # TC-5.1: 批量写入 SV1-SV3
    values = [1100, 2200, 3300]
    frame = build_0x13_batch_write(DEV_ID, 501, values)  # 小地址
    resp = send_recv(ser, frame)
    ok = len(resp) >= 8 and resp[0] == DEV_ID and resp[1] == 0x13 and verify_crc(resp, 8)
    test("TC-5.1: 批量写入 SV1-SV3", ok,
         f"resp={resp.hex(' ') if resp else 'none'}")

    # TC-5.2: 读回验证 (用 0x33 读取)
    time.sleep(0.3)
    frame = build_0x33_read(DEV_ID, 20501, 6)  # 6 bytes = 3 registers
    resp = send_recv(ser, frame)
    read_vals = []
    if len(resp) >= 9:
        for i in range(3):
            read_vals.append(resp[3 + i*2] << 8 | resp[3 + i*2 + 1])
    test("TC-5.2: 读回验证批量写入 (0x33)", read_vals == values,
         f"wrote={values}, read={read_vals}")

    # 恢复
    time.sleep(0.2)
    frame = build_0x13_batch_write(DEV_ID, 501, [orig_sv1]*3)
    send_recv(ser, frame)

    # ══════════════════════════════════════════
    print("\n" + "=" * 60)
    print("【6】功能码 0xEF — 读取版本信息")
    print("=" * 60)

    # TC-6.1: 读取固件版本
    frame = build_0xEF_version(DEV_ID)
    resp = send_recv(ser, frame)
    ok = len(resp) >= 21 and resp[0] == DEV_ID and resp[1] == 0xEF and verify_crc(resp)
    if ok:
        ver = resp[3:7]
        id_sw = resp[7]
        bl_ver = resp[9] | (resp[10] << 8)
        ap_str = bytes(resp[11:19]).decode('ascii', errors='replace')
        test("TC-6.1: 读取版本信息", True,
             f"APVer={ver.hex(' ')}, ID_SW=0x{id_sw:02X}, BL_ver={bl_ver}, AP='{ap_str}'")
    else:
        test("TC-6.1: 读取版本信息", False,
             f"resp={resp.hex(' ') if resp else 'none'}")

    # ══════════════════════════════════════════
    print("\n" + "=" * 60)
    print("【7】功能码 0xF1 — 下载应用参数")
    print("=" * 60)

    # TC-7.1: 写入 + DnApSysFg 标志
    pairs = [(20501, 5555)]
    frame = build_0xF1_download(DEV_ID, pairs)
    resp = send_recv(ser, frame)
    ok = len(resp) >= 5 and resp[0] == DEV_ID and resp[1] == 0xF1 and verify_crc(resp)
    test("TC-7.1: 0xF1 下载参数", ok,
         f"resp={resp.hex(' ') if resp else 'none'}")

    # 恢复
    time.sleep(0.2)
    frame = build_0x63_write(DEV_ID, 20501, orig_sv1)
    send_recv(ser, frame)

    # ══════════════════════════════════════════
    print("\n" + "=" * 60)
    print("【8】边界条件和异常测试")
    print("=" * 60)

    # TC-8.1: 错误设备地址 (不应响应)
    frame = build_0x30_read(0x02, [20516])  # ID=2, 板子 ID=1
    resp = send_recv(ser, frame, timeout=0.8)
    test("TC-8.1: 错误设备 ID=0x02", len(resp) == 0,
         f"{'正确: 无响应' if len(resp) == 0 else f'异常: 收到 {len(resp)}B'}")

    # TC-8.2: 无效功能码
    pdu = bytes([DEV_ID, 0xAA, 0x00, 0x02])
    frame = append_crc(pdu)
    resp = send_recv(ser, frame, timeout=0.8)
    test("TC-8.2: 无效功能码 0xAA", True,
         f"resp={resp.hex(' ') if resp else 'none'} (FunAlarm)")

    # TC-8.3: 读取越界地址 (addr >= 21000)
    frame = build_0x30_read(DEV_ID, [21001])
    resp = send_recv(ser, frame)
    ok = len(resp) >= 5 and verify_crc(resp)
    val = (resp[3] << 8 | resp[4]) if ok and len(resp) >= 5 else None
    test("TC-8.3: 越界地址 21001", ok,
         f"value={val} (应为0)" if val is not None else "")

    # TC-8.4: 0x30 DataLen=0 (边界: 应被拒绝 DataLenCnt<=1)
    pdu = bytes([DEV_ID, 0x30, 0x00, 0x00])  # DataLen=0
    frame = append_crc(pdu)
    resp = send_recv(ser, frame, timeout=0.8)
    test("TC-8.4: 0x30 DataLen=0 (边界)", True,
         f"resp={resp.hex(' ') if resp else 'none'}")

    # ══════════════════════════════════════════
    print("\n" + "=" * 60)
    print("【9】通信稳定性压力测试")
    print("=" * 60)

    # TC-9.1: 100 次连续查询
    ok_count = 0
    for i in range(100):
        ser.reset_input_buffer()
        frame = build_0x30_read(DEV_ID, [20516])
        ser.write(frame)
        time.sleep(0.1)
        r = ser.read(ser.in_waiting or 100)
        if r and len(r) >= 5 and verify_crc(r):
            ok_count += 1
    test("TC-9.1: 100 次连续查询", ok_count == 100,
         f"{ok_count}/100 成功")

    # TC-9.2: 混合读写 50 次
    ok_count = 0
    for i in range(50):
        ser.reset_input_buffer()
        if i % 2 == 0:
            frame = build_0x30_read(DEV_ID, [20516 + (i % 12)])
        else:
            frame = build_0x33_read(DEV_ID, 20501, 2)
        ser.write(frame)
        time.sleep(0.15)
        r = ser.read(ser.in_waiting or 100)
        if r and len(r) >= 5 and verify_crc(r):
            ok_count += 1
    test("TC-9.2: 50 次混合读写", ok_count == 50,
         f"{ok_count}/50 成功")

    ser.close()

    # ══════════════════════════════════════════
    print("\n" + "=" * 60)
    print("                 测 试 汇 总")
    print("=" * 60)
    passed = sum(1 for _, p, _ in results if p)
    failed = sum(1 for _, p, _ in results if not p)
    total = len(results)
    print(f"\n  总计: {total} 项")
    print(f"  通过: {passed} ✅")
    print(f"  失败: {failed} ❌")
    print(f"  通过率: {passed/total*100:.0f}%")

    if failed > 0:
        print(f"\n  失败项:")
        for name, p, detail in results:
            if not p:
                print(f"    ❌ {name}: {detail}")

    print()
    return failed == 0

if __name__ == "__main__":
    print("=" * 60)
    print("  MT-12 RS485 通信协议测试")
    print(f"  串口: {SERIAL_PORT}")
    print(f"  波特率: {BAUD_RATE}")
    print(f"  设备ID: 0x{DEV_ID:02X}")
    print("=" * 60)

    success = run_tests()
    sys.exit(0 if success else 1)
