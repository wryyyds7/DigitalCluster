import QtQuick

/**
 * 档位显示
 * 显示在车速表中央
 */
Item {
    id: gearDisplayRoot

    property string gearText: "N"
    property int powerState: 0

    width: 120
    height: 80

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: Qt.rgba(0, 0, 0, 0.7)
        border.color: powerState >= 2 ? theme.accent : Qt.rgba(1, 1, 1, 0.2)
        border.width: 2
    }

    Text {
        anchors.centerIn: parent
        text: gearText
        color: powerState >= 2 ? theme.accent : Qt.rgba(0.5, 0.5, 0.5, 0.5)
        font.pixelSize: 48
        font.bold: true
        font.family: "Consolas"
    }

    // 档位标签
    Text {
        anchors.top: parent.bottom
        anchors.topMargin: 4
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("GEAR")
        color: theme.textSecondary
        font.pixelSize: 10
        font.family: "Consolas"
    }
}
