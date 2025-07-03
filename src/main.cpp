#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include <QTranslator>
#include <QLocale>
#include <QDir>
#include <iostream>

#include "models/VehicleModel.h"
#include "viewmodels/ClusterViewModel.h"
#include "safety/HeartbeatSender.h"

int main(int argc, char* argv[])
{
    // Qt 高 DPI 支持
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);
    app.setOrganizationName("DigitalCluster");
    app.setApplicationName("DigitalCluster");

    // ── 全局 QML 样式 ──
    QQuickStyle::setStyle("Basic");

    // ── 多语言：默认加载系统 locale 对应的翻译 ──
    QTranslator translator;
    QString localeName = QLocale::system().name();  // e.g. "zh_CN"
    QString langCode = localeName.section('_', 0, 0);  // "zh" or "en"

    // 尝试从 i18n 目录加载 .qm 文件
    // 搜索路径：可执行文件目录/i18n、share/digitalcluster/i18n
    QStringList i18nPaths = {
        QGuiApplication::applicationDirPath() + "/i18n",
        QGuiApplication::applicationDirPath() + "/../i18n",
        QGuiApplication::applicationDirPath() + "/../share/digitalcluster/i18n",
    };

    for (const auto& path : i18nPaths) {
        QString qmFile = path + "/" + langCode + ".qm";
        if (QFile::exists(qmFile)) {
            if (translator.load(qmFile)) {
                app.installTranslator(&translator);
                std::cout << "[i18n] Loaded translation: " << qmFile.toStdString() << "\n";
                break;
            }
        }
    }

    // ── Model 层 ──
    VehicleModel model;

    // 解析命令行参数选择 CAN 后端
    std::string canChannel = "virtual";
    bool autoSim = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--channel" && i + 1 < argc) {
            canChannel = argv[++i];
        }
        if (arg == "--no-sim") {
            autoSim = false;
        }
    }

    if (!model.initialize(canChannel, autoSim)) {
        std::cerr << "Failed to initialize CAN interface\n";
        return -1;
    }

    // ── ViewModel 层 ──
    ClusterViewModel viewModel(&model);

    // 注入翻译器用于多语言热切换
    viewModel.setTranslator(&translator);
    // 找到实际加载成功的 i18n 目录
    for (const auto& path : i18nPaths) {
        QString qmFile = path + "/" + langCode + ".qm";
        if (QFile::exists(qmFile)) {
            viewModel.setI18nDir(path);
            break;
        }
    }

    // ── 功能安全：心跳发送器 ──
    HeartbeatSender heartbeat;
    heartbeat.setDataProvider([&viewModel]() {
        return std::make_tuple(
            viewModel.speed(),
            viewModel.leftTurn(),
            viewModel.rightTurn()
        );
    });
    heartbeat.start();

    // ── QML 引擎 ──
    QQmlApplicationEngine engine;

    // 注册 ViewModel 到 QML 上下文
    engine.rootContext()->setContextProperty("clusterVM", &viewModel);

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        std::cerr << "Failed to load QML\n";
        return -1;
    }

    // 自动触发上电序列（OFF → ACC → ON）
    QTimer::singleShot(1000, [&viewModel]() { viewModel.powerOn(); });
    QTimer::singleShot(2000, [&viewModel]() { viewModel.startEngine(); });

    return app.exec();
}
