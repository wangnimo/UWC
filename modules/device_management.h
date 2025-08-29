// device_management.h
#pragma once
#include <QWidget>
#include <QTimer>
#include <QList>
#include <QDateTime>
#include <opencv2/opencv.hpp>
#include <QProgressDialog>
#include "cmvcamera.h"          // 新增
#include "ui_device_management.h"

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

//设备信息
struct DeviceInfo
{
    int         nIndex = -1;
    QString     name;
    QString     model;
    QString     serialNumber;
    QString     ipAddress;
    bool        isConnected = false;

    // SDK 相关
    CMvCamera   camera;                   // 直接持有 CMvCamera 实例
    MV_CC_DEVICE_INFO* pHikInfo = nullptr;
};

//设备配置
struct DeviceConfig
{
    QString name;
    QString deviceSerial;
    int     exposure = 0;
    int     gain     = 0;
    int     fps      = 0;
    QString resolution;
    QDateTime saveTime;
};

/* 标定数据结构体 */
struct CalibrationData {
    cv::Mat  image;         // 原始图像
    QImage   qImage;        // Qt 图像格式
    QString  filename;      //文件名
    QString  timestamp;     // 采集时间
};

class DeviceManagementModule : public QWidget
{
    Q_OBJECT
public:
    explicit DeviceManagementModule(QWidget* parent = nullptr);
    ~DeviceManagementModule();

    bool getConnectedDevice();
    // 创建进度对话框
    QProgressDialog* progressDialog = new QProgressDialog("Loading...", nullptr, 0, 0);

signals:
    //状态更改
    void statusChanged(const QString& msg);
    //设备连接
    void deviceConnected(DeviceInfo* device);
    //设备断连
    void deviceDisconnected();
    //收到新一帧图像
    void newFrameReceived(const QImage& img);

private slots:
    /* 以下所有槽函数保持原声明不变 */
    void onRefreshButtonClicked();          //刷新
    void onConnectButtonClicked();          //链接
    void onDisconnectButtonClicked();       //断开
    void onApplySettingsButtonClicked();    //应用设置参数
    void updateFrame();                     //更新帧
    void onUpdateFrame();
    void onStartPreviewClicked();           //预览
    void onStopPreviewClicked();            //停止预览
    void onCaptureImageClicked();           //捕获图像

private:
    bool connectHikVisionDevice(DeviceInfo* device);
    bool disconnectHikVisionDevice();
    bool updateDeviceParameters();
    bool startStream();
    bool stopStream();
    bool grabImage(cv::Mat& frame);              // 改为 OpenCV Mat
    void scanHikVisionDevices();
    void refreshDeviceListUI();
    void displayDeviceInfo(DeviceInfo* device);
    void autoCaptureImage();
    QImage cvMatToQImage(const cv::Mat& mat);
    void stopPreview();

    Ui::DeviceManagementModule* ui;
    QTimer* m_frameTimer;
    QList<DeviceInfo*>  m_deviceList;
    QList<DeviceConfig*> m_configList;
    DeviceInfo*          m_connectedDevice;
    bool m_isStreaming = false;
    QTimer* m_previewTimer;
    QTimer* m_autoCaptureTimer;
    bool m_isPreviewing;
    bool m_isAutoCapturing;
    QList<CalibrationData> m_calibrationData;
};
