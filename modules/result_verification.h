#ifndef RESULT_VERIFICATION_MODULE_H
#define RESULT_VERIFICATION_MODULE_H

#include <QWidget>
#include <opencv2/opencv.hpp>
#include "calibration.h"
#include <QtCharts>

namespace Ui {
class ResultVerificationModule;
}


class ResultVerificationModule : public QWidget
{
    Q_OBJECT

public:
    explicit ResultVerificationModule(QWidget *parent = nullptr);
    ~ResultVerificationModule() override;

public slots:
    // 标定完成回调
    void onCalibrationCompleted(const CalibrationResult& result);
    // void onLoadVerificationDataClicked();

signals:
    // 状态变化信号
    void statusChanged(const QString& message);

private slots:
    // UI交互槽函数
    void onVisualizeErrorsClicked();
    // void onShowReprojectionClicked();
    // void onAdjustCornersClicked();
    void onSaveVisualizationClicked();
    // void onExportErrorsClicked();
    // void onImageSelectionChanged(int index);
    void onImageIndexChanged(int index);

private:
    Ui::ResultVerificationModule *ui;

    // 当前标定结果
    CalibrationResult m_currentResult;

    // 标定数据
    QList<CalibrationData> m_verificationData;

    // 图表对象
    QChartView* m_errorChartView;
    QChart* m_errorChart;

    // 初始化UI
    void initUI();

    void initConnections();

    // 初始化图表
    void initCharts();

    // 显示标定结果摘要
    void displayResultSummary(const CalibrationResult& result);

    // 可视化重投影误差
    void visualizeReprojectionErrors();

    // // 显示重投影结果
    // void showReprojectionResults();

    // // 生成误差热图
    // QImage generateErrorHeatmap(const cv::Mat& heatmapData);

    // // 生成误差柱状图
    // void generateErrorBarChart(const std::vector<double>& errors);

    // // 生成极线偏差曲线
    // void generateEpipolarCurve(const std::vector<double>& errors);


    QString generateErrorStatsHtml();
};

#endif // RESULT_VERIFICATION_MODULE_H

