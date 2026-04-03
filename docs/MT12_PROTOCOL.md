# MT-12 温度控制板 RS485 通信协议

## 硬件连接

| 项目 | 参数 |
|------|------|
| 接口 | RS485（A、B 两线） |
| **GND** | **必须与主机共地，否则无响应** |
| A/B 接反 | 无响应（可互换测试） |

## 串口参数

| 参数 | 值 |
|------|----|
| 波特率 | **38400** |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | 无（8N1） |

> 注：固件判断 EEPROM 标志 `VariableBaud != 0xA5A5` 时使用默认 38400。

## 板子地址（ID）

- 由拨码开关决定，S1~S8 对应 bit0~bit7
- S2 上拨其他下拨 → **ID = 0x01**
- 地址范围：0x00~0x07

---

## 私有协议（非标准 Modbus）

### 命令码

| 命令 | 功能 |
|------|------|
| `0x30` | 读取指定地址列表 |
| `0x60` | 写入单个地址（多笔格式） |
| `0xF0` | 异常响应 |

> `0x33`（读）、`0x63`（单笔写）存在 CRC 校验问题，建议使用 `0x30` 和 `0x60`。

---

## CRC16 计算规则

- 算法：标准 CRC16 查表法
- 初始值：`0xFFFF`（CRCHi = 0xFF，CRCLo = 0xFF）
- **索引方式：`uIndex = CRCHi ^ byte`（用 Hi 字节做索引）**
- 传输顺序：**CRCLo 先发，CRCHi 后发**

```python
def crc16(data):
    # auchCRCHi / auchCRCLo 为标准 CRC16 查表
    hi, lo = 0xFF, 0xFF
    for b in data:
        idx = hi ^ b          # 用 Hi 字节做索引
        hi = lo ^ auchCRCHi[idx]
        lo = auchCRCLo[idx]
    return hi | (lo << 8)     # 返回 Hi|(Lo<<8)，传输时 Lo 先发
```

---

## 读命令 0x30

### 请求帧格式

```
[ID] [0x30] [LenHi] [LenLo] [Addr1Hi] [Addr1Lo] ... [AddrNHi] [AddrNLo] [CRCLo] [CRCHi]
```

- `Length` = 地址数 × 2（字节数）
- CRC 覆盖从 ID 到最后一个地址（不含 CRC 本身）

### 响应帧格式

```
[ID] [0x30] [?] [Val1Hi] [Val1Lo] ... [ValNHi] [ValNLo] [CRCLo] [CRCHi]
```

### 示例：读 CH1 PV 温度（地址 20516）

```
发送：01 30 00 02 50 14 xx xx
         └─ ID  └─ 0x30  └─ Len=2  └─ 20516=0x5014  └─ CRC
响应：01 30 xx [ValHi] [ValLo] xx xx
```

值 × 0.1 = 实际温度（°C），如 `0x00F6` = 246 = 24.6°C

---

## 写命令 0x60

### 请求帧格式

```
[ID] [0x60] [00] [04] [AddrHi] [AddrLo] [ValHi] [ValLo] [CRCLo] [CRCHi]
```

- 固定长度字段 `[00][04]`（Length = 4）
- CRC 覆盖全部前 8 字节

### 响应帧格式（写成功）

```
[ID] [0x60] [04] [AddrHi] [AddrLo] [ValHi] [ValLo] [CRCLo] [CRCHi]
```

### 示例：全部切换为 J 型热电偶

```
发送：01 60 00 04 4F B7 0F FF [CRCLo] [CRCHi]
         └─ 地址 20407=0x4FB7  └─ 值 0x0FFF（12路全J型）
```

---

## 关键寄存器地址

### PV / SV / PWM

| 地址 | 含义 | 单位 | 备注 |
|------|------|------|------|
| 20516~20527 | **PV 实时温度** CH1~12 | ×0.1°C | 经 30 次滑动平均 |
| 20001~20012 | SV 设定温度 CH1~12 | ×0.1°C | EEPROM 存储 |
| 20501~20512 | TpControl（SV 镜像）| ×0.1°C | PID 内部用，非 PV |
| 20601~20612 | PWM 输出 CH1~12 | 0~32767 = 0~100% | |

### 配置寄存器

| 地址 | 含义 | 说明 |
|------|------|------|
| 20407 | KAndJSel（K/J 类型） | bit0~bit11 对应 CH1~12，0=K型，1=J型 |
| 20808 | BURN_DETECT \| ADErrorFlag | bit1~bit12 = CH1~12 断偶标志 |
| 20101 | HeatCtrlMd（加热控制模式）| |

### 特殊值

| 值 | 含义 |
|----|------|
| PV = 12000（raw） | 断偶（开路），显示 1200.0°C |
| PV ≈ 室温 | 短接（无热电偶接入时的参考温度） |
| 0x012C = 300 | EEPROM 初始默认值（未配置时读到） |

---

## 连不上排查清单

1. **GND 没共地** → 最常见原因，必须接
2. **A/B 接反** → 互换 A/B 测试
3. **波特率错误** → 确认 38400
4. **板子 ID 不对** → 逐一尝试 0x01~0x07
5. **串口被占用** → 关闭其他程序（如 GUI）
6. **上电未就绪** → 上电后等 1~2 秒再发帧

---

## Python 通信脚本

完整实现见：`scripts/mt12_modbus.py`  
GUI 监控工具：`scripts/mt12_monitor_gui.py`（tkinter，深色主题，700×470）

启动 GUI：
```bash
/opt/homebrew/bin/python3 scripts/mt12_monitor_gui.py
```
