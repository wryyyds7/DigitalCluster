#pragma once

#include "models/VehicleModel.h"
#include "core/PowerStateMachine.h"
#include <QObject>
#include <QString>
#include <QColor>
#include <QTimer>
#include <QTranslator>

/**
 * @brief 仪表 ViewModel（MVVM ViewModel 层）。
 *
 * 职责：
 *   1. 持有 VehicleModel 引用
 *   2. 将 VehicleData 中的字段通过 Q_PROPERTY 暴露给 QML
 *   3. 提供命令方法（Q_INVOKABLE）供 QML 调用（如切电源、切主题）
 *   4. 数据格式化与 UI 辅助逻辑（如档位字符串转换、告警级别映射）
 *
 * 严禁在此层直接操作 QML 组件或 UI 渲染。
 */
class ClusterViewModel : public QObject
{
    Q_OBJECT

    // ── 动力总成属性 ──
    Q_PROPERTY(double  speed          READ speed          NOTIFY speedChanged)
    Q_PROPERTY(double  rpm            READ rpm            NOTIFY rpmChanged)
    Q_PROPERTY(int     gear           READ gear           NOTIFY gearChanged)
    Q_PROPERTY(QString gearText       READ gearText       NOTIFY gearChanged)
    Q_PROPERTY(double  coolantTemp    READ coolantTemp    NOTIFY coolantTempChanged)
    Q_PROPERTY(double  fuelLevel      READ fuelLevel      NOTIFY fuelLevelChanged)
    Q_PROPERTY(double  oilPressure    READ oilPressure    NOTIFY oilPressureChanged)
    Q_PROPERTY(bool    checkEngine    READ checkEngine    NOTIFY checkEngineChanged)
    Q_PROPERTY(bool    upShift        READ upShift        NOTIFY upShiftChanged)

    // ── 车身属性 ──
    Q_PROPERTY(bool    leftTurn       READ leftTurn       NOTIFY leftTurnChanged)
    Q_PROPERTY(bool    rightTurn      READ rightTurn      NOTIFY rightTurnChanged)
    Q_PROPERTY(bool    highBeam       READ highBeam       NOTIFY highBeamChanged)
    Q_PROPERTY(bool    handbrake       READ handbrake      NOTIFY handbrakeChanged)
    Q_PROPERTY(bool    doorOpen       READ doorOpen       NOTIFY doorOpenChanged)
    Q_PROPERTY(bool    seatbeltUnfastened READ seatbeltUnfastened NOTIFY seatbeltUnfastenedChanged)
    Q_PROPERTY(double  interiorTemp   READ interiorTemp   NOTIFY interiorTempChanged)
    Q_PROPERTY(double  ambientTemp    READ ambientTemp    NOTIFY ambientTempChanged)

    // ── ADAS 属性 ──
    Q_PROPERTY(bool    fcwActive      READ fcwActive      NOTIFY adasChanged)
    Q_PROPERTY(bool    ldwActive      READ ldwActive      NOTIFY adasChanged)
    Q_PROPERTY(bool    aebActive      READ aebActive      NOTIFY adasChanged)
    Q_PROPERTY(int     fcwLevel       READ fcwLevel       NOTIFY adasChanged)
    Q_PROPERTY(int     ldwDirection   READ ldwDirection   NOTIFY adasChanged)
    Q_PROPERTY(double  distanceToVehicle READ distanceToVehicle NOTIFY adasChanged)
    Q_PROPERTY(int     speedLimit     READ speedLimit     NOTIFY adasChanged)
    Q_PROPERTY(QString adasMessage    READ adasMessage    NOTIFY adasChanged)

    // ── 仪表属性 ──
    Q_PROPERTY(uint32_t odometer      READ odometer       NOTIFY odometerChanged)
    Q_PROPERTY(double   tripA         READ tripA          NOTIFY tripAChanged)

    // ── 电源状态属性 ──
    Q_PROPERTY(int     powerState     READ powerState     NOTIFY powerStateChanged)
    Q_PROPERTY(QString powerStateText READ powerStateText NOTIFY powerStateChanged)
    Q_PROPERTY(bool    isSystemActive READ isSystemActive NOTIFY powerStateChanged)
    Q_PROPERTY(bool    isCranking     READ isCranking     NOTIFY powerStateChanged)

    // ── 告警属性 ──
    Q_PROPERTY(bool    hasWarning     READ hasWarning     NOTIFY adasChanged)
    Q_PROPERTY(int     warningLevel   READ warningLevel   NOTIFY adasChanged)

public:
    explicit ClusterViewModel(VehicleModel* model, QObject* parent = nullptr);

    // ── 动力总成 getters ──
    double  speed()          const { return m_data.speed; }
    double  rpm()            const { return m_data.rpm; }
    int     gear()           const { return m_data.gear; }
    QString gearText()       const;
    double  coolantTemp()    const { return m_data.coolantTemp; }
    double  fuelLevel()      const { return m_data.fuelLevel; }
    double  oilPressure()    const { return m_data.oilPressure; }
    bool    checkEngine()    const { return m_data.checkEngine; }
    bool    upShift()        const { return m_data.upShift; }

    // ── 车身 getters ──
    bool    leftTurn()       const { return m_data.leftTurn; }
    bool    rightTurn()      const { return m_data.rightTurn; }
    bool    highBeam()       const { return m_data.highBeam; }
    bool    handbrake()      const { return m_data.handbrake; }
    bool    doorOpen()       const { return m_data.doorOpen; }
    bool    seatbeltUnfastened() const { return m_data.seatbeltUnfastened; }
    double  interiorTemp()  const { return m_data.interiorTemp; }
    double  ambientTemp()   const { return m_data.ambientTemp; }

    // ── ADAS getters ──
    bool    fcwActive()      const { return m_data.fcwActive; }
    bool    ldwActive()      const { return m_data.ldwActive; }
    bool    aebActive()      const { return m_data.aebActive; }
    int     fcwLevel()       const { return m_data.fcwLevel; }
    int     ldwDirection()   const { return m_data.ldwDirection; }
    double  distanceToVehicle() const { return m_data.distanceToVehicle; }
    int     speedLimit()     const { return m_data.speedLimit; }
    QString adasMessage()   const;

    // ── 仪表 getters ──
    uint32_t odometer()      const { return m_data.odometer; }
    double   tripA()         const { return m_data.tripA; }

    // ── 电源状态 getters ──
    int     powerState()     const;
    QString powerStateText() const;
    bool    isSystemActive() const;
    bool    isCranking()     const;

    // ── 告警 getters ──
    bool    hasWarning()     const;
    int     warningLevel()   const;

    // ── QML 可调用命令 ──
    Q_INVOKABLE void powerOn()     { m_model->requestPowerOn(); }
    Q_INVOKABLE void powerOff()    { m_model->requestPowerOff(); }
    Q_INVOKABLE void startEngine();

    /// 用于 ThemeManager 热切换主题
    Q_INVOKABLE void setTheme(const QString& themeName);
    Q_INVOKABLE QString currentTheme() const { return m_themeName; }

    /// 用于多语言热切换
    Q_INVOKABLE void setLanguage(const QString& langCode);
    Q_INVOKABLE QString currentLanguage() const { return m_language; }

    /// 设置 QTranslator 指针用于热切换语言（由 main.cpp 注入）
    void setTranslator(QTranslator* translator) { m_translator = translator; }
    void setI18nDir(const QString& dir) { m_i18nDir = dir; }

signals:
    void speedChanged();
    void rpmChanged();
    void gearChanged();
    void coolantTempChanged();
    void fuelLevelChanged();
    void oilPressureChanged();
    void checkEngineChanged();
    void upShiftChanged();
    void leftTurnChanged();
    void rightTurnChanged();
    void highBeamChanged();
    void handbrakeChanged();
    void doorOpenChanged();
    void seatbeltUnfastenedChanged();
    void interiorTempChanged();
    void ambientTempChanged();
    void adasChanged();
    void odometerChanged();
    void tripAChanged();
    void powerStateChanged();
    void themeChanged();
    void languageChanged();
    void warningTriggered(const QString& message, int level);

private slots:
    void onDataChanged(const VehicleData& data);
    void onPowerStateChanged(PowerState state);

private:
    VehicleModel*  m_model;
    VehicleData    m_data;
    PowerState     m_powerState = PowerState::OFF;
    QString        m_themeName   = "dark";
    QString        m_language   = "en";
    QTranslator*   m_translator = nullptr;
    QString        m_i18nDir;

    // CRANK 状态计时器
    QTimer         m_crankTimer;
};
