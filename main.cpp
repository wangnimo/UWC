#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用风格
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // 设置应用信息
    QApplication::setApplicationName("UnderwaterCalibrator");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("OceanTech");
    QApplication::setWindowIcon(QIcon(":/icons/imgs/logo.png"));
    try {
        MainWindow w;
        w.show();
        return a.exec();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "致命错误",
                              QString("程序启动失败: %1").arg(e.what()));
        return 1;
    } catch (...) {
        QMessageBox::critical(nullptr, "致命错误", "发生未知错误，程序无法启动");
        return 1;
    }
}
