#ifndef DATA_ACQUISITION_MODULE_H
#define DATA_ACQUISITION_MODULE_H

#include <QWidget>
#include <QList>
#include <opencv2/opencv.hpp>
#include <QImage>
#include "device_management.h"

namespace Ui { class DataAcquisitionModule; }

/* 前向声明 */
class MainWindow;


class DataAcquisitionModule : public QWidget
{
    Q_OBJECT

public:
    explicit DataAcquisitionModule(MainWindow* mainWindow, QWidget *parent = nullptr);
    ~DataAcquisitionModule() override;

    QList<CalibrationData> getCalibrationData() const { return m_calibrationData; }

signals:
    void statusChanged(const QString& message);
    void dataReady(const QList<CalibrationData>& data);

private slots:
    /* UI 交互槽函数 */
    void onDeleteImageClicked();
    void onClearAllClicked();
    void onLoadImagesClicked();
    void onSaveImagesClicked();

    // void onAcquisitionModeChanged();
    // void onAdjustParametersClicked();

private:
    void initUI();
    void initConnections();
    void updateDataList();
    QImage  cvMatToQImage(const cv::Mat& mat);
    cv::Mat qImageToCvMat(const QImage& image);
    bool saveCalibrationData(const QString& dir);
    bool loadCalibrationData(const QString& dir);

    Ui::DataAcquisitionModule* ui;
    MainWindow*  m_mainWindow;
    QList<CalibrationData> m_calibrationData;

};

#endif // DATA_ACQUISITION_MODULE_H
