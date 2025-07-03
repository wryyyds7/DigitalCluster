import QtQuick

/**
 * 仪表主页面
 * 布局：左侧转速表 | 中央车速表+档位 | 右侧 ADAS/多媒体面板
 */
Item {
    id: clusterPage

    // ── 左侧：转速表 ──
    TachGauge {
        id: tachGauge
        anchors.left: parent.left
        anchors.leftMargin: 80
        anchors.verticalCenter: parent.verticalCenter
        width: 400
        height: 400
        rpm: clusterVM.rpm
        powerState: clusterVM.powerState
        isCranking: clusterVM.isCranking
    }

    // ── 中央：车速表 + 档位 ──
    Item {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: 500
        height: 500

        SpeedGauge {
            id: speedGauge
            anchors.fill: parent
            speed: clusterVM.speed
            powerState: clusterVM.powerState
        }

        // 档位显示叠加在车速表中央
        GearDisplay {
            anchors.centerIn: parent
            gearText: clusterVM.gearText
            powerState: clusterVM.powerState
        }
    }

    // ── 右侧：ADAS + 多媒体 ──
    Item {
        anchors.right: parent.right
        anchors.rightMargin: 80
        anchors.verticalCenter: parent.verticalCenter
        width: 380
        height: 500

        AdasPanel {
            id: adasPanel
            anchors.top: parent.top
            width: parent.width
            height: 240
            fcwActive: clusterVM.fcwActive
            ldwActive: clusterVM.ldwActive
            aebActive: clusterVM.aebActive
            fcwLevel: clusterVM.fcwLevel
            ldwDirection: clusterVM.ldwDirection
            distanceToVehicle: clusterVM.distanceToVehicle
            speedLimit: clusterVM.speedLimit
            adasMessage: clusterVM.adasMessage
            powerState: clusterVM.powerState
        }

        MediaPanel {
            id: mediaPanel
            anchors.top: adasPanel.bottom
            anchors.topMargin: 20
            width: parent.width
            height: 240
            powerState: clusterVM.powerState
        }
    }

    // ── 顶部：指示灯 ──
    TellTales {
        id: tellTales
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        width: 600
        height: 60
        leftTurn: clusterVM.leftTurn
        rightTurn: clusterVM.rightTurn
        highBeam: clusterVM.highBeam
        handbrake: clusterVM.handbrake
        doorOpen: clusterVM.doorOpen
        seatbeltUnfastened: clusterVM.seatbeltUnfastened
        checkEngine: clusterVM.checkEngine
        fuelLevel: clusterVM.fuelLevel
        coolantTemp: clusterVM.coolantTemp
        powerState: clusterVM.powerState
    }

    // ── 底部信息栏：里程、温度等 ──
    Item {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 60
        anchors.horizontalCenter: parent.horizontalCenter
        width: 800
        height: 40

        Text {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("ODO") + ": " + Math.floor(clusterVM.odometer).toLocaleString(Qt.locale(), 'f', 0) + " " + qsTr("km")
            color: theme.textSecondary
            font.pixelSize: 16
            font.family: "Consolas"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("TRIP A") + ": " + clusterVM.tripA.toFixed(1) + " " + qsTr("km")
            color: theme.textSecondary
            font.pixelSize: 16
            font.family: "Consolas"
        }

        Text {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("AMBIENT") + ": " + clusterVM.ambientTemp.toFixed(0) + "°C"
            color: theme.textSecondary
            font.pixelSize: 16
            font.family: "Consolas"
        }
    }

    // ── 告警弹窗 ──
    WarningPopup {
        id: warningPopup
        anchors.fill: parent
        z: 100
        message: clusterVM.adasMessage
        level: clusterVM.warningLevel
        visible: clusterVM.hasWarning
    }
}
