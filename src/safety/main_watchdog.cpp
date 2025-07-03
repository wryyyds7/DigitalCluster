#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <iostream>
#include "safety/WatchdogProcess.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("DigitalCluster");
    app.setApplicationName("ClusterWatchdog");

    WatchdogProcess watchdog;
    if (!watchdog.start("DigitalClusterWatchdog")) {
        std::cerr << "Failed to start watchdog server\n";
        return -1;
    }

    std::cout << "[ClusterWatchdog] Watchdog running. Waiting for main process...\n";
    std::cout << "[ClusterWatchdog] Will enter safety mode if heartbeat lost > 500ms\n";

    return app.exec();
}
