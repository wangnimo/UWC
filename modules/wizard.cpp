#include "wizard.h"
#include "ui_wizard.h"
#include "mainwindow.h"
#include "modules/device_management.h"
#include "modules/data_acquisition.h"
#include "modules/calibration.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QProgressBar>
#include <QMessageBox>
#include <QTimer>

CalibrationWizard::CalibrationWizard(MainWindow* mainWindow, QWidget *parent)
    : QWizard(parent), ui(new Ui::CalibrationWizard), m_mainWindow(mainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("相机标定向导"));
    setWizardStyle(QWizard::ModernStyle);
    setPixmap(QWizard::LogoPixmap, QPixmap(":/icons/calibration_wizard.png"));
    
    initPages();
    
    connect(this, &QWizard::currentIdChanged, this, &CalibrationWizard::onCurrentIdChanged);
}

CalibrationWizard::~CalibrationWizard()
{
    delete ui;
}

void CalibrationWizard::startWizard()
{
    m_calibrationData.clear();
    m_calibrationResult = CalibrationResult();
    restart();
    show();
}

void CalibrationWizard::initPages()
{
    addPage(createIntroductionPage());
    addPage(createDeviceSelectionPage());
    addPage(createCalibrationSettingsPage());
    addPage(createDataAcquisitionPage());
    addPage(createCalibrationProcessPage());
    addPage(createResultReviewPage());
    addPage(createCompletionPage());
}

QWizardPage* CalibrationWizard::createIntroductionPage()
{
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("欢迎使用相机标定向导"));
    page->setSubTitle(tr("本向导将引导您完成相机标定的全过程"));
    
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    QLabel* label = new QLabel(tr("相机标定是获取相机内参和畸变系数的过程，\n"
                                 "用于后续的图像校正和三维测量。\n\n"
                                 "标定过程将引导您完成以下步骤：\n"
                                 "1. 选择要标定的相机设备\n"
                                 "2. 设置标定参数\n"
                                 "3. 采集标定图像\n"
                                 "4. 执行标定计算\n"
                                 "5. 查看和保存标定结果\n\n"
                                 "请准备好棋盘格标定板，然后点击下一步。"));
    label->setWordWrap(true);
    layout->addWidget(label);
    
    return page;
}

QWizardPage* CalibrationWizard::createDeviceSelectionPage()
{
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("选择相机设备"));
    page->setSubTitle(tr("请选择要进行标定的相机"));
    
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    QListWidget* deviceListWidget = new QListWidget;
    deviceListWidget->setObjectName("deviceListWidget");
    layout->addWidget(deviceListWidget);
    
    QPushButton* refreshButton = new QPushButton(tr("刷新设备列表"));
    layout->addWidget(refreshButton);
    
    connect(refreshButton, &QPushButton::clicked, this, [=]() {
        updateDeviceList();
    });
    
    return page;
}

QWizardPage* CalibrationWizard::createCalibrationSettingsPage()
{
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("标定参数设置"));
    page->setSubTitle(tr("请设置标定板参数和计算选项"));
    
    QFormLayout* layout = new QFormLayout(page);
    
    // 棋盘格宽度
    QSpinBox* boardWidthSpin = new QSpinBox;
    boardWidthSpin->setObjectName("boardWidthSpin");
    boardWidthSpin->setRange(3, 20);
    boardWidthSpin->setValue(9);
    layout->addRow(tr("棋盘格宽度(内角点):"), boardWidthSpin);
    
    // 棋盘格高度
    QSpinBox* boardHeightSpin = new QSpinBox;
    boardHeightSpin->setObjectName("boardHeightSpin");
    boardHeightSpin->setRange(3, 20);
    boardHeightSpin->setValue(6);
    layout->addRow(tr("棋盘格高度(内角点):"), boardHeightSpin);
    
    // 棋盘格大小
    QDoubleSpinBox* squareSizeSpin = new QDoubleSpinBox;
    squareSizeSpin->setObjectName("squareSizeSpin");
    squareSizeSpin->setRange(1.0, 100.0);
    squareSizeSpin->setValue(25.0);
    squareSizeSpin->setSuffix(tr(" mm"));
    layout->addRow(tr("棋盘格方块大小:"), squareSizeSpin);
    
    // 畸变模型
    QComboBox* distortionModelCombo = new QComboBox;
    distortionModelCombo->setObjectName("distortionModelCombo");
    distortionModelCombo->addItem(tr("5参数畸变模型"), 0);
    distortionModelCombo->addItem(tr("8参数畸变模型"), 1);
    distortionModelCombo->addItem(tr("无畸变"), 2);
    layout->addRow(tr("畸变模型:"), distortionModelCombo);
    
    // 最小采集图像数量
    QSpinBox* minImagesSpin = new QSpinBox;
    minImagesSpin->setObjectName("minImagesSpin");
    minImagesSpin->setRange(5, 50);
    minImagesSpin->setValue(15);
    layout->addRow(tr("最小采集图像数量:"), minImagesSpin);
    
    return page;
}

QWizardPage* CalibrationWizard::createDataAcquisitionPage()
{
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("标定图像采集"));
    page->setSubTitle(tr("请从不同角度和位置拍摄标定板图像"));
    
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // 预览窗口
    QLabel* previewLabel = new QLabel;
    previewLabel->setObjectName("previewLabel");
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setMinimumHeight(400);
    previewLabel->setStyleSheet("background-color: #333; border: 1px solid #666;");
    previewLabel->setText(tr("相机预览将显示在这里"));
    layout->addWidget(previewLabel);
    
    // 图像计数
    QLabel* countLabel = new QLabel(tr("已采集图像: 0"));
    countLabel->setObjectName("countLabel");
    countLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(countLabel);
    
    // 控制按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    
    QPushButton* captureButton = new QPushButton(tr("采集图像"));
    captureButton->setObjectName("captureButton");
    captureButton->setMinimumHeight(40);
    captureButton->setStyleSheet("font-size: 14px;");
    buttonLayout->addWidget(captureButton);
    
    QPushButton* deleteButton = new QPushButton(tr("删除最后一张"));
    deleteButton->setObjectName("deleteButton");
    buttonLayout->addWidget(deleteButton);
    
    layout->addLayout(buttonLayout);
    
    connect(captureButton, &QPushButton::clicked, this, &CalibrationWizard::onCaptureImage);
    
    return page;
}

QWizardPage* CalibrationWizard::createCalibrationProcessPage()
{
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("执行标定计算"));
    page->setSubTitle(tr("正在计算相机参数，请稍候..."));
    
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    QProgressBar* progressBar = new QProgressBar;
    progressBar->setObjectName("progressBar");
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    layout->addWidget(progressBar);
    
    QLabel* statusLabel = new QLabel(tr("准备开始标定..."));
    statusLabel->setObjectName("statusLabel");
    statusLabel->setWordWrap(true);
    layout->addWidget(statusLabel);
    
    QPushButton* startButton = new QPushButton(tr("开始标定"));
    startButton->setObjectName("startButton");
    startButton->setMinimumHeight(40);
    startButton->setStyleSheet("font-size: 14px;");
    layout->addWidget(startButton);
    
    connect(startButton, &QPushButton::clicked, this, &CalibrationWizard::onStartCalibration);
    
    return page;
}

QWizardPage* CalibrationWizard::createResultReviewPage()
{
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("标定结果查看"));
    page->setSubTitle(tr("请查看标定结果和误差分析"));
    
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    // 结果摘要
    QTextEdit* resultTextEdit = new QTextEdit;
    resultTextEdit->setObjectName("resultTextEdit");
    resultTextEdit->setReadOnly(true);
    layout->addWidget(resultTextEdit);
    
    // 误差可视化
    QLabel* errorLabel = new QLabel(tr("误差可视化将显示在这里"));
    errorLabel->setObjectName("errorLabel");
    errorLabel->setAlignment(Qt::AlignCenter);
    errorLabel->setMinimumHeight(200);
    errorLabel->setStyleSheet("background-color: #333; border: 1px solid #666;");
    layout->addWidget(errorLabel);
    
    return page;
}

QWizardPage* CalibrationWizard::createCompletionPage()
{
    QWizardPage* page = new QWizardPage;
    page->setTitle(tr("标定完成"));
    page->setSubTitle(tr("相机标定过程已完成"));
    
    QVBoxLayout* layout = new QVBoxLayout(page);
    
    QLabel* label = new QLabel(tr("恭喜，相机标定已成功完成！\n\n"
                                 "您可以选择以下操作：\n"
                                 "- 生成标定报告\n"
                                 "- 保存标定参数\n"
                                 "- 重新进行标定\n\n"
                                 "点击完成按钮退出向导。"));
    label->setWordWrap(true);
    layout->addWidget(label);
    
    // 选项
    QCheckBox* reportCheckBox = new QCheckBox(tr("生成标定报告"));
    reportCheckBox->setObjectName("reportCheckBox");
    reportCheckBox->setChecked(true);
    layout->addWidget(reportCheckBox);
    
    QCheckBox* saveParamsCheckBox = new QCheckBox(tr("保存标定参数"));
    saveParamsCheckBox->setObjectName("saveParamsCheckBox");
    saveParamsCheckBox->setChecked(true);
    layout->addWidget(saveParamsCheckBox);
    
    QCheckBox* newCalibrationCheckBox = new QCheckBox(tr("重新进行标定"));
    newCalibrationCheckBox->setObjectName("newCalibrationCheckBox");
    layout->addWidget(newCalibrationCheckBox);
    
    return page;
}

void CalibrationWizard::updateDeviceList()
{
    QListWidget* deviceList = findChild<QListWidget*>("deviceListWidget");
    if (!deviceList) return;
    
    deviceList->clear();
    
    // 从设备管理模块获取设备列表
    if (m_mainWindow) {
        DeviceManagementModule* deviceModule = m_mainWindow->getDeviceManagementModule();
        // 假设DeviceManagementModule有获取设备列表的方法
        // QList<DeviceInfo> devices = deviceModule->getDeviceList();
        
        // 这里只是示例，实际应从设备管理模块获取真实设备列表
        deviceList->addItem(tr("GigE相机: MV-CA050-10GM (SN: GC123456)"));
        deviceList->addItem(tr("USB3相机: MV-CE013-50GM (SN: UC654321)"));
        
        deviceList->setCurrentRow(0);
    }
}

void CalibrationWizard::onCurrentIdChanged(int id)
{
    switch (id) {
        case Page_DeviceSelection:
            updateDeviceList();
            break;
        case Page_DataAcquisition:
            // 启动相机预览
            if (m_mainWindow) {
                DataAcquisitionModule* acquisitionModule = m_mainWindow->getDataAcquisitionModule();
                // 启动预览
            }
            break;
        case Page_Completion:
            // 显示最终结果
            break;
        default:
            break;
    }
}

void CalibrationWizard::onCaptureImage()
{
    // 采集图像并添加到标定数据列表
    // 这里只是示例实现
    QLabel* countLabel = findChild<QLabel*>("countLabel");
    if (countLabel) {
        static int count = 0;
        count++;
        countLabel->setText(tr("已采集图像: %1").arg(count));
    }
    
    // 实际实现中应从相机获取图像并添加到m_calibrationData
}

void CalibrationWizard::onStartCalibration()
{
    QProgressBar* progressBar = findChild<QProgressBar*>("progressBar");
    QLabel* statusLabel = findChild<QLabel*>("statusLabel");
    
    // 模拟标定过程
    QTimer* timer = new QTimer(this);
    int progress = 0;
    
    connect(timer, &QTimer::timeout, this, [=]() {
        progress += 5;
        if (progress > 100) {
            progress = 100;
            timer->stop();
            timer->deleteLater();
            
            statusLabel->setText(tr("标定完成！重投影误差: 0.35像素"));
            QMessageBox::information(this, tr("标定完成"), tr("相机标定已成功完成！"));
        } else if (progress == 30) {
            statusLabel->setText(tr("正在提取角点..."));
        } else if (progress == 60) {
            statusLabel->setText(tr("正在计算内参矩阵..."));
        } else if (progress == 80) {
            statusLabel->setText(tr("正在计算畸变系数..."));
        }
        
        if (progressBar) progressBar->setValue(progress);
    });
    
    timer->start(100);
    statusLabel->setText(tr("开始标定..."));
}

bool CalibrationWizard::validatePage()
{
    int currentId = currentPage()->nextId();
    
    // 验证设备选择页面
    if (currentId == Page_DeviceSelection) {
        QListWidget* deviceList = findChild<QListWidget*>("deviceListWidget");
        if (!deviceList || deviceList->currentItem() == nullptr) {
            QMessageBox::warning(this, tr("选择设备"), tr("请选择要标定的相机设备"));
            return false;
        }
    }
    // 验证数据采集页面
    else if (currentId == Page_DataAcquisition) {
        QLabel* countLabel = findChild<QLabel*>("countLabel");
        QString text = countLabel->text();
        int count = text.split(": ").last().toInt();
        
        QSpinBox* minImagesSpin = findChild<QSpinBox*>("minImagesSpin");
        int minImages = minImagesSpin ? minImagesSpin->value() : 10;
        
        if (count < minImages) {
            QMessageBox::warning(this, tr("图像不足"), 
                               tr("采集的图像数量不足，请至少采集%1张图像").arg(minImages));
            return false;
        }
    }
    // 验证标定过程页面
    else if (currentId == Page_CalibrationProcess) {
        if (m_calibrationResult.success == false) {
            QMessageBox::warning(this, tr("标定失败"), tr("标定计算未成功完成"));
            return false;
        }
    }
    
    return true;
}
