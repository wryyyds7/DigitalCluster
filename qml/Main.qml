import QtQuick
import QtQuick.Controls

/**
 * 数字仪表主窗口
 * 1920×720（车规级双屏仪表标准分辨率）
 */
ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1920
    height: 720
    title: "Digital Cluster"
    color: "black"
    visibility: Window.Maximized

    // ── 主题管理 ──
    property var theme: themeManager

    ThemeManager { id: themeManager }

    // ── 背景 ──
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: theme.bgColorTop }
            GradientStop { position: 1.0; color: theme.bgColorBottom }
        }
    }

    // ── 仪表页 ──
    ClusterPage {
        id: clusterPage
        anchors.fill: parent
    }

    // ── 底部控制栏 ──
    Rectangle {
        id: controlBar
        anchors.bottom: parent.bottom
        width: parent.width
        height: 50
        color: Qt.rgba(0, 0, 0, 0.6)

        Row {
            anchors.centerIn: parent
            spacing: 30

            Button {
                text: "Power ACC"
                onClicked: clusterVM.powerOn()
                palette.button: theme.accentDim
                palette.buttonText: "white"
            }
            Button {
                text: "Start Engine"
                onClicked: clusterVM.startEngine()
                palette.button: theme.success
                palette.buttonText: "white"
            }
            Button {
                text: "Power OFF"
                onClicked: clusterVM.powerOff()
                palette.button: theme.danger
                palette.buttonText: "white"
            }
            Button {
                text: "Theme: Dark"
                onClicked: clusterVM.setTheme("dark")
            }
            Button {
                text: "Theme: Sport"
                onClicked: clusterVM.setTheme("sport")
            }
            Button {
                text: "Theme: Eco"
                onClicked: clusterVM.setTheme("eco")
            }
            Button {
                text: "Lang: EN"
                onClicked: clusterVM.setLanguage("en")
            }
            Button {
                text: "Lang: ZH"
                onClicked: clusterVM.setLanguage("zh")
            }
        }
    }
}
