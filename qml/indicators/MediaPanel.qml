import QtQuick

/**
 * 多媒体信息面板
 * 显示：模拟媒体播放信息（歌曲名、艺术家、播放进度）
 */
Rectangle {
    id: mediaPanel

    property int powerState: 0

    radius: 10
    color: Qt.rgba(0, 0, 0, 0.6)
    border.color: Qt.rgba(1, 1, 1, 0.15)
    border.width: 1

    // 模拟播放列表
    property var playlist: [
        { title: "Highway Star",    artist: "Deep Purple",     duration: 372 },
        { title: "Radar Love",      artist: "Golden Earring",   duration: 395 },
        { title: "Drive",           artist: "The Cars",         duration: 183 },
        { title: "Born to Run",     artist: "Bruce Springsteen", duration: 270 }
    ]
    property int currentTrack: 0
    property real progress: 0.0

    // 播放进度模拟
    Timer {
        id: progressTimer
        interval: 1000
        running: powerState >= 1  // ACC 状态即可播放
        repeat: true
        onTriggered: {
            mediaPanel.progress += 1.0 / mediaPanel.playlist[mediaPanel.currentTrack].duration
            if (mediaPanel.progress >= 1.0) {
                mediaPanel.progress = 0.0
                mediaPanel.currentTrack = (mediaPanel.currentTrack + 1) % mediaPanel.playlist.length
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Text {
            text: "MEDIA"
            color: theme.textSecondary
            font.pixelSize: 14
            font.family: "Consolas"
            font.bold: true
        }

        // 歌曲信息
        Item {
            width: parent.width
            height: 50

            Text {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: playlist[currentTrack].title
                color: theme.textPrimary
                font.pixelSize: 18
                font.bold: true
                font.family: "Consolas"
            }
            Text {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: playlist[currentTrack].artist
                color: theme.textSecondary
                font.pixelSize: 14
                font.family: "Consolas"
            }
        }

        // 进度条
        Rectangle {
            width: parent.width
            height: 4
            radius: 2
            color: Qt.rgba(1, 1, 1, 0.15)

            Rectangle {
                width: parent.width * progress
                height: parent.height
                radius: parent.radius
                color: theme.accent

                Behavior on width { NumberAnimation { duration: 200 } }
            }
        }

        // 时间
        Item {
            width: parent.width
            height: 20

            Text {
                anchors.left: parent.left
                text: formatTime(progress * playlist[currentTrack].duration)
                color: theme.textSecondary
                font.pixelSize: 12
                font.family: "Consolas"
            }
            Text {
                anchors.right: parent.right
                text: formatTime(playlist[currentTrack].duration)
                color: theme.textSecondary
                font.pixelSize: 12
                font.family: "Consolas"
            }
        }

        // 控制按钮（模拟）
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20

            // 上一曲
            Text {
                text: "◄◄"
                color: theme.accent
                font.pixelSize: 20
                font.family: "Consolas"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        progress = 0.0
                        currentTrack = (currentTrack - 1 + playlist.length) % playlist.length
                    }
                }
            }

            // 播放/暂停
            Text {
                text: progressTimer.running ? "❚❚" : "►"
                color: theme.accent
                font.pixelSize: 24
                font.family: "Consolas"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (progressTimer.running) progressTimer.stop()
                        else progressTimer.start()
                    }
                }
            }

            // 下一曲
            Text {
                text: "►►"
                color: theme.accent
                font.pixelSize: 20
                font.family: "Consolas"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        progress = 0.0
                        currentTrack = (currentTrack + 1) % playlist.length
                    }
                }
            }
        }

        // 温度信息
        Item {
            width: parent.width
            height: 25
            anchors.bottom: parent.bottom

            Text {
                anchors.left: parent.left
                text: "INT: " + clusterVM.interiorTemp.toFixed(0) + "°C"
                color: theme.textSecondary
                font.pixelSize: 12
                font.family: "Consolas"
            }
            Text {
                anchors.right: parent.right
                text: "AMB: " + clusterVM.ambientTemp.toFixed(0) + "°C"
                color: theme.textSecondary
                font.pixelSize: 12
                font.family: "Consolas"
            }
        }
    }

    function formatTime(seconds) {
        var m = Math.floor(seconds / 60)
        var s = Math.floor(seconds % 60)
        return m + ":" + (s < 10 ? "0" : "") + s
    }
}
