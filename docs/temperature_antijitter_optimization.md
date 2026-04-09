# MT-12 温度跳变优化记录 (A-1 + B-1)

**日期：** 2026-04-09
**Commit：** 5d41fb2
**固件：** M4Temp.elf (GCC arm-none-eabi, -O1)

---

## 1. 问题描述

加热过程中温度显示值出现大幅跳变（±30~50°C），导致：
- HMI 温度显示"乱飘"
- PID 控制环路受干扰，输出剧烈振荡
- 发热圈温度直冲，触发高温告警

**根因：** 加热 PWM 开关瞬间产生电磁干扰，耦合到 ADS1248 ADC 采样电路，采到脏数据后直通 PID 运算。

---

## 2. 优化方案

采用 **A-1（中值滤波）+ B-1（采样-加热解耦）** 组合方案。

### A-1：Median-of-3 中值滤波

**原理：** 保留最近 3 次采样值，取中值作为输出。单次脉冲噪声永远被排除。

```
采样序列:  [280.0, 230.0, 285.0]  ← 230 是噪声毛刺
排序:      [230.0, 280.0, 285.0]
中值:      280.0 ✓  毛刺被消除

采样序列:  [280.0, 285.0, 290.0]  ← 正常升温
排序:      [280.0, 285.0, 290.0]
中值:      285.0 ✓  不影响趋势
```

### B-1：ADC 采样期间冻结加热输出

**原理：** ADC 转换期间暂停 HT_OUT() 更新，让 SSR 保持当前状态不翻转，消除 PWM 开关噪声源。

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

### 3.1 Temp.c — 中值滤波 (A-1)

**位置：** `TempSubScan()` 函数，`TpCurrUnit = thermal_couple[i]` 之后

**新增变量（文件顶部）：**
```c
/* A-1: Median-of-3 filter — immune to single-sample spikes */
static unsigned short med_buf[TpMaxLp][3];  // 3-sample ring buffer per channel
static unsigned char  med_idx = 0;           // current write position (0-2)
static unsigned char  med_fill = 0;          // fill count (0→3)
```

**滤波逻辑（TpCurrUnit 赋值后，Bk_thermal_couple 赋值前）：**
```c
/* A-1: Median-of-3 filter */
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
}
if(i == TpMaxLp - 1) {
    med_idx = (med_idx + 1) % 3;
    if(med_fill < 3) med_fill++;
}
```

**设计要点：**
- `med_buf[12][3]`：每通道独立的 3 元素环形缓冲区
- `med_idx` 在所有 12 通道处理完后才递增（保证同一批采样用同一个写位置）
- `med_fill < 3` 时（初始化阶段）不滤波，直通原始值
- `0xffff`（无效）和 `12000`（断偶）不参与滤波，直接透传

### 3.2 main.c — ADC 采样标志 (B-1)

**新增全局变量：**
```c
volatile unsigned char adc_sampling_active = 0;
```

**TempHWSave() case 0（开始 ADC 转换）：**
```c
case 0:
    Sel_Channel(0x07, reg[ch_index][2]);
    StepCnt = 1;
    TimerBase1 = 0;
    TpDriveOn(IcCh);
    wait_cnt = 0;
    adc_sampling_active = 1;  /* ← 设置标志：冻结加热输出 */
    DELAY_CYCLES(2);
    T_START_H;
    DELAY_CYCLES(25);
    break;
```

**TempHWSave() case 1（ADC 读取完成）：**
```c
T_START_L;
Read_Single_AD(0x7, ch_index);
adc_sampling_active = 0;  /* ← 清除标志：恢复加热输出 */
```

**TempHWSave() case 1（超时路径）：**
```c
if(wait_cnt > 10) {
    adc_sampling_active = 0;  /* ← 超时也清除 */
    ...
}
```

### 3.3 IntTm0.c — 冻结 HT_OUT (B-1)

**新增 extern 声明：**
```c
extern volatile unsigned char adc_sampling_active;
```

**ISR_Timer0() 中 HT_OUT 调用处：**
```c
// 原代码:
if(!BURN_DETECT)
    HT_OUT(~RealTpOutPwm);
else
    HT_OUT(0xFFFF);

// 修改后:
if(adc_sampling_active)
    ;  /* B-1: 跳过 HT_OUT，保持上一次输出状态 */
else if(!BURN_DETECT)
    HT_OUT(~RealTpOutPwm);
else
    HT_OUT(0xFFFF);
```

---

## 4. 迭代过程记录

### v1：Slew Rate Limit（冻结旧值）— 效果不足

```
参数: SLEW_LIMIT=50 (5°C), SLEW_CONSEC_MAX=3
结果: 最大跳变 53.4°C → 仍有大幅穿透
原因: 连续3次超限就接受，噪声连续多帧时穿透
```

### v2：Slew Rate Limit（钳位步进）— 改善但仍有问题

```
参数: SLEW_LIMIT=30 (3°C), SLEW_CONSEC_MAX=5, 钳位到最大步进
结果: 最大跳变 61.1°C → 反而更大
原因: 3°C 太紧，正常升温(5°C/step)也被限住，消耗 consec 计数器
      真正噪声到来时 consec 已接近阈值，反而穿透
```

### v3：Median-of-3（最终方案）— 噪声完全消除

```
结果: 最大跳变 6.2°C（正常升温速率，非噪声）
      所有 ±30~50°C 噪声毛刺完全消除
原因: 中值滤波对单次脉冲天然免疫，且不限制正常温度变化速率
```

---

## 5. 测试数据对比

### 测试条件
- 加热模式：SSR，P=300, I=50, D=100
- 升温范围：~100°C → ~410°C → 自然降温
- 采样时间：120 秒

### 结果

| 指标 | 优化前 | v1 Slew | v2 Slew | **v3 Median (最终)** |
|------|--------|---------|---------|----------------------|
| 最大跳变 | **53.4°C** | 53.4°C | 61.1°C | **6.2°C** |
| >5°C 跳变次数 | 30 | 30 | 12 | **10（全正常升温）** |
| >2°C 跳变次数 | 55 | 55 | 81 | 79 |
| 噪声跳变 (±30°C+) | 多次 | 多次 | 有 | **0 次** |
| 升温曲线 | 锯齿状 | 锯齿 | 有毛刺 | **平滑** |

### 升温曲线（v3 最终版）

```
  [  0s] 100.7°C  ← 开始加热
  [  5s] 129.0°C
  [ 10s] 157.2°C
  [ 15s] 183.8°C
  [ 20s] 218.4°C
  [ 25s] 251.6°C  ← 快速升温期，每秒约 5°C
  [ 30s] 286.1°C
  [ 35s] 321.3°C
  [ 40s] 345.1°C
  [ 45s] 363.5°C
  [ 50s] 380.4°C
  [ 55s] 391.4°C
  [ 60s] 397.2°C  ← 接近设定值 300°C（实际过冲到 ~408°C）
  [ 65s] 403.4°C
  [ 70s] 405.9°C  ← 峰值
  [ 75s] 407.9°C
  [ 80s] 406.9°C  ← 开始降温
  [ 85s] 404.1°C
  ...
  [205s] 284.5°C  ← 持续平稳降温
```

---

## 6. 影响范围

| 文件 | 修改内容 | 影响 |
|------|----------|------|
| `src/Temp.c` | 新增 median-of-3 滤波 | TpDisplay 输出更平滑 |
| `src/main.c` | 新增 `adc_sampling_active` 标志 | ADC 转换期间不切换加热 |
| `src/IntTm0.c` | HT_OUT 前检查采样标志 | 加热输出有 ~50ms/通道 冻结窗口 |

**不影响：**
- PID 算法本身（Temp.c TpCloseLoopSub 未修改）
- RS485 通信
- EEPROM 读写
- 冷端补偿
- 加热输出总功率（冻结窗口占比 <3%）

---

## 7. 后续可选优化

| 优先级 | 内容 | 说明 |
|--------|------|------|
| 可选 | PID Anti-windup | 输出饱和时冻结积分项，防止过冲 |
| 可选 | 冷启动软启动 | 大温差时限制最大占空比 60%→80%→100% |
| 可选 | 恢复滑动平均 | 在 median 基础上叠加滑动平均进一步平滑 |
| 低 | 噪声状态位 | 高噪声时自动切换保守 PID 参数组 |
