// device_management.cpp
#include "device_management.h"
#include "../Drawer.h"
#include <QMessageBox>
#include <QSettings>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <opencv2/opencv.hpp>
#include <QImage>

DeviceManagementModule::DeviceManagementModule(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::DeviceManagementModule)
    , m_frameTimer(new QTimer(this))
    , m_connectedDevice(nullptr)
    , m_previewTimer(new QTimer(this))
    , m_autoCaptureTimer(new QTimer(this))
    , m_isPreviewing(false)
{
    ui->setupUi(this);
    ui->cam2->hide();
    ui->captureImageButton->setEnabled(false);
    m_previewTimer->setInterval(33);          // ~30 fps
    m_autoCaptureTimer->setInterval(5000);    // 默认 5 s

    // 按钮绑定
    connect(ui->refreshButton, &QPushButton::clicked, this, &DeviceManagementModule::onRefreshButtonClicked);
    connect(ui->connectButton, &QPushButton::clicked, this, &DeviceManagementModule::onConnectButtonClicked);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &DeviceManagementModule::onDisconnectButtonClicked);
    connect(ui->applySettingsButton, &QPushButton::clicked, this, &DeviceManagementModule::onApplySettingsButtonClicked);
    // connect(ui->deviceListWidget, &QListWidget::itemClicked, this, &DeviceManagementModule::onDeviceSelected);
    connect(m_frameTimer, &QTimer::timeout, this, &DeviceManagementModule::updateFrame);

    connect(ui->toggleBtn, &QPushButton::clicked, ui->drawer_widget, &Drawer::toggle);
    connect(ui->startPreviewButton,  &QPushButton::clicked, this, &DeviceManagementModule::onStartPreviewClicked);
    connect(ui->stopPreviewButton,   &QPushButton::clicked, this, &DeviceManagementModule::onStopPreviewClicked);
    connect(ui->captureImageButton,  &QPushButton::clicked, this, &DeviceManagementModule::onCaptureImageClicked);
    // 计时器绑定
    connect(m_previewTimer,    &QTimer::timeout, this, &DeviceManagementModule::onUpdateFrame);
    connect(m_autoCaptureTimer,&QTimer::timeout, this, &DeviceManagementModule::autoCaptureImage);
    scanHikVisionDevices();
}

DeviceManagementModule::~DeviceManagementModule()
{
    stopStream();
    qDeleteAll(m_deviceList);
    qDeleteAll(m_configList);
    delete progressDialog;
    delete ui;
}

//  ---------------- 扫描/枚举 ----------------
void DeviceManagementModule::scanHikVisionDevices()
{
    qDeleteAll(m_deviceList);
    m_deviceList.clear();
    ui->usb_listWidget->clear();
    ui->gige_listWidget->clear();
    MV_CC_DEVICE_INFO_LIST stDeviceList{};
    int ret = CMvCamera::EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
    if (ret != MV_OK) {
        emit statusChanged(tr("设备枚举失败: %1").arg(ret));
        return;
    }

    for (unsigned int i = 0; i < stDeviceList.nDeviceNum; ++i) {
        DeviceInfo* dev = new DeviceInfo;
        dev->nIndex  = i;
        dev->pHikInfo = stDeviceList.pDeviceInfo[i];

        if (dev->pHikInfo->nTLayerType == MV_GIGE_DEVICE) {
            MV_GIGE_DEVICE_INFO* gige = &dev->pHikInfo->SpecialInfo.stGigEInfo;
            unsigned int ip = gige->nCurrentIp;
            dev->ipAddress  = QString("%1.%2.%3.%4")
                                 .arg((ip >> 24) & 0xFF).arg((ip >> 16) & 0xFF)
                                 .arg((ip >> 8) & 0xFF).arg(ip & 0xFF);
            dev->model      = QString::fromLocal8Bit((const char*)gige->chModelName);
            dev->serialNumber = QString::fromLocal8Bit((const char*)gige->chSerialNumber);
            dev->name       = tr("GigE相机: %1").arg(dev->model);
            ui->gige_listWidget->addItem(dev->name);
        } else if (dev->pHikInfo->nTLayerType == MV_USB_DEVICE) {
            MV_USB3_DEVICE_INFO* usb = &dev->pHikInfo->SpecialInfo.stUsb3VInfo;
            dev->model      = QString::fromLocal8Bit((const char*)usb->chModelName);
            dev->serialNumber = QString::fromLocal8Bit((const char*)usb->chSerialNumber);
            dev->name       = tr("USB相机: %1").arg(dev->model);
            dev->ipAddress  = tr("USB连接");
            ui->usb_listWidget->addItem(dev->name);
        }
        m_deviceList.append(dev);
    }
    progressDialog->close();
    emit statusChanged(tr("发现%1个设备").arg(m_deviceList.size()));

}

//  ---------------- 连接/断开 ----------------
bool DeviceManagementModule::connectHikVisionDevice(DeviceInfo* device)
{
    if (!device || !device->pHikInfo) return false;

    int ret = device->camera.Open(device->pHikInfo);
    if (ret != MV_OK) {
        emit statusChanged(tr("打开设备失败: %1").arg(ret));
        return false;
    }

    // 千兆网相机设置包长
    if (device->pHikInfo->nTLayerType == MV_GIGE_DEVICE) {
        device->camera.SetIntValue("GevSCPSPacketSize", 1500);
    }
    return true;
}


// ---------------- 断开hik工业摄像头链接 ----------------
bool DeviceManagementModule::disconnectHikVisionDevice()
{
    if (!m_connectedDevice) return true;

    stopStream();
    m_connectedDevice->camera.Close();

    m_connectedDevice->isConnected = false;
    m_connectedDevice = nullptr;

    ui->disconnectButton->setEnabled(false);
    ui->connectButton->setEnabled(true);
    emit deviceDisconnected();
    emit statusChanged(tr("设备已断开连接"));
    return true;
}

//  ---------------- 相机参数读取/设置 ----------------
bool DeviceManagementModule::updateDeviceParameters()
{
    if (!m_connectedDevice) return false;
    CMvCamera& cam = m_connectedDevice->camera;

    int ret = cam.SetIntValue("ExposureTime", ui->exposureSpinBox->value());
    if (ret != MV_OK) { emit statusChanged(tr("设置曝光失败:%1").arg(ret)); return false; }

    ret = cam.SetIntValue("Gain", ui->gainSpinBox->value());
    if (ret != MV_OK) { emit statusChanged(tr("设置增益失败:%1").arg(ret)); return false; }

    ret = cam.SetFloatValue("AcquisitionFrameRate", ui->fpsSpinBox->value());
    if (ret != MV_OK) { emit statusChanged(tr("设置帧率失败:%1").arg(ret)); return false; }

    emit statusChanged(tr("设备参数已更新"));
    return true;
}

void DeviceManagementModule::displayDeviceInfo(DeviceInfo* device)
{
    if (!device) return;
    // UI 展示同上，略

    if (device->isConnected) {
        CMvCamera& cam = device->camera;
        MVCC_INTVALUE_EX vInt = {};
        MVCC_FLOATVALUE vFlt = {};

        if (cam.GetIntValue("ExposureTime", &vInt) == MV_OK)
            ui->exposureSpinBox->setValue(vInt.nCurValue);

        if (cam.GetIntValue("Gain", &vInt) == MV_OK)
            ui->gainSpinBox->setValue(vInt.nCurValue);

        if (cam.GetFloatValue("AcquisitionFrameRate", &vFlt) == MV_OK)
            ui->fpsSpinBox->setValue((int)vFlt.fCurValue);

        ui->id_value->setText(QString::number(device->nIndex));
        ui->name_value->setText(device->name);
        ui->sn_value->setText(device->serialNumber);
        ui->model_value->setText(device->model);
        ui->ip_value->setText(device->ipAddress);
    }
}

//  ---------------- 视频流 ----------------
bool DeviceManagementModule::startStream()
{
    if (!m_connectedDevice || m_isStreaming) return false;
    int ret = m_connectedDevice->camera.StartGrabbing();
    if (ret != MV_OK) {
        emit statusChanged(tr("启动流失败:%1").arg(ret));
        return false;
    }
    m_isStreaming = true;
    m_frameTimer->start(30);
    emit statusChanged(tr("视频流已启动"));
    return true;
}

bool DeviceManagementModule::stopStream()
{
    if (!m_connectedDevice || !m_isStreaming) return true;
    m_frameTimer->stop();
    m_connectedDevice->camera.StopGrabbing();
    m_isStreaming = false;
    emit statusChanged(tr("视频流已停止"));
    return true;
}

// 更新视频帧
void DeviceManagementModule::updateFrame()
{
    if (!m_connectedDevice || !m_isStreaming) return;

    cv::Mat frame;
    if (grabImage(frame) && !frame.empty()) {
        // Mat -> QImage
        QImage qimg(frame.data, frame.cols, frame.rows,
                    frame.step, QImage::Format_Grayscale8);
        emit newFrameReceived(qimg.copy());
    }
}

bool DeviceManagementModule::grabImage(cv::Mat& frame)
{
    if (!m_connectedDevice || !m_isStreaming) return false;

    CMvCamera& cam = m_connectedDevice->camera;
    MV_FRAME_OUT frameOut{};
    int ret = cam.GetImageBuffer(&frameOut, 1000);
    if (ret != MV_OK) return false;

    // 构造 cv::Mat（浅拷贝）
    cv::Mat raw(frameOut.stFrameInfo.nHeight,
                frameOut.stFrameInfo.nWidth,
                CV_8UC1, frameOut.pBufAddr);

    raw.copyTo(frame);              // 深拷贝到外部
    cam.FreeImageBuffer(&frameOut); // 归还 SDK 缓存
    return true;
}


/*-------------------------------- 工具函数 --------------------------------*/

QImage DeviceManagementModule::cvMatToQImage(const cv::Mat& mat)
{
    if (mat.type() == CV_8UC1)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    if (mat.type() == CV_8UC3) {
        cv::Mat rgb; cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    }
    return QImage();
}
// =============槽函数==========================
void DeviceManagementModule::onRefreshButtonClicked()
{
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setCancelButton(nullptr);
    progressDialog->show();
    scanHikVisionDevices();
}

void DeviceManagementModule::onConnectButtonClicked()
{
    if (!m_connectedDevice) {
        QMessageBox::warning(this, "警告", "请链接相机后重试！");
        return;
    }
    if (connectHikVisionDevice(m_connectedDevice)) {
        m_connectedDevice->isConnected = true;
        ui->connectButton->setEnabled(false);
        ui->disconnectButton->setEnabled(true);
        displayDeviceInfo(m_connectedDevice);
        emit deviceConnected(m_connectedDevice);
    }
}

void DeviceManagementModule::onDisconnectButtonClicked()
{
    disconnectHikVisionDevice();
}

void DeviceManagementModule::onApplySettingsButtonClicked()
{
    updateDeviceParameters();
}


bool DeviceManagementModule::getConnectedDevice()
{
    if (!m_connectedDevice) return false;
    return true;
}

void DeviceManagementModule::onStartPreviewClicked()
{

    if (!getConnectedDevice()) {
        QMessageBox::warning(this, tr("警告"), tr("请先在设备管理模块连接相机"));
        return;
    }
    m_isPreviewing = true;
    m_previewTimer->start();
    ui->startPreviewButton->setEnabled(false);
    ui->stopPreviewButton->setEnabled(true);
    ui->captureImageButton->setEnabled(true);
    emit statusChanged(tr("预览已启动"));
}

void DeviceManagementModule::autoCaptureImage()
{
    onCaptureImageClicked();
}

void DeviceManagementModule::onCaptureImageClicked()
{
    cv::Mat frame;
    if (!grabImage(frame) || frame.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("无法获取图像帧"));
        return;
    }
    CalibrationData data;
    data.image      = frame.clone();
    data.qImage     = cvMatToQImage(frame);
    data.timestamp  = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");

    m_calibrationData.append(data);
    // updateDataList();
    // ui->deleteImageButton->setEnabled(true);
    // ui->clearAllButton->setEnabled(true);
    // ui->saveImagesButton->setEnabled(true);
    emit statusChanged(tr("已采集图像 %1").arg(m_calibrationData.size()));
}


void DeviceManagementModule::onStopPreviewClicked() { stopPreview(); }

void DeviceManagementModule::stopPreview()
{
    if (!m_isPreviewing) return;
    m_isPreviewing = false;
    m_previewTimer->stop();
    if (m_isAutoCapturing) {
        m_isAutoCapturing = false;
        m_autoCaptureTimer->stop();

    }
    ui->cam1->setText(tr("预览已停止"));
    ui->startPreviewButton->setEnabled(true);
    ui->stopPreviewButton->setEnabled(false);
    ui->captureImageButton->setEnabled(false);
    emit statusChanged(tr("预览已停止"));
}


void DeviceManagementModule::onUpdateFrame()
{
    cv::Mat frame;
    if (!grabImage(frame) || frame.empty()) return;
    QImage img = cvMatToQImage(frame);
    ui->cam1->setPixmap(QPixmap::fromImage(img)
                                    .scaled(ui->cam1->size(),
                                            Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation));
}
