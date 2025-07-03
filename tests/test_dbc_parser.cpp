/**
 * DBC 解析器单元测试
 * 验证 CAN 报文 → 物理值转换的正确性
 *
 * 编译: g++ -std=c++17 -I../src test_dbc_parser.cpp ../src/can/DbcParser.cpp -o test_dbc_parser
 * 运行: ./test_dbc_parser
 */

#include "../src/can/DbcParser.h"
#include "../src/can/VirtualCan.h"
#include "../src/can/CanFrame.h"
#include "../src/core/PowerStateMachine.h"
#include <cassert>
#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>

void test_engineFrame() {
    DbcParser dbc;

    // 构造引擎帧：RPM=3000, Coolant=90°C, Fuel=50%
    CanFrame f(0x100, {0xE8, 0x2E, 0x82, 0x32, 0x2C, 0x01, 0x01, 0x00});

    auto signals = dbc.parseFrame(f);
    assert(signals.find("EngineRPM") != signals.end());
    assert(signals.find("CoolantTemp") != signals.end());
    assert(signals.find("FuelLevel") != signals.end());

    // RPM: raw=0x2EE8=11992, *0.25=2998 (≈3000)
    assert(std::abs(signals["EngineRPM"] - 2998.0) < 1.0);
    // Coolant: raw=0x82=130, -40=90
    assert(std::abs(signals["CoolantTemp"] - 90.0) < 1.0);
    // Fuel: raw=0x32=50, *0.4=20
    assert(std::abs(signals["FuelLevel"] - 20.0) < 0.5);

    std::cout << "[PASS] test_engineFrame\n";
}

void test_transmissionFrame() {
    DbcParser dbc;

    // Gear=3, Speed=120.00 km/h
    uint16_t speedRaw = 12000;
    CanFrame f(0x200, {0x03,
                       static_cast<uint8_t>(speedRaw & 0xFF),
                       static_cast<uint8_t>((speedRaw >> 8) & 0xFF),
                       0x50, 0x00, 0, 0, 0});

    auto signals = dbc.parseFrame(f);
    assert(signals["Gear"] == 3.0);
    assert(std::abs(signals["VehicleSpeed"] - 120.0) < 0.1);

    std::cout << "[PASS] test_transmissionFrame\n";
}

void test_bodyFrame() {
    DbcParser dbc;

    // LeftTurn=1, HighBeam=1, Handbrake=1
    CanFrame f(0x400, {0x0D, 0xC8, 0x3A, 0, 0, 0, 0, 0});

    auto signals = dbc.parseFrame(f);
    assert(signals["LeftTurnSignal"] == 1.0);
    assert(signals["RightTurnSignal"] == 0.0);
    assert(signals["HighBeam"] == 1.0);
    assert(signals["Handbrake"] == 1.0);
    assert(signals["DoorOpen"] == 0.0);

    std::cout << "[PASS] test_bodyFrame\n";
}

void test_adasFrame() {
    DbcParser dbc;

    // FCW=1, Level=2, Distance=500cm
    CanFrame f(0x300, {0x01, 0x02, 0x00, 0xF4, 0x01, 0x78, 0, 0});

    auto signals = dbc.parseFrame(f);
    assert(signals["FCW_Active"] == 1.0);
    assert(signals["LDW_Active"] == 0.0);
    assert(signals["FCW_Level"] == 2.0);
    assert(std::abs(signals["DistanceToVehicle"] - 500.0) < 1.0);

    std::cout << "[PASS] test_adasFrame\n";
}

void test_virtualCan() {
    VirtualCan vcan(42);

    int frameCount = 0;
    vcan.setFrameCallback([&frameCount](const CanFrame& f) {
        frameCount++;
    });

    assert(vcan.open("test"));
    vcan.startAutoSimulation();

    // 等待接收几帧
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    assert(frameCount > 5);  // 至少收到 5 帧（100ms * 5 ECU = 25 帧）
    std::cout << "[PASS] test_virtualCan (" << frameCount << " frames received)\n";

    vcan.close();
}

void test_powerStateMachine() {
    PowerStateMachine sm;

    assert(sm.currentState() == PowerState::OFF);
    assert(sm.transition(PowerEvent::KEY_ACC));
    assert(sm.currentState() == PowerState::ACC);
    assert(sm.transition(PowerEvent::KEY_ON));
    assert(sm.currentState() == PowerState::ON);
    assert(sm.transition(PowerEvent::KEY_START));
    assert(sm.currentState() == PowerState::START);
    assert(sm.transition(PowerEvent::KEY_START));
    assert(sm.currentState() == PowerState::CRANK);
    assert(sm.transition(PowerEvent::ENGINE_STARTED));
    assert(sm.currentState() == PowerState::ON);
    assert(sm.transition(PowerEvent::KEY_OFF));
    assert(sm.currentState() == PowerState::OFF);

    // 非法转移
    assert(!sm.transition(PowerEvent::ENGINE_STARTED));

    std::cout << "[PASS] test_powerStateMachine\n";
}

int main() {
    std::cout << "=== DigitalCluster Unit Tests ===\n\n";

    test_engineFrame();
    test_transmissionFrame();
    test_bodyFrame();
    test_adasFrame();
    test_virtualCan();
    test_powerStateMachine();

    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}
