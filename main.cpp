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
    QString msg = QString("\n接收到中断信号 (%1)，正在安全退出...\n").arg(signum);
    qDebug() << msg;
    if (g_cliHandler) g_cliHandler->shutdown();
    exit(signum);
}


// 主程序入口
int main(int argc, char *argv[]) {
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
    std::string configFile = "config.json";
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
    MainWindow w;
    w.show();
    return a.exec();
}
