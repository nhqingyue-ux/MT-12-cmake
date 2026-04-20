# MT-12 开发调试工作流

> 本文档面向**日常固件迭代**（已有 bootloader、不动硬件）。
> 板卡量产级烧录（含 bootloader）见 `FLASH_SOP.md`。

---

## 一、环境

| 工具 | 用途 | 安装 |
|------|------|------|
| `arm-none-eabi-gcc` | 编译固件 | `brew install arm-none-eabi-gcc` |
| `cmake` + `ninja` | 构建系统 | `brew install cmake ninja` |
| `stlink` (`st-flash`, `st-util`) | 烧录/调试 | `brew install stlink` |
| `/opt/homebrew/bin/python3` | 运行 GUI（sniffer）| Homebrew 默认提供 |

**系统 Python（`/usr/bin/python3`）不可用于 GUI**：Tk 版本检查会报
`macOS 26 (2603) or later required, have instead 16 (1603)`，直接崩溃。一律走 homebrew。

---

## 二、日常迭代循环（改代码 → 验证）

```
┌─────────────┐     ┌─────────┐     ┌─────────┐     ┌──────────┐     ┌───────────┐
│  改 src/*.c │ --> │  Build  │ --> │  Flash  │ --> │ Sniffer  │ --> │  分析 CSV  │
└─────────────┘     └─────────┘     └─────────┘     └──────────┘     └───────────┘
                      ninja            app only       实时曲线         Python/pandas
                     ~数秒             ~3 秒          + 导出
```

### 1. 构建

项目根目录（`MT-12-cmake_V2.0/`）下：

```bash
# 首次：初始化构建目录
cmake --preset default

# 日常：增量编译（主固件）
cmake --build --preset main
```

**输出：** `build/default/M4Temp.{elf,bin,hex}`

**体积参考：** 当前 `.text ≈ 49804 B`, `.data ≈ 252 B`, `.bss ≈ 32292 B`。
任何改动导致 `.text` 剧烈跳动（±10 KB）需警觉——通常是意外包含了新库。

### 2. 烧录（仅 App 区）

```bash
st-flash write build/default/M4Temp.bin 0x08008000
```

**关键约束：**
- ❌ 不要 `st-flash erase`（会连 bootloader 一起抹掉，必须重烧整张盘）
- ❌ 地址必须是 `0x08008000`（App 入口），不是 `0x08080000`
- ✅ bootloader 在 `0x08000000`，日常迭代不需要动它
- ✅ 烧完会自动复位运行新固件，无需手动重启

**成功标志：**
```
Flash written and verified! jolly good!
```

**失败：`Failed to enter SWD mode`** → 检查 NRST 是否接（见 `FLASH_SOP.md`）。

### 3. 打开 Sniffer GUI

```bash
/opt/homebrew/bin/python3 scripts/mt12_sniffer_gui.py
```

- 自动扫描 `/dev/cu.usbserial-*`，默认选第一个
- 有两个 USB 串口时（常见：一路转 ST-Link + 一路 RS485）手动在下拉菜单切换
- 38400 8N1，被动监听总线，**不发任何指令**（不会干扰主机↔板子的通信）

**UI 功能：**
- 实时曲线：选通道后自动绘制
- 跳变自动标注：`|Δ|` 超阈值（默认 5°C）自动标红
- CSV 导出：点右上角"导出 CSV"，保存到 `logs/` 目录

### 4. CSV 文件

导出到 `logs/` 下，文件名格式：

```
logs/mt12_log_YYYYMMDD_HHMMSS.csv     # 完整时序数据（每个采样1行）
logs/mt12_jumps_YYYYMMDD_HHMMSS.csv   # 仅跳变事件
```

**log CSV 列：**
```
时间戳, 时间, CH1_PV, CH1_SV, CH1_PWM%, CH2_PV, ..., CH12_PWM%, 跳变
```

- 时间戳：UNIX 浮点秒（分析脚本用）
- 时间：`HH:MM:SS.ms`（人眼阅读）
- 采样率：约 **0.5 s/样本**（受 sniffer GUI 循环限制，不是固件 scan 率）

**jumps CSV 列：**
```
时间, 通道, 跳变前(°C), 跳变后(°C), 差值(°C)
```

`logs/` 已加入 `.gitignore`，CSV 不会进版本库。

---

## 三、快速分析模板（Python）

### 3.1 快速浏览某次测试

```python
import csv

with open('logs/mt12_log_YYYYMMDD_HHMMSS.csv', encoding='utf-8-sig') as f:
    rows = list(csv.DictReader(f))

print(f"样本: {len(rows)}, 时长: {rows[0]['时间']} → {rows[-1]['时间']}")

for ch in range(1, 13):
    pvs = [float(r[f'CH{ch}_PV']) for r in rows if float(r[f'CH{ch}_PV']) < 1100]
    if pvs and max(pvs) - min(pvs) > 5:
        print(f"  CH{ch}: {min(pvs):.1f} ~ {max(pvs):.1f}°C")
```

### 3.2 统计跳变次数（heating vs cooling 分开）

```python
def jump_stats(rows, ch):
    sv = float(rows[-1][f'CH{ch}_SV'])
    pvs = [float(r[f'CH{ch}_PV']) for r in rows]
    deltas = [pvs[i+1]-pvs[i] for i in range(len(pvs)-1)
              if pvs[i] < 1100 and pvs[i+1] < 1100]  # 排除断偶
    return {
        '≥+10°C': sum(1 for d in deltas if d >= 10),
        '≤-10°C': sum(1 for d in deltas if d <= -10),
        '≥+50°C': sum(1 for d in deltas if d >= 50),
        '最大+': max(deltas) if deltas else 0,
        '最大-': min(deltas) if deltas else 0,
    }
```

### 3.3 找时间分段（SV 变化点）

```python
last_svs = None
for r in rows:
    svs = tuple(r[f'CH{ch}_SV'] for ch in range(1, 13))
    if svs != last_svs:
        print(f"{r['时间']}  SV变化: {svs}")
        last_svs = svs
```

### 3.4 过滤断偶值（1200°C）

分析温度变化时，**必须先排除 ≥1100°C 的样本**——它们是 `12000` 断偶哨兵值除 10 后显示，
若参与 delta 计算会产生 ±1100°C 的伪跳变，污染统计。

```python
deltas = [pvs[i+1]-pvs[i] for i in range(len(pvs)-1)
          if pvs[i] < 1100 and pvs[i+1] < 1100]
```

---

## 四、Git 工作流

### 4.1 提交规范

按 repo 现有风格，**一个逻辑改动 = 一个 commit**，title 清晰具体：

```
Cooling filter: Median-3 → Median-5 for better noise rejection
Adaptive filter v4: PWM-aware mode + cooling up-only slew
Update product ID: MX1-M4I12040 → MS-TEMP02617
```

Body 用英文，陈述动机 + 数据对比 + 取舍。保留 `Co-Authored-By: Claude ...` 行。

### 4.2 哪些不提交

`.gitignore` 已排除：
```
build/          # 编译产物
*.elf *.bin *.hex *.map *.o
.DS_Store
.!*!*           # macOS smbfs 锁文件
logs/           # 测试 CSV 导出
```

### 4.3 遇到 smbfs 锁文件（`.!XXXXX!filename`）

Windows 共享目录上 macOS Finder 偶发残留，**不要手动删**（可能是其他进程打开中）。
已 gitignore，直接忽略即可。若多了影响观察，可选择性 `rm src/.!*!*`（谨慎）。

---

## 五、调试（GDB + ST-Link）

平时验证功能不需要 GDB，Sniffer 看波形就够。以下适用于**疑难排查**（卡死、ISR 问题、寄存器状态异常）。

### 5.1 启动 GDB Server

```bash
st-util &    # 默认监听 :4242
```

### 5.2 连接 GDB

```bash
arm-none-eabi-gdb build/default/M4Temp.elf
(gdb) target remote :4242
(gdb) monitor reset halt
(gdb) load              # 可选：重烧一次（比 st-flash 略慢但集成化）
(gdb) b TempSubScan     # 设断点
(gdb) c                 # continue
```

### 5.3 常用断点/观察点

| 场景 | 命令 |
|------|------|
| 进入 TIM2 ISR | `b ISR_Timer0` |
| 进入温度滤波 | `b TempSubScan` |
| 监控某通道滤波输出 | `watch Bk_thermal_couple[0]` |
| 查看 ADC 原始值 | `p thermal_couple[0]` |
| 查看 PWM 状态 | `p/x RealTpOutPwm` |
| 看堆栈（卡死时）| `bt` |

### 5.4 常见陷阱

- **ISR 断点会拖慢主循环**，可能引起 RS485 心跳丢包。排查完立刻 `delete` 断点。
- **`-O0` 编译会让 TIM2 ISR 超时**（> 0.2 ms），主循环饿死，RS485 TX 不发。定位 PID 问题时改 `-Og` 比 `-O0` 好。
- **watchpoint 数量受限**（Cortex-M4 只有 4 个），用完会报错。

---

## 六、本地验证 checklist（每次改动完成前）

- [ ] `cmake --build --preset main` 编译通过，无新 warning 增量
- [ ] `.text` 大小变化合理（< ±1 KB 通常 OK，大变化要追问）
- [ ] `st-flash write ... 0x08008000` 烧录验证 `jolly good!`
- [ ] 板子重启后 RUN 灯正常闪烁，sniffer 显示"已连接"
- [ ] 覆盖目标功能场景跑一次，导出 CSV
- [ ] Python 快速统计跳变 / 断偶 / 偏差 3 项指标
- [ ] commit + push（若验证通过）

---

## 七、参考

- `docs/FLASH_SOP.md` — 量产级烧录（含 bootloader、硬件检查清单）
- `docs/MT12_PROTOCOL.md` — RS485 通信协议
- `docs/register_map.md` — Modbus 寄存器地址
- `docs/cooling_filter_optimization_20260417.md` — 温度抗抖滤波优化全记录
- `CLAUDE.md` — 项目整体说明、迁移坑位
