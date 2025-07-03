import QtQuick

/**
 * 转速表（0 - 8000 rpm，红线 6500）
 */
Item {
    id: tachGaugeRoot

    property real rpm: 0.0
    property int powerState: 0
    property bool isCranking: false
    property bool active: powerState >= 2

    CircularGauge {
        id: gauge
        anchors.fill: parent
        value: tachGaugeRoot.active ? tachGaugeRoot.rpm : 0
        minValue: 0
        maxValue: 8000
        redlineStart: 6500
        majorTicks: 8   // 每 1000 rpm 一个主刻度
        minorTicks: 5

        active: tachGaugeRoot.active

        ringColor: isCranking ? Qt.rgba(0.5, 0.3, 0, 0.5) : theme.gaugeRing
        fillColor: isCranking ? theme.warning : theme.gaugeFill
        needleColor: isCranking ? theme.warning : theme.accent

        // RPM 数字
        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 40
            text: Math.round(tachGaugeRoot.rpm / 100) * 100
            color: tachGaugeRoot.rpm > 6500 ? theme.redline : theme.textPrimary
            font.pixelSize: 50
            font.family: "Consolas"
            font.weight: Font.Bold
        }

        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 80
            text: qsTr("x1000")
            color: theme.textSecondary
            font.pixelSize: 14
            font.family: "Consolas"
        }

        // RPM 标签
        Text {
            anchors.top: parent.top
            anchors.topMargin: 60
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("RPM")
            color: theme.textSecondary
            font.pixelSize: 16
            font.family: "Consolas"
        }
    }

    // 换挡提示
    Text {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("UPSHIFT")
        color: theme.warning
        font.pixelSize: 20
        font.bold: true
        font.family: "Consolas"
        visible: clusterVM.upShift && tachGaugeRoot.rpm > 5000
        opacity: 0.9

        // 闪烁
        SequentialAnimation on opacity {
            running: clusterVM.upShift && tachGaugeRoot.rpm > 5000
            loops: Animation.Infinite
            NumberAnimation { from: 0.3; to: 1.0; duration: 300 }
            NumberAnimation { from: 1.0; to: 0.3; duration: 300 }
        }
    }

    // CRANK 状态提示
    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -20
        text: qsTr("CRANKING...")
        color: theme.warning
        font.pixelSize: 24
        font.bold: true
        font.family: "Consolas"
        visible: isCranking
        opacity: 0.9

        SequentialAnimation on opacity {
            running: isCranking
            loops: Animation.Infinite
            NumberAnimation { from: 0.3; to: 1.0; duration: 200 }
            NumberAnimation { from: 1.0; to: 0.3; duration: 200 }
        }
    }
}
