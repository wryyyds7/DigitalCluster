import QtQuick

/**
 * 车速表（0 - 260 km/h）
 */
Item {
    id: speedGaugeRoot

    property real speed: 0.0
    property int powerState: 0
    property bool active: powerState >= 2  // ON 及以上状态才激活

    // 扫针动画使用的独立显示值
    // 不使用绑定表达式（NumberAnimation 会永久破坏绑定），
    // 改为 Connections 手动同步：非动画期间 displayValue 跟随 speed
    property real displayValue: 0
    property bool startupAnimRunning: powerState === 2 && speed < 1

    CircularGauge {
        id: gauge
        anchors.fill: parent
        value: displayValue
        minValue: 0
        maxValue: 260
        redlineStart: 200
        majorTicks: 13   // 每 20 km/h 一个主刻度
        minorTicks: 4

        active: speedGaugeRoot.active

        // 大号数字显示在中央偏下
        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 40
            text: Math.round(speedGaugeRoot.speed).toString()
            color: theme.textPrimary
            font.pixelSize: 80
            font.family: "Consolas"
            font.weight: Font.Bold
        }

        // 单位
        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 90
            text: qsTr("km/h")
            color: theme.textSecondary
            font.pixelSize: 18
            font.family: "Consolas"
        }
    }

    // 上电扫针动画：操作 displayValue 而非 gauge.value
    SequentialAnimation {
        id: startupAnim
        running: speedGaugeRoot.startupAnimRunning

        NumberAnimation {
            target: speedGaugeRoot
            property: "displayValue"
            from: 0
            to: 260
            duration: 800
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: speedGaugeRoot
            property: "displayValue"
            from: 260
            to: 0
            duration: 400
            easing.type: Easing.InBounce
        }
    }

    // 非动画期间持续同步 displayValue ← speed
    // 动画结束后也立即同步一次
    Connections {
        target: speedGaugeRoot
        function onSpeedChanged() {
            if (!startupAnimRunning) {
                speedGaugeRoot.displayValue = speedGaugeRoot.speed
            }
        }
        function onActiveChanged() {
            if (!startupAnimRunning) {
                speedGaugeRoot.displayValue = speedGaugeRoot.active ? speedGaugeRoot.speed : 0
            }
        }
        function onStartupAnimRunningChanged() {
            if (!startupAnimRunning) {
                speedGaugeRoot.displayValue = speedGaugeRoot.speed
            }
        }
    }
}
