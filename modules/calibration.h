#ifndef CALIBRATION_MODULE_H
#define CALIBRATION_MODULE_H

#include <QWidget>
#include <QThread>
#include <QMutex>
#include <opencv2/opencv.hpp>
#include <vector>
#include "data_acquisition.h"
#include "settings.h"

namespace Ui {
class CalibrationModule;
}

// 标定参数结构体
struct CalibrationParameters {
    // 内参矩阵
    cv::Mat cameraMatrix;
    // 畸变系数
    cv::Mat distCoeffs;
    // 旋转向量
    std::vector<cv::Mat> rvecs;
    // 平移向量
    std::vector<cv::Mat> tvecs;
    // 重投影误差
    double reprojectionError;
    // 标定板尺寸
    cv::Size boardSize;
    // 棋盘格方块大小(mm)
    float squareSize;
    // 标定时间
    QString timestamp;
    // 设备信息
    QString deviceInfo;
};

// 标定结果结构体
struct CalibrationResult {
    CalibrationParameters params;
    std::vector<double> perViewErrors;
    cv::Mat errorHeatmap;
    bool success = false;
    QString message;
};

// 标定工作线程
class CalibrationWorker : public QObject
{
    Q_OBJECT
    
public:
    CalibrationWorker(const QList<CalibrationData>& data,
                     const cv::Size& boardSize, 
                     float squareSize,
                     bool useUndistortion = true);

public slots:
    void doWork();
    void abort();
    
signals:
    void progressUpdated(int progress);
    void workFinished(CalibrationResult result);
    void errorOccurred(QString error);
    
private:
    QList<CalibrationData> m_data;
    cv::Size m_boardSize;
    float m_squareSize;
    int m_distortionModel;
    bool m_useUndistortion;
    QMutex m_mutex;
    bool m_abort;
    
    // 执行标定
    CalibrationResult performCalibration();
    
    // 提取角点
    bool findChessboardCorners(const cv::Mat& image,
                             cv::Size boardSize,
                             std::vector<cv::Point2f>& corners);
};

class CalibrationModule : public QWidget
{
    Q_OBJECT

public:
    explicit CalibrationModule(QWidget *parent = nullptr);
    ~CalibrationModule() override;
    
    // 获取当前标定结果
    CalibrationResult getCurrentResult() const { return m_currentResult; }

public slots:
    // 数据就绪回调
    void onDataReady(const QList<CalibrationData>& data);
    // 取消标定
    void onCancelCalibration();
    //更新标定设置
    void updateCalibSetting(const AppSettings& settings);

signals:
    // 状态变化信号
    void statusChanged(const QString& message);
    
    // 标定完成信号
    void calibrationComplete(const CalibrationResult& result);

private slots:
    // UI交互槽函数
    void onStartCalibrationClicked();
    void onLoadDataClicked();
    void onSaveParametersClicked();
    void onCalibrationProgress(int progress);
    void onCalibrationFinished(CalibrationResult result);
    void onCalibrationError(QString error);
    void onCalibrationTypeChanged(int index);

private:
    Ui::CalibrationModule *ui;
    
    // 标定数据和结果
    QList<CalibrationData> m_calibrationData;
    CalibrationResult m_currentResult;
    
    // 标定线程
    QThread* m_workerThread;
    CalibrationWorker* m_worker;
    
    // 初始化UI
    void initUI();

    void initConnections();
    
    // 显示标定参数
    void displayCalibrationParameters(const CalibrationParameters& params);

    // 填充表格数据
    void fillTables(const CalibrationParameters& params);
    
    // 保存标定参数
    bool saveCalibrationParameters(const CalibrationParameters& params, const QString& filePath);
};

#endif // CALIBRATION_MODULE_H
    
