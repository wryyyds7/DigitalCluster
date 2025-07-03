#pragma once

#include <cstdint>

/**
 * @brief 车辆信号数据 Model（非 QObject，纯数据结构）。
 *
 * 存储 CAN 解析后的所有物理值，由 VehicleModel 统一管理，
 * 通过信号变更通知 ViewModel 刷新。
 */
struct VehicleData
{
    // ── 动力总成 ──
    double  speed          = 0.0;   ///< km/h
    double  rpm            = 0.0;   ///< rpm
    int     gear           = 0;    ///< -1=R, 0=N/P, 1-8
    double  coolantTemp    = 0.0;  ///< °C
    double  oilPressure    = 0.0;  ///< kPa
    double  fuelLevel      = 0.0;  ///< %
    bool    engineRunning  = false;
    bool    checkEngine    = false;
    bool    upShift        = false;
    bool    downShift      = false;
    double  transTemp      = 0.0;  ///< °C

    // ── 车身 ──
    bool    leftTurn       = false;
    bool    rightTurn      = false;
    bool    highBeam       = false;
    bool    handbrake       = false;
    bool    doorOpen       = false;
    bool    seatbeltUnfastened = false;
    double  ambientLight   = 0.0;  ///< lux
    double  interiorTemp   = 0.0;  ///< °C

    // ── ADAS ──
    bool    fcwActive      = false;
    bool    ldwActive      = false;
    bool    aebActive      = false;
    bool    bsmActive      = false;
    int     fcwLevel       = 0;
    int     ldwDirection   = 0;
    double  distanceToVehicle = 0.0;  ///< cm
    int     speedLimit     = 0;       ///< km/h

    // ── 仪表 ──
    uint32_t odometer      = 0;
    double   tripA         = 0.0;   ///< km
    double   ambientTemp   = 0.0;  ///< °C
};
