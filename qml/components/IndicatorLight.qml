import QtQuick

/**
 * 指示灯组件（通用）
 * 用于 TellTales 中的所有状态指示灯
 */
Item {
    id: lightRoot

    property string text: ""
    property color activeColor: "white"
    property color inactiveColor: Qt.rgba(1, 1, 1, 0.15)
    property bool active: false
    property bool blinking: false
    property int fontSize: 18

    width: 40
    height: 40

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: lightRoot.active ? lightRoot.activeColor : lightRoot.inactiveColor
        border.color: lightRoot.active ? "white" : Qt.rgba(1, 1, 1, 0.1)
        border.width: lightRoot.active ? 2 : 1

        Behavior on color { ColorAnimation { duration: 150 } }
    }

    Text {
        anchors.centerIn: parent
        text: lightRoot.text
        color: lightRoot.active ? "black" : Qt.rgba(1, 1, 1, 0.4)
        font.pixelSize: lightRoot.fontSize
        font.bold: lightRoot.active
        font.family: "Consolas"
    }

    // 闪烁
    SequentialAnimation on opacity {
        running: lightRoot.active && lightRoot.blinking
        loops: Animation.Infinite
        NumberAnimation { from: 1.0; to: 0.2; duration: 300 }
        NumberAnimation { from: 0.2; to: 1.0; duration: 300 }
    }
}
