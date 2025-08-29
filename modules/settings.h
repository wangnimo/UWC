#ifndef SETTINGS_MODULE_H
#define SETTINGS_MODULE_H

#include <QWidget>
#include <QSettings>

namespace Ui {
class SettingsModule;
}

// 应用设置结构体
struct AppSettings {
    // 界面设置
    QString language;          // 语言
    bool darkMode;             // 深色模式
    bool showTooltips;         // 显示工具提示
    
    // 路径设置
    QString defaultSavePath;   // 默认保存路径
    QString templatePath;      // 模板路径
    QString databasePath;      // 数据库路径
    
    // 相机设置
    int defaultExposure;       // 默认曝光时间(微秒)
    int defaultGain;           // 默认增益
    bool autoWhiteBalance;     // 自动白平衡
    
    // 标定设置
    int defaultBoardWidth;     // 默认棋盘格宽度
    int defaultBoardHeight;    // 默认棋盘格高度
    double defaultSquareSize;  // 默认棋盘格大小(mm)
};

class SettingsModule : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsModule(QWidget *parent = nullptr);
    ~SettingsModule() override;
    
    // 加载设置
    void loadSettings();
    
    // 保存设置
    void saveSettings();
    
    // 获取当前设置
    AppSettings getCurrentSettings() const { return m_settings; }

signals:
    // 状态变化信号
    void statusChanged(const QString& message);
    
    // 设置已更改信号
    void settingsChanged(const AppSettings& settings);

private slots:
    // UI交互槽函数
    void onBrowseDefaultPathClicked();
    void onBrowseTemplatePathClicked();
    void onBrowseDatabasePathClicked();
    void onRestoreDefaultsClicked();
    void onSaveSettingsClicked();
    void onCancelClicked();

    void on_settingPathButton_clicked();

private:
    Ui::SettingsModule *ui;
    
    // 应用设置
    AppSettings m_settings;
    QSettings* m_qsettings;
    
    // 初始化UI
    void initUI();
    
    // 应用设置到UI
    void applySettingsToUI();
    
    // 从UI更新设置
    void updateSettingsFromUI();
    
    // 设置默认值
    void setDefaultSettings();
};

#endif // SETTINGS_MODULE_H
