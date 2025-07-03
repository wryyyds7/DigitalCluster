import QtQuick

/**
 * 主题管理器 —— 热切换颜色方案，无需重启应用
 * 绑定到 ClusterViewModel.themeChanged 信号实现热切换
 */
QtObject {
    id: themeManager

    // 当前主题名称（由 ViewModel 驱动）
    property string currentTheme: clusterVM ? clusterVM.currentTheme : "dark"

    // ── 颜色方案 ──
    readonly property var themes: ({
        "dark": {
            "bgGradTop":    "#0a0e14",
            "bgGradBottom": "#161b26",
            "accent":       "#00d4ff",
            "accentDim":    "#0088aa",
            "warning":       "#ff9500",
            "danger":       "#ff3b30",
            "success":      "#34c759",
            "textPrimary":  "#ffffff",
            "textSecondary":"#8e8e93",
            "gaugeRing":    "#2a2a2a",
            "gaugeFill":    "#00d4ff",
            "redline":      "#ff3b30"
        },
        "light": {
            "bgGradTop":    "#e8eaed",
            "bgGradBottom": "#c6c8cc",
            "accent":       "#007aff",
            "accentDim":    "#0055aa",
            "warning":       "#ff9500",
            "danger":       "#ff3b30",
            "success":      "#34c759",
            "textPrimary":  "#000000",
            "textSecondary":"#6e6e73",
            "gaugeRing":    "#d0d0d0",
            "gaugeFill":    "#007aff",
            "redline":      "#ff3b30"
        },
        "sport": {
            "bgGradTop":    "#1a0500",
            "bgGradBottom": "#2d0a00",
            "accent":       "#ff6b00",
            "accentDim":    "#aa4400",
            "warning":       "#ffd700",
            "danger":       "#ff0000",
            "success":      "#00ff00",
            "textPrimary":  "#ffffff",
            "textSecondary":"#ff8866",
            "gaugeRing":    "#331100",
            "gaugeFill":    "#ff6b00",
            "redline":      "#ff0000"
        },
        "eco": {
            "bgGradTop":    "#00150a",
            "bgGradBottom": "#00220f",
            "accent":       "#00ff88",
            "accentDim":    "#00aa55",
            "warning":       "#ffd700",
            "danger":       "#ff3b30",
            "success":      "#00ff88",
            "textPrimary":  "#ffffff",
            "textSecondary":"#66aa88",
            "gaugeRing":    "#00220f",
            "gaugeFill":    "#00ff88",
            "redline":      "#ff3b30"
        }
    })

    // 当前激活的颜色
    // 注意：颜色属性不使用 readonly，以便 Behavior 能捕获绑定驱动的值变化
    property var t: themes[currentTheme] || themes["dark"]

    // 便捷属性 — 可写以支持 Behavior 动画
    property color bgColorTop:    t.bgGradTop
    property color bgColorBottom:  t.bgGradBottom
    property color accent:         t.accent
    property color accentDim:      t.accentDim
    property color warning:        t.warning
    property color danger:         t.danger
    property color success:        t.success
    property color textPrimary:     t.textPrimary
    property color textSecondary:  t.textSecondary
    property color gaugeRing:      t.gaugeRing
    property color gaugeFill:      t.gaugeFill
    property color redline:        t.redline

    // 主题切换动画：对可写属性使用 Behavior 实现平滑过渡
    Behavior on bgColorTop    { ColorAnimation { duration: 300 } }
    Behavior on bgColorBottom  { ColorAnimation { duration: 300 } }
    Behavior on accent         { ColorAnimation { duration: 300 } }
    Behavior on accentDim      { ColorAnimation { duration: 300 } }
    Behavior on warning        { ColorAnimation { duration: 300 } }
    Behavior on danger         { ColorAnimation { duration: 300 } }
    Behavior on success        { ColorAnimation { duration: 300 } }
    Behavior on textPrimary    { ColorAnimation { duration: 300 } }
    Behavior on textSecondary  { ColorAnimation { duration: 300 } }
    Behavior on gaugeRing      { ColorAnimation { duration: 300 } }
    Behavior on gaugeFill      { ColorAnimation { duration: 300 } }
    Behavior on redline        { ColorAnimation { duration: 300 } }
}
