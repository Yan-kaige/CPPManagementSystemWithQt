#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QStringList>

// CLI 模块引入
#include <iostream>
#include <csignal>
#include "include/CLIHandler.h"
#include "include/ConfigManager.h"
#include "include/Logger.h"
#include <QDebug>

CLIHandler* g_cliHandler = nullptr;

// 处理 Ctrl+C 等信号安全退出
void signalHandler(int signum) {
    std::cout << "\n接收到中断信号 (" << signum << ")，正在安全退出...\n";
    if (g_cliHandler) g_cliHandler->shutdown();
    exit(signum);
}

// CLI 启动入口
int runCliMode(int argc1, char* argv[]) {
    int argc=1;
    std::string configFile = "config.json";

    std::string executeCommand = "";
    bool showHelp = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            showHelp = true;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) configFile = argv[++i];
        } else if (arg == "-e" || arg == "--execute") {
            if (i + 1 < argc) executeCommand = argv[++i];
        }
    }

    if (showHelp) {
        std::cout << "用法: app [-c config.json] [-e \"command\"]\n";
        return 0;
    }

    ConfigManager::getInstance()->loadConfig(configFile);
    Logger::getInstance()->initialize("cli.log", LogLevel::INFO, 1024 * 1024, 3);

    CLIHandler cli;
    g_cliHandler = &cli;

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if (!cli.initialize()) {
        std::cerr << "初始化 CLIHandler 失败\n";
        return 1;
    }

    if (!executeCommand.empty()) {
        bool success = cli.executeCommand(executeCommand);
        cli.shutdown();
        return success ? 0 : 1;
    }

    cli.run();
    cli.shutdown();
    return 0;
}

// 主程序入口
int main(int argc, char *argv[]) {



    if (argc > 1) {
        qDebug() <<"CLI 模式";
        // CLI 模式
        return runCliMode(argc, argv);
    }

    // GUI 模式
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "testcppqt_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    MainWindow w;
    w.show();
    return a.exec();
}
