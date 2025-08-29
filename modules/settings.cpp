#include "settings.h"
#include "ui_settings.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>
#include <QStyle>

SettingsModule::SettingsModule(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsModule),
    m_qsettings(nullptr)
{
    ui->setupUi(this);
    initUI();
    
    // 初始化设置存储
    QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    ui->settingPathEdit->setText(settingsPath);
    QDir().mkpath(settingsPath);
    m_qsettings = new QSettings(settingsPath + "/UnderwaterCalibrator.ini", QSettings::IniFormat, this);
    
    loadSettings();
    applySettingsToUI();

    // connect(ui->intervalSpinBox,     QOverload<int>::of(&QSpinBox::valueChanged),
    //         this, &DataAcquisitionModule::onAutoCaptureIntervalChanged);
}

SettingsModule::~SettingsModule()
{
    delete m_qsettings;
    delete ui;
}

void SettingsModule::initUI()
{
    // 设置语言选择
    ui->languageCombo->addItem(tr("简体中文"), "zh_CN");
    ui->languageCombo->addItem(tr("English"), "en");
    
    // 设置样式表
    setStyleSheet("QGroupBox { font-weight: bold; margin-top: 10px; }"
                  "QGroupBox::title { subcontrol-origin: margin; left: 10px; }");
    
    // 设置路径编辑框只读
    ui->defaultPathEdit->setReadOnly(true);
    ui->templatePathEdit->setReadOnly(true);
    ui->settingPathEdit->setReadOnly(true);
    ui->databasePathEdit->setReadOnly(true);
}

void SettingsModule::loadSettings()
{
    // 加载界面设置
    m_settings.language = m_qsettings->value("Interface/Language", "zh_CN").toString();
    m_settings.darkMode = m_qsettings->value("Interface/DarkMode", false).toBool();
    m_settings.showTooltips = m_qsettings->value("Interface/ShowTooltips", true).toBool();
    
    // 加载路径设置
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/UnderwaterCalibrator";
    QDir().mkpath(defaultPath);
    
    m_settings.defaultSavePath = m_qsettings->value("Paths/DefaultSavePath", defaultPath).toString();
    m_settings.templatePath = m_qsettings->value("Paths/TemplatePath", defaultPath + "/templates").toString();
    m_settings.databasePath = m_qsettings->value("Paths/DatabasePath", defaultPath + "/database").toString();
    
    // 确保路径存在
    QDir().mkpath(m_settings.templatePath);
    QDir().mkpath(m_settings.databasePath);
    
    // 加载相机设置
    m_settings.defaultExposure = m_qsettings->value("Camera/DefaultExposure", 10000).toInt();
    m_settings.defaultGain = m_qsettings->value("Camera/DefaultGain", 0).toInt();
    m_settings.autoWhiteBalance = m_qsettings->value("Camera/AutoWhiteBalance", true).toBool();
    
    // 加载标定设置
    m_settings.defaultBoardWidth = m_qsettings->value("Calibration/DefaultBoardWidth", 9).toInt();
    m_settings.defaultBoardHeight = m_qsettings->value("Calibration/DefaultBoardHeight", 6).toInt();
    m_settings.defaultSquareSize = m_qsettings->value("Calibration/DefaultSquareSize", 25.0).toDouble();


}

void SettingsModule::saveSettings()
{
    updateSettingsFromUI();
    
    // 保存界面设置
    m_qsettings->setValue("Interface/Language", m_settings.language);
    m_qsettings->setValue("Interface/DarkMode", m_settings.darkMode);
    m_qsettings->setValue("Interface/ShowTooltips", m_settings.showTooltips);
    
    // 保存路径设置
    m_qsettings->setValue("Paths/DefaultSavePath", m_settings.defaultSavePath);
    m_qsettings->setValue("Paths/TemplatePath", m_settings.templatePath);
    m_qsettings->setValue("Paths/DatabasePath", m_settings.databasePath);
    
    // 保存相机设置
    m_qsettings->setValue("Camera/DefaultExposure", m_settings.defaultExposure);
    m_qsettings->setValue("Camera/DefaultGain", m_settings.defaultGain);
    m_qsettings->setValue("Camera/AutoWhiteBalance", m_settings.autoWhiteBalance);
    
    // 保存标定设置
    m_qsettings->setValue("Calibration/DefaultBoardWidth", m_settings.defaultBoardWidth);
    m_qsettings->setValue("Calibration/DefaultBoardHeight", m_settings.defaultBoardHeight);
    m_qsettings->setValue("Calibration/DefaultSquareSize", m_settings.defaultSquareSize);
    
    m_qsettings->sync();
    
    emit statusChanged(tr("设置已保存"));
    emit settingsChanged(m_settings);
}

void SettingsModule::applySettingsToUI()
{
    // 应用界面设置
    int langIndex = ui->languageCombo->findData(m_settings.language);
    if (langIndex >= 0) {
        ui->languageCombo->setCurrentIndex(langIndex);
    }
    ui->darkModeCheck->setChecked(m_settings.darkMode);
    ui->showTooltipsCheck->setChecked(m_settings.showTooltips);
    
    // 应用路径设置
    ui->defaultPathEdit->setText(m_settings.defaultSavePath);
    ui->templatePathEdit->setText(m_settings.templatePath);
    ui->databasePathEdit->setText(m_settings.databasePath);
    
    // 应用相机设置
    ui->exposureSpin->setValue(m_settings.defaultExposure);
    ui->gainSpin->setValue(m_settings.defaultGain);
    ui->whiteBalanceCheck->setChecked(m_settings.autoWhiteBalance);
    
    // 应用标定设置
    ui->boardWidthSpin->setValue(m_settings.defaultBoardWidth);
    ui->boardHeightSpin->setValue(m_settings.defaultBoardHeight);
    ui->squareSizeSpin->setValue(m_settings.defaultSquareSize);

    emit settingsChanged(m_settings);
}

void SettingsModule::updateSettingsFromUI()
{
    // 从UI更新界面设置
    m_settings.language = ui->languageCombo->currentData().toString();
    m_settings.darkMode = ui->darkModeCheck->isChecked();
    m_settings.showTooltips = ui->showTooltipsCheck->isChecked();
    
    // 从UI更新路径设置
    m_settings.defaultSavePath = ui->defaultPathEdit->text();
    m_settings.templatePath = ui->templatePathEdit->text();
    m_settings.databasePath = ui->databasePathEdit->text();
    
    // 从UI更新相机设置
    m_settings.defaultExposure = ui->exposureSpin->value();
    m_settings.defaultGain = ui->gainSpin->value();
    m_settings.autoWhiteBalance = ui->whiteBalanceCheck->isChecked();
    
    // 从UI更新标定设置
    m_settings.defaultBoardWidth = ui->boardWidthSpin->value();
    m_settings.defaultBoardHeight = ui->boardHeightSpin->value();
    m_settings.defaultSquareSize = ui->squareSizeSpin->value();
}

void SettingsModule::setDefaultSettings()
{
    // 设置默认值
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/UnderwaterCalibrator";
    
    m_settings.language = "zh_CN";
    m_settings.darkMode = false;
    m_settings.showTooltips = true;
    
    m_settings.defaultSavePath = defaultPath;
    m_settings.templatePath = defaultPath + "/templates";
    m_settings.databasePath = defaultPath + "/database";
    
    m_settings.defaultExposure = 10000;
    m_settings.defaultGain = 0;
    m_settings.autoWhiteBalance = true;
    
    m_settings.defaultBoardWidth = 9;
    m_settings.defaultBoardHeight = 6;
    m_settings.defaultSquareSize = 25.0;
    
    // 应用到UI
    applySettingsToUI();
}

void SettingsModule::onBrowseDefaultPathClicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("选择默认保存路径"), 
                                                     m_settings.defaultSavePath);
    if (!path.isEmpty()) {
        ui->defaultPathEdit->setText(path);
    }
}

void SettingsModule::onBrowseTemplatePathClicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("选择模板路径"), 
                                                     m_settings.templatePath);
    if (!path.isEmpty()) {
        ui->templatePathEdit->setText(path);
    }
}

void SettingsModule::on_settingPathButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("选择设置路径"),
                                                     ui->settingPathEdit->text());
    if (!path.isEmpty()) {
        ui->settingPathEdit->setText(path);
    }
}

void SettingsModule::onBrowseDatabasePathClicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("选择数据库路径"), 
                                                     m_settings.databasePath);
    if (!path.isEmpty()) {
        ui->databasePathEdit->setText(path);
    }
}

void SettingsModule::onRestoreDefaultsClicked()
{
    if (QMessageBox::question(this, tr("恢复默认设置"), 
                             tr("确定要恢复默认设置吗？这将覆盖当前所有设置。")) == QMessageBox::Yes) {
        setDefaultSettings();
        emit statusChanged(tr("已恢复默认设置"));
    }
}

void SettingsModule::onSaveSettingsClicked()
{
    saveSettings();
}

void SettingsModule::onCancelClicked()
{
    // 重新加载设置，放弃当前修改
    loadSettings();
    applySettingsToUI();
    emit statusChanged(tr("已取消设置修改"));
}

