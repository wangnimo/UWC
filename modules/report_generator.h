#ifndef REPORT_GENERATOR_MODULE_H
#define REPORT_GENERATOR_MODULE_H

#include <QWidget>
#include "calibration.h"
#include <QTextDocument>
#include <QPrinter>

namespace Ui {
class ReportGeneratorModule;
}

class ReportGeneratorModule : public QWidget
{
    Q_OBJECT

public:
    explicit ReportGeneratorModule(QWidget *parent = nullptr);
    ~ReportGeneratorModule() override;
    
    // 设置标定结果
    void setCalibrationResult(const CalibrationResult& result);
    
    // 设置原始标定数据
    void setCalibrationData(const QList<CalibrationData>& data);

signals:
    // 状态变化信号
    void statusChanged(const QString& message);

private slots:
    // UI交互槽函数
    void onGenerateReportClicked();
    void onPreviewReportClicked();
    void onExportPdfClicked();
    // void onExportWordClicked();
    void onExportCsvClicked();
    void onExportJsonClicked();
    void onSaveTemplateClicked();
    void onLoadTemplateClicked();

private:
    Ui::ReportGeneratorModule *ui;
    void initConnections();
    
    // 当前标定结果
    CalibrationResult m_currentResult;
    
    // 原始标定数据
    QList<CalibrationData> m_calibrationData;
    
    // 报告文档
    QTextDocument m_reportDocument;
    
    // 初始化UI
    void initUI();
    
    // 生成报告内容
    QString generateReportContent();
    
    // 生成HTML格式报告
    QString generateHtmlReport();
    
    // 生成PDF报告
    bool exportToPdf(const QString& filePath);
    
    // 生成Word报告
    bool exportToWord(const QString& filePath);
    
    // 生成CSV报告
    bool exportToCsv(const QString& filePath);
    
    // 生成JSON报告
    bool exportToJson(const QString& filePath);
    
    // 加载报告模板
    bool loadReportTemplate(const QString& filePath);
    
    // 保存报告模板
    bool saveReportTemplate(const QString& filePath);
    
    // 格式化矩阵为HTML表格
    QString formatMatrixAsHtml(const cv::Mat& matrix);
};

#endif // REPORT_GENERATOR_MODULE_H
    
