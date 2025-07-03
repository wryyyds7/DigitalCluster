import QtQuick

/**
 * 圆形仪表基础组件
 * 使用 Canvas 绘制刻度环 + Canvas 指针
 * 优化：指针使用独立 Canvas + NumberAnimation 实现流畅动画
 * 避免在 onPaint 中做耗时计算，所有数据绑定到属性
 */
Item {
    id: root

    // ── 输入属性 ──
    property real value: 0.0
    property real minValue: 0.0
    property real maxValue: 100.0
    property real redlineStart: 80.0
    property int majorTicks: 10
    property int minorTicks: 5

    // ── 颜色 ──
    property color ringColor:     theme.gaugeRing
    property color fillColor:     theme.gaugeFill
    property color redlineColor:  theme.redline
    property color tickColor:     theme.textSecondary
    property color labelColor:    theme.textSecondary
    property color needleColor:   theme.accent

    // ── 角度范围（从 -220° 到 40°，总弧长 260°）──
    property real startAngle: -220
    property real endAngle: 40
    property real totalAngle: endAngle - startAngle

    // ── 激活状态 ──
    property bool active: true

    // ── 值→角度 转换 ──
    readonly property real valueAngle: {
        var pct = Math.max(0, Math.min(1, (value - minValue) / (maxValue - minValue)))
        return startAngle + pct * totalAngle
    }

    // ── 平滑动画值（用于指针，不用于刻度绘制）──
    property real animatedValue: value
    Behavior on animatedValue {
        SpringAnimation {
            spring: 3
            damping: 0.4
            epsilon: 0.01
        }
    }

    onValueChanged: {
        animatedValue = value
    }

    // ── 背景环 ──
    Canvas {
        id: bgRing
        anchors.fill: parent
        renderStrategy: Canvas.Threaded  // 线程化渲染，避免阻塞 UI 线程

        // 主题切换时颜色变化需手动触发重绘
        onRingColorChanged: requestPaint()
        onTickColorChanged: requestPaint()
        onRedlineColorChanged: requestPaint()
        onLabelColorChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var cx = width / 2
            var cy = height / 2
            var r = Math.min(width, height) / 2 - 20

            // 背景圆环
            ctx.lineWidth = 8
            ctx.strokeStyle = ringColor
            ctx.beginPath()
            ctx.arc(cx, cy, r, (root.startAngle - 90) * Math.PI / 180,
                    (root.endAngle - 90) * Math.PI / 180, false)
            ctx.stroke()

            // 主动刻度
            var angleStep = root.totalAngle / root.majorTicks
            for (var i = 0; i <= root.majorTicks; ++i) {
                var angle = (root.startAngle + i * angleStep - 90) * Math.PI / 180
                var x1 = cx + Math.cos(angle) * (r - 12)
                var y1 = cy + Math.sin(angle) * (r - 12)
                var x2 = cx + Math.cos(angle) * (r - 2)
                var y2 = cy + Math.sin(angle) * (r - 2)

                ctx.lineWidth = 2
                ctx.strokeStyle = tickColor
                ctx.beginPath()
                ctx.moveTo(x1, y1)
                ctx.lineTo(x2, y2)
                ctx.stroke()
            }

            // 次要刻度
            var minorStep = angleStep / root.minorTicks
            for (var j = 0; j < root.majorTicks; ++j) {
                for (var k = 1; k < root.minorTicks; ++k) {
                    var a = (root.startAngle + (j + k / root.minorTicks) * angleStep - 90) * Math.PI / 180
                    var mx1 = cx + Math.cos(a) * (r - 8)
                    var my1 = cy + Math.sin(a) * (r - 8)
                    var mx2 = cx + Math.cos(a) * (r - 2)
                    var my2 = cy + Math.sin(a) * (r - 2)

                    ctx.lineWidth = 1
                    ctx.strokeStyle = Qt.rgba(tickColor.r, tickColor.g, tickColor.b, 0.5)
                    ctx.beginPath()
                    ctx.moveTo(mx1, my1)
                    ctx.lineTo(mx2, my2)
                    ctx.stroke()
                }
            }

            // 红线区域
            if (root.redlineStart < root.maxValue) {
                var redPct = (root.redlineStart - root.minValue) / (root.maxValue - root.minValue)
                var redStartAngle = (root.startAngle + redPct * root.totalAngle - 90) * Math.PI / 180

                ctx.lineWidth = 8
                ctx.strokeStyle = redlineColor
                ctx.beginPath()
                ctx.arc(cx, cy, r, redStartAngle,
                        (root.endAngle - 90) * Math.PI / 180, false)
                ctx.stroke()
            }

            // 刻度数字
            ctx.font = "14px Consolas"
            ctx.fillStyle = labelColor
            ctx.textAlign = "center"
            ctx.textBaseline = "middle"
            for (var n = 0; n <= root.majorTicks; ++n) {
                var na = (root.startAngle + n * angleStep - 90) * Math.PI / 180
                var labelVal = Math.round(root.minValue + n / root.majorTicks * (root.maxValue - root.minValue))
                var lx = cx + Math.cos(na) * (r - 28)
                var ly = cy + Math.sin(na) * (r - 28)
                ctx.fillText(labelVal.toString(), lx, ly)
            }
        }
    }

    // ── 值填充弧 ──
    Canvas {
        id: fillArc
        anchors.fill: parent
        renderStrategy: Canvas.Threaded

        property real animValue: root.animatedValue

        onAnimValueChanged: {
            // 动画期间重绘
            requestPaint()
        }
        onFillColorChanged: requestPaint()
        onRedlineColorChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var cx = width / 2
            var cy = height / 2
            var r = Math.min(width, height) / 2 - 20

            var pct = Math.max(0, Math.min(1, (animValue - root.minValue) /
                                            (root.maxValue - root.minValue)))
            if (pct <= 0.001) return

            var fillEnd = (root.startAngle + pct * root.totalAngle - 90) * Math.PI / 180

            // 渐变填充
            var grad = ctx.createLinearGradient(0, 0, width, 0)
            grad.addColorStop(0, fillColor)
            grad.addColorStop(1, pct > 0.8 ? redlineColor : fillColor)

            ctx.lineWidth = 8
            ctx.strokeStyle = grad
            ctx.beginPath()
            ctx.arc(cx, cy, r, (root.startAngle - 90) * Math.PI / 180, fillEnd, false)
            ctx.stroke()
        }
    }

    // ── 指针 ──
    // 使用独立 Canvas + SpringAnimation 实现 60 FPS 动画
    // 指针仅依赖 animatedValue，避免在 onPaint 中计算
    Canvas {
        id: needle
        anchors.fill: parent
        renderStrategy: Canvas.Threaded

        property real animValue: root.animatedValue

        onAnimValueChanged: requestPaint()
        onNeedleColorChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var cx = width / 2
            var cy = height / 2
            var r = Math.min(width, height) / 2 - 20

            if (!root.active) return

            var pct = Math.max(0, Math.min(1, (animValue - root.minValue) /
                                            (root.maxValue - root.minValue)))
            var angle = (root.startAngle + pct * root.totalAngle - 90) * Math.PI / 180

            // 指针主体
            ctx.lineWidth = 4
            ctx.strokeStyle = needleColor
            ctx.shadowColor = needleColor
            ctx.shadowBlur = 10
            ctx.beginPath()
            ctx.moveTo(cx, cy)
            ctx.lineTo(cx + Math.cos(angle) * (r - 15),
                       cy + Math.sin(angle) * (r - 15))
            ctx.stroke()
            ctx.shadowBlur = 0

            // 指针尾部
            ctx.lineWidth = 2
            ctx.beginPath()
            ctx.moveTo(cx, cy)
            ctx.lineTo(cx - Math.cos(angle) * 20,
                       cy - Math.sin(angle) * 20)
            ctx.stroke()

            // 中心圆点
            ctx.fillStyle = needleColor
            ctx.beginPath()
            ctx.arc(cx, cy, 8, 0, Math.PI * 2)
            ctx.fill()

            ctx.fillStyle = Qt.rgba(0, 0, 0, 0.8)
            ctx.beginPath()
            ctx.arc(cx, cy, 4, 0, Math.PI * 2)
            ctx.fill()
        }
    }
}
