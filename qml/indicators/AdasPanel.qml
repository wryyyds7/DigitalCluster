import QtQuick

/**
 * ADAS 高级驾驶辅助面板
 * 显示：前车碰撞预警 (FCW)、车道偏离 (LDW)、自动紧急制动 (AEB)
 *       车距、限速
 */
Rectangle {
    id: adasPanel

    property bool fcwActive: false
    property bool ldwActive: false
    property bool aebActive: false
    property int fcwLevel: 0
    property int ldwDirection: 0
    property real distanceToVehicle: 0
    property int speedLimit: 0
    property string adasMessage: ""
    property int powerState: 0

    radius: 10
    color: Qt.rgba(0, 0, 0, 0.6)
    border.color: aebActive ? theme.danger : (fcwActive ? theme.warning : Qt.rgba(1, 1, 1, 0.15))
    border.width: aebActive ? 3 : (fcwActive ? 2 : 1)

    Behavior on border.color { ColorAnimation { duration: 200 } }

    Column {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // 标题
        Text {
            text: "ADAS"
            color: theme.textSecondary
            font.pixelSize: 14
            font.family: "Consolas"
            font.bold: true
        }

        // 限速标志
        Item {
            width: parent.width
            height: 50

            visible: speedLimit > 0

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 44
                height: 44
                radius: 22
                color: "white"
                border.color: "red"
                border.width: 3

                Text {
                    anchors.centerIn: parent
                    text: speedLimit.toString()
                    color: "black"
                    font.pixelSize: 18
                    font.bold: true
                    font.family: "Consolas"
                }
            }
        }

        // 车距显示
        Item {
            width: parent.width
            height: 30
            visible: fcwActive || distanceToVehicle > 0

            Text {
                anchors.left: parent.left
                text: "Distance:"
                color: theme.textSecondary
                font.pixelSize: 14
                font.family: "Consolas"
            }
            Text {
                anchors.right: parent.right
                text: (distanceToVehicle / 100).toFixed(1) + " m"
                color: distanceToVehicle < 1000 ? theme.warning : theme.textPrimary
                font.pixelSize: 14
                font.family: "Consolas"
            }
        }

        // ADAS 状态图标行
        Row {
            width: parent.width
            spacing: 16
            anchors.horizontalCenter: parent.horizontalCenter

            // FCW 图标
            Rectangle {
                width: 40; height: 40
                radius: 6
                color: fcwActive ? (fcwLevel >= 3 ? theme.danger : theme.warning) :
                                  Qt.rgba(1, 1, 1, 0.05)
                border.color: fcwActive ? "white" : Qt.rgba(1, 1, 1, 0.1)
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "FCW"
                    color: fcwActive ? "white" : Qt.rgba(1, 1, 1, 0.3)
                    font.pixelSize: 11
                    font.bold: fcwActive
                    font.family: "Consolas"
                }
            }

            // LDW 图标
            Rectangle {
                width: 40; height: 40
                radius: 6
                color: ldwActive ? theme.warning : Qt.rgba(1, 1, 1, 0.05)
                border.color: ldwActive ? "white" : Qt.rgba(1, 1, 1, 0.1)
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "LDW"
                    color: ldwActive ? "white" : Qt.rgba(1, 1, 1, 0.3)
                    font.pixelSize: 11
                    font.bold: ldwActive
                    font.family: "Consolas"
                }
            }

            // AEB 图标
            Rectangle {
                width: 40; height: 40
                radius: 6
                color: aebActive ? theme.danger : Qt.rgba(1, 1, 1, 0.05)
                border.color: aebActive ? "white" : Qt.rgba(1, 1, 1, 0.1)
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "AEB"
                    color: aebActive ? "white" : Qt.rgba(1, 1, 1, 0.3)
                    font.pixelSize: 11
                    font.bold: aebActive
                    font.family: "Consolas"
                }
            }
        }

        // 告警消息
        Text {
            width: parent.width
            text: adasMessage
            color: aebActive ? theme.danger : (fcwLevel >= 2 ? theme.warning : theme.accent)
            font.pixelSize: 14
            font.family: "Consolas"
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: adasMessage.length > 0

            // 闪烁
            SequentialAnimation on opacity {
                running: aebActive || fcwLevel >= 3
                loops: Animation.Infinite
                NumberAnimation { from: 0.3; to: 1.0; duration: 250 }
                NumberAnimation { from: 1.0; to: 0.3; duration: 250 }
            }
        }
    }
}
