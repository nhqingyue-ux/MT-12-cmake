## MT-12 冷却阶段滤波器优化记录

**日期：** 2026-04-17
**修改文件：** `src/Temp.c`
**相关 Commits：**
- `6b211b8` 10:08 — Adaptive median window: Median-3 (cooling) / Median-9 (heating)
- `2320e8d` 11:11 — Cooling filter: Median-3 only, drop Slew/EMA
- `455e8b1` 11:55 — Cooling filter: Median-3 → Median-5

---

## 1. 背景

之前的滤波链 (`temperature_antijitter_optimization.md`，2026-04-09 ~ 04-10) 在加热阶段表现良好：
**Median-9 + Asymmetric Slew + EMA(α=0.3)**，对 PWM 切换产生的 EMI 抑制充分。

但在 **冷却阶段** (温度 > 设定值，加热环已关) 存在两个问题：
1. **下行跳变残留：** V3（裸 ADC，无滤波）实测有 8 次明显的"假下跌"事件
2. **显示滞后：** 若沿用加热期完整滤波链，Median-9 引入约 1.3°C 滞后 + EMA 引入约 0.75 s 时间常数，操作员看到的温度明显落后实际冷却

冷却时 PWM 不切换，EMI 噪声源消失；但 ADC 自身仍存在零星脏样本，需要轻量保护。

---

## 2. 设计目标

| 维度 | 加热阶段 (heating) | 冷却阶段 (cooling) | OFF |
|------|-------------------|---------------------|------|
| 主要噪声源 | PWM 切换 EMI（持续） | ADC 零星脏样本（稀疏） | 无 |
| 抑噪强度需求 | 高 | 中 | 低 |
| 显示延迟容忍度 | 中 | 低（操作员盯看冷却速率） | 低 |
| 选择策略 | Median-9 + Slew + EMA | 仅中值滤波 | 无滤波（恢复 Keil 行为） |

判定条件：以 `thermal_couple[i]` 与 `*(tempData[i].TpControl)` (设定值) 比较。

---

## 3. 三次迭代过程

### 迭代一 (6b211b8) — 自适应中值窗口：Median-3 (cooling) / Median-9 (heating)

**思路：** 复用现有 9 槽环形缓冲，冷却时只取最近 3 个样本求中值。

```c
} else if(cooling) {
    /* Median-3: 取 ring buffer 最近 3 个样本 */
    unsigned short s[3];
    for(k = 0; k < 3; k++) {
        unsigned char idx = (med_idx + MED_WIN - k) % MED_WIN;
        s[k] = med_buf[i][idx];
    }
    /* insertion sort + 取中位 */
    TpCurrUnit = s[1];
}
```

**效果：** 中值滞后从 ~1.3°C 降到 ~0.5°C，但 Slew/EMA 仍保留 → 显示仍偏滞后。

---

### 迭代二 (2320e8d) — 冷却时仅 Median-3，跳过 Slew + EMA

**思路：** 冷却阶段无 EMI，Slew 的非对称限幅和 EMA 平滑反而是纯粹的延迟来源；删除它们。

关键改动：把 Slew/EMA 整段包进 `if(heating)` 分支，并在末尾同步状态以确保模式切换时 Slew/EMA 不残留旧值：

```c
if(heating) {
    /* Slew + EMA 仅加热使用 */
    ...
}
/* Sync filter state for clean mode transitions */
slew_out[i] = TpCurrUnit;
ema_out[i] = TpCurrUnit;
```

**效果：** 冷却显示几乎零滞后，符合操作员预期。
**遗留问题：** Median-3 仅能容忍 1/3 (33%) 坏样本。实测冷却阶段仍有 36 次 >2°C 的下行尖刺。

---

### 迭代三 (455e8b1) — Median-3 → Median-5

**根因：** Median-3 抗噪能力弱，连续两次脏样本即穿透。

升级为 Median-5：容错率 2/5 (40%)，相比 Median-3 提升约 7 个百分点。代价是滞后从 ~0.5°C 提升到约 1.0°C，仍远低于 Median-9 的 1.3°C。

```c
} else if(cooling) {
    /* Median-5: tolerates 2/5 bad samples */
    unsigned short s[5];
    for(k = 0; k < 5; k++) {
        unsigned char idx = (med_idx + MED_WIN - k) % MED_WIN;
        s[k] = med_buf[i][idx];
    }
    /* insertion sort */
    ...
    TpCurrUnit = s[2];
}
```

**实测对比：**

| 指标 | Median-3 | Median-5 |
|------|----------|----------|
| >2°C 下行尖刺次数 | 36 | 10（↓ 72%）|
| 平均冷却速率 (°C/s) | 1.05 | 1.00（几乎无影响）|
| 中值滞后 | ~0.5°C | ~1.0°C |

---

## 4. 最终滤波链总览

```
原始 ADC (thermal_couple[i])
        │
        ├──── 始终写入 9 槽 ring buffer（med_buf）
        │
   ┌────┴────┐
   ▼         ▼
heating?   cooling?    ← 通过 thermal_couple vs TpControl 判定
   │         │
Median-9   Median-5    ← 复用同一 ring buffer，cooling 取最近 5 样本
   │         │
Slew       (跳过)
   │         │
EMA(α=0.3) (跳过)
   │         │
   └────┬────┘
        ▼
slew_out[i] = ema_out[i] = TpCurrUnit   ← 状态同步，避免模式切换抖动
        ▼
   TpDisplay (HMI)
```

OFF 时（`Temp == OFF`）跳过整个 if/else if 链，恢复 Keil 原始无滤波行为。

---

## 5. 关键工程决策

| 决策 | 理由 |
|------|------|
| 复用 9 槽 ring buffer 而非新建 5 槽 | 不额外占 RAM (TpMaxLp×2B×5 = 120B 节省)，且模式切换时 buffer 已是热的 |
| 冷却用环形回看（最近 N 个） | 避免引入额外索引；环形缓冲已经按时间序写入 |
| 状态同步 `slew_out = ema_out = TpCurrUnit` | heating ↔ cooling 模式切换时，Slew/EMA 内部状态不再持有"旧加热期数值"，下次进入 heating 不会出现起步抖动 |
| 跳过 Slew/EMA 而非保留弱化版 | 任何额外滤波都是延迟；冷却时 EMI 消失，只需中值即可 |
| 选 Median-5 而非 Median-7 | 7 太接近 9 的滞后；5 是性价比拐点（容错率 +7%，滞后只 +0.5°C） |

---

## 6. 验证与回归

- ✅ 加热阶段行为未变化（同一份 Median-9 + Slew + EMA 代码路径）
- ✅ 冷却阶段下行尖刺由 36 次降至 10 次
- ✅ 冷却显示速率与实际曲线偏差 < 1°C
- ✅ heating ↔ cooling 切换瞬间无可见抖动（得益于 Slew/EMA 状态同步）
- ⚠️ 待长时间烧录验证：连续多小时的稳态保温阶段是否会出现新的边界行为

---

## 7. 后续可考虑

1. ~~若 10 次残留尖刺仍不可接受，可在 cooling 分支再叠加一个**单向 slew**~~ → **已实施，方向调整为限上跳，见 §9**
2. ~~当前判据 `thermal_couple[i] vs TpControl` 用的是**未滤波 ADC**~~ → **已被 §8 的 PWM-aware 判据取代**
3. Median-5 的插入排序可改为硬编码 5 元比较网络，当前性能富余，未做

---

## 8. 后续优化 — PWM-aware mode 判据 (2026-04-20)

**修改文件：** `src/Temp.c`
**起因：** 多通道交叉加热场景下，cooling 分支被滥用导致大跳

### 8.1 问题发现

测试场景：CH1/CH2 SV=50°C，但被旁边 CH3 加热环（SV=300°C，PWM 满载）外部加热到 ~387°C。

数据现象（commit 455e8b1）：
- CH1 出现 **±150°C 剧烈振荡**（311.7→165.8→334.2→173.3→346.4...）
- 单次 +60.2°C / +95.6°C 大上跳
- 单次 -160.7°C / -144.0°C 大下跌

### 8.2 根因分析

旧 mode 判据：
```c
heating = (Temp != OFF) && (thermal_couple[i] <= TpControl);
cooling = (Temp != OFF) && (thermal_couple[i] >  TpControl);
```

隐含假设：**"PV > SV → 本通道加热环已关 → 无 EMI → Median-5 够用"**

现实推翻：
- CH1 加热环关了不代表 CH3 的也关了
- CH3 满载 PWM 制造的 EMI 通过电源/地耦合进 CH1 的 ADC
- Median-5 在持续 EMI 下被穿透（≥3/5 样本被污染）→ 输出在两个值间剧烈摆动

### 8.3 方案

新增"任一通道 PWM 输出中"作为强制走 heating 链路的条件：

```c
extern unsigned short RealTpOutPwm; /* bit-i = heater i currently driving */

unsigned char any_pwm = (RealTpOutPwm != 0);
unsigned char heating = (Temp != OFF)
    && ((thermal_couple[i] <= *(tempData[i].TpControl)) || any_pwm);
unsigned char cooling = (Temp != OFF)
    && (thermal_couple[i] > *(tempData[i].TpControl))
    && !any_pwm;
```

**逻辑：**
- 任一通道 PWM 触发 → **所有通道**走 Median-9 + Slew + EMA（强抗 EMI）
- 仅当所有加热器全关、且本通道 PV > SV 时，才走 cooling 轻量链

### 8.4 验证结果（test 104555）

| 指标 | 旧 (094439) | PWM-aware (104555) | 改善 |
|------|------------|--------------------|------|
| CH1 ±150°C 振荡 | 4 次 | 0 次 | ✅ 消失 |
| CH1 最大下跌 | -160.7°C | -2.9°C | ↓ 98% |
| CH1 ≥+50°C 大跳 | 3 次 | 1 次 | ↓ 67% |

**遗留：** CH1 仍有 1 次 +115.3°C 上跳。怀疑是 PWM 占空比间隙瞬间使 `RealTpOutPwm == 0`，cooling 分支被误进入。

---

## 9. 后续优化 — Cooling 单向上跳 Slew (2026-04-20)

**修改文件：** `src/Temp.c`
**起因：** §8.4 遗留的 +115°C 单次上跳

### 9.1 选型对比

针对遗留问题考虑过三个方案：

| 方案 | 思路 | 选择 |
|------|------|------|
| A | PWM 信号加 sticky window（任一 PWM 触发后强制 heating N 个 cycle） | ❌ 依赖外部信号，参数 N 难调 |
| B | 弃用 cooling 分支，全程 Median-9 + Slew + EMA | ❌ 冷却显示恢复 ~2.8°C 滞后，违背 cooling 优化初衷 |
| **C** | cooling 分支保留 Median-5，加**仅限上跳的单向 Slew** | ✅ 选用 |

**C 方案优势：**
- 真冷却（下跳）→ Slew 不参与 → 0 额外延迟，与当前一致
- EMI 上跳 → Slew 限到 +6°C/cycle → 大跳消失
- 改 4 行，零回归风险

### 9.2 实现

```c
} else if(cooling) {
    /* Median-5 已计算 */
    TpCurrUnit = s[2];
    /* 单向 slew：仅限上跳 */
    if(slew_out_init) {
        short delta = (short)TpCurrUnit - (short)slew_out[i];
        if(delta > SLEW_RISE_LIMIT)
            TpCurrUnit = slew_out[i] + SLEW_RISE_LIMIT;
    }
}
```

### 9.3 冷却延迟分析

Median-5 中位数 = 5 样本中第 3 个 = 窗口中点 ≈ 实际滞后 ~1 cycle (~0.5s)。

| 真实冷却速率 | 延迟温度 |
|------------|---------|
| 0.5°C/s（自然慢冷） | ~0.25°C |
| 1°C/s（典型）| ~0.5°C |
| 2°C/s（强冷）| ~1°C |

**与 commit 455e8b1（仅 Median-5）完全一致**，单向 Slew 对下跳无任何影响。

### 9.4 验证结果（test 114311）

| 指标 | 094439 旧 | 104555 PWM-aware | **114311 +C 方案** |
|------|---------|----------------|--------------------|
| CH1 最大上跳 | +173.1°C | +115.3°C | **+18.0°C** |
| CH1 ≥+50°C 大跳 | 3 次 | 1 次 | **0 次** |
| CH1 最大下跌 | -160.7°C | -2.9°C | -2.6°C |
| CH1 ≤-50°C 大跌 | 2 次 | 0 次 | **0 次** |
| CH2 最大上跳 | +95.6°C | +60.4°C | **+12.0°C** |

**说明：** `≥+10°C` 跳变次数从 1 涨到 12，是因为原本 1 次 +115°C 的瞬移现在被 Slew 摊成 ~10 个 +12°C 台阶（GUI 0.5s 采样跨 ~2 firmware cycle = +6×2 = +12°C）。视觉上是"快速爬升"而非"瞬移"——理想的工程行为。

### 9.5 完整滤波链最终态

```
heating 模式（任一通道 PWM 触发 OR 本通道 PV ≤ SV）：
    Raw ADC → Median-9 → 双向 Slew(±6 / -0.2°C/cycle) → EMA(α=0.3) → Display

cooling 模式（所有 PWM 关 AND 本通道 PV > SV）：
    Raw ADC → Median-5 → 单向 Slew(仅限+6°C/cycle 上跳) → Display

OFF 模式（Temp == OFF）：
    Raw ADC → Display（恢复 Keil 原始行为）
```
