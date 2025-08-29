#ifndef DATA_MANAGEMENT_MODULE_H
#define DATA_MANAGEMENT_H

#include <QWidget>
#include <QList>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include "calibration.h"

namespace Ui {
class DataManagementModule;
}

// 标定记录结构体
struct CalibrationRecord {
    int id;                     // 记录ID
    QString name;               // 记录名称
    QString deviceSerial;       // 设备序列号
    QString calibrationDate;    // 标定日期
    double reprojectionError;   // 重投影误差
    QString parametersPath;     // 参数文件路径
    QString notes;              // 备注
    QString deviceModel;        // 设备型号
    int imageCount;             // 标定图像数量
};

class DataManagementModule : public QWidget
{
    Q_OBJECT

public:
    explicit DataManagementModule(QWidget *parent = nullptr);
    ~DataManagementModule() override;
    
    // 添加新的标定记录
    bool addCalibrationRecord(const CalibrationResult& result, const QString& notes);

signals:
    // 状态变化信号
    void statusChanged(const QString& message);
    
    // 加载标定结果信号
    void loadCalibrationResult(const CalibrationResult& result);

private slots:
    // UI交互槽函数
    void onLoadRecordClicked();
    void onDeleteRecordClicked();
    void onExportRecordClicked();
    void onSearchRecordsClicked();
    void onFilterRecordsClicked();
    void onRecordDoubleClicked(const QModelIndex& index);
    void onSyncWithDatabaseClicked();
    void onBackupDatabaseClicked();
    void onRestoreDatabaseClicked();

private:
    Ui::DataManagementModule *ui;
    
    // 数据库连接
    QSqlDatabase m_db;
    
    // 数据模型
    QSqlTableModel* m_recordsModel;
    
    // 初始化UI
    void initUI();
    
    // 初始化数据库
    bool initDatabase();
    
    // 加载所有记录
    void loadAllRecords();
    
    // 搜索记录
    void searchRecords(const QString& keyword);
    
    // 过滤记录
    void filterRecords(double maxError);
    
    // 从文件加载标定结果
    CalibrationResult loadCalibrationFromFile(const QString& filePath);
    
    // 导出记录
    bool exportRecords(const QString& filePath);
    
    // 备份数据库
    bool backupDatabase(const QString& filePath);
    
    // 恢复数据库
    bool restoreDatabase(const QString& filePath);
    void initConnections();
};

#endif // DATA_MANAGEMENT_MODULE_H
    
