cmake_minimum_required(VERSION 3.21)

project(DigitalCluster VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Qt 找包 ──────────────────────────────────────────────────
find_package(Qt6 COMPONENTS Core Gui Qml Quick QuickControls2 Network REQUIRED)

qt_standard_project_setup(REQUIRES 6.2)

# ── 编译选项 ─────────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0 -DQT_QML_DEBUG)
    endif()
endif()

# ── 全局包含路径 ─────────────────────────────────────────────
include_directories(${CMAKE_SOURCE_DIR}/src)

# ── 收集源文件 ───────────────────────────────────────────────
file(GLOB_RECURSE CLUSTER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)
# 排除 watchdog 入口，它属于 ClusterWatchdog 目标
list(REMOVE_ITEM CLUSTER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/safety/main_watchdog.cpp"
)
# 排除 POSIX 专属文件（Windows 不支持 Unix Domain Socket）
if(WIN32)
    list(REMOVE_ITEM CLUSTER_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/can/IpcCanReceiver.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/can/IpcCanReceiver.h"
    )
endif()

# ── 主进程可执行文件 ────────────────────────────────────────
qt_add_executable(DigitalCluster
    ${CLUSTER_SOURCES}
)

qt_add_qml_module(DigitalCluster
    URI DigitalCluster
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/pages/ClusterPage.qml
        qml/gauges/SpeedGauge.qml
        qml/gauges/TachGauge.qml
        qml/gauges/GearDisplay.qml
        qml/indicators/AdasPanel.qml
        qml/indicators/MediaPanel.qml
        qml/indicators/TellTales.qml
        qml/components/CircularGauge.qml
        qml/components/IndicatorLight.qml
        qml/components/WarningPopup.qml
        qml/themes/ThemeManager.qml
)

target_link_libraries(DigitalCluster PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
)

# ── 看门狗/备用进程可执行文件 ────────────────────────────────
# Watchdog 只需要自身入口 + WatchdogProcess，不需要 CAN 解析层
file(GLOB SAFETY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/safety/main_watchdog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/safety/WatchdogProcess.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/safety/WatchdogProcess.h"
)

qt_add_executable(ClusterWatchdog
    ${SAFETY_SOURCES}
)

target_link_libraries(ClusterWatchdog PRIVATE
    Qt6::Core
    Qt6::Network
)

# ── 编译 .ts → .qm 翻译文件 ────────────────────────────────
find_package(Qt6LinguistTools QUIET)
if(Qt6LinguistTools_FOUND)
    qt_add_translation(QM_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/en.ts"
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/zh.ts"
    )
    # 将 .qm 文件放到可执行文件同目录的 i18n/ 下
    add_custom_target(CompileTranslations ALL DEPENDS ${QM_FILES})
    add_dependencies(DigitalCluster CompileTranslations)
endif()

# ── 安装目标 ─────────────────────────────────────────────────
install(TARGETS DigitalCluster ClusterWatchdog
    RUNTIME DESTINATION bin
)

install(DIRECTORY config DESTINATION share/digitalcluster)

# 安装 .qm 文件（如果 lrelease 可用）
if(Qt6LinguistTools_FOUND AND QM_FILES)
    install(FILES ${QM_FILES} DESTINATION share/digitalcluster/i18n)
else()
    install(DIRECTORY i18n DESTINATION share/digitalcluster FILES_MATCHING PATTERN "*.ts")
endif()
