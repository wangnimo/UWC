#ifndef WIZARD_MODULE_H
#define WIZARD_MODULE_H

#include <QWizard>
#include <QWidget>
#include "calibration.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CalibrationWizard; }
QT_END_NAMESPACE

// 标定向导页面枚举
enum WizardPage {
    Page_Introduction,
    Page_DeviceSelection,
    Page_CalibrationSettings,
    Page_DataAcquisition,
    Page_CalibrationProcess,
    Page_ResultReview,
    Page_Completion
};

class MainWindow;

class CalibrationWizard : public QWizard
{
    Q_OBJECT

public:
    CalibrationWizard(MainWindow* mainWindow, QWidget *parent = nullptr);
    ~CalibrationWizard() override;
    
    // 开始向导
    void startWizard();

private slots:
    // 页面切换槽函数
    void onCurrentIdChanged(int id);
    
    // 采集图像槽函数
    void onCaptureImage();
    
    // 开始标定槽函数
    void onStartCalibration();

private:
    Ui::CalibrationWizard *ui;
    MainWindow* m_mainWindow;
    
    // 初始化页面
    void initPages();
    
    // 设备选择页面
    QWizardPage* createIntroductionPage();
    QWizardPage* createDeviceSelectionPage();
    QWizardPage* createCalibrationSettingsPage();
    QWizardPage* createDataAcquisitionPage();
    QWizardPage* createCalibrationProcessPage();
    QWizardPage* createResultReviewPage();
    QWizardPage* createCompletionPage();
    
    // 更新设备列表
    void updateDeviceList();
    
    // 检查是否可以进入下一步
    bool validatePage() override;
    
    // 标定数据
    QList<CalibrationData> m_calibrationData;
    CalibrationResult m_calibrationResult;
};

#endif // WIZARD_MODULE_H
