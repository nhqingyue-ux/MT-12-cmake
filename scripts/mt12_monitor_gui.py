#!/usr/bin/env python3
"""
MT-12 温控板实时监控 GUI
用 0x30 命令读取 12 路 PV/SV 温度
"""
import sys
import time
import threading
import tkinter as tk
from tkinter import ttk, messagebox

# pyserial 路径
sys.path.insert(0, '/Users/nhqy/Library/Python/3.9/lib/python/site-packages')
import serial

# ── CRC16（按固件 main.c 实现）────────────────────────────────────
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

def crc16(data: bytes) -> int:
    hi, lo = 0xFF, 0xFF
    for b in data:
        idx = hi ^ b
        hi = lo ^ auchCRCHi[idx]
        lo = auchCRCLo[idx]
    return hi | (lo << 8)

def build_cmd30(board_id: int, addrs: list) -> bytes:
    """构建 0x30 读命令"""
    data_len = len(addrs) * 2
    pdu = bytes([board_id, 0x30, (data_len >> 8) & 0xFF, data_len & 0xFF])
    for a in addrs:
        pdu += bytes([(a >> 8) & 0xFF, a & 0xFF])
    c = crc16(pdu)
    return pdu + bytes([c & 0xFF, (c >> 8) & 0xFF])

def raw_to_temp(raw: int) -> float:
    """raw (unsigned short) → °C"""
    if raw >= 0x8000:
        raw -= 0x10000
    return raw * 0.1

# PV 地址（当前温度）：20501~20512
PV_ADDRS = list(range(20516, 20528))
# SV 地址（设定温度）：20001~20012
SV_ADDRS = list(range(20001, 20013))
# PWM 输出：20601~20612
PWM_ADDRS = list(range(20601, 20613))
# 断偶标志：20808 = BURN_DETECT | ADErrorFlag（bit1~bit12 对应 CH1~CH12）
ADERR_ADDR = [20808]


class MT12Monitor(tk.Tk):
    PORT = '/dev/cu.usbserial-21220'
    BOARD_ID = 0x01
    POLL_MS = 1000  # 刷新间隔 ms

    def __init__(self):
        super().__init__()
        self.title("MT-12 温控板实时监控")
        self.resizable(False, False)
        self.geometry("700x470")
        self.ser = None
        self.running = False
        self.lock = threading.Lock()

        self._build_ui()
        self._connect()
        self._schedule_poll()

    # ── UI ──────────────────────────────────────────────────────────
    def _build_ui(self):
        BG = "#1e1e2e"
        HDR = "#cdd6f4"
        self.configure(bg=BG)

        # 状态栏
        top = tk.Frame(self, bg=BG, width=560)
        top.pack(padx=10, pady=(8, 4), anchor="w")
        tk.Label(top, text="MT-12 温控板", font=("Helvetica", 16, "bold"),
                 fg=HDR, bg=BG).grid(row=0, column=0, sticky="w")
        self.time_var = tk.StringVar(value="")
        tk.Label(top, textvariable=self.time_var, font=("Helvetica", 10),
                 fg="#6c7086", bg=BG).grid(row=0, column=1, padx=20)
        self.status_var = tk.StringVar(value="● 连接中...")
        self.status_lbl = tk.Label(top, textvariable=self.status_var,
                                   font=("Helvetica", 11), fg="#a6e3a1", bg=BG)
        self.status_lbl.grid(row=0, column=2)

        # 分隔线
        tk.Frame(self, bg="#313244", height=1).pack(fill=tk.X, padx=10)

        # 用 grid 布局，列宽固定，表头和数据完全对齐
        grid = tk.Frame(self, bg=BG)
        grid.pack(padx=10, pady=(6, 0), anchor="w")

        # 列配置（像素宽度）
        COL_PX = [60, 130, 130, 100, 130]
        HEADERS = ["路", "PV 实温 (°C)", "SV 设定 (°C)", "PWM (%)", "状态"]
        for c, (hdr, px) in enumerate(zip(HEADERS, COL_PX)):
            grid.columnconfigure(c, minsize=px)
            tk.Label(grid, text=hdr, font=("Helvetica", 10, "bold"),
                     fg="#89b4fa", bg=BG, anchor="center").grid(
                row=0, column=c, sticky="ew", pady=(0, 2))

        tk.Frame(self, bg="#45475a", height=1, width=560).pack(padx=10, pady=2, anchor="w")

        # 数据行容器
        data_grid = tk.Frame(self, bg=BG)
        data_grid.pack(padx=10, anchor="w")   # anchor=w 靠左，不拉伸
        for c, px in enumerate(COL_PX):
            data_grid.columnconfigure(c, minsize=px, uniform="col")

        self.rows = []
        for i in range(12):
            row_bg = "#1e1e2e" if i % 2 == 0 else "#24273a"
            r = i + 1  # grid row

            tk.Label(data_grid, text=f"CH{i+1:02d}", font=("Courier", 11, "bold"),
                     fg="#cba6f7", bg=row_bg, anchor="center").grid(
                row=r, column=0, sticky="ew", ipady=3)

            pv_var = tk.StringVar(value="---")
            pv_lbl = tk.Label(data_grid, textvariable=pv_var,
                              font=("Courier", 13, "bold"), fg="#f38ba8", bg=row_bg, anchor="center")
            pv_lbl.grid(row=r, column=1, sticky="ew")

            sv_var = tk.StringVar(value="---")
            tk.Label(data_grid, textvariable=sv_var,
                     font=("Courier", 12), fg="#a6e3a1", bg=row_bg, anchor="center").grid(
                row=r, column=2, sticky="ew")

            pwm_var = tk.StringVar(value="---")
            tk.Label(data_grid, textvariable=pwm_var,
                     font=("Courier", 12), fg="#fab387", bg=row_bg, anchor="center").grid(
                row=r, column=3, sticky="ew")

            stat_var = tk.StringVar(value="")
            stat_lbl = tk.Label(data_grid, textvariable=stat_var,
                     font=("Helvetica", 10), fg="#f9e2af", bg=row_bg, anchor="center")
            stat_lbl.grid(row=r, column=4, sticky="ew")

            self.rows.append(dict(pv=pv_var, sv=sv_var, pwm=pwm_var, stat=stat_var,
                                  pv_lbl=pv_lbl, stat_lbl=stat_lbl, bg=row_bg))

        tk.Frame(self, bg="#45475a", height=1).pack(fill=tk.X, padx=10, pady=4)

        # 底部：原始帧
        bot = tk.Frame(self, bg=BG)
        bot.pack(anchor="w", padx=10, pady=(0, 8))
        tk.Label(bot, text="最后原始帧:", font=("Helvetica", 9),
                 fg="#6c7086", bg=BG).pack(side=tk.LEFT)
        self.raw_var = tk.StringVar(value="")
        tk.Label(bot, textvariable=self.raw_var, font=("Courier", 9),
                 fg="#585b70", bg=BG).pack(side=tk.LEFT, padx=6)

    # ── 串口 ────────────────────────────────────────────────────────
    def _connect(self):
        try:
            self.ser = serial.Serial(self.PORT, baudrate=38400,
                                     bytesize=8, parity='N', stopbits=1, timeout=0.8)
            self.running = True
            self.status_var.set(f"● {self.PORT} @ 38400")
            self.status_lbl.config(fg="#a6e3a1")
        except Exception as e:
            self.status_var.set(f"❌ 串口打开失败: {e}")
            self.status_lbl.config(fg="#f38ba8")

    # ── 轮询 ────────────────────────────────────────────────────────
    def _schedule_poll(self):
        if self.running:
            threading.Thread(target=self._poll, daemon=True).start()
        self.after(self.POLL_MS, self._schedule_poll)

    def _poll(self):
        if not self.ser:
            return
        try:
            all_addrs = PV_ADDRS + SV_ADDRS + PWM_ADDRS + ADERR_ADDR
            all_vals = self._read30(all_addrs)
            if all_vals and len(all_vals) >= 37:
                pvs    = all_vals[0:12]
                svs    = all_vals[12:24]
                pwms   = all_vals[24:36]
                aderr  = all_vals[36]
            else:
                pvs  = self._read30(PV_ADDRS)
                time.sleep(0.3)
                svs  = self._read30(SV_ADDRS)
                time.sleep(0.3)
                pwms = self._read30(PWM_ADDRS)
                time.sleep(0.3)
                er = self._read30(ADERR_ADDR)
                aderr = er[0] if er else 0
            self.after(0, self._update_ui, pvs, svs, pwms, aderr)
        except Exception as e:
            self.after(0, self._set_error, str(e))

    def _read30(self, addrs: list):
        cmd = build_cmd30(self.BOARD_ID, addrs)
        expected = 3 + len(addrs) * 2 + 2
        with self.lock:
            self.ser.reset_input_buffer()
            self.ser.write(cmd)
            time.sleep(0.3)
            resp = self.ser.read(expected + 4)  # 多读几字节防截断
        if not resp or len(resp) < 5:
            return None
        if resp[1] == 0xF0:
            return None
        # 解析
        values = []
        for i in range(len(addrs)):
            offset = 3 + i * 2
            if offset + 1 >= len(resp):
                break
            raw = (resp[offset] << 8) | resp[offset + 1]
            values.append(raw)
        self.after(0, lambda r=resp: self.raw_var.set(r.hex(' ')))
        return values if len(values) == len(addrs) else None

    # ── UI 更新（主线程）────────────────────────────────────────────
    def _update_ui(self, pvs, svs, pwms, aderr=0):
        self.time_var.set(time.strftime("%H:%M:%S"))
        for i, row in enumerate(self.rows):
            # PV
            if pvs and i < len(pvs):
                pv = raw_to_temp(pvs[i])
                # 固件断偶时写 12000 (1200.0°C)
                broken = (pvs[i] >= 12000)
                if broken:
                    row['pv'].set("断偶")
                    row['pv_lbl'].config(fg="#6c7086")
                else:
                    row['pv'].set(f"{pv:.1f}")
                    color = "#ff5555" if pv > 350 else "#f38ba8"
                    row['pv_lbl'].config(fg=color)
            else:
                broken = False
                row['pv'].set("ERR")

            # SV
            if svs and i < len(svs):
                sv = raw_to_temp(svs[i])
                row['sv'].set(f"{sv:.1f}")
            else:
                sv = 0
                row['sv'].set("ERR")

            # PWM
            if pwms and i < len(pwms):
                pwm_pct = pwms[i] / 32767 * 100
                row['pwm'].set(f"{pwm_pct:.1f}%")
            else:
                row['pwm'].set("ERR")

            # 状态（纯文字，避免 emoji 渲染问题）
            if broken:
                row['stat'].set("断偶!")
                row['stat_lbl'].config(fg="#ff5555")
            elif pvs and i < len(pvs):
                pv = raw_to_temp(pvs[i])
                diff = abs(pv - sv)
                if pv > 400:
                    row['stat'].set("过温!")
                    row['stat_lbl'].config(fg="#ff5555")
                elif diff < 5:
                    row['stat'].set("稳定")
                    row['stat_lbl'].config(fg="#a6e3a1")
                elif pv < sv:
                    row['stat'].set("升温中")
                    row['stat_lbl'].config(fg="#f9e2af")
                else:
                    row['stat'].set("冷却中")
                    row['stat_lbl'].config(fg="#89dceb")

    def _set_error(self, msg):
        self.status_var.set(f"❌ {msg}")
        self.status_lbl.config(fg="#f38ba8")

    def on_close(self):
        self.running = False
        if self.ser:
            self.ser.close()
        self.destroy()


if __name__ == "__main__":
    app = MT12Monitor()
    app.protocol("WM_DELETE_WINDOW", app.on_close)
    app.mainloop()
