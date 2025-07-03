import QtQuick

/**
 * 告警弹窗组件
 * 在收到 ADAS 告警时覆盖在仪表上层
 */
Item {
    id: warningPopupRoot

    property string message: ""
    property int level: 0  // 0=info, 1=caution, 2=warning, 3=critical

    // visible 由外部直接控制，内部不再声明 visible_
    opacity: visible ? 1.0 : 0.0
    Behavior on opacity { NumberAnimation { duration: 200 } }

    // ── 背景遮罩 ──
    Rectangle {
        anchors.fill: parent
        color: level >= 3 ? Qt.rgba(0.8, 0, 0, 0.4) :
               level >= 2 ? Qt.rgba(0.8, 0.5, 0, 0.3) :
                            Qt.rgba(0, 0, 0, 0.2)
    }

    // ── 告警框 ──
    Rectangle {
        anchors.centerIn: parent
        width: 400
        height: 80
        radius: 10
        color: Qt.rgba(0, 0, 0, 0.9)
        border.color: level >= 3 ? theme.danger :
                      level >= 2 ? theme.warning :
                                   theme.accent
        border.width: 3

        Text {
            anchors.centerIn: parent
            text: warningPopupRoot.message
            color: level >= 3 ? theme.danger :
                    level >= 2 ? theme.warning :
                                 theme.accent
            font.pixelSize: 24
            font.bold: true
            font.family: "Consolas"
            horizontalAlignment: Text.AlignHCenter
        }

        // 闪烁动画
        SequentialAnimation on scale {
            running: warningPopupRoot.visible && level >= 3
            loops: Animation.Infinite
            NumberAnimation { from: 0.95; to: 1.05; duration: 200 }
            NumberAnimation { from: 1.05; to: 0.95; duration: 200 }
        }
    }
}
