#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // 初始化组件
    initUI();
    initModules();
    initToolBar();
    initConnections();
    minilizeIcon();
    
    // 默认显示设备管理模块
    switchToDeviceManagement();
    
    updateStatusBar(tr("欢迎使用相机标定系统 v1.0"));

}
MainWindow::~MainWindow()
{
    delete m_deviceManagement;
    delete m_dataAcquisition;
    delete m_calibration;
    // delete m_resultVerification;
    // delete m_reportGenerator;
    // delete m_dataManagement;
    delete m_setting;
    delete ui;
}

// UI参数初始化
void MainWindow::initUI()
{
    // 设置窗口属性
    setWindowTitle(tr("业余相机标定系统"));
    setMinimumSize(1280, 800);
}

// 子模块初始化
void MainWindow::initModules()
{
    // 创建所有功能模块
    m_deviceManagement = new DeviceManagementModule(this);
    m_dataAcquisition = new DataAcquisitionModule(this);
    m_calibration = new CalibrationModule(this);
    m_resultVerification = new ResultVerificationModule(this);
    m_reportGenerator = new ReportGeneratorModule(this);
    // m_dataManagement = new DataManagementModule(this);
    m_setting = new SettingsModule(this);
    
    // 将模块添加到堆叠窗口
    ui->stackedWidget->addWidget(m_deviceManagement);
    ui->stackedWidget->addWidget(m_dataAcquisition);
    ui->stackedWidget->addWidget(m_calibration);
    ui->stackedWidget->addWidget(m_resultVerification);
    ui->stackedWidget->addWidget(m_reportGenerator);
    // ui->stackedWidget->addWidget(m_dataManagement);
    ui->stackedWidget->addWidget(m_setting);
}

// 初始化工具栏
void MainWindow::initToolBar()
{
    // 创建工具栏动作
    m_actionDeviceManagement = new QAction(QIcon(":/icons/imgs/device.png"), tr("设备管理"), this);
    m_actionDataAcquisition = new QAction(QIcon(":/icons/imgs/acquisition.png"), tr("图像管理"), this);
    m_actionCalibration = new QAction(QIcon(":/icons/imgs/calibration.png"), tr("参数计算"), this);
    m_actionResultVerification = new QAction(QIcon(":/icons/imgs/result.png"), tr("结果验证"), this);
    m_actionReportGeneration = new QAction(QIcon(":/icons/imgs/report.png"), tr("报告输出"), this);
    // m_actionDataManagement = new QAction(QIcon(":/icons/imgs/data.png"), tr("数据管理"), this);
    // m_actionWizard = new QAction(QIcon(":/icons/imgs/wizard.png"), tr("标定向导"), this);
    m_actionSettings = new QAction(QIcon(":/icons/imgs/settings.png"), tr("设  置"), this);
    // m_actionHelp = new QAction(QIcon(":/icons/imgs/help.png"), tr("帮  助"), this);
    
    // 设置动作提示
    m_actionDeviceManagement->setToolTip(tr("管理和配置相机设备"));
    m_actionDataAcquisition->setToolTip(tr("管理标定所需图像数据"));
    m_actionCalibration->setToolTip(tr("执行相机标定计算"));
    m_actionResultVerification->setToolTip(tr("验证和分析标定结果"));
    m_actionReportGeneration->setToolTip(tr("生成标定报告文档"));
    // m_actionDataManagement->setToolTip(tr("管理标定数据和历史记录"));
    // m_actionWizard->setToolTip(tr("快速标定向导"));
    m_actionSettings->setToolTip(tr("系统设置"));
    // m_actionHelp->setToolTip(tr("使用帮助和文档"));
    
    // 添加到工具栏
    ui->mainToolBar->addAction(m_actionDeviceManagement);
    ui->mainToolBar->addAction(m_actionDataAcquisition);
    ui->mainToolBar->addAction(m_actionCalibration);
    ui->mainToolBar->addAction(m_actionResultVerification);
    ui->mainToolBar->addAction(m_actionReportGeneration);
    // ui->mainToolBar->addAction(m_actionDataManagement);
    ui->mainToolBar->addSeparator();
    // ui->mainToolBar->addAction(m_actionWizard);
    ui->mainToolBar->addAction(m_actionSettings);
    // ui->mainToolBar->addAction(m_actionHelp);
    
    // 设置工具栏样式
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->mainToolBar->setIconSize(QSize(32, 32));
}

// 初始化槽连接
void MainWindow::initConnections()
{
    // 工具栏动作连接
    connect(m_actionDeviceManagement, &QAction::triggered, 
            this, &MainWindow::switchToDeviceManagement);
    connect(m_actionDataAcquisition, &QAction::triggered,
            this, &MainWindow::switchToDataAcquisition);
    connect(m_actionCalibration, &QAction::triggered,
            this, &MainWindow::switchToCalibration);
    connect(m_actionResultVerification, &QAction::triggered,
            this, &MainWindow::switchToResultVerification);
    connect(m_actionReportGeneration, &QAction::triggered,
            this, &MainWindow::switchToReportGenerator);
    // connect(m_actionDataManagement, &QAction::triggered,
    //         this, &MainWindow::switchToDataManagement);
    // connect(m_actionWizard, &QAction::triggered,
    //         this, &MainWindow::switchToHelp);
    connect(m_actionSettings, &QAction::triggered,
            this, &MainWindow::switchToSetting);

    
    // 设备管理模块信号连接
    connect(m_deviceManagement, &DeviceManagementModule::statusChanged,
            this, &MainWindow::updateStatusBar);
    // connect(m_deviceManagement, &DeviceManagementModule::deviceConnected,
    //         m_dataAcquisition, &DataAcquisitionModule::onDeviceConnected);
    
    // 图像管理模块信号连接
    connect(m_dataAcquisition, &DataAcquisitionModule::statusChanged,
            this, &MainWindow::updateStatusBar);
    connect(m_dataAcquisition, &DataAcquisitionModule::dataReady,
            m_calibration, &CalibrationModule::onDataReady);
    
    // 标定模块信号连接
    connect(m_calibration, &CalibrationModule::statusChanged,
            this, &MainWindow::updateStatusBar);
    connect(m_calibration, &CalibrationModule::calibrationComplete,
            m_resultVerification, &ResultVerificationModule::onCalibrationCompleted);
    
    // 结果验证模块信号连接
    connect(m_resultVerification, &ResultVerificationModule::statusChanged,
            this, &MainWindow::updateStatusBar);
    // connect(m_resultVerification, &ResultVerificationModule::resultsAccepted,
    //         m_reportGenerator, &ReportGeneratorModule::onResultsAvailable);

     // 设置模块信号连接
    connect(m_setting, &SettingsModule::statusChanged,
            this, &MainWindow::updateStatusBar);
    connect(m_setting, &SettingsModule::settingsChanged,
            this, &MainWindow::updateSetting);
    connect(m_setting, &SettingsModule::settingsChanged,
            m_calibration, &CalibrationModule::updateCalibSetting);
}


/* ================== 页面切换槽函数 ============= */
void MainWindow::switchToDeviceManagement()
{
    ui->stackedWidget->setCurrentWidget(m_deviceManagement);
    setWindowTitle(tr("业余相机标定系统 - 设备管理"));
}

void MainWindow::switchToDataAcquisition()
{
    // 检查是否有连接的设备
    if (m_deviceManagement->getConnectedDevice()) {
        QMessageBox::warning(this, tr("设备未连接"),
                           tr("请先在设备管理模块连接相机设备"));
        return;
    }
    
    ui->stackedWidget->setCurrentWidget(m_dataAcquisition);
    setWindowTitle(tr("业余相机标定系统 - 数据采集"));
}

void MainWindow::switchToCalibration()
{
    ui->stackedWidget->setCurrentWidget(m_calibration);
    setWindowTitle(tr("业余相机标定系统 - 参数计算"));
}

void MainWindow::switchToResultVerification()
{
    ui->stackedWidget->setCurrentWidget(m_resultVerification);
    setWindowTitle(tr("业余相机标定系统 - 结果验证"));
}

void MainWindow::switchToReportGenerator()
{
    ui->stackedWidget->setCurrentWidget(m_reportGenerator);
    setWindowTitle(tr("业余相机标定系统 - 报告输出"));
}

// void MainWindow::switchToDataManagement()
// {
//     ui->stackedWidget->setCurrentWidget(m_dataManagement);
//     setWindowTitle(tr("业余相机标定系统 - 数据管理"));
// }

void MainWindow::switchToSetting()
{
    ui->stackedWidget->setCurrentWidget(m_setting);
    setWindowTitle(tr("业余相机标定系统 - 设置"));
}

// 状态栏更新函数
void MainWindow::updateStatusBar(const QString &message)
{
    statusBar()->showMessage(message);
    qDebug() << "[STATUS]" << QDateTime::currentDateTime().toString() << message;
}
// 更新设置
void MainWindow::updateSetting(const AppSettings& settings)
{
    //待完善
    Q_UNUSED(settings);
}

QWidget *MainWindow::getCurrentModule() const
{
    return ui->stackedWidget->currentWidget();
}



//托盘事件
void MainWindow::minilizeIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/icons/imgs/logo.png"));

    trayMenu = new QMenu(this);
    showAction = new QAction("显示窗口", this);
    connect(showAction, &QAction::triggered, this, &MainWindow::show);
    trayMenu->addAction(showAction);

    quitAction = new QAction("退出", this);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    trayMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
}

// 重写关闭事件
void MainWindow::closeEvent(QCloseEvent *event)
{
    // 创建退出提醒消息框
    QMessageBox msgBox;msgBox.resize(300,300);
    msgBox.setWindowTitle("确认操作");
    msgBox.setText("你确定要关闭窗口吗？");

    // 添加三个按钮
    QPushButton *cancelButton = msgBox.addButton("取消", QMessageBox::RejectRole);
    QPushButton *hideButton = msgBox.addButton("隐藏", QMessageBox::ActionRole);
    QPushButton *okButton = msgBox.addButton("确定", QMessageBox::AcceptRole);

    // 显示消息框并获取用户点击的按钮
    msgBox.exec();

    if (msgBox.clickedButton() == okButton) {
        // 用户点击了确定按钮，退出应用程序
        event->accept();
    } else if (msgBox.clickedButton() == hideButton) {
        // 用户点击了隐藏按钮，隐藏主窗口
        if (trayIcon->isVisible()) {
            hide();
            event->ignore();
        } else {
            event->accept();
        }
    } else if (msgBox.clickedButton() == cancelButton) {
        // 用户点击了取消按钮，不做任何操作
       event->ignore();
    }
}
