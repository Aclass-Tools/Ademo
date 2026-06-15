/**
 * @file main.cpp
 * @brief Qt工业客户端应用程序入口点
 * @author Claude Code
 * @date 2026-04-10
 */

#include <QApplication>
#include "ui/MainWindow.h"

/**
 * @brief 应用程序入口点
 *
 * 初始化Qt应用程序，设置应用程序元数据，
 * 创建并显示主窗口。
 *
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 应用程序退出码
 */
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("QtIndustrialClient");
    QApplication::setOrganizationName("Spin");

    MainWindow window;
    window.show();

    return app.exec();
}
