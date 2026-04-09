# MT-12 加热控制逻辑文档

## 1. HeatCtrlMd 寄存器 (CDM 20101)

主机通过 RS485 写入此寄存器控制加热模式和通道开关。

### 位定义

| 位 | 名称 | 说明 |
|----|------|------|
| [1:0] | Temp1 (主加热模式) | 0=OFF, 1=加热, 2=保温(PerHeat) |
| [2] | 保留 | - |
| [3] | Temp2 (副加热模式) | 0=无, 1=启用副加热 |
| [15:4] | SecTempSw (通道开关) | 每位控制一个通道，**反相逻辑** |

### 加热模式选择 (Temp.c:88-104)

```c
Temp1 = *(tempData[0].HeatCtrlMd) & 0x03;   // 主模式
Temp2 = (*(tempData[0].HeatCtrlMd) & 0x08) >> 3; // 副模式

if(Temp1)       Temp = Temp1;    // 优先主模式
else if(Temp2)  Temp = Temp2;    // 其次副模式
else            Temp = OFF;      // 都为0则关闭
```

### SecTempSw 通道开关 (Temp.c:106)

```c
SecTempSw = ~((*tempData[0].HeatCtrlMd >> 4) & 0xfff);
```

**反相逻辑**：HeatCtrlMd 的 bit N+4 = 0 → 通道 N **启用**；= 1 → 通道 N **禁用**

| HeatCtrlMd 值 | 二进制 (bits[15:4]) | SecTempSw | 启用通道 |
|---------------|---------------------|-----------|----------|
| 0xFFC8 (65480) | 1111 1111 1100 | 0x003 | 仅通道 0,1 |
| 0x0008 | 0000 0000 0000 | 0xFFF | 全部 12 通道 |
| 0x0018 | 0000 0000 0001 | 0xFFE | 通道 1-11 |

---

## 2. 加热相关寄存器地址表

### 温度设定 / PID 参数

| CDM 地址 | 字段 | 数量 | 说明 |
|----------|------|------|------|
| 20001+i | HeatSet | 12 | 加热目标温度 (×0.1°C) |
| 20016+i | PerHeatSet | 12 | 保温目标温度 |
| 20201+i | PIDp | 12 | PID P 参数 |
| 20216+i | PIDi | 12 | PID I 参数 |
| 20231+i | PIDd | 12 | PID D 参数 |
| 20246+i | PwmOffSet | 12 | PWM 偏移量 |
| 20291+i | Proportion | 12 | 比例带 |

### 控制模式 / 开关

| CDM 地址 | 字段 | 说明 |
|----------|------|------|
| 20101 | HeatCtrlMd | 加热控制模式 + 通道开关 |
| 20106+i | TwoHeatTm | 二段加热时间 (12通道) |
| 20261 | ThermostatFun | 恒温器功能开关 (位映射) |
| 20262 | SlowTpUpSw | 慢升温开关 |
| 20346+i | CycleTm | PWM 周期 (12通道) |
| 20401 | ContrlFun | 控制功能选择 |
| 20402 | AuTnFun | 自整定功能开关 (位映射) |
| 20403 | AutoHeatBranchSwitch | 自动加热分支开关 |
| 20404 | AutoPerHeatBranchSwitch | 自动保温分支开关 |
| 20405 | OilHeat | 油加热 |
| 20407 | KAndJSel | K/J 型热电偶选择 |
| 20408 | TwoUpFun | 二段升温功能 |
| 20409 | SynchronFun | 同步功能开关 |
| 20411 | TwoHeatMd | 二段加热模式 |
| 20412 | TwoHeatPercentMd | 二段加热百分比模式 |
| 20573 | AutoHeatMainSwitch | 自动加热主开关 |

### 显示 / 状态

| CDM 地址 | 字段 | 说明 |
|----------|------|------|
| 20501+i | TpControl | 控制温度值 (PID 设定点) |
| 20516+i | TpDisplay | 当前显示温度 (PV) |
| 20531+i | TpConAddHi | 控制温度 + 上限报警 |
| 20554 | HeatFlashBit | 加热闪烁位 (RealTpOutPwm) |
| 20555 | HeatErrSts | 加热错误状态 (位映射) |
| 20556 | HiAlmSts | 高温报警状态 |
| 20557 | LowAlmSts | 低温报警状态 |
| 20601+i | PwmOutput | PWM 输出值 (12通道) |
| 20640+i | PwmOutputCyc | PWM 输出周期计数 |
| 20800+i | TpVersion | 版本/调试信息 |
| 20850 | AutoTingSts | 自整定状态 |

### 定时 / 日期

| CDM 地址 | 字段 | 说明 |
|----------|------|------|
| 20391+i | HeatTm | 加热时间 (前7通道) |
| 20491+i | HeatPerTm | 保温时间 (前7通道) |
| 20551 | HeatHour | 加热小时 |
| 20552 | HeatMin | 加热分钟 |
| 20553 | HeatWeek | 加热星期 |
| 20558 | DataInitSet触发 | 非0时触发参数初始化 |
| 20559 | 下载模式标志 | 0x1368=下载模式 |

### 报警

| CDM 地址 | 字段 | 说明 |
|----------|------|------|
| 20031+i | HiAlarm | 高温报警偏差 (12通道) |
| 20046+i | LoAlarm | 低温报警偏差 (12通道) |
| 20061+i | CoolSet | 冷却设定 (12通道) |

---

## 3. 加热输出控制链路

### 数据流

```
主机 RS485 0x60 写入 → CDM 寄存器 (tempData[] 指针)
         ↓
TIM3 ISR (250ms) → TempSubScan() → TpCloseLoopSub() (PID)
         ↓
OrgTpOutPwm[12] (PID 输出值, 0~32767)
         ↓
TIM2 ISR (0.2ms) → RealTpOutMd() → RealTpOutPwm (16-bit 位映射)
         ↓
HT_OUT(~RealTpOutPwm) → GPIO (GPIOB/E/F/G)
```

### ISR_Timer0 加热条件 (IntTm0.c:62-111)

```
if (DnApSysFg != 0x2479) && (CDM[20559] != 0x1368)  // 非下载模式
│
├── for each channel i (0..11):
│   ├── if AuTnFun[i] && TpClLpFg3[i] && RsTxRxSts
│   │   → 自整定模式: 100% 或 0% 输出
│   │
│   ├── elif (ThermostatFun[i] && RsTxRxSts) ||
│   │        (TpClLpFg3[i] && HeatErrIR==0 && RsTxRxSts)
│   │   && ADErrorFlag == 0
│   │   → 正常加热: RealTpOutMd() 计算 PWM
│   │
│   └── else
│       → 关闭该通道: RealTpOutPwm &= ~(1<<i)
│
├── FirstRsTxRxStsOKFlag 管理 (11秒延迟)
│   if 首次 RS485 通信后 11 秒 → FirstRsTxRxStsOKFlag = ON
│   if RS485 断连 || ADError → 强制 HeatCtrlMd = 0 (关加热)
│
└── HT_OUT(~RealTpOutPwm)  // 输出到 GPIO (反相)
    if BURN_DETECT → HT_OUT(0xFFFF) (全关)
```

### TpClLpFg3 激活条件 (Temp.c:373)

```c
if ((Temp != 0)                           // 加热模式已开启
 && (TpControl[i] != 0)                   // 控制温度非0
 && (TpControl[i] != 0xFFFF)              // 控制温度有效
 && (BkPidPUnit[i] != 0xFFFF)             // PID P 有效
 && (BkPidPUnit[i] != 0)                  // PID P 非0
 && (TpPIDdata[i].Curr <= 5800)           // 当前温度 ≤ 580°C
 && (SecTempSw & (1 << i)))               // 通道开关启用
    TpClLpFg3 |= (1 << i);               // 激活 PID 环路
```

---

## 4. PID 控制算法

### 调用时机

TIM3 ISR (250ms) → `TempSubScan()` → 每 11.5 秒调用 `TpCloseLoopSub()`

### MJ86 模式 (ThermostatFun bit15 set, Temp.c:1538-1649)

```
Error = SetPoint - Current
P_term = (Error × 100) / P_param
I_term = (84 × I_param × Error) / (600 × P_param)
D_term = ((Current - OldCurrent) × 600 × D_param) / (84 × P_param)
Output = Isum + P_term - D_term
Output 限幅: [0, OutputPwmMaxUnit]
```

### MT-12 默认模式 (Temp.c:1650-1880)

```
if Error ≤ 0:        Output = 0 (已达温)
if Error > P_band:   Output = MAX (32767, 全功率加热)
if 0 < Error ≤ P:    PID 计算 + I 项累积
```

---

## 5. PWM 输出模式 (IntTm0.c RealTpOutMd)

### 三种模式

| 模式 | 触发条件 | 周期 | 说明 |
|------|----------|------|------|
| 恒温器 | ThermostatFun[i] | ThermostatTm[i] | ON/OFF 开关控制 |
| 继电器 | CycleTm > Sec025 | RelayTm[i] | 比例时间控制 |
| SSR | 默认 | 50ms (Sec025) | 连续 PWM |

### PWM 占空比计算

```c
ON_time = (OrgTpOutPwm[i] × Cycle_Period) / PwmCounterUnit;
// PwmCounterUnit = 32767
// 每 0.2ms 递增 PwmOutputCyc 计数器
// PwmOutputCyc < ON_time → 输出 ON
// PwmOutputCyc >= ON_time → 输出 OFF
```

---

## 6. HT_OUT GPIO 映射 (IntTm2.c:62-81)

### 数据位 → GPIO 引脚

```
输入 data (16-bit, 反相后):
  bit  0 → GPIOB Pin 0  (加热通道 0)
  bit  1 → GPIOB Pin 1  (加热通道 1)
  bit  2 → GPIOB Pin 2  (加热通道 2)
  bit  3 → GPIOF Pin 11 (加热通道 3)
  bit  4 → GPIOF Pin 12 (加热通道 4)
  bit  5 → GPIOF Pin 13 (加热通道 5)
  bit  6 → GPIOF Pin 14 (加热通道 6)
  bit  7 → GPIOF Pin 15 (加热通道 7)
  bit  8 → GPIOG Pin 0  (加热通道 8)
  bit  9 → GPIOG Pin 1  (加热通道 9)
  bit 10 → GPIOE Pin 7  (加热通道 10)
  bit 11 → GPIOE Pin 8  (加热通道 11)
```

### 位提取公式

```c
p_b = data & 0x07;              // bits [2:0] → PB[2:0]
p_f = (data & 0x00F8) << 8;     // bits [7:3] → PF[15:11]
p_g = (data & 0x0300) >> 8;     // bits [9:8] → PG[1:0]
p_e = (data & 0x0C00) >> 3;     // bits [11:10] → PE[8:7]
```

### 输出逻辑

- `HT_OUT(~RealTpOutPwm)` — 取反后输出
- RealTpOutPwm bit=1 → GPIO=0 → **加热 ON** (低电平有效)
- HT_CS (PC5) 作为锁存器片选

---

## 7. 加热错误检测 (HeatErrMd, main.c:2418-2451)

### 监测逻辑

```
每 250ms 累加温度变化量 OldTempSetSum[i]
经过 750 秒 (TimerBase2 >= 3000) 后检查:
  if OldTempSetSum[i] <= 10 (温度变化 ≤ 1.0°C)
     && 加热模式开启
     && 控制温度有效
     && 当前温度 ≤ 590°C
  → HeatErrSts |= (1<<i)     // 该通道加热异常

HeatErrIR = (HeatErrSts != 0) ? 1 : 0
```

### 对加热输出的影响

ISR_Timer0 条件: `TpClLpFg3[i] && (HeatErrIR == 0)`
- HeatErrIR=1 时，非恒温器模式的通道全部停止加热
- 恒温器模式 (ThermostatFun[i]) 不受 HeatErrIR 影响

---

## 8. RS485 寄存器写入 (IntTm3.c)

### 0x60 功能码 — 写多个寄存器

```
包结构: [ID] [0x60] [LenH] [LenL] {[AddrH][AddrL][DataH][DataL]}... [CRCH][CRCL]
每 4 字节为一组: 2 字节地址 + 2 字节数据
写入范围: 20001 ≤ addr < 21000
```

### PID 参数特殊处理 (IntTm3.c:543-556)

```c
if (addr >= 20201 && addr <= 20212)
    BkPidPUnit[addr - 20201] = data;   // P 参数实时更新
if (addr >= 20216 && addr <= 20227)
    BkPidIUnit[addr - 20216] = data;   // I 参数实时更新
if (addr >= 20231 && addr <= 20242)
    BkPidDUnit[addr - 20231] = data;   // D 参数实时更新
```

### 0x33 功能码 — 下载模式

写入后设置 `DnApSysFg = 0x1368` → ISR_Timer0 跳过加热 → `HT_OUT(0xFFFF)` 全关

---

## 9. 安全保护机制

| 保护 | 条件 | 动作 |
|------|------|------|
| RS485 断连 | RsTxRxSts=0 (10秒无通信) | 加热输出关闭 |
| ADC 错误 | ADErrorFlag != 0 | 加热输出关闭 + HeatCtrlMd=0 |
| 过温切断 | TpCurrUnit > 12000 (1200°C) | 该通道 TpCutFg 置位 |
| 断偶检测 | thermal = 0 或 0xFFFFFF | thermocouple_zero_cnt 累计 → ADErrorFlag |
| 过流检测 | BURN_DETECT = 1 | HT_OUT(0xFFFF) 全关 |
| 下载模式 | DnApSysFg = 0x2479 | HT_OUT(0xFFFF) 全关 |
| 加热异常 | 750秒内温变 ≤ 1°C | HeatErrIR=1 → PID 通道关闭 |
| 首次通信延迟 | 启动后 11 秒内 | FirstRsTxRxStsOKFlag=OFF → 不报错 |
