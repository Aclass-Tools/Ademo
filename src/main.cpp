// 应用程序入口。
// 职责：
// 1) 初始化 Qt 事件循环。
// 2) 设置应用全局图标。
// 3) 构建并显示主窗口外壳。
// 4) 进入 QApplication::exec() 消息循环。

#include <QApplication>
#include <QIcon>
#include <QFileInfo>
#include <QDir>

#include "ui/mainwindow.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // 显式设置 AppUserModelID，确保任务栏图标分组与显示稳定。
    // 使用动态加载方式，避免不同 MinGW/SDK 头文件缺少声明导致编译失败。
    using SetAppIdFn = HRESULT (WINAPI *)(PCWSTR);
    HMODULE shell32 = LoadLibraryW(L"shell32.dll");
    if (shell32) {
        auto setAppId =
            reinterpret_cast<SetAppIdFn>(GetProcAddress(shell32, "SetCurrentProcessExplicitAppUserModelID"));
        if (setAppId) {
            setAppId(L"com.aclass.tools");
        }
        FreeLibrary(shell32);
    }
#endif

    QApplication a(argc, argv);

    // 正式模式：优先从 qrc 加载，磁盘路径仅作为兜底。
    const QString appDir = QCoreApplication::applicationDirPath();
    QIcon appIcon(QStringLiteral(":/icons/app_icon.ico"));
    if (appIcon.isNull()) {
        appIcon = QIcon(QStringLiteral(":/icons/app_icon.png"));
    }

    const QStringList diskCandidates = {
        QDir::cleanPath(appDir + "/resources/icons/app_icon.ico"),
        QDir::cleanPath(appDir + "/resources/icons/app_icon.png"),
        QDir::cleanPath(appDir + "/../resources/icons/app_icon.ico"),
        QDir::cleanPath(appDir + "/../resources/icons/app_icon.png"),
        QDir::cleanPath(appDir + "/../../resources/icons/app_icon.ico"),
        QDir::cleanPath(appDir + "/../../resources/icons/app_icon.png")
    };

    if (appIcon.isNull()) {
        for (const QString &path : diskCandidates) {
            if (!QFileInfo::exists(path)) {
                continue;
            }
            QIcon icon(path);
            if (!icon.isNull()) {
                appIcon = icon;
                break;
            }
        }
    }
    a.setWindowIcon(appIcon);

    MainWindow w;
    // 再次设置主窗口图标，避免部分系统未继承应用级图标。
    w.setWindowIcon(appIcon);
    w.show();

    return a.exec();
}
