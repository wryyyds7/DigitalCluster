#include "ClusterViewModel.h"
#include <QGuiApplication>

ClusterViewModel::ClusterViewModel(VehicleModel* model, QObject* parent)
    : QObject(parent), m_model(model)
{
    // 连接 Model 信号
    connect(m_model, &VehicleModel::dataChanged,
            this, &ClusterViewModel::onDataChanged);
    connect(m_model, &VehicleModel::powerStateChanged,
            this, &ClusterViewModel::onPowerStateChanged);

    // CRANK 超时计时器：模拟发动机启动
    m_crankTimer.setSingleShot(true);
    m_crankTimer.setInterval(3000);  // 3 秒后启动成功
    connect(&m_crankTimer, &QTimer::timeout, this, [this]() {
        // 引擎启动成功，回到 ON
        // 通过 model 的电源状态机发起转移
        m_model->powerStateMachine().transition(PowerEvent::ENGINE_STARTED);
    });
}

// ── 档位文本转换 ───────────────────────────────────────────
QString ClusterViewModel::gearText() const
{
    switch (m_data.gear) {
        case -1: return "R";
        case  0: return "N";
        case  1: return "1";
        case  2: return "2";
        case  3: return "3";
        case  4: return "4";
        case  5: return "5";
        case  6: return "6";
        case  7: return "7";
        case  8: return "8";
        default: return "--";
    }
}

// ── ADAS 消息 ───────────────────────────────────────────────
QString ClusterViewModel::adasMessage() const
{
    if (m_data.aebActive) return "AEB: AUTO BRAKING!";
    if (m_data.fcwActive) {
        if (m_data.fcwLevel >= 3) return "FCW: CRITICAL";
        if (m_data.fcwLevel >= 2) return "FCW: WARNING";
        return "FCW: CAUTION";
    }
    if (m_data.ldwActive) {
        return m_data.ldwDirection == 1 ? "LDW: LEFT" : "LDW: RIGHT";
    }
    return "";
}

// ── 电源状态 ───────────────────────────────────────────────
int ClusterViewModel::powerState() const
{
    return static_cast<int>(m_powerState);
}

QString ClusterViewModel::powerStateText() const
{
    switch (m_powerState) {
        case PowerState::OFF:   return "OFF";
        case PowerState::ACC:   return "ACC";
        case PowerState::ON:    return "ON";
        case PowerState::START: return "START";
        case PowerState::CRANK: return "CRANK";
    }
    return "UNKNOWN";
}

bool ClusterViewModel::isSystemActive() const
{
    return m_powerState != PowerState::OFF;
}

bool ClusterViewModel::isCranking() const
{
    return m_powerState == PowerState::CRANK;
}

// ── 告警 ───────────────────────────────────────────────────
bool ClusterViewModel::hasWarning() const
{
    return m_data.fcwActive || m_data.ldwActive || m_data.aebActive ||
           m_data.checkEngine;
}

int ClusterViewModel::warningLevel() const
{
    if (m_data.aebActive)      return 3;  // 最高级
    if (m_data.fcwLevel >= 3)  return 3;
    if (m_data.fcwLevel >= 2)  return 2;
    if (m_data.checkEngine)    return 2;
    if (m_data.fcwActive || m_data.ldwActive) return 1;
    return 0;
}

// ── 数据更新 ───────────────────────────────────────────────
void ClusterViewModel::onDataChanged(const VehicleData& data)
{
    // 比较变更并发射对应信号（减少不必要的 QML 绑定刷新）
    if (m_data.speed != data.speed) {
        m_data.speed = data.speed;
        emit speedChanged();
    }
    if (m_data.rpm != data.rpm) {
        m_data.rpm = data.rpm;
        emit rpmChanged();
    }
    if (m_data.gear != data.gear) {
        m_data.gear = data.gear;
        emit gearChanged();
    }
    if (m_data.coolantTemp != data.coolantTemp) {
        m_data.coolantTemp = data.coolantTemp;
        emit coolantTempChanged();
    }
    if (m_data.fuelLevel != data.fuelLevel) {
        m_data.fuelLevel = data.fuelLevel;
        emit fuelLevelChanged();
    }
    if (m_data.oilPressure != data.oilPressure) {
        m_data.oilPressure = data.oilPressure;
        emit oilPressureChanged();
    }
    if (m_data.checkEngine != data.checkEngine) {
        m_data.checkEngine = data.checkEngine;
        emit checkEngineChanged();
    }
    if (m_data.upShift != data.upShift) {
        m_data.upShift = data.upShift;
        emit upShiftChanged();
    }
    if (m_data.leftTurn != data.leftTurn) {
        m_data.leftTurn = data.leftTurn;
        emit leftTurnChanged();
    }
    if (m_data.rightTurn != data.rightTurn) {
        m_data.rightTurn = data.rightTurn;
        emit rightTurnChanged();
    }
    if (m_data.highBeam != data.highBeam) {
        m_data.highBeam = data.highBeam;
        emit highBeamChanged();
    }
    if (m_data.handbrake != data.handbrake) {
        m_data.handbrake = data.handbrake;
        emit handbrakeChanged();
    }
    if (m_data.doorOpen != data.doorOpen) {
        m_data.doorOpen = data.doorOpen;
        emit doorOpenChanged();
    }
    if (m_data.seatbeltUnfastened != data.seatbeltUnfastened) {
        m_data.seatbeltUnfastened = data.seatbeltUnfastened;
        emit seatbeltUnfastenedChanged();
    }
    if (m_data.interiorTemp != data.interiorTemp) {
        m_data.interiorTemp = data.interiorTemp;
        emit interiorTempChanged();
    }
    if (m_data.ambientTemp != data.ambientTemp) {
        m_data.ambientTemp = data.ambientTemp;
        emit ambientTempChanged();
    }
    if (m_data.odometer != data.odometer) {
        m_data.odometer = data.odometer;
        emit odometerChanged();
    }
    if (m_data.tripA != data.tripA) {
        m_data.tripA = data.tripA;
        emit tripAChanged();
    }

    // ADAS 信号
    if (m_data.fcwActive != data.fcwActive ||
        m_data.ldwActive != data.ldwActive ||
        m_data.aebActive != data.aebActive ||
        m_data.fcwLevel != data.fcwLevel ||
        m_data.ldwDirection != data.ldwDirection ||
        m_data.distanceToVehicle != data.distanceToVehicle ||
        m_data.speedLimit != data.speedLimit) {

        m_data.fcwActive = data.fcwActive;
        m_data.ldwActive = data.ldwActive;
        m_data.aebActive = data.aebActive;
        m_data.fcwLevel = data.fcwLevel;
        m_data.ldwDirection = data.ldwDirection;
        m_data.distanceToVehicle = data.distanceToVehicle;
        m_data.speedLimit = data.speedLimit;

        emit adasChanged();

        if (hasWarning()) {
            emit warningTriggered(adasMessage(), warningLevel());
        }
    }
}

void ClusterViewModel::onPowerStateChanged(PowerState state)
{
    m_powerState = state;

    if (state == PowerState::CRANK) {
        m_crankTimer.start();  // 启动计时器
    } else {
        m_crankTimer.stop();
    }

    emit powerStateChanged();
}

void ClusterViewModel::startEngine()
{
    // 根据当前状态逐步推进：ON → START → CRANK
    auto& sm = m_model->powerStateMachine();
    PowerState state = sm.currentState();

    if (state == PowerState::ON) {
        // ON → START
        sm.transition(PowerEvent::KEY_START);
        // START → CRANK
        sm.transition(PowerEvent::KEY_START);
    } else if (state == PowerState::START) {
        // START → CRANK
        sm.transition(PowerEvent::KEY_START);
    }
    // 其他状态（OFF/ACC/CRANK）下忽略启动请求
}

// ── 主题热切换 ─────────────────────────────────────────────
void ClusterViewModel::setTheme(const QString& themeName)
{
    if (m_themeName == themeName) return;
    m_themeName = themeName;
    emit themeChanged();
}

// ── 多语言热切换 ───────────────────────────────────────────
void ClusterViewModel::setLanguage(const QString& langCode)
{
    if (m_language == langCode) return;

    // 先尝试加载新翻译，加载成功后再移除旧翻译，避免中间状态丢失翻译
    if (m_translator && !m_i18nDir.isEmpty()) {
        QString qmFile = m_i18nDir + "/" + langCode + ".qm";
        if (m_translator->load(qmFile)) {
            QGuiApplication::removeTranslator(m_translator);
            QGuiApplication::installTranslator(m_translator);
            m_language = langCode;
        } else {
            // 加载失败，保持原有语言不变
            return;
        }
    } else {
        m_language = langCode;
    }

    emit languageChanged();
}
