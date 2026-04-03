#!/usr/bin/env python3
"""
MT-12 溫度控制卡 RS485 調試工具
協議：盟立私有協議（非標準 Modbus）
文件參考：MT12-溫度控制卡-V2.1.doc / 模溫機資料定義.xlsx

命令碼：
  0x03  多筆連續位址讀取（啟始位址 + 長度）
  0x30  多筆指定位址讀取（位址列表）
  0x06  單筆寫入
  0x60  多筆不同位址寫入（位址+內容交替）
  0xF0  異常回應（板側發出）

板 ID：0x00 ~ 0x07（指撥開關設定）
CRC：CRC16，回傳格式 Hi Byte + Lo Byte（Big-endian）

用法：
  python mt12_modbus.py --port /dev/tty.usbserial-xxxx --id 0 connect
  python mt12_modbus.py --port /dev/tty.usbserial-xxxx --id 0 monitor
  python mt12_modbus.py --port /dev/tty.usbserial-xxxx --id 0 pid --ch 0
  python mt12_modbus.py --port /dev/tty.usbserial-xxxx --id 0 pid --ch 0 --P 700 --I 15
  python mt12_modbus.py --port /dev/tty.usbserial-xxxx --id 0 stress
  python mt12_modbus.py --port /dev/tty.usbserial-xxxx --id 0 dump
"""

import argparse
import csv
import sys
import struct
import time
from datetime import datetime

try:
    import serial
except ImportError:
    print("請先安裝: pip install pyserial")
    sys.exit(1)

# ─────────────────────────────────────────────────────────────────────────────
# CRC16（與文件附錄 C 程式一致）
# ─────────────────────────────────────────────────────────────────────────────

_CRC_HI = bytes([
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
_CRC_LO = bytes([
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

def crc16(data: bytes) -> int:
    """CRC16，回傳值格式：Hi Byte 在高 8 位（與文件一致）"""
    hi = 0xFF
    lo = 0xFF
    for b in data:
        idx = lo ^ b
        lo = hi ^ _CRC_HI[idx]
        hi = _CRC_LO[idx]
    return (hi << 8) | lo


def append_crc(data: bytes) -> bytes:
    c = crc16(data)
    return data + bytes([c >> 8, c & 0xFF])  # Hi Byte first


def check_crc(data: bytes) -> bool:
    if len(data) < 3:
        return False
    body = data[:-2]
    recv_crc = (data[-2] << 8) | data[-1]
    return crc16(body) == recv_crc


# ─────────────────────────────────────────────────────────────────────────────
# 寄存器地址映射（從 模溫機資料定義.xlsx 提取，完整版）
# ─────────────────────────────────────────────────────────────────────────────
#  格式：reg_addr -> (名稱, 單位說明, 縮放因子, 讀寫)
#  縮放：顯示值 = 寄存器值 * scale
REGS = {
    # ── 每段參數（ch0~ch11，基址 + 段號）─────────────────────────────────
    20001: ("加熱值",       "0.1°C", 0.1, "W"),
    20016: ("預熱值",       "0.1°C", 0.1, "W"),
    20031: ("高溫偏差",     "0.1°C", 0.1, "W"),
    20046: ("低溫偏差",     "0.1°C", 0.1, "W"),
    20061: ("冷卻設定",     "0.1°C", 0.1, "W"),
    20076: ("二段溫度設定", "0.1°C", 0.1, "W"),
    20101: ("加熱模式",     "0=不加熱/1=加熱/2=預熱", 1, "W"),
    20106: ("加熱模式(每段)","0=加熱/1=不加熱", 1, "W"),
    20201: ("P值",          "1:1",  1, "RW"),
    20216: ("I值",          "1:1",  1, "RW"),
    20231: ("D值",          "1:1",  1, "RW"),
    20246: ("PWM偏置",      "1:1",  1, "RW"),
    20261: ("百分比功能",   "位元 0~11", 1, "RW"),
    20262: ("漸近式加熱",   "位元 0~11", 1, "W"),
    20276: ("百分比週期",   "0.001s", 0.001, "W"),
    20291: ("百比比率",     "1%",   1, "W"),
    20306: ("同步升溫",     "位元",  1, "W"),
    20321: ("二段升溫偏移", "位元",  1, "W"),
    20346: ("控制週期",     "0.001s", 0.001, "W"),
    20401: ("控制模式功能", "位元 0~11", 1, "RW"),
    20402: ("自調諧功能",   "位元 0~11", 1, "RW"),
    20407: ("K/J選擇",      "0=K型/1=J型", 1, "RW"),
    20408: ("二段升溫功能", "位元 0~11", 1, "W"),
    20409: ("同步升溫功能", "位元 0~11", 1, "W"),
    20410: ("全程PID自調",  "位元",  1, "W"),
    20411: ("二段溫度開關", "位元 0~11", 1, "W"),
    # ── 狀態（只讀）────────────────────────────────────────────────────────
    20501: ("控制值顯示",   "0.1°C（PV當前溫度）", 0.1, "R"),
    20516: ("目前值",       "0.1°C（顯示溫度）",  0.1, "R"),
    20531: ("溫度補償",     "0.1°C", 0.1, "R"),
    20554: ("加熱狀態",     "位元 0~11", 1, "R"),
    20555: ("加熱異常",     "位元 0~11", 1, "R"),
    20556: ("高偏差異常",   "位元 0~11", 1, "R"),
    20557: ("低偏差異常",   "位元 0~11", 1, "R"),
    20558: ("資料初始化",   "0=無動作/1=初始化", 1, "RW"),
    20560: ("同步升溫狀態", "位元 0~11", 1, "R"),
    20561: ("自調狀態",     "位元 0~11", 1, "R"),
    20601: ("輸出PWM值",    "0~32767 (0~100%)", 1, "R"),
    20625: ("J/K線性表錯誤","位元",  1, "R"),
    20800: ("固件版本 ch1", "***", 1, "R"),
    20801: ("固件版本 ch2", "***", 1, "R"),
    20802: ("固件版本 ch3", "***", 1, "R"),
    20803: ("固件版本 ch4", "***", 1, "R"),
    20804: ("固件版本 ch5", "***", 1, "R"),
    20805: ("固件版本 ch6", "***", 1, "R"),
    20806: ("左常溫",       "***", 1, "R"),
    20807: ("右常溫",       "***", 1, "R"),
    20808: ("過電流保護",   "位元 0~11", 1, "R"),
    20809: ("熱電偶斷線",   "位元 0~11", 1, "R"),
    20811: ("F/W版本",      "***", 1, "R"),
    20850: ("線性表錯誤",   "位元",  1, "R"),
}

ERR_CODES = {
    0x81: "資料長度異常",
    0x82: "資料內容異常",
    0x83: "CRC 計算異常",
}


# ─────────────────────────────────────────────────────────────────────────────
# 協議幀構建
# ─────────────────────────────────────────────────────────────────────────────

def build_0x03(board_id: int, start_addr: int, count: int) -> bytes:
    """0x03：連續讀取（啟始位址 + 筆數）
    發送：[ID][0x03][AddrHi][AddrLo][LenHi][LenLo][CRCHi][CRCLo]
    長度欄位 = count * 2（Bytes）
    """
    pdu = bytes([board_id, 0x03,
                 (start_addr >> 8) & 0xFF, start_addr & 0xFF,
                 (count * 2 >> 8) & 0xFF,  (count * 2) & 0xFF])
    return append_crc(pdu)


def build_0x30(board_id: int, addrs: list) -> bytes:
    """0x30：指定位址讀取（位址列表）
    發送：[ID][0x30][LenByte][Addr1Hi][Addr1Lo]...[CRCHi][CRCLo]
    LenByte = len(addrs) * 2
    """
    data_len = len(addrs) * 2
    pdu = bytes([board_id, 0x30, data_len])
    for a in addrs:
        pdu += bytes([(a >> 8) & 0xFF, a & 0xFF])
    return append_crc(pdu)


def build_0x06(board_id: int, addr: int, value: int) -> bytes:
    """0x06：單筆寫入
    發送：[ID][0x06][AddrHi][AddrLo][ValHi][ValLo][CRCHi][CRCLo]
    """
    v = value & 0xFFFF
    pdu = bytes([board_id, 0x06,
                 (addr >> 8) & 0xFF, addr & 0xFF,
                 (v >> 8) & 0xFF,    v & 0xFF])
    return append_crc(pdu)


def build_0x60(board_id: int, addr_val_pairs: list) -> bytes:
    """0x60：多筆不同位址寫入（[(addr, val), ...]）
    發送：[ID][0x60][LenByte][Addr1Hi][Addr1Lo][Val1Hi][Val1Lo]...[CRCHi][CRCLo]
    LenByte = len(pairs) * 4
    """
    data_len = len(addr_val_pairs) * 4
    pdu = bytes([board_id, 0x60, data_len])
    for addr, val in addr_val_pairs:
        v = val & 0xFFFF
        pdu += bytes([(addr >> 8) & 0xFF, addr & 0xFF,
                      (v >> 8) & 0xFF,    v & 0xFF])
    return append_crc(pdu)


# ─────────────────────────────────────────────────────────────────────────────
# 響應解析
# ─────────────────────────────────────────────────────────────────────────────

def parse_read_response(data: bytes, expected_count: int):
    """解析 0x03 / 0x30 讀取響應
    回傳：[val1, val2, ...] 或 None（失敗）
    """
    # 最小：[ID][CMD][LenByte][Hi][Lo]...[CRCHi][CRCLo]
    min_len = 3 + expected_count * 2 + 2
    if len(data) < min_len:
        return None

    # 異常響應 0xF0
    if len(data) >= 6 and data[1] == 0xF0:
        err = data[2]
        print(f"  ⚠️  板回傳異常 0xF0: {ERR_CODES.get(err, f'未知 0x{err:02X}')}")
        return None

    if not check_crc(data):
        print(f"  ⚠️  CRC 錯誤，原始數據: {data.hex()}")
        return None

    byte_count = data[2]
    vals = []
    for i in range(byte_count // 2):
        v = (data[3 + i*2] << 8) | data[4 + i*2]
        vals.append(v)
    return vals


def parse_write_response(data: bytes, cmd: int) -> bool:
    """解析寫入響應（板側回傳同樣的幀）"""
    if len(data) < 4:
        return False
    if data[1] == 0xF0:
        err = data[2]
        print(f"  ⚠️  寫入異常 0xF0: {ERR_CODES.get(err, f'未知 0x{err:02X}')}")
        return False
    if not check_crc(data):
        print(f"  ⚠️  回應 CRC 錯誤: {data.hex()}")
        return False
    return data[1] == cmd


# ─────────────────────────────────────────────────────────────────────────────
# MT12 客戶端
# ─────────────────────────────────────────────────────────────────────────────

class MT12Client:
    def __init__(self, port: str, board_id: int, baud: int = 19200, timeout: float = 0.5):
        self.id = board_id
        self.timeout = timeout
        self.ser = serial.Serial(
            port=port, baudrate=baud,
            bytesize=8, parity='N', stopbits=1,
            timeout=timeout
        )
        time.sleep(0.1)

    def close(self):
        self.ser.close()

    def _txrx(self, frame: bytes, resp_len: int) -> bytes:
        self.ser.reset_input_buffer()
        self.ser.write(frame)
        resp = self.ser.read(resp_len)
        return resp

    # ── 讀取 ────────────────────────────────────────────────────────────────

    def read_seq(self, start_addr: int, count: int):
        """0x03 連續讀 count 個 Word，回傳 list 或 None"""
        frame = build_0x03(self.id, start_addr, count)
        resp_len = 3 + count * 2 + 2
        resp = self._txrx(frame, resp_len)
        return parse_read_response(resp, count)

    def read_addrs(self, addrs: list):
        """0x30 讀指定位址列表，回傳 list 或 None"""
        frame = build_0x30(self.id, addrs)
        resp_len = 3 + len(addrs) * 2 + 2
        resp = self._txrx(frame, resp_len)
        return parse_read_response(resp, len(addrs))

    # ── 寫入 ────────────────────────────────────────────────────────────────

    def write_one(self, addr: int, value: int) -> bool:
        """0x06 單筆寫入"""
        frame = build_0x06(self.id, addr, value)
        resp = self._txrx(frame, 8)
        return parse_write_response(resp, 0x06)

    def write_many(self, addr_val_pairs: list) -> bool:
        """0x60 多筆寫入"""
        frame = build_0x60(self.id, addr_val_pairs)
        resp_len = 3 + len(addr_val_pairs) * 4 + 2
        resp = self._txrx(frame, resp_len)
        return parse_write_response(resp, 0x60)

    # ── 常用高層 API ─────────────────────────────────────────────────────────

    def read_all_pv(self):
        """讀全部 12 段目前溫度 PV（20501~20512）"""
        vals = self.read_seq(20501, 12)
        if vals is None:
            return None
        return [v * 0.1 if v < 0x8000 else (v - 0x10000) * 0.1 for v in vals]

    def read_all_sv(self):
        """讀全部 12 段設定溫度 SV（20001~20012）"""
        vals = self.read_seq(20001, 12)
        if vals is None:
            return None
        return [v * 0.1 for v in vals]

    def read_all_pwm(self):
        """讀全部 12 段 PWM 輸出值（20601~20612）"""
        vals = self.read_seq(20601, 12)
        if vals is None:
            return None
        return [v / 327.67 for v in vals]  # 0~32767 → 0~100%

    def read_pid(self, ch: int):
        """讀指定通道 PID（0-based）"""
        vals = self.read_addrs([20201 + ch, 20216 + ch, 20231 + ch])
        if vals:
            return {'P': vals[0], 'I': vals[1], 'D': vals[2]}
        return None

    def write_pid(self, ch: int, P: int, I: int, D: int) -> bool:
        """寫指定通道 PID（0x60 一次發送）"""
        return self.write_many([
            (20201 + ch, P),
            (20216 + ch, I),
            (20231 + ch, D),
        ])

    def read_status(self):
        """讀板狀態：左/右常溫、斷線、過電流"""
        vals = self.read_addrs([20806, 20807, 20808, 20809, 20811])
        if vals is None:
            return None
        return {
            'board_temp_L': vals[0] * 0.1,
            'board_temp_R': vals[1] * 0.1,
            'overcurrent':  f"0x{vals[2]:04X}",
            'burn_detect':  f"0x{vals[3]:04X}",
            'fw_version':   f"0x{vals[4]:04X}",
        }


# ─────────────────────────────────────────────────────────────────────────────
# 子命令
# ─────────────────────────────────────────────────────────────────────────────

def cmd_connect(c: MT12Client, _args):
    print(f"=== 連接測試 (ID=0x{c.id:02X}) ===")

    sts = c.read_status()
    if sts:
        print(f"板溫 L={sts['board_temp_L']:.1f}°C  R={sts['board_temp_R']:.1f}°C")
        print(f"過電流={sts['overcurrent']}  斷線={sts['burn_detect']}  FW={sts['fw_version']}")
    else:
        print("⚠️  板狀態讀取失敗（繼續嘗試溫度）")

    pvs = c.read_all_pv()
    svs = c.read_all_sv()
    if pvs is None:
        print("❌ 溫度讀取失敗 → 請檢查：串口名、波特率(19200)、板 ID(指撥開關)、接線 A/B")
        return False

    print(f"\n{'段':>4}  {'SV(設定)':>10}  {'PV(目前)':>10}")
    print("-" * 30)
    for i in range(12):
        sv = svs[i] if svs else "?"
        pv = pvs[i]
        flag = " ⚠️" if isinstance(pv, float) and pv > 400 else ""
        print(f"  {i+1:2d}  {sv:>10}  {pv:>10.1f}{flag}")

    print("\n✅ 連接正常")
    return True


def cmd_monitor(c: MT12Client, args):
    duration = getattr(args, 'duration', 60)
    interval = getattr(args, 'interval', 0.5)
    outfile  = getattr(args, 'output', None)

    f = writer = None
    if outfile:
        f = open(outfile, 'w', newline='', encoding='utf-8-sig')
        writer = csv.writer(f)
        writer.writerow(
            ['timestamp'] +
            [f'ch{i+1}_SV' for i in range(12)] +
            [f'ch{i+1}_PV' for i in range(12)] +
            [f'ch{i+1}_PWM%' for i in range(12)]
        )

    hdr = f"{'時間':12s} " + "  ".join(f"{'ch'+str(i+1):>7}" for i in range(12))
    print(f"=== 溫度監控（{duration}s, 間隔{interval}s）===")
    print(hdr + "  [PV]")
    print("-" * 115)

    start = time.time()
    ok = err = 0
    try:
        while time.time() - start < duration:
            t0 = time.time()
            pvs  = c.read_all_pv()
            svs  = c.read_all_sv()
            pwms = c.read_all_pwm()
            ok += 1
            ts = datetime.now().strftime('%H:%M:%S.%f')[:12]

            if pvs:
                line = f"{ts} " + "  ".join(f"{p:7.1f}" for p in pvs)
                print(line)
                if writer and svs and pwms:
                    writer.writerow([ts] +
                        [f'{s:.1f}' for s in svs] +
                        [f'{p:.1f}' for p in pvs] +
                        [f'{w:.1f}' for w in pwms])
            else:
                err += 1
                print(f"{ts} ❌ 讀取失敗 ({err}/{ok})")

            time.sleep(max(0, interval - (time.time() - t0)))
    except KeyboardInterrupt:
        print("\n用戶中斷")
    finally:
        if f:
            f.close()

    total = ok + err
    print(f"\n統計: 總={total}  失敗={err}  成功率={100*(total-err)/max(total,1):.1f}%")
    if outfile:
        print(f"CSV 已保存: {outfile}")


def cmd_pid(c: MT12Client, args):
    ch = args.ch
    print(f"=== ch{ch+1} PID 參數（位址: P={20201+ch} I={20216+ch} D={20231+ch}）===")

    cur = c.read_pid(ch)
    if cur is None:
        print("❌ 讀取失敗")
        return

    print(f"目前: P={cur['P']}  I={cur['I']}  D={cur['D']}")

    if getattr(args, 'P', None) is not None:
        P = args.P
        I = args.I if args.I is not None else cur['I']
        D = args.D if args.D is not None else cur['D']
        print(f"寫入: P={P}  I={I}  D={D}")

        ok = c.write_pid(ch, P, I, D)
        if ok:
            verify = c.read_pid(ch)
            if verify and verify['P'] == P and verify['I'] == I and verify['D'] == D:
                print("✅ 寫入並回讀驗證成功")
            else:
                print(f"⚠️  寫入後回讀不符: {verify}")
        else:
            print("❌ 寫入失敗")


def cmd_stress(c: MT12Client, args):
    count    = getattr(args, 'count', 500)
    interval = getattr(args, 'interval', 0.1)

    print(f"=== 壓力測試 {count} 次，間隔{interval}s ===")
    ok_n = err_n = 0
    rtts = []

    for i in range(count):
        t0 = time.time()
        result = c.read_seq(20501, 12)
        rtt = (time.time() - t0) * 1000

        if result:
            ok_n += 1
            rtts.append(rtt)
        else:
            err_n += 1

        if (i + 1) % 50 == 0:
            avg = sum(rtts[-50:]) / min(50, len(rtts)) if rtts else 0
            print(f"  [{i+1:4d}/{count}] ok={ok_n} err={err_n} avg_rtt={avg:.1f}ms")

        time.sleep(interval)

    print(f"\n結果: 成功={ok_n}, 失敗={err_n}, 成功率={100*ok_n/count:.1f}%")
    if rtts:
        print(f"RTT: min={min(rtts):.1f}ms  avg={sum(rtts)/len(rtts):.1f}ms  max={max(rtts):.1f}ms")


def cmd_dump(c: MT12Client, _args):
    print("=== 全暫存器 Dump ===")
    blocks = [
        (20001, 12, "設定溫度 SV"),
        (20016, 12, "預熱值"),
        (20031, 12, "高溫偏差"),
        (20046, 12, "低溫偏差"),
        (20101, 12, "加熱模式"),
        (20201, 12, "P 值"),
        (20216, 12, "I 值"),
        (20231, 12, "D 值"),
        (20261, 1,  "百分比功能"),
        (20401, 12, "控制模式功能"),
        (20501, 12, "目前溫度 PV"),
        (20516, 12, "顯示溫度"),
        (20554, 1,  "加熱狀態"),
        (20555, 1,  "加熱異常"),
        (20556, 1,  "高偏差異常"),
        (20557, 1,  "低偏差異常"),
        (20601, 12, "PWM 輸出值"),
        (20806, 6,  "左右常溫/過電流/斷線/FW"),
    ]
    for start, cnt, name in blocks:
        vals = c.read_seq(start, cnt)
        if vals:
            print(f"\n[{start}~{start+cnt-1}] {name}:")
            for i, v in enumerate(vals):
                signed = v if v < 0x8000 else v - 0x10000
                scale = REGS.get(start + i, (None, None, 1, None))[2]
                display = f"= {signed * scale:.1f}" if scale != 1 else ""
                print(f"  {start+i}: 0x{v:04X} = {v:5d} {display}")
        else:
            print(f"\n[{start}] ❌ 讀取失敗")


# ─────────────────────────────────────────────────────────────────────────────
# 主程序
# ─────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description='MT-12 溫度控制卡 RS485 私有協議調試工具',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
協議命令碼：
  0x03  連續讀（啟始位址+長度）
  0x30  指定位址讀
  0x06  單筆寫
  0x60  多筆不同位址寫
  0xF0  異常回應（板發出）

板 ID 由指撥開關設定（0x00~0x07）
CRC16 格式：Hi Byte 在前
        """)

    parser.add_argument('--port', required=True,
                        help='串口設備, e.g. /dev/tty.usbserial-xxxx')
    parser.add_argument('--id', type=lambda x: int(x, 0), default=0,
                        help='板 ID (0~7，對應指撥開關，default: 0)')
    parser.add_argument('--baud', type=int, default=19200,
                        help='波特率 (default: 19200)')
    parser.add_argument('--timeout', type=float, default=0.5,
                        help='串口超時秒 (default: 0.5)')

    sub = parser.add_subparsers(dest='cmd', required=True)

    sub.add_parser('connect', help='連接驗證：讀板狀態 + 全部溫度')

    p_mon = sub.add_parser('monitor', help='持續監控所有通道溫度')
    p_mon.add_argument('--duration', type=float, default=60, help='監控時長秒 (default: 60)')
    p_mon.add_argument('--interval', type=float, default=0.5, help='讀取間隔秒 (default: 0.5)')
    p_mon.add_argument('--output', type=str, default=None, help='CSV 輸出路徑')

    p_pid = sub.add_parser('pid', help='讀寫 PID 參數')
    p_pid.add_argument('--ch', type=int, required=True, help='通道 0~11')
    p_pid.add_argument('--P', type=int, default=None, help='P 值')
    p_pid.add_argument('--I', type=int, default=None, help='I 值')
    p_pid.add_argument('--D', type=int, default=None, help='D 值')

    p_st = sub.add_parser('stress', help='壓力測試')
    p_st.add_argument('--count', type=int, default=500, help='測試次數 (default: 500)')
    p_st.add_argument('--interval', type=float, default=0.1, help='間隔秒')

    sub.add_parser('dump', help='Dump 全部暫存器')

    args = parser.parse_args()

    try:
        client = MT12Client(args.port, args.id, args.baud, args.timeout)
        print(f"串口已打開: {args.port}  baud={args.baud}  ID=0x{args.id:02X}")
    except Exception as e:
        print(f"❌ 打開串口失敗: {e}")
        print("  macOS 串口名格式: /dev/tty.usbserial-xxxx 或 /dev/cu.usbserial-xxxx")
        sys.exit(1)

    try:
        dispatch = {
            'connect': cmd_connect,
            'monitor': cmd_monitor,
            'pid':     cmd_pid,
            'stress':  cmd_stress,
            'dump':    cmd_dump,
        }
        dispatch[args.cmd](client, args)
    finally:
        client.close()


if __name__ == '__main__':
    main()
