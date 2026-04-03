#!/usr/bin/env python3
"""
MT-12 RS485 被动监听 GUI
- 只读总线，不发任何指令
- 实时曲线图（选择通道）
- 数据记录（检测跳变自动标注，导出 CSV）
"""
import sys, os, time, threading, csv
from datetime import datetime

# pyserial: 优先用 homebrew 自带路径，再 fallback 到 3.9 路径
try:
    import serial
except ImportError:
    sys.path.insert(0, '/Users/nhqy/Library/Python/3.9/lib/python/site-packages')
    import serial
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib
matplotlib.use('TkAgg')
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import matplotlib.ticker as mticker

# ── 配置 ──────────────────────────────────────────────
# 自动寻找 usbserial 设备，也可手动指定
import glob as _glob
def _find_port():
    cands = _glob.glob('/dev/cu.usbserial-*')
    if cands:
        return sorted(cands)[0]
    return '/dev/cu.usbserial-21220'  # fallback
PORT      = _find_port()
BAUD      = 38400
BURN_RAW  = 12000       # raw ≥ 12000 → 断偶
MAX_HIST  = 300         # 曲线最多保留300个点（约5分钟@1s）
JUMP_THR  = 50          # 跳变检测阈值 raw（5.0°C）

# 记录文件输出目录（与脚本同级的 logs 目录）
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LOG_DIR    = os.path.join(SCRIPT_DIR, '..', 'logs')
os.makedirs(LOG_DIR, exist_ok=True)

# ── 颜色 ──────────────────────────────────────────────
BG    = '#1e1e1e'
FG    = '#e0e0e0'
RED   = '#ff5555'
GRN   = '#50fa7b'
ORG   = '#ffb86c'
YEL   = '#f1fa8c'
BLU   = '#8be9fd'
GREY  = '#6272a4'
PANEL = '#2d2d2d'

CH_COLORS = ['#ff5555','#50fa7b','#8be9fd','#ffb86c',
             '#ff79c6','#bd93f9','#f1fa8c','#6be5fd',
             '#ff9580','#a8ff60','#94efff','#ffd580']

# ── CRC16 ─────────────────────────────────────────────
_H=[0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40]
_L=[0x00,0xC0,0xC1,0x01,0xC3,0x03,0x02,0xC2,0xC6,0x06,0x07,0xC7,0x05,0xC5,0xC4,0x04,0xCC,0x0C,0x0D,0xCD,0x0F,0xCF,0xCE,0x0E,0x0A,0xCA,0xCB,0x0B,0xC9,0x09,0x08,0xC8,0xD8,0x18,0x19,0xD9,0x1B,0xDB,0xDA,0x1A,0x1E,0xDE,0xDF,0x1F,0xDD,0x1D,0x1C,0xDC,0x14,0xD4,0xD5,0x15,0xD7,0x17,0x16,0xD6,0xD2,0x12,0x13,0xD3,0x11,0xD1,0xD0,0x10,0xF0,0x30,0x31,0xF1,0x33,0xF3,0xF2,0x32,0x36,0xF6,0xF7,0x37,0xF5,0x35,0x34,0xF4,0x3C,0xFC,0xFD,0x3D,0xFF,0x3F,0x3E,0xFE,0xFA,0x3A,0x3B,0xFB,0x39,0xF9,0xF8,0x38,0x28,0xE8,0xE9,0x29,0xEB,0x2B,0x2A,0xEA,0xEE,0x2E,0x2F,0xEF,0x2D,0xED,0xEC,0x2C,0xE4,0x24,0x25,0xE5,0x27,0xE7,0xE6,0x26,0x22,0xE2,0xE3,0x23,0xE1,0x21,0x20,0xE0,0xA0,0x60,0x61,0xA1,0x63,0xA3,0xA2,0x62,0x66,0xA6,0xA7,0x67,0xA5,0x65,0x64,0xA4,0x6C,0xAC,0xAD,0x6D,0xAF,0x6F,0x6E,0xAE,0xAA,0x6A,0x6B,0xAB,0x69,0xA9,0xA8,0x68,0x78,0xB8,0xB9,0x79,0xBB,0x7B,0x7A,0xBA,0xBE,0x7E,0x7F,0xBF,0x7D,0xBD,0xBC,0x7C,0xB4,0x74,0x75,0xB5,0x77,0xB7,0xB6,0x76,0x72,0xB2,0xB3,0x73,0xB1,0x71,0x70,0xB0,0x50,0x90,0x91,0x51,0x93,0x53,0x52,0x92,0x96,0x56,0x57,0x97,0x55,0x95,0x94,0x54,0x9C,0x5C,0x5D,0x9D,0x5F,0x9F,0x9E,0x5E,0x5A,0x9A,0x9B,0x5B,0x99,0x59,0x58,0x98,0x88,0x48,0x49,0x89,0x4B,0x8B,0x8A,0x4A,0x4E,0x8E,0x8F,0x4F,0x8D,0x4D,0x4C,0x8C,0x44,0x84,0x85,0x45,0x87,0x47,0x46,0x86,0x82,0x42,0x43,0x83,0x41,0x81,0x80,0x40]
def crc16(data):
    hi, lo = 0xFF, 0xFF
    for b in data:
        idx = hi ^ b; hi = lo ^ _H[idx]; lo = _L[idx]
    return hi | (lo << 8)

# ── 帧解析常量 ──────────────────────────────────────
RESP_FRAME_LEN = 185   # 3 + 180 + 2
SV_IDX  = list(range(0, 12))
PV_IDX  = list(range(12, 24))
PWM_IDX = list(range(26, 38))


# ── Sniffer（后台线程） ───────────────────────────────
class Sniffer:
    def __init__(self):
        self.pv  = [None] * 12
        self.sv  = [None] * 12
        self.pwm = [None] * 12
        self.frames_ok = 0
        self.last_ts   = None
        self.connected = False
        self._buf      = bytearray()
        self._lock     = threading.Lock()
        self._running  = False

    def start(self):
        self._running = True
        threading.Thread(target=self._run, daemon=True).start()

    def stop(self):
        self._running = False

    def _run(self):
        while self._running:
            try:
                ser = serial.Serial(PORT, BAUD, timeout=0.2)
                self.connected = True
            except Exception:
                self.connected = False
                time.sleep(1)
                continue
            while self._running:
                try:
                    chunk = ser.read(512)
                except Exception:
                    break
                if chunk:
                    self._buf.extend(chunk)
                    self._parse()
                if len(self._buf) > 8192:
                    self._buf = self._buf[-512:]
            self.connected = False
            try: ser.close()
            except: pass
            if self._running:
                time.sleep(0.5)

    def _parse(self):
        data = bytes(self._buf)
        i = 0
        last_valid = -1
        while i <= len(data) - RESP_FRAME_LEN:
            if data[i] == 0x01 and data[i+1] == 0x30 and data[i+2] == 0xB4:
                frame = data[i:i+RESP_FRAME_LEN]
                c = crc16(frame[:-2])
                if c == (frame[-2] | (frame[-1] << 8)):
                    words = [(frame[3+j*2]<<8)|frame[4+j*2] for j in range(90)]
                    with self._lock:
                        self.sv  = [words[k] for k in SV_IDX]
                        self.pv  = [words[k] for k in PV_IDX]
                        self.pwm = [words[k] for k in PWM_IDX]
                    self.frames_ok += 1
                    self.last_ts = time.time()
                    last_valid = i + RESP_FRAME_LEN
                    i += RESP_FRAME_LEN
                    continue
            i += 1
        if last_valid > 0:
            self._buf = bytearray(data[last_valid:])


# ── 主 GUI ────────────────────────────────────────────
class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title('MT-12 RS485 被动监听')
        self.configure(bg=BG)
        self.geometry('1080x680')
        self.resizable(True, True)

        # 历史数据（列表，每项是 dict）
        self._history      = []          # [{ts, pv[12], sv[12], pwm[12]}]
        self._recording    = False
        self._monitoring   = True        # False = 停止监听（曲线冻结）
        self._rec_data     = []          # 记录中的数据
        self._jump_events  = []          # [(ts, ch, old, new)]
        self._selected_chs = set(range(12))   # 曲线图显示的通道
        self._frozen_hist  = []          # 停止时保留的快照

        self._sniffer = Sniffer()
        self._build_ui()
        self._sniffer.start()
        self._tick()

    # ── 构建 UI ──────────────────────────────────────
    def _build_ui(self):
        # 顶部状态行
        top = tk.Frame(self, bg=BG)
        top.pack(fill='x', padx=10, pady=(8, 2))

        tk.Label(top, text='MT-12 RS485 被动监听', bg=BG, fg=BLU,
                 font=('Helvetica', 13, 'bold')).pack(side='left')
        tk.Label(top, text=f'  {PORT}  {BAUD}bps  [只读]',
                 bg=BG, fg=GREY, font=('Helvetica', 9)).pack(side='left')

        self._conn_dot = tk.Label(top, text=' ● ', bg=BG, fg=RED,
                                   font=('Helvetica', 12, 'bold'))
        self._conn_dot.pack(side='left')
        self._conn_lbl = tk.Label(top, text='串口断开',
                                   bg=BG, fg=RED, font=('Helvetica', 9))
        self._conn_lbl.pack(side='left')

        self._cnt_lbl = tk.Label(top, text='帧: 0', bg=BG, fg=GREY,
                                  font=('Helvetica', 9))
        self._cnt_lbl.pack(side='right', padx=8)

        # 主区：左边表格，右边曲线
        main = tk.Frame(self, bg=BG)
        main.pack(fill='both', expand=True, padx=8, pady=4)

        # ── 左：数值表格 ──────────────────────────────
        left = tk.Frame(main, bg=BG, width=430)
        left.pack(side='left', fill='y', padx=(0, 6))
        left.pack_propagate(False)

        cols = ('CH', 'PV', 'SV', 'PWM%', '状态')
        self._tree = ttk.Treeview(left, columns=cols, show='headings', height=12)
        for c, w in zip(cols, [50, 100, 100, 75, 95]):
            self._tree.heading(c, text=c)
            self._tree.column(c, width=w, anchor='center')

        st = ttk.Style(self)
        st.theme_use('clam')
        st.configure('Treeview', background=PANEL, foreground=FG,
                     fieldbackground=PANEL, rowheight=27, font=('Courier', 11))
        st.configure('Treeview.Heading', background='#3d3d3d', foreground=BLU,
                     font=('Helvetica', 10, 'bold'))

        self._tree.tag_configure('burn', foreground=RED)
        self._tree.tag_configure('ok',   foreground=GRN)
        self._tree.tag_configure('heat', foreground=ORG)
        self._tree.tag_configure('na',   foreground=GREY)

        for i in range(12):
            self._tree.insert('', 'end', iid=str(i),
                values=(f'CH{i+1:02d}', '--', '--', '--', '等待'),
                tags=('na',))
        self._tree.pack(fill='both', expand=True)

        # 通道选择复选框
        ch_sel_frame = tk.LabelFrame(left, text='曲线显示通道',
                                      bg=BG, fg=GREY, font=('Helvetica', 8))
        ch_sel_frame.pack(fill='x', pady=(4, 0))
        self._ch_vars = []
        for i in range(12):
            v = tk.BooleanVar(value=True)
            cb = tk.Checkbutton(ch_sel_frame, text=f'CH{i+1}', variable=v,
                                bg=BG, fg=CH_COLORS[i], selectcolor=PANEL,
                                activebackground=BG, font=('Helvetica', 8),
                                command=self._on_ch_sel)
            cb.grid(row=i//6, column=i%6, padx=2, pady=1, sticky='w')
            self._ch_vars.append(v)

        # ── 右：曲线图 + 控制 ────────────────────────
        right = tk.Frame(main, bg=BG)
        right.pack(side='left', fill='both', expand=True)

        # 曲线图
        self._fig = Figure(figsize=(6, 4), dpi=90, facecolor='#1a1a2e')
        self._ax  = self._fig.add_subplot(111)
        self._ax.set_facecolor('#12122a')
        self._ax.tick_params(colors=GREY, labelsize=8)
        self._ax.spines[:].set_color('#3d3d6d')
        self._ax.set_xlabel('Time (s)', color=GREY, fontsize=8)
        self._ax.set_ylabel('Temp (C)', color=GREY, fontsize=8)
        self._ax.yaxis.set_major_formatter(mticker.FuncFormatter(lambda x, _: f'{x:.0f}'))
        self._canvas = FigureCanvasTkAgg(self._fig, master=right)
        self._canvas.get_tk_widget().pack(fill='both', expand=True)

        # 控制按钮行
        btn_row = tk.Frame(right, bg=BG)
        btn_row.pack(fill='x', pady=(4, 2))

        self._rec_btn = tk.Button(btn_row, text='⏺ 开始记录',
                                   bg='#3a3a5a', fg=YEL,
                                   font=('Helvetica', 10, 'bold'),
                                   relief='flat', padx=10, pady=4,
                                   command=self._toggle_record)
        self._rec_btn.pack(side='left', padx=4)

        tk.Button(btn_row, text='💾 导出 CSV',
                  bg='#3a3a5a', fg=GRN,
                  font=('Helvetica', 10),
                  relief='flat', padx=10, pady=4,
                  command=self._export_csv).pack(side='left', padx=4)

        tk.Button(btn_row, text='🔍 查看跳变',
                  bg='#3a3a5a', fg=ORG,
                  font=('Helvetica', 10),
                  relief='flat', padx=10, pady=4,
                  command=self._show_jumps).pack(side='left', padx=4)

        self._mon_btn = tk.Button(btn_row, text='⏸ 停止监听',
                  bg='#3a3a5a', fg=BLU,
                  font=('Helvetica', 10),
                  relief='flat', padx=10, pady=4,
                  command=self._toggle_monitor)
        self._mon_btn.pack(side='left', padx=4)

        tk.Button(btn_row, text='清空历史',
                  bg='#3a3a5a', fg=GREY,
                  font=('Helvetica', 9),
                  relief='flat', padx=8, pady=4,
                  command=self._clear_history).pack(side='left', padx=4)

        # X 轴调整滑块（仅停止监听时生效）
        slider_row = tk.Frame(right, bg=BG)
        slider_row.pack(fill='x', pady=(2, 0))
        tk.Label(slider_row, text='← X偏移 →', bg=BG, fg=GREY,
                 font=('Helvetica', 8)).pack(side='left', padx=(4,2))
        self._xoffset_var = tk.DoubleVar(value=0)
        self._xoffset_scale = tk.Scale(slider_row, variable=self._xoffset_var,
            from_=0, to=1, resolution=0.01, orient='horizontal',
            bg=BG, fg=GREY, troughcolor=PANEL, highlightthickness=0,
            length=200, showvalue=False,
            command=lambda _: self._redraw_frozen())
        self._xoffset_scale.pack(side='left')
        tk.Label(slider_row, text='  宽度', bg=BG, fg=GREY,
                 font=('Helvetica', 8)).pack(side='left', padx=(8,2))
        self._xwidth_var = tk.DoubleVar(value=1.0)
        self._xwidth_scale = tk.Scale(slider_row, variable=self._xwidth_var,
            from_=0.05, to=1.0, resolution=0.05, orient='horizontal',
            bg=BG, fg=GREY, troughcolor=PANEL, highlightthickness=0,
            length=200, showvalue=False,
            command=lambda _: self._redraw_frozen())
        self._xwidth_scale.pack(side='left')
        self._slider_label = tk.Label(slider_row, text='(实时模式)', bg=BG, fg=GREY,
                                       font=('Helvetica', 8))
        self._slider_label.pack(side='left', padx=6)

        self._rec_stat = tk.Label(btn_row, text='', bg=BG, fg=RED,
                                   font=('Helvetica', 9))
        self._rec_stat.pack(side='right', padx=8)

    def _on_ch_sel(self):
        self._selected_chs = {i for i, v in enumerate(self._ch_vars) if v.get()}

    # ── 记录控制 ──────────────────────────────────────
    def _toggle_record(self):
        if not self._recording:
            self._recording = True
            self._rec_data = []
            self._jump_events = []
            self._rec_btn.config(text='⏹ 停止记录', fg=RED)
            self._rec_stat.config(text='● 记录中...')
        else:
            self._recording = False
            self._rec_btn.config(text='⏺ 开始记录', fg=YEL)
            n = len(self._rec_data)
            j = len(self._jump_events)
            self._rec_stat.config(text=f'已记录 {n} 条，{j} 个跳变')

    def _export_csv(self):
        if not self._rec_data:
            messagebox.showinfo('提示', '没有记录数据，请先点击"开始记录"')
            return
        ts = datetime.now().strftime('%Y%m%d_%H%M%S')
        fpath = os.path.join(LOG_DIR, f'mt12_log_{ts}.csv')
        with open(fpath, 'w', newline='', encoding='utf-8-sig') as f:
            w = csv.writer(f)
            header = ['时间戳', '时间']
            for i in range(12):
                header += [f'CH{i+1}_PV', f'CH{i+1}_SV', f'CH{i+1}_PWM%']
            header.append('跳变')
            w.writerow(header)
            jump_ts = {e[0] for e in self._jump_events}
            for row in self._rec_data:
                line = [f'{row["ts"]:.3f}',
                        datetime.fromtimestamp(row['ts']).strftime('%H:%M:%S.%f')[:12]]
                for i in range(12):
                    pv  = row['pv'][i]
                    sv  = row['sv'][i]
                    pwm = row['pwm'][i]
                    line.append(f'{pv*0.1:.1f}'  if pv  is not None else '')
                    line.append(f'{sv*0.1:.1f}'  if sv  is not None else '')
                    line.append(f'{pwm/32767*100:.1f}' if pwm is not None else '')
                line.append('Y' if row['ts'] in jump_ts else '')
                w.writerow(line)
        # 跳变汇总追加
        if self._jump_events:
            jump_path = os.path.join(LOG_DIR, f'mt12_jumps_{ts}.csv')
            with open(jump_path, 'w', newline='', encoding='utf-8-sig') as f:
                w = csv.writer(f)
                w.writerow(['时间', '通道', '跳变前(°C)', '跳变后(°C)', '差值(°C)'])
                for ev in self._jump_events:
                    t, ch, old, new = ev
                    w.writerow([
                        datetime.fromtimestamp(t).strftime('%H:%M:%S.%f')[:12],
                        f'CH{ch+1}',
                        f'{old*0.1:.1f}', f'{new*0.1:.1f}',
                        f'{(new-old)*0.1:+.1f}'
                    ])
            messagebox.showinfo('导出完成',
                f'数据：{os.path.basename(fpath)}\n跳变：{os.path.basename(jump_path)}\n目录：{LOG_DIR}')
        else:
            messagebox.showinfo('导出完成',
                f'{os.path.basename(fpath)}\n目录：{LOG_DIR}')

    def _show_jumps(self):
        if not self._jump_events:
            messagebox.showinfo('跳变记录', '暂无跳变事件（阈值 ±5°C）')
            return
        win = tk.Toplevel(self)
        win.title('温度跳变事件')
        win.configure(bg=BG)
        win.geometry('480x360')
        cols = ('时间', '通道', '跳变前', '跳变后', '差值')
        tree = ttk.Treeview(win, columns=cols, show='headings')
        for c, w in zip(cols, [120, 60, 90, 90, 80]):
            tree.heading(c, text=c); tree.column(c, width=w, anchor='center')
        tree.tag_configure('pos', foreground=RED)
        tree.tag_configure('neg', foreground=BLU)
        for ev in self._jump_events:
            t, ch, old, new = ev
            diff = (new - old) * 0.1
            tag = 'pos' if diff > 0 else 'neg'
            tree.insert('', 'end', tags=(tag,), values=(
                datetime.fromtimestamp(t).strftime('%H:%M:%S.%f')[:12],
                f'CH{ch+1}',
                f'{old*0.1:.1f}°C', f'{new*0.1:.1f}°C',
                f'{diff:+.1f}°C'
            ))
        tree.pack(fill='both', expand=True, padx=8, pady=8)

    def _toggle_monitor(self):
        if self._monitoring:
            # 停止监听：冻结当前历史
            self._monitoring = False
            self._frozen_hist = list(self._history)
            self._mon_btn.config(text='▶ 继续监听', fg=GRN)
            self._slider_label.config(text='拖动调整视图')
            # 重置滑块
            self._xoffset_var.set(0)
            self._xwidth_var.set(1.0)
        else:
            self._monitoring = True
            self._frozen_hist = []
            self._mon_btn.config(text='⏸ 停止监听', fg=BLU)
            self._slider_label.config(text='(实时模式)')

    def _redraw_frozen(self):
        """停止时，根据滑块位置重绘曲线"""
        if self._monitoring or not self._frozen_hist:
            return
        hist = self._frozen_hist
        total_t = hist[-1]['ts'] - hist[0]['ts'] if len(hist) > 1 else 1
        width_ratio = max(0.05, self._xwidth_var.get())
        window = total_t * width_ratio
        max_offset = total_t - window
        offset = self._xoffset_var.get() * max(0, max_offset)
        t0_abs = hist[0]['ts']
        x_start = offset
        x_end   = offset + window
        self._draw_chart(hist, xlim=(x_start, x_end))

    def _clear_history(self):
        self._history.clear()
        self._rec_data.clear()
        self._jump_events.clear()
        self._frozen_hist.clear()

    # ── 定时刷新 ──────────────────────────────────────
    def _tick(self):
        with self._sniffer._lock:
            pv  = list(self._sniffer.pv)
            sv  = list(self._sniffer.sv)
            pwm = list(self._sniffer.pwm)

        ts = time.time()

        # 连接指示灯
        if self._sniffer.connected:
            self._conn_dot.config(fg=GRN)
            self._conn_lbl.config(text='已连接', fg=GRN)
        else:
            self._conn_dot.config(fg=RED)
            self._conn_lbl.config(text='串口断开', fg=RED)
        self._cnt_lbl.config(text=f'帧: {self._sniffer.frames_ok}')

        # 跳变检测
        if self._history and pv[0] is not None:
            prev = self._history[-1]['pv']
            for i in range(12):
                if prev[i] is not None and pv[i] is not None:
                    if abs(pv[i] - prev[i]) >= JUMP_THR:
                        if self._recording:
                            self._jump_events.append((ts, i, prev[i], pv[i]))

        # 更新历史
        if pv[0] is not None:
            snap = {'ts': ts, 'pv': list(pv), 'sv': list(sv), 'pwm': list(pwm)}
            self._history.append(snap)
            if len(self._history) > MAX_HIST:
                self._history.pop(0)
            if self._recording:
                self._rec_data.append(snap)

        # 更新表格
        any_burn = False
        for i in range(12):
            pv_v  = pv[i]
            sv_v  = sv[i]
            pwm_v = pwm[i]
            sv_s  = '--' if sv_v is None else (f'{sv_v*0.1:.1f}°C' if sv_v else '未设定')
            pwm_s = '--' if pwm_v is None else f'{pwm_v/32767*100:.1f}%'

            if pv_v is None:
                self._tree.item(str(i), values=(f'CH{i+1:02d}', '--', sv_s, pwm_s, '等待'), tags=('na',))
            elif pv_v >= BURN_RAW:
                self._tree.item(str(i), values=(f'CH{i+1:02d}', '1200.0°C', sv_s, pwm_s, '⚠断偶'), tags=('burn',))
                any_burn = True
            elif pwm_v is not None and pwm_v > 0:
                self._tree.item(str(i), values=(f'CH{i+1:02d}', f'{pv_v*0.1:.1f}°C', sv_s,
                                                 pwm_s, f'🔥{pwm_v/32767*100:.0f}%'), tags=('heat',))
            else:
                if sv_v and pv_v is not None and sv_v - pv_v > 20:
                    st = '待加热'
                elif sv_v and pv_v is not None and abs(sv_v - pv_v) <= 20:
                    st = '✅到温'
                else:
                    st = '停止'
                self._tree.item(str(i), values=(f'CH{i+1:02d}', f'{pv_v*0.1:.1f}°C',
                                                 sv_s, pwm_s, st), tags=('ok',))

        # 更新曲线图（实时模式才自动刷新）
        if self._monitoring and len(self._history) >= 2:
            self._draw_chart(self._history)

        self.after(500, self._tick)

    def _draw_chart(self, hist, xlim=None):
        """通用绘图函数，hist 为数据列表，xlim=(x0,x1) 可选"""
        self._ax.cla()
        self._ax.set_facecolor('#12122a')
        self._ax.tick_params(colors=GREY, labelsize=8)
        self._ax.spines[:].set_color('#3d3d6d')
        self._ax.set_xlabel('Time (s)', color=GREY, fontsize=8)
        self._ax.set_ylabel('Temp (C)', color=GREY, fontsize=8)

        if not hist:
            self._canvas.draw_idle()
            return

        t0 = hist[0]['ts']
        times = [h['ts'] - t0 for h in hist]

        drawn = False
        for i in self._selected_chs:
            vals = [h['pv'][i] for h in hist]
            if any(v is not None and v < BURN_RAW for v in vals):
                y = [v * 0.1 if (v is not None and v < BURN_RAW) else None for v in vals]
                tx = [times[j] for j, v in enumerate(y) if v is not None]
                ty = [v for v in y if v is not None]
                if len(tx) >= 2:
                    self._ax.plot(tx, ty, color=CH_COLORS[i], linewidth=1.2,
                                  label=f'CH{i+1}')
                    drawn = True

        if drawn:
            self._ax.legend(loc='upper left', fontsize=7,
                            facecolor='#1a1a2e', labelcolor=FG, framealpha=0.6)
        if xlim:
            self._ax.set_xlim(xlim)
        self._ax.yaxis.set_major_formatter(mticker.FuncFormatter(lambda x, _: f'{x:.0f}'))
        self._fig.tight_layout(pad=0.5)
        self._canvas.draw_idle()

    def destroy(self):
        self._sniffer.stop()
        super().destroy()


if __name__ == '__main__':
    app = App()
    app.mainloop()
