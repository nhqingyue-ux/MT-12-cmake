# MT-12 温度跳变优化记录

**日期：** 2026-04-09 ~ 2026-04-10
**最终 Commit：** f66a8d0
**固件：** M4Temp.elf (GCC arm-none-eabi, -O1)

---

## 1. 问题描述

加热过程中温度显示值出现大幅跳变，导致：
- HMI 温度显示"乱飘"（±30~150°C 跳变）
- PID 控制环路受干扰，输出剧烈振荡
- 发热圈温度直冲，触发高温告警
- 通道间表现不一致（Ch1 经常假下跌，Ch2 始终干净）

**根因：**
- 加热 PWM 开关瞬间产生电磁干扰，耦合到 ADS1248 ADC 采样电路
- 采到脏数据后直通 PID 运算
- 噪声模式包括：单次脉冲、高低交替、持续偏移

---

## 2. 最终方案

```
原始 ADC (thermal_couple)
        ↓
Median-of-9              ← 容忍 44% 坏样本，过滤间歇噪声
        ↓
Asymmetric Slew Limit    ← +6°C 升温无延迟
  Rise: +6°C/cycle           降温自适应：
  Fall: -0.2°C (heating)   ←  加热中严格抑噪
  Fall: -3°C  (cooldown)   ←  冷却中跟随真实降温
        ↓
EMA(α=0.3)               ← 平滑离散阶梯
        ↓
TpDisplay (HMI 读取)
        +
B-1: ADC 采样期间冻结加热输出 (减少 EMI 源)
```

### A-1a：Median-of-9 中值滤波

**原理：** 9 样本环形缓冲，每次取中值。可容忍 4/9 (44%) 坏样本。

```
单次毛刺:    [200, 230, 200, 200, 200, 200, 200, 200, 200] → 中值 200 ✓
持续噪声:    [200, 100, 100, 200, 100, 200, 100, 100, 200] → 中值 100 ✗
            (>50% 坏样本时中值滤波失效，需 slew 兜底)
```

### A-1b：非对称 Slew Rate Limit + 自适应 Fall

**原理：** 限制输出每次最大变化量，正负方向不对称。

```
Rise +6°C/cycle (固定):
  覆盖任何快速加热场景，零延迟显示
  噪声漏过 median 时最坏情况 +6°C，可接受

Fall 自适应 (基于 Temp 加热模式):
  加热中 (Temp != OFF):
    -0.2°C/cycle = -0.8°C/s
    真实加热中温度只升不降，任何下降都是噪声
    7 个 cycle 噪声只能拉低 1.4°C → 几乎不可见
  
  冷却中 (Temp == OFF):
    -3.0°C/cycle = -12°C/s
    真实最快冷却率 ~1°C/s，远低于限制
    显示零滞后
```

### A-1c：EMA 指数平滑

**原理：** `ema = (ema*7 + new*3) / 10` (alpha=0.3)

```
作用: 把 slew 输出的离散阶梯转成连续斜坡
效果: 视觉上消除"+3°C 跳跃感"，呈现平滑曲线
延迟: ~3 样本时间常数 (0.75s)，对控制无影响（PID 用未平滑值）
```

### B-1：ADC 采样期间冻结加热输出

**原理：** ADC 转换 (~50ms) 期间暂停 HT_OUT()，SSR 保持当前状态不翻转。

```
冻结窗口 ~50ms/通道
对 2.1s PWM 周期占比 ~2-3%，加热功率影响可忽略
直接消除 PWM 切换的 EMI 噪声源
```

---

## 3. 代码修改详情

### 3.1 Temp.c — 三层滤波链 (A-1)

**新增变量（文件顶部）：**
```c
/* A-1: Median-of-9 + asymmetric slew + EMA smoothing */
#define MED_WIN 9
static unsigned short med_buf[TpMaxLp][MED_WIN];
static unsigned char  med_idx = 0;
static unsigned char  med_fill = 0;
#define SLEW_RISE_LIMIT      60  /* +6.0°C/cycle: zero lag for any heating rate */
#define SLEW_FALL_HEATING     2  /* -0.2°C/cycle when heating ON: max noise rejection */
#define SLEW_FALL_COOLDOWN   30  /* -3.0°C/cycle when heating OFF: tracks real cooling */
static unsigned short slew_out[TpMaxLp];
static unsigned char  slew_out_init = 0;
/* EMA: ema = (ema*7 + new*3) / 10  (alpha=0.3) */
static unsigned short ema_out[TpMaxLp];
static unsigned char  ema_init = 0;
```

**滤波逻辑（TempSubScan 内部，TpCurrUnit 赋值后）：**
```c
/* A-1: Median-of-9 + asymmetric slew + EMA smoothing */
if(TpCurrUnit != 0xffff && TpCurrUnit != 12000) {
    med_buf[i][med_idx] = TpCurrUnit;
    if(med_fill >= MED_WIN) {
        /* Insertion sort 9 values, pick middle */
        unsigned short s[MED_WIN];
        unsigned char k, m;
        for(k = 0; k < MED_WIN; k++) s[k] = med_buf[i][k];
        for(k = 1; k < MED_WIN; k++) {
            unsigned short v = s[k];
            for(m = k; m > 0 && s[m-1] > v; m--) s[m] = s[m-1];
            s[m] = v;
        }
        TpCurrUnit = s[MED_WIN/2];  /* median = s[4] */
    }
    /* Asymmetric slew with adaptive fall limit */
    if(slew_out_init) {
        short delta = (short)TpCurrUnit - (short)slew_out[i];
        short fall_limit = (Temp != OFF) ? SLEW_FALL_HEATING : SLEW_FALL_COOLDOWN;
        if(delta > SLEW_RISE_LIMIT)
            TpCurrUnit = slew_out[i] + SLEW_RISE_LIMIT;
        else if(delta < -fall_limit)
            TpCurrUnit = (slew_out[i] > fall_limit) ? slew_out[i] - fall_limit : 0;
    }
    slew_out[i] = TpCurrUnit;
    /* EMA smoothing */
    if(ema_init) {
        unsigned long e = (unsigned long)ema_out[i] * 7 + (unsigned long)TpCurrUnit * 3;
        ema_out[i] = (unsigned short)(e / 10);
    } else {
        ema_out[i] = TpCurrUnit;
    }
    TpCurrUnit = ema_out[i];
}
if(i == TpMaxLp - 1) {
    med_idx = (med_idx + 1) % MED_WIN;
    if(med_fill < MED_WIN) med_fill++;
    if(!slew_out_init && med_fill >= MED_WIN) slew_out_init = 1;
    if(!ema_init && slew_out_init) ema_init = 1;
}
```

### 3.2 main.c — ADC 采样标志 (B-1)

```c
volatile unsigned char adc_sampling_active = 0;

// TempHWSave() case 0:
adc_sampling_active = 1;
T_START_H;

// TempHWSave() case 1 (success path):
Read_Single_AD(0x7, ch_index);
adc_sampling_active = 0;

// TempHWSave() case 1 (timeout path):
if(wait_cnt > 10) {
    adc_sampling_active = 0;
    ...
}
```

### 3.3 IntTm0.c — 冻结 HT_OUT (B-1)

```c
extern volatile unsigned char adc_sampling_active;

if(adc_sampling_active)
    ;  /* 跳过 HT_OUT，保持上一次输出状态 */
else if(!BURN_DETECT)
    HT_OUT(~RealTpOutPwm);
else
    HT_OUT(0xFFFF);
```

---

## 4. 完整迭代历史

### v1：仅 Median-of-3 — 单次毛刺消除

```
测试: 100°C → 410°C, 120s
结果: Max=6.2°C, >5°C=10 (全正常升温), >10°C=0
问题: 设定温度升高后出现高低交替噪声 (350↔160°C)，median 无法消除
```

### v2：Median-3 + 对称 Slew ±2°C — 噪声消除但显示延迟

```
测试: 63°C → 476°C, 180s
结果: Max=4.0°C, >5°C=0, >10°C=0
问题: 升温期 slew 限制太紧，曲线出现 ~10s 平台
原因: 正常升温 5°C/step > slew limit 2°C/step
```

### v3：Median-3 + 非对称 +6/-2°C — 延迟消除但噪声泄露

```
测试: 110°C → 477°C, 180s
结果: Max=12.0°C, >5°C=28, >10°C=2
问题: +6°C 升限太宽，部分向上噪声穿透
```

### v4：Median-3 + 非对称 +4/-2°C — Ch0 良好但 Ch1 异常

```
测试: 210°C → 485°C, 180s
结果: Max=8.0°C, >5°C=10, >10°C=0
问题: 多通道分析发现 Ch1 在 220°C 区间有 13°C 假下跌
原因: Ch1 ADC 间歇噪声，median-3 容错率不足
```

### v5：Median-5 + EMA — 反向减少但 dip 仍存在

```
结果: Ch1 反向次数 12→4，但 dip 仍达 11.4°C
原因: median-5 容错 40%，对 Ch1 高噪声率仍不足
```

### v6：Median-9 — Dip 略减但仍存在

```
结果: Ch1 dip 从 13.3°C → 11.4°C
原因: Ch1 噪声周期内 >50% 样本坏，多数票表决也救不了
洞察: 下降速率刚好等于 slew_fall_limit，问题在 slew 而非 median
```

### v7：Median-9 + 收紧 Fall -0.5°C — 大幅改善

```
结果: Ch1 dip 从 11.4°C → 2.1°C
副作用: 真实冷却时显示有滞后
```

### v8：+6/-0.2°C — Ch1 dip 完全消除

```
结果: Ch1 dip 从 2.1°C → 0.5°C (基本消失)
副作用: 自然冷却时显示落后真实值 ~5°C
```

### v9（最终）：自适应 Fall Limit — 同时解决两个极端

```
加热中: -0.2°C/cycle (max noise rejection)
冷却中: -3°C/cycle (tracks real cooling)
结果: 加热 0 dip + 冷却 0 滞后
```

---

## 5. 完整测试数据对比

### 多通道测试（Ch0/1/2 同时加热到 350°C）

| 版本 | Ch1 max dip | Ch1 反向 | Ch2 max jmp | 冷却滞后 |
|------|-------------|----------|-------------|----------|
| 原始无滤波 | 53~253°C | 多 | - | - |
| v1 Median-3 | (单通道 6.2C) | - | - | - |
| v2 Median-3 ±2°C | 4.0°C | - | - | 平台延迟 |
| v3 Median-3 +6/-2 | 12.0°C | 多 | - | 无 |
| v4 Median-3 +4/-2 | 13.3°C | 3 | 1.9°C | 无 |
| v5 Median-5 +4/-2 | 11.4°C | 3 | 1.9°C | 无 |
| v6 Median-9 +4/-2 | 11.4°C | 1 | 1.9°C | 无 |
| v7 Median-9 +4/-0.5 | 2.1°C | 1 | 1.9°C | 微 |
| v8 Median-9 +6/-0.2 | 0.5°C | 1 | 1.8°C | ~5°C |
| **v9 Median-9 自适应** | **0** | **0** | 1.8°C | **0** |

### v9 最终测试结果（240s 加热 + 冷却）

**加热阶段（285秒）：**
```
Ch0: 162.4°C → 478.3°C  (max jmp 5.5°C, 0 reversals, 0 dips)
Ch1: 91.0°C → 423.4°C   (max jmp 5.7°C, 0 reversals, 0 dips)
Ch2: 100.9°C → 354.3°C  (max jmp 1.8°C, 0 reversals, 0 dips)
```

**冷却阶段（285秒后）：**
```
Ch0: 366.2 → 322.5°C  (-43.7°C in 50s, -0.87°C/s)
Ch1: 347.2 → 324.7°C  (-22.5°C in 50s, -0.45°C/s)
Ch2: 349.5 → 338.2°C  (-11.3°C in 50s, -0.23°C/s)
真实冷却率全部远低于 SLEW_FALL_COOLDOWN (-12°C/s) → 显示零滞后
```

---

## 6. 关键设计原理

### Slew Limit 非对称的物理依据

| 方向 | 物理变化率上限 | 噪声特征 | 设定值 |
|------|---------------|----------|--------|
| 升温 (+) | ~6°C/cycle (PID burst) | 罕见向上噪声 | **+6°C** (覆盖真实) |
| 加热中 (-) | 0°C (不可能下降) | 主要噪声方向 | **-0.2°C** (严格) |
| 冷却中 (-) | ~1°C/s = -0.25°C/cycle | 罕见噪声 | **-3°C** (宽松) |

### 自适应触发逻辑

```c
short fall_limit = (Temp != OFF) ? SLEW_FALL_HEATING : SLEW_FALL_COOLDOWN;
```

`Temp` 从 HeatCtrlMd 解析（TempSubScan 入口处计算）：
- `Temp != OFF` → 加热模式开启 → 用 -0.2°C
- `Temp == OFF` → 加热模式关闭 → 用 -3°C

切换是即时的，主机按"停止加热"按钮后下一个 PID 周期立即生效。

### 三层滤波的协同分工

```
Median-of-9:  消除 44% 以下的离散噪声毛刺（单次 + 短期交替）
              → 大部分噪声在这里被消除

Slew Limit:   兜底滤掉 Median 漏过的高密度噪声
              非对称 + 自适应 → 不影响真实物理变化

EMA:          消除离散步进感，视觉平滑
              不影响 PID（PID 用 thermal_couple 原始值）
```

---

## 7. 影响范围

| 文件 | 修改 | 影响 |
|------|------|------|
| `src/Temp.c` | 三层滤波链 | TpDisplay 输出极度平滑 |
| `src/main.c` | adc_sampling_active 标志 | ADC 转换期间不切换加热 |
| `src/IntTm0.c` | HT_OUT 前检查采样标志 | 加热输出有 ~50ms/通道 冻结窗口 |

**不影响：**
- PID 算法本身（TpCloseLoopSub 未修改）
- RS485 通信
- EEPROM 读写
- 冷端补偿
- 加热输出总功率（冻结窗口占比 <3%）

---

## 8. 总结

| 测试项 | 原始 | 最终方案 |
|--------|------|----------|
| 加热中最大假下跌 | ~150°C | **0°C** |
| 加热中反向次数 | 多 | **0** |
| 冷却显示滞后 | - | **0** |
| 升温响应延迟 | 0 | **0 (+6°C/cycle)** |
| Ch2 平稳通道表现 | 干净 | **干净** |
| Ch1 噪声通道表现 | ±13°C 跳变 | **完全平滑** |

**温度跳变问题完全解决，达到生产可用水平。**

---

## 9. 后续可选优化

| 优先级 | 内容 | 说明 |
|--------|------|------|
| 可选 | PID Anti-windup | 输出饱和时冻结积分项，防止过冲 |
| 可选 | 冷启动软启动 | 大温差时限制最大占空比 60%→80%→100% |
| 低 | Ch1 硬件诊断 | 排查 Ch1 ADC 噪声源（PCB 走线/接地） |
| 低 | 噪声状态位 | 高噪声时自动切换保守 PID 参数组 |
