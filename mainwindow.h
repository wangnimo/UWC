#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QToolBar>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QSystemTrayIcon>
#include <QMenu>
// 添加子模块
#include "modules/device_management.h"
#include "modules/data_acquisition.h"
#include "modules/calibration.h"
#include "modules/report_generator.h"
#include "modules/result_verification.h"
// #include "modules/settings.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // 获取当前激活的模块
    QWidget* getCurrentModule() const;
    
    // 获取设备管理模块（供其他模块调用）
    DeviceManagementModule* getDeviceManagementModule() const { return m_deviceManagement; }

public slots:
    // 模块切换槽函数
    void switchToDeviceManagement();
    void switchToDataAcquisition();
    void switchToCalibration();
    void switchToResultVerification();
    void switchToReportGenerator();
    // void switchToDataManagement();
    // void switchToHelp();
    void switchToSetting();
    
    // 状态更新槽函数
    void updateStatusBar(const QString& message);

    //设置更新槽函数
    void updateSetting(const AppSettings& settings);

protected:
    // 重写关闭事件
    void closeEvent(QCloseEvent *event) override;
private:
    Ui::MainWindow *ui;

    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QAction *showAction;
    QAction *quitAction;
    
    // 模块实例
    DeviceManagementModule* m_deviceManagement;
    DataAcquisitionModule* m_dataAcquisition;
    CalibrationModule* m_calibration;
    ResultVerificationModule* m_resultVerification;
    ReportGeneratorModule* m_reportGenerator;
    // DataManagementModule* m_dataManagement;
    // HelpModule* m_help;
    SettingsModule* m_setting;
    
    // 工具栏动作
    QAction* m_actionDeviceManagement;
    QAction* m_actionDataAcquisition;
    QAction* m_actionCalibration;
    QAction* m_actionResultVerification;
    QAction* m_actionReportGeneration;
    QAction* m_actionDataManagement;
    QAction* m_actionWizard;
    QAction* m_actionSettings;
    // QAction* m_actionHelp;
    
    
    // 初始化UI组件
    void initUI();
    
    // 初始化模块
    void initModules();
    
    // 初始化工具栏
    void initToolBar();
    
    // 初始化信号槽连接
    void initConnections();

    // 最小化到托盘
    void minilizeIcon();
};
#endif // MAINWINDOW_H
    
