import QtQuick

/**
 * 顶部指示灯条（Tell-Tales）
 * 显示所有车辆状态指示灯：转向灯、远光灯、手刹、车门、安全带、
 *                        发动机故障、低油量、高温
 */
Item {
    id: tellTalesRoot

    property bool leftTurn: false
    property bool rightTurn: false
    property bool highBeam: false
    property bool handbrake: false
    property bool doorOpen: false
    property bool seatbeltUnfastened: false
    property bool checkEngine: false
    property real fuelLevel: 0.0
    property real coolantTemp: 0.0
    property int powerState: 0

    Row {
        anchors.centerIn: parent
        spacing: 30

        // ── 左转向灯 ──
        IndicatorLight {
            text: "◄"
            activeColor: "#34d400"
            inactiveColor: Qt.rgba(1, 1, 1, 0.15)
            active: leftTurn && powerState >= 2
            blinking: true
            fontSize: 28
        }

        // ── 远光灯 ──
        IndicatorLight {
            text: "☀"
            activeColor: "#0066ff"
            inactiveColor: Qt.rgba(1, 1, 1, 0.15)
            active: highBeam && powerState >= 2
            fontSize: 24
        }

        // ── 手刹 ──
        IndicatorLight {
            text: "(P)"
            activeColor: "#ff3b30"
            inactiveColor: Qt.rgba(1, 1, 1, 0.15)
            active: handbrake && powerState >= 2
            fontSize: 22
        }

        // ── 车门开 ──
        IndicatorLight {
            text: "DOI"
            activeColor: "#ff9500"
            inactiveColor: Qt.rgba(1, 1, 1, 0.15)
            active: doorOpen && powerState >= 2
            fontSize: 16
        }

        // ── 安全带未系 ──
        IndicatorLight {
            text: "BELT"
            activeColor: "#ff3b30"
            inactiveColor: Qt.rgba(1, 1, 1, 0.15)
            active: seatbeltUnfastened && powerState >= 2
            blinking: true
            fontSize: 16
        }

        // ── 发动机故障 ──
        IndicatorLight {
            text: "ENG"
            activeColor: "#ff9500"
            inactiveColor: Qt.rgba(1, 1, 1, 0.15)
            active: checkEngine && powerState >= 2
            blinking: true
            fontSize: 16
        }

        // ── 低油量 ──
        IndicatorLight {
            text: "⛽"
            activeColor: "#ff3b30"
            inactiveColor: Qt.rgba(1, 1, 1, 0.15)
            active: fuelLevel < 10.0 && powerState >= 2
            blinking: fuelLevel < 10.0
            fontSize: 22
        }

        // ── 高温 ──
        IndicatorLight {
            text: "TEMP"
            activeColor: "#ff3b30"
            inactiveColor: Qt.rgba(1, 1, 1, 0.15)
            active: coolantTemp > 105.0 && powerState >= 2
            blinking: coolantTemp > 105.0
            fontSize: 16
        }

        // ── 右转向灯 ──
        IndicatorLight {
            text: "►"
            activeColor: "#34d400"
            inactiveColor: Qt.rgba(1, 1, 1, 0.15)
            active: rightTurn && powerState >= 2
            blinking: true
            fontSize: 28
        }
    }
}
