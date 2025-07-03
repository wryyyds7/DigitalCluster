# DigitalCluster — 基于 Qt/QML + CAN 模拟的智能数字仪表系统

完整还原车规级 **"信号解析 → 逻辑处理 → UI 渲染"** 数据链路的数字仪表原型系统。

## 架构总览

```
┌─────────────────────────────────────────────────────────────────┐
│                        QML View Layer                           │
│  SpeedGauge  TachGauge  GearDisplay  AdasPanel  TellTales  ...   │
├─────────────────────────────────────────────────────────────────┤
│                     ViewModel Layer (Q_PROPERTY)                │
│                     ClusterViewModel                            │
├─────────────────────────────────────────────────────────────────┤
│                       Model Layer                               │
│  VehicleModel  ←  PowerStateMachine  ←  SignalTimeoutDetection   │
├─────────────────────────────────────────────────────────────────┤
│                      CAN Communication Layer                    │
│  CanInterface (abstract)  →  VirtualCan / SocketCAN / PCAN      │
│  DbcParser (DBC → physical values)                              │
├─────────────────────────────────────────────────────────────────┤
│                    Functional Safety Layer                      │
│  HeartbeatSender (主进程) ←→ WatchdogProcess (备用进程, 500ms)   │
└─────────────────────────────────────────────────────────────────┘
```

## 项目结构

```
DigitalCluster/
├── CMakeLists.txt              # CMake 构建配置
├── src/
│   ├── main.cpp                # 主进程入口
│   ├── can/                    # CAN 通信层
│   │   ├── CanFrame.h          # CAN 帧结构体
│   │   ├── CanInterface.h      # CAN 接口抽象基类
│   │   ├── VirtualCan.h/.cpp   # 虚拟 CAN 后端（含 ECU 模拟器）
│   │   └── DbcParser.h/.cpp    # DBC 文件解析 + 信号提取
│   ├── core/                   # 核心模块
│   │   └── PowerStateMachine.h/.cpp  # 上下电状态机
│   ├── models/                 # MVVM Model 层
│   │   ├── VehicleData.h       # 车辆信号数据结构
│   │   └── VehicleModel.h/.cpp # 信号处理与业务逻辑
│   ├── viewmodels/             # MVVM ViewModel 层
│   │   └── ClusterViewModel.h/.cpp  # Q_PROPERTY 暴露给 UI
│   └── safety/                 # 功能安全模块
│       ├── WatchdogProcess.h/.cpp   # 看门狗备用进程
│       ├── HeartbeatSender.h/.cpp   # 主进程心跳发送器
│       └── main_watchdog.cpp        # 看门狗入口
├── qml/
│   ├── Main.qml               # 主窗口
│   ├── pages/ClusterPage.qml   # 仪表主页面
│   ├── gauges/                 # 仪表组件
│   │   ├── SpeedGauge.qml      # 车速表 (0-260 km/h)
│   │   ├── TachGauge.qml       # 转速表 (0-8000 rpm)
│   │   └── GearDisplay.qml     # 档位显示
│   ├── indicators/             # 指示灯与面板
│   │   ├── AdasPanel.qml       # ADAS 驾驶辅助面板
│   │   ├── MediaPanel.qml      # 多媒体信息面板
│   │   └── TellTales.qml        # 指示灯条
│   ├── components/              # 通用组件
│   │   ├── CircularGauge.qml   # 圆形仪表（Canvas + Spring 动画）
│   │   ├── IndicatorLight.qml  # 指示灯
│   │   └── WarningPopup.qml    # 告警弹窗
│   └── themes/
│       └── ThemeManager.qml    # 主题热切换管理器
├── resources/
│   └── qml.qrc                 # QML 资源文件
├── config/
│   └── cluster.dbc             # DBC 信号定义文件
├── i18n/
│   ├── en.ts                   # 英文翻译
│   └── zh.ts                   # 中文翻译
├── scripts/
│   └── can_simulator.py        # Python CAN 信号模拟器
├── tests/
│   └── test_dbc_parser.cpp     # DBC 解析器单元测试
└── docs/
```

## 关键技术点实现

### 1. 通信层：CAN 信号解析

- **CAN 接口抽象** (`CanInterface`)：统一 SocketCAN (Linux) / PCAN (Windows) / VirtualCAN 接口
- **DBC 解析库** (`DbcParser`)：
  - 支持标准 `.dbc` 文件加载
  - 内置硬编码信号定义（无需外部文件即可运行）
  - 支持 Intel (little-endian) 和 Motorola (big-endian) 字节序
  - 支持有符号数缩放与偏移
- **VirtualCAN 模拟器**：内置 5 个虚拟 ECU（引擎、变速箱、ADAS、车身、仪表），自动模拟车速/转速/温度/ADAS 等信号

### 2. MVVM 架构

严格遵守分层：

| 层 | 职责 | 禁止 |
|---|---|---|
| **Model** | CAN 信号接收、DBC 解析、业务逻辑、状态机 | 操作 UI |
| **ViewModel** | Q_PROPERTY 暴露数据、Q_INVOKABLE 命令、数据格式化 | 直接操作 QML 组件 |
| **View** | 纯 QML 渲染、动画、布局 | 写复杂 C++ 逻辑 |

### 3. 渲染优化（60 FPS）

- `Canvas.renderStrategy: Canvas.Threaded`：线程化渲染，不阻塞 UI 线程
- `SpringAnimation` 弹簧动画：指针平滑跟随，避免抖动
- 指针仅依赖 `animatedValue` 属性，`onPaint` 中不做耗时计算
- 背景刻度与值填充弧分离为独立 `Canvas`，避免不必要重绘
- `Behavior` + `ColorAnimation`：主题切换平滑过渡

### 4. 上下电状态机

完整状态序列：
```
OFF → ACC → ON → START → CRANK → ON
```

- 状态转移表驱动，支持非法转移检测
- CRANK 状态自动 3 秒后启动成功回到 ON
- 不同电源状态下 UI 表现不同：
  - OFF：仪表全灭
  - ACC：部分信息显示（多媒体、时钟）
  - ON：仪表自检扫针动画
  - CRANK：转速表闪烁 "CRANKING..." 提示

### 5. 功能安全模拟

双进程架构：

```
DigitalCluster (主进程)
    │
    ├── 每 200ms 发送心跳（车速 + 转向灯）
    │
    ▼
ClusterWatchdog (备用进程)
    │
    ├── 500ms 未收到心跳 → 进入安全模式
    │
    └── 安全模式：显示最低限度信息
        - 车速
        - 转向灯状态
        - "SYSTEM ERROR - SAFETY MODE"
```

IPC 方案：`QLocalServer` / `QLocalSocket`（命名管道），心跳数据以 JSON 格式传输。

### 6. 多语言/主题热切换

- **主题**：4 套预置主题（dark / light / sport / eco），通过 `ViewModel.setTheme()` 即时切换
- **多语言**：Qt Linguist `.ts` 翻译文件，通过 `ViewModel.setLanguage()` 即时切换
- 均无需重启应用

## 构建与运行

### 依赖

- Qt 6.2+ (Core, Gui, Qml, Quick, QuickControls2, Network)
- CMake 3.21+
- C++17 编译器
- Python 3 (可选，运行 CAN 模拟器脚本)

### 构建

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

### 运行

```bash
# 1. 先启动看门狗备用进程
./ClusterWatchdog &

# 2. 启动主仪表进程
./DigitalCluster --channel virtual
```

### 运行 CAN 信号模拟器（独立模式）

```bash
# 使用 python-can 虚拟接口（无需 root）
pip install python-can
python3 scripts/can_simulator.py --channel virtual --interface virtual

# 使用 Linux SocketCAN（需 root）
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
python3 scripts/can_simulator.py --channel vcan0 --interface socketcan
```

### 运行单元测试

```bash
cd tests
g++ -std=c++17 -I../src test_dbc_parser.cpp ../src/can/DbcParser.cpp ../src/can/VirtualCan.cpp ../src/core/PowerStateMachine.cpp -o test_dbc_parser
./test_dbc_parser
```

## CAN 报文定义

| ID    | ECU          | 周期  | 关键信号                                |
|-------|-------------|-------|----------------------------------------|
| 0x100 | Engine      | 50ms  | RPM, CoolantTemp, FuelLevel, OilPressure |
| 0x200 | Transmission| 50ms  | Gear, VehicleSpeed, TransTemp           |
| 0x300 | ADAS        | 50ms  | FCW/LDW/AEB, Distance, SpeedLimit       |
| 0x400 | Body        | 50ms  | TurnSignals, HighBeam, Handbrake, Door  |
| 0x500 | Cluster     | 50ms  | Odometer, TripA, AmbientTemp           |

详见 `config/cluster.dbc`。

## 开源参考

- [Qt IVI Cluster](https://doc.qt.io/qt-ivi/qtivi-cluster-example.html) — Qt 官方 IVI 仪表示例
- [openDsh](https://github.com/openDsh) — 开源汽车仪表项目
