#include "data_acquisition.h"
#include "ui_data_acquisition.h"
#include "../mainwindow.h"
#include "device_management.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <opencv2/opencv.hpp>
#include <QBuffer>
#include <QImageWriter>

DataAcquisitionModule::DataAcquisitionModule(MainWindow* mainWindow, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataAcquisitionModule)
    , m_mainWindow(mainWindow)
{
    ui->setupUi(this);
    initUI();
    initConnections();
}

DataAcquisitionModule::~DataAcquisitionModule()
{
    delete ui;
}

/*-------------------------------- 初始化 --------------------------------*/
void DataAcquisitionModule::initUI()
{
    ui->previewLabel->setAlignment(Qt::AlignCenter);
    ui->previewLabel->setMinimumSize(640, 480);
    ui->previewLabel->setStyleSheet("background-color:#222;color:white;");
    ui->previewLabel->setText(tr("图像预览"));

    ui->dataListWidget->setAlternatingRowColors(true);
    ui->dataListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);


    ui->deleteImageButton->setEnabled(false);
    ui->clearAllButton->setEnabled(false);
    ui->saveImagesButton->setEnabled(false);
}

void DataAcquisitionModule::initConnections()
{
    connect(ui->deleteImageButton,   &QPushButton::clicked, this, &DataAcquisitionModule::onDeleteImageClicked);
    connect(ui->clearAllButton,      &QPushButton::clicked, this, &DataAcquisitionModule::onClearAllClicked);
    connect(ui->loadImagesButton,    &QPushButton::clicked, this, &DataAcquisitionModule::onLoadImagesClicked);
    connect(ui->saveImagesButton,    &QPushButton::clicked, this, &DataAcquisitionModule::onSaveImagesClicked);
    // 连接 itemClicked 信号到槽函数
    QObject::connect(ui->dataListWidget, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
        int index = item->data(Qt::UserRole).toInt();
        const auto& d = m_calibrationData[index];
        ui->previewLabel->setPixmap(QPixmap::fromImage(d.qImage));
    });
}

/*-------------------------------- 槽函数 --------------------------------*/
// 删除列表选中
void DataAcquisitionModule::onDeleteImageClicked()
{
    QList<QListWidgetItem*> sel = ui->dataListWidget->selectedItems();
    if (sel.isEmpty()) { QMessageBox::warning(this, tr("警告"), tr("请先选择要删除的图像")); return; }
    if (QMessageBox::question(this, tr("确认"), tr("确定删除选中的 %1 张图像？").arg(sel.size()))
        != QMessageBox::Yes) return;

    QList<int> idxs;
    for (auto* it : sel) idxs.append(it->data(Qt::UserRole).toInt());
    std::sort(idxs.begin(), idxs.end(), std::greater<int>());
    for (int i : idxs) m_calibrationData.removeAt(i);
    updateDataList();
    if (m_calibrationData.isEmpty()) {
        ui->deleteImageButton->setEnabled(false);
        ui->clearAllButton->setEnabled(false);
        ui->saveImagesButton->setEnabled(false);
    }
    emit statusChanged(tr("已删除 %1 张图像").arg(sel.size()));
}
// 清空列表
void DataAcquisitionModule::onClearAllClicked()
{
    if (m_calibrationData.isEmpty()) return;
    if (QMessageBox::question(this, tr("确认"), tr("确定清除所有 %1 张图像？").arg(m_calibrationData.size()))
        != QMessageBox::Yes) return;
    m_calibrationData.clear();
    updateDataList();
    ui->deleteImageButton->setEnabled(false);
    ui->clearAllButton->setEnabled(false);
    ui->saveImagesButton->setEnabled(false);
    emit statusChanged(tr("已清除所有图像"));
}

void DataAcquisitionModule::onLoadImagesClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择图像目录"));
    if (dir.isEmpty()) return;
    QDir d(dir);
    QStringList files = d.entryList({"*.jpg","*.jpeg","*.png","*.bmp","*.tif","*.tiff"}, QDir::Files);
    if (files.isEmpty()) { QMessageBox::information(this, tr("提示"), tr("目录中没有图像")); return; }

    int loaded = 0;
    for (const QString& f : files) {
        QImage img(d.filePath(f));
        if (img.isNull()) continue;
        CalibrationData data;
        data.qImage    = img;
        data.image     = qImageToCvMat(img);
        data.timestamp = QFileInfo(d.filePath(f)).lastModified().toString("yyyy-MM-dd HH:mm:ss");
        data.filename   =  QFileInfo(d.filePath(f)).fileName();
        m_calibrationData.append(data); ++loaded;
    }
    updateDataList();
    if (!m_calibrationData.isEmpty()) {
        ui->deleteImageButton->setEnabled(true);
        ui->clearAllButton->setEnabled(true);
        ui->saveImagesButton->setEnabled(true);
    }
    emit statusChanged(tr("已加载 %1 张图像").arg(loaded));
    emit dataReady(m_calibrationData);
}

void DataAcquisitionModule::onSaveImagesClicked()
{
    if (m_calibrationData.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("没有图像可保存")); return;
    }
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择保存目录"));
    if (dir.isEmpty()) return;
    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString saveDir = QDir(dir).filePath(ts);
    if (!QDir().mkpath(saveDir)) {
        QMessageBox::warning(this, tr("警告"), tr("无法创建保存目录")); return;
    }

    QFile csv(saveDir + "/metadata.csv");
    if (csv.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&csv);
        out << "文件名,时间戳,备注\n";
        for (int i = 0; i < m_calibrationData.size(); ++i) {
            const auto& d = m_calibrationData[i];
            QString fn = QString("%1_%2.jpg").arg(ts).arg(i + 1, 4, 10, QLatin1Char('0'));
            d.qImage.save(saveDir + "/" + fn);
            out << QString("\"%1\",\"%2\",%3,%4,\"%5\"\n")
                       .arg(fn).arg(d.timestamp);
        }
    }
    emit statusChanged(tr("已保存 %1 张图像到 %2").arg(m_calibrationData.size()).arg(saveDir));
    // emit dataReady(m_calibrationData);
}


// void DataAcquisitionModule::onAutoCaptureToggled(bool checked)
// {
//     if (checked) {
//         if (!m_isPreviewing) onStartPreviewClicked();
//         m_isAutoCapturing = true;
//         m_autoCaptureTimer->start();
//         ui->intervalSpinBox->setEnabled(false);
//         emit statusChanged(tr("自动采集已启动，间隔 %1 秒")
//                                .arg(m_autoCaptureTimer->interval() / 1000.0));
//     } else {
//         m_isAutoCapturing = false;
//         m_autoCaptureTimer->stop();
//         ui->intervalSpinBox->setEnabled(true);
//         emit statusChanged(tr("自动采集已停止"));
//     }
// }

// void DataAcquisitionModule::onAutoCaptureIntervalChanged(int msec)
// {
//     m_autoCaptureTimer->setInterval(msec);
// }



/*-------------------------------- 工具函数 --------------------------------*/

QImage DataAcquisitionModule::cvMatToQImage(const cv::Mat& mat)
{
    if (mat.type() == CV_8UC1)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    if (mat.type() == CV_8UC3) {
        cv::Mat rgb; cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    }
    return QImage();
}

cv::Mat DataAcquisitionModule::qImageToCvMat(const QImage& img)
{
    QImage i = (img.format() != QImage::Format_RGB888) ?
                   img.convertToFormat(QImage::Format_RGB888) : img;
    cv::Mat mat(i.height(), i.width(), CV_8UC3,
                const_cast<uchar*>(i.bits()), i.bytesPerLine());
    cv::Mat ret; cv::cvtColor(mat, ret, cv::COLOR_RGB2BGR);
    return ret.clone();
}


//更新图像显示列表
void DataAcquisitionModule::updateDataList()
{
    ui->dataListWidget->clear();
    for (int i = 0; i < m_calibrationData.size(); ++i) {
        const auto& d = m_calibrationData[i];
        auto* item = new QListWidgetItem(tr("%1 : %2 \t %3").arg(i + 1).arg(d.filename).arg(d.timestamp));
        item->setData(Qt::UserRole, i);
        // item->setIcon(QIcon(QPixmap::fromImage(d.qImage)
        //                         .scaled(64, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        ui->dataListWidget->addItem(item);
    }
}



