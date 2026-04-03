# MT-12(M4) 温度板烧录 SOP

> 适用型号：MX1-M4I12040（MT-12I，12段料管温度板）  
> 烧录器：ST-Link V2（st-flash CLI）  
> 整理日期：2026-04-02

---

## 一、文件说明

| 文件 | 烧录地址 | 说明 |
|------|----------|------|
| `Temp_M4_Bootloader_A_v7.bin` | `0x08000000` | Bootloader，A型对应MT-12I |
| `M4Temp.bin`（燒入用原檔 或 源程序） | `0x08008000` | 应用程序 |
| `M4Temp_mix_A.bin` | `0x08000000` | Bootloader+应用合并版，一次烧完 |

> ⚠️ 应用程序地址是 **`0x08008000`**，不是 `0x08080000`，写错板子不响应。

文件来源目录（Windows共享）：
```
/Windows_Share/temp test/code ref/MT-12-M4溫度板出貨程式/
├── 測試程式/Bootloader_v7_test_007/(A)MT-12I 40611603F/燒入用原檔/
│   ├── Temp_M4_Bootloader_A_v7.bin   ← Bootloader
│   └── M4Temp_A.bin                  ← 应用（测试版）
├── 出貨程式/12040/(A)MT-12I 40611603F/MX1-M4I12040--12段料管/
│   └── M4Temp.bin                    ← 出货应用
源程序/
└── M4Temp.bin                        ← 最新源程序编译版（推荐）
    Temp_M4_Bootloader_A_v7.bin
```

---

## 二、硬件连接

### J6 调试接口引脚
```
J6 Pin1 = 3.3V
J6 Pin2 = SWCLK
J6 Pin3 = GND
J6 Pin4 = SWDIO
J6 Pin5/6 = NRST（必须接！否则无法烧录）
```

> ⚠️ **NRST 必须接**，否则 Bootloader 循环运行时 SWD 进不去，报 `Failed to enter SWD mode`。

### SW1 拨码开关
| Pin | 状态 | 说明 |
|-----|------|------|
| Pin1 | **OFF（下拨）** | 正常运行应用程序 |
| Pin1 | ON | 进入 Bootloader 等待升级模式（LD21红灯常亮） |
| Pin2 | ON（上拨）| RS485 地址 = 0x01（第一片温度板） |
| Pin3~8 | OFF | 其余地址位 |

---

## 三、烧录命令

### 方案A：分开烧（推荐，可单独更新应用）

```bash
# 1. 全片擦除
st-flash erase

# 2. 烧 Bootloader
st-flash write Temp_M4_Bootloader_A_v7.bin 0x08000000

# 3. 烧应用程序
st-flash write M4Temp.bin 0x08008000
```

### 方案B：合并版一次烧完

```bash
st-flash erase
st-flash write M4Temp_mix_A.bin 0x08000000
```

### 烧录成功标志
```
Flash written and verified! jolly good!
```

---

## 四、烧录后验证

1. 断开 ST-Link，给板子重新上电（或 st-flash 会自动软复位）
2. 确认 SW1 Pin1 = **OFF**
3. 正常启动：**RUN 灯闪烁**，温度输出灯跑马灯
4. RS485 通信：主机发送查询，板子正常响应（sniffer GUI 显示绿色"已连接"）

---

## 五、常见问题

### `Failed to enter SWD mode`
- 原因：NRST 未接，CPU 卡在 Bootloader 循环
- 解决：接上 NRST 再烧，或使用 `--connect-under-reset` 参数

### 烧录完板子无响应
- 检查 SW1 Pin1 是否在 OFF
- 检查应用烧录地址是否是 `0x08008000`（不是 `0x08080000`）
- 检查 RS485 A/B/GND 接线

### 串口设备名变化
- 每次插拔 ST-Link 后 USB 重新枚举，`/dev/cu.usbserial-XXXXXX` 后缀会变
- sniffer GUI 已设置自动扫描，无需手动修改

---

## 六、通过 Bootloader 在线升级（不需要 ST-Link）

1. SW1 Pin1 拨到 **ON**
2. 将 `M4Temp.bin` 放入 U 盘根目录
3. U 盘插入 MX1 主机 USB 口，开机
4. LD21 红灯常亮 = 等待升级
5. 在 MX1 操作界面选择程序更新
6. 升级中：红灯灭，进度条增长；完成：LD22 绿灯亮
7. **将 SW1 Pin1 拨回 OFF**，重启运行新程序

---

*参考文档：`data ref/MT-12(M4)生產燒錄流程.pdf`，`data ref/MX1 全套控制器 測試及維修手冊 v1.5.docx`*
