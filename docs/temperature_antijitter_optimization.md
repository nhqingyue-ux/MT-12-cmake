# MT-12 温度跳变优化记录 (A-1 + B-1)

**日期：** 2026-04-09
**最终 Commit：** cb67d5f
**固件：** M4Temp.elf (GCC arm-none-eabi, -O1)

---

## 1. 问题描述

加热过程中温度显示值出现大幅跳变（±30~150°C），导致：
- HMI 温度显示"乱飘"
- PID 控制环路受干扰，输出剧烈振荡
- 发热圈温度直冲，触发高温告警

**根因：** 加热 PWM 开关瞬间产生电磁干扰，耦合到 ADS1248 ADC 采样电路，采到脏数据后直通 PID 运算。噪声模式包括单次脉冲和高低交替两种。

---

## 2. 优化方案

采用 **A-1（Median-of-3 中值滤波 + 非对称 Slew Limit）+ B-1（采样-加热解耦）** 组合方案。

### A-1a：Median-of-3 中值滤波

**原理：** 保留最近 3 次采样值，取中值作为输出。单次脉冲噪声永远被排除。

```
采样序列:  [280.0, 230.0, 285.0]  ← 230 是噪声毛刺
排序:      [230.0, 280.0, 285.0]
中值:      280.0 ✓  毛刺被消除

采样序列:  [280.0, 285.0, 290.0]  ← 正常升温
排序:      [280.0, 285.0, 290.0]
中值:      285.0 ✓  不影响趋势
```

**局限：** 对「交替噪声」无效 — [高,低,高] 取中值=高，但 [低,高,低] 取中值=低，交替穿透。

### A-1b：非对称 Slew Rate Limit

**原理：** 限制输出值每次最大变化量，正方向（升温）和负方向（降温）使用不同阈值。

```
升温方向: +4°C/step → 覆盖正常加热速率 (~5°C/step)，极小延迟
降温方向: -2°C/step → 加热中温度不可能瞬降 150°C，严格限制

噪声特征: 加热中脏数据总是大幅下跳 (350→160°C)
正常升温: 只有正方向 +5°C/step
→ 放宽正方向不放过噪声，收紧负方向卡住噪声
```

**交替噪声处理示例：**
```
真实温度 ~350°C，噪声交替: 350, 160, 360, 160, 370...
Median 输出:               350, 160, 360, 160, 370  (交替穿透)
+Slew limit:
  prev=350, in=350 → 350  (no change)
  prev=350, in=160 → 350-2=348  (降幅限制)
  prev=348, in=360 → 348+4=352  (升幅限制)
  prev=352, in=160 → 352-2=350  (降幅限制)
  prev=350, in=370 → 350+4=354  (升幅限制)
→ 输出稳定在 348~354 范围 (±3°C 波动)
```

### B-1：ADC 采样期间冻结加热输出

**原理：** ADC 转换期间暂停 HT_OUT() 更新，SSR 保持当前状态不翻转，消除 PWM 开关噪声源。

```
时序:
  ┌─── ADC 转换 (~50ms) ───┐
  │  adc_sampling_active=1  │
  │  HT_OUT 冻结（不调用）   │
  │  SSR 保持上一状态        │
  └─────────────────────────┘
  │  adc_sampling_active=0  │
  │  HT_OUT 恢复正常        │
```

冻结窗口 ~50ms/通道，对 2.1s PWM 周期占比 ~2-3%，对加热功率影响可忽略。

---

## 3. 代码修改详情

### 3.1 Temp.c — Median + 非对称 Slew (A-1)

**位置：** `TempSubScan()` 函数，`TpCurrUnit = thermal_couple[i]` 之后

**新增变量（文件顶部）：**
```c
/* A-1: Median-of-3 + asymmetric output slew limit */
static unsigned short med_buf[TpMaxLp][3];  // 3-sample ring buffer per channel
static unsigned char  med_idx = 0;           // current write position (0-2)
static unsigned char  med_fill = 0;          // fill count (0→3)
#define SLEW_RISE_LIMIT  40  /* +4.0°C/cycle: near normal heating rate, minimal lag */
#define SLEW_FALL_LIMIT  20  /* -2.0°C/cycle: blocks noise drops (real cooling <1°C/step) */
static unsigned short slew_out[TpMaxLp];     // previous filtered output per channel
static unsigned char  slew_out_init = 0;
```

**滤波逻辑（TpCurrUnit 赋值后，Bk_thermal_couple 赋值前）：**
```c
/* A-1: Median-of-3 + output slew limit */
if(TpCurrUnit != 0xffff && TpCurrUnit != 12000) {
    med_buf[i][med_idx] = TpCurrUnit;
    if(med_fill >= 3) {
        /* Sort 3 values, pick median */
        unsigned short a = med_buf[i][0], b = med_buf[i][1], c = med_buf[i][2];
        if(a > b) { unsigned short t=a; a=b; b=t; }
        if(b > c) { unsigned short t=b; b=c; c=t; }
        if(a > b) { unsigned short t=a; a=b; b=t; }
        TpCurrUnit = b;  /* median */
    }
    /* Asymmetric slew: rise +4°C (normal heating), fall -2°C (block noise) */
    if(slew_out_init) {
        short delta = (short)TpCurrUnit - (short)slew_out[i];
        if(delta > SLEW_RISE_LIMIT)
            TpCurrUnit = slew_out[i] + SLEW_RISE_LIMIT;
        else if(delta < -SLEW_FALL_LIMIT)
            TpCurrUnit = (slew_out[i] > SLEW_FALL_LIMIT) ?
                slew_out[i] - SLEW_FALL_LIMIT : 0;
    }
    slew_out[i] = TpCurrUnit;
}
if(i == TpMaxLp - 1) {
    med_idx = (med_idx + 1) % 3;
    if(med_fill < 3) med_fill++;
    if(!slew_out_init && med_fill >= 3) slew_out_init = 1;
}
```

**设计要点：**
- `med_buf[12][3]`：每通道独立的 3 元素环形缓冲区
- `med_idx` 在所有 12 通道处理完后才递增
- `0xffff`（无效）和 `12000`（断偶）不参与滤波，直接透传
- Slew limit 在 median 之后执行，两层防护
- 非对称设计：+4°C 容许正常升温，-2°C 严格限制异常下降

### 3.2 main.c — ADC 采样标志 (B-1)

**新增全局变量：**
```c
volatile unsigned char adc_sampling_active = 0;
```

**TempHWSave() case 0（开始 ADC 转换）：**
```c
adc_sampling_active = 1;  /* 冻结加热输出 */
T_START_H;
```

**TempHWSave() case 1（ADC 读取完成 / 超时）：**
```c
Read_Single_AD(0x7, ch_index);
adc_sampling_active = 0;  /* 恢复加热输出 */

// 超时路径也清除:
if(wait_cnt > 10) {
    adc_sampling_active = 0;
    ...
}
```

### 3.3 IntTm0.c — 冻结 HT_OUT (B-1)

**ISR_Timer0() 中 HT_OUT 调用处：**
```c
if(adc_sampling_active)
    ;  /* 跳过 HT_OUT，保持上一次输出状态 */
else if(!BURN_DETECT)
    HT_OUT(~RealTpOutPwm);
else
    HT_OUT(0xFFFF);
```

---

## 4. 迭代过程记录

### v1：仅 Median-of-3 — 单次毛刺消除，交替噪声穿透

```
测试: 100°C → 410°C, 120s
结果: Max=6.2°C, >5°C=10 (全正常升温), >10°C=0
问题: 设定温度升高后出现交替噪声 (350↔160°C)，median 无法消除
```

### v2：Median + 对称 Slew ±2°C — 噪声完全消除，但显示延迟

```
测试: 63°C → 476°C, 180s
结果: Max=4.0°C, >5°C=0, >10°C=0 ✓✓✓
问题: 升温期 slew limit 太紧，曲线出现 ~10s 平台 (显示追不上真实温度)
原因: 正常升温 5°C/step > slew limit 2°C/step → 每步被钳位
```

### v3：Median + 非对称 Slew +6/-2°C — 延迟消除，但噪声泄露

```
测试: 110°C → 477°C, 180s
结果: Max=12.0°C, >5°C=28, >10°C=2
问题: +6°C 升限太宽，部分向上噪声穿透
```

### v4（最终）：Median + 非对称 Slew +4/-2°C — 最佳平衡

```
测试: 210°C → 485°C, 180s
结果: Max=8.0°C, >5°C=10 (全正常升温), >10°C=0 ✓
优点: 无显示平台，无异常噪声跳变，升温曲线平滑
```

---

## 5. 测试数据完整对比

### 测试条件
- 加热模式：SSR，P=300, I=50, D=100
- 升温范围：~60~210°C → ~480°C → 自然降温
- 采样时间：120~180 秒

### 结果汇总

| 指标 | 原始无滤波 | v1 Median | v2 ±2°C | v3 +6/-2°C | **v4 +4/-2°C** |
|------|-----------|-----------|---------|------------|----------------|
| **最大跳变** | 53~253°C | 6.2°C | 4.0°C | 12.0°C | **8.0°C** |
| >10°C 跳变 | 22+ | 0 | 0 | 2 | **0** |
| >5°C 跳变 | 30+ | 10 | 0 | 28 | **10(正常)** |
| >2°C 跳变 | 55+ | 79 | 26 | 91 | **74** |
| 显示延迟 | 无 | 无 | **有平台** | 无 | **无** |
| 噪声穿透 | 严重 | 交替穿透 | **无** | 部分泄露 | **无** |

### v4 最终版升温曲线

```
  [  0s] 210.0°C  ← 开始加热
  [  8s] 262.5°C  ← 快速升温 ~6°C/s
  [ 15s] 319.3°C
  [ 22s] 340.8°C
  [ 30s] 358.8°C
  [ 38s] 402.8°C
  [ 45s] 472.2°C
  [ 52s] 483.5°C  ← 峰值
  [ 60s] 484.5°C
  [ 68s] 482.1°C  ← 开始降温
  [ 75s] 475.9°C
  ...
  [180s] 341.5°C  ← 稳态附近
  ...
  [308s] 317.4°C  ← 持续平稳降温
```

---

## 6. 非对称 Slew Limit 原理说明

### 核心逻辑

```c
delta = 当前中值 - 上次输出

if(delta > +40)       // 升温超过 +4°C
    输出 = 上次 + 40   // 钳位到 +4°C
else if(delta < -20)  // 降温超过 -2°C
    输出 = 上次 - 20   // 钳位到 -2°C
else
    输出 = 当前中值     // 正常通过
```

### 为什么非对称

| 方向 | 正常物理变化率 | 噪声特征 | 设定值 |
|------|---------------|----------|--------|
| 升温 (+) | ~5°C/250ms (加热中) | 很少向上噪声 | **+4°C** (接近正常) |
| 降温 (-) | ~0.5°C/250ms (冷却中) | 主要噪声方向 (350→160) | **-2°C** (严格限制) |

### Median + Slew 分工

```
Median-of-3:     消除「单次」毛刺     [好,坏,好] → 好 ✓
Slew Limit:      消除「连续交替」噪声   限制每步最大变化量 ✓
非对称设计:      升温不延迟 + 降温噪声严格过滤 ✓
三者组合:        所有噪声模式都能处理
```

---

## 7. 影响范围

| 文件 | 修改内容 | 影响 |
|------|----------|------|
| `src/Temp.c` | Median-of-3 + 非对称 Slew | TpDisplay 输出更平滑 |
| `src/main.c` | `adc_sampling_active` 标志 | ADC 转换期间不切换加热 |
| `src/IntTm0.c` | HT_OUT 前检查采样标志 | 加热输出有 ~50ms/通道 冻结窗口 |

**不影响：**
- PID 算法本身（Temp.c TpCloseLoopSub 未修改）
- RS485 通信
- EEPROM 读写
- 冷端补偿
- 加热输出总功率（冻结窗口占比 <3%）

---

## 8. 后续可选优化

| 优先级 | 内容 | 说明 |
|--------|------|------|
| 可选 | PID Anti-windup | 输出饱和时冻结积分项，防止过冲 |
| 可选 | 冷启动软启动 | 大温差时限制最大占空比 60%→80%→100% |
| 低 | 噪声状态位 | 高噪声时自动切换保守 PID 参数组 |
| 低 | Slew 参数自适应 | 根据当前加热状态动态调整 rise/fall limit |
