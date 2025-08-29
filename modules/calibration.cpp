#include "calibration.h"
#include "ui_calibration.h"
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QProgressDialog>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <QtConcurrent>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarSeries>
#include <QtCharts/QChartView>


/*-------------------------------- CalibrationWorker --------------------------------*/
CalibrationWorker::CalibrationWorker(const QList<CalibrationData>& data,
                                     const cv::Size& boardSize,
                                     float squareSize,
                                     bool useUndistortion)
    : m_data(data), m_boardSize(boardSize), m_squareSize(squareSize),
    m_useUndistortion(useUndistortion), m_abort(false)
{}

void CalibrationWorker::abort()
{
    QMutexLocker locker(&m_mutex);
    m_abort = true;
}

void CalibrationWorker::doWork()
{
    CalibrationResult result = performCalibration();
    // if (m_abort) return;
    emit workFinished(result);
}

CalibrationResult CalibrationWorker::performCalibration()
{
    CalibrationResult out;
    out.success = false;

    if (m_data.isEmpty()) {
        out.message = tr("无有效图像数据");
        return out;
    }

    std::vector<std::vector<cv::Point3f>> objectPoints;
    std::vector<std::vector<cv::Point2f>> imagePoints;

    // 生成标定板 3D 坐标
    std::vector<cv::Point3f> obj;
    for (int i = 0; i < m_boardSize.height; ++i)
        for (int j = 0; j < m_boardSize.width; ++j)
            obj.emplace_back(j * m_squareSize, i * m_squareSize, 0.0f);

    int progress = 0;
    for (const auto& data : m_data) {
        if (m_abort) break;

        std::vector<cv::Point2f> corners;
        bool ok = findChessboardCorners(data.image, m_boardSize, corners);
        if (ok) {
            imagePoints.emplace_back(corners);
            objectPoints.emplace_back(obj);
        }
        emit progressUpdated(++progress * 100 / m_data.size());
    }

    if (imagePoints.empty()) {
        out.message = tr("未找到任何棋盘格角点");
        return out;
    }

    // 开始标定
    cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    cv::Mat distCoeffs = cv::Mat::eye(1, 5, CV_64F);
    std::vector<cv::Mat> rvecs, tvecs;

    double reprojErr = cv::calibrateCamera(objectPoints, imagePoints,
                                           m_data.first().image.size(),
                                           cameraMatrix, distCoeffs,
                                           rvecs, tvecs);

    out.params.cameraMatrix = cameraMatrix.clone();
    out.params.distCoeffs   = distCoeffs.clone();
    out.params.rvecs        = rvecs;
    out.params.tvecs        = tvecs;
    out.params.reprojectionError = reprojErr;
    out.params.boardSize    = m_boardSize;
    out.params.squareSize   = m_squareSize;
    out.params.timestamp    = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    out.success = true;
    out.message = tr("标定完成，重投影误差：%1 像素").arg(reprojErr);

    //计算每幅图的重投影误差
    for (size_t i = 0; i < objectPoints.size(); ++i) {
        std::vector<cv::Point2f> projectedPoints;
        // 将世界坐标系中的 3D 点投影到图像平面上
        cv::projectPoints(objectPoints[i], rvecs[i], tvecs[i], cameraMatrix, distCoeffs, projectedPoints);

        double error = 0.0;
        // 计算投影点与实际检测到的图像点之间的误差
        for (size_t j = 0; j < imagePoints[i].size(); ++j) {
            cv::Point2f diff = imagePoints[i][j] - projectedPoints[j];
            error += std::sqrt(diff.x * diff.x + diff.y * diff.y);
        }
        // 计算平均误差
        error /= static_cast<double>(projectedPoints.size());
        out.perViewErrors.push_back(error);
    }

    // 输出每幅图像的重投影误差
    for (size_t i = 0; i < out.perViewErrors.size(); ++i) {
        std::cout << "Image " << i + 1 << " reprojection error: " << out.perViewErrors[i] << " pixels" << std::endl;
    }
    return out;
}

bool CalibrationWorker::findChessboardCorners(const cv::Mat& image,
                                              cv::Size boardSize,
                                              std::vector<cv::Point2f>& corners)
{
    cv::Mat gray;
    if (image.channels() == 3)
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    else
        gray = image.clone();

    bool ok = cv::findChessboardCorners(gray, boardSize, corners,
                                        cv::CALIB_CB_ADAPTIVE_THRESH +
                                            cv::CALIB_CB_FAST_CHECK +
                                            cv::CALIB_CB_NORMALIZE_IMAGE);
    if (ok)
        cv::cornerSubPix(gray, corners, cv::Size(11,11), cv::Size(-1,-1),
                         cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.1));
    return ok;
}

/*-------------------------------- CalibrationModule --------------------------------*/
CalibrationModule::CalibrationModule(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CalibrationModule)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
{
    ui->setupUi(this);
    initUI();
    initConnections();
}

CalibrationModule::~CalibrationModule()
{
    if (m_workerThread) {
        onCancelCalibration();
        m_workerThread->wait();
        delete m_workerThread;
    }
    delete ui;
}
//初始化参数
void CalibrationModule::initUI()
{
    // ui->boardWidthSpin->setValue(9);
    // ui->boardHeightSpin->setValue(6);
    // ui->squareSizeSpin->setValue(25.0);
    ui->calibrationProgressBar->setVisible(false);

}
// 初始化连接
void CalibrationModule::initConnections()
{
    connect(ui->startCalibrationButton, &QPushButton::clicked,
            this, &CalibrationModule::onStartCalibrationClicked);
    connect( ui->cancelCalibrationButton, &QPushButton::clicked,
            this, &CalibrationModule::onCancelCalibration);
    // connect(ui->loadDataButton, &QPushButton::clicked,
    //         this, &CalibrationModule::onLoadDataClicked);
    connect(ui->saveParametersButton, &QPushButton::clicked,
            this, &CalibrationModule::onSaveParametersClicked);
    connect(ui->calibrationType, &QComboBox::currentIndexChanged,
            this, &CalibrationModule::onCalibrationTypeChanged);
}
//更新设置信息
void CalibrationModule::updateCalibSetting(const AppSettings& settings){
    ui->boardWidthSpin->setValue(settings.defaultBoardWidth);
    ui->boardHeightSpin->setValue(settings.defaultBoardHeight);
    ui->squareSizeSpin->setValue(settings.defaultSquareSize);
}
void CalibrationModule::onDataReady(const QList<CalibrationData>& data)
{
    m_calibrationData = data;
    QMessageBox::information(this, tr("提示"),
                             tr("已收到 %1 张图像数据").arg(data.size()));
}
// 开始标定 槽函数
void CalibrationModule::onStartCalibrationClicked()
{
    if (m_calibrationData.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载或采集图像"));
        return;
    }

    cv::Size boardSize(ui->boardWidthSpin->value(), ui->boardHeightSpin->value());
    float squareSize = static_cast<float>(ui->squareSizeSpin->value());

    m_workerThread = new QThread(this);
    m_worker = new CalibrationWorker(m_calibrationData, boardSize, squareSize);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &CalibrationWorker::doWork);
    connect(m_worker, &CalibrationWorker::progressUpdated, ui->calibrationProgressBar, &QProgressBar::setValue);
    connect(m_worker, &CalibrationWorker::workFinished, this, &CalibrationModule::onCalibrationFinished);
    connect(m_worker, &CalibrationWorker::errorOccurred, this, &CalibrationModule::onCalibrationError);

    ui->calibrationProgressBar->setVisible(true);
    ui->calibrationProgressBar->setValue(0);
    ui->cancelCalibrationButton->setEnabled(true);
    ui->startCalibrationButton->setEnabled(false);
    m_workerThread->start();
}
// 读取标定参数
void CalibrationModule::onLoadDataClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("加载标定图像"),
                                                    QString(), tr("YAML (*.yml *.yaml)"));
    if (fileName.isEmpty()) return;
    // TODO：实现 YAML 解析
    QMessageBox::information(this, tr("提示"), tr("YAML 加载待实现"));
}

void CalibrationModule::onSaveParametersClicked()
{
    if (!m_currentResult.success) {
        QMessageBox::warning(this, tr("警告"), tr("无可保存的标定参数"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("保存标定参数"),
                                                    QString(), tr("YAML (*.yml *.yaml)"));
    if (fileName.isEmpty()) return;
    saveCalibrationParameters(m_currentResult.params, fileName);
}
//保存标定参数
bool CalibrationModule::saveCalibrationParameters(const CalibrationParameters& params,
                                                  const QString& filePath)
{
    cv::FileStorage fs(filePath.toStdString(), cv::FileStorage::WRITE);
    if (!fs.isOpened()) return false;

    fs << "camera_matrix" << params.cameraMatrix;
    fs << "dist_coeffs"   << params.distCoeffs;
    fs << "board_size"    << params.boardSize;
    fs << "square_size"   << params.squareSize;
    fs << "timestamp"     << params.timestamp.toStdString();
    fs.release();
    QMessageBox::information(this, tr("提示"), tr("参数已保存"));
    return true;
}


//标定进度条
void CalibrationModule::onCalibrationProgress(int progress)
{
    ui->calibrationProgressBar->setValue(progress);
}
//标定结束
void CalibrationModule::onCalibrationFinished(CalibrationResult result)
{

    ui->calibrationProgressBar->setVisible(false);
    ui->cancelCalibrationButton->setEnabled(false);
    ui->startCalibrationButton->setEnabled(true);
    if (result.success){
        m_currentResult = result;
        displayCalibrationParameters(result.params);
        // QMessageBox::information(this, tr("结果"), result.message);
        emit calibrationComplete(result);
    }
    else
        QMessageBox::critical(this, tr("警告"), "标定失败！");

}
//标定错误
void CalibrationModule::onCalibrationError(QString error)
{
    ui->calibrationProgressBar->setVisible(false);
    QMessageBox::critical(this, tr("错误"), error);
}
//取消标定
void CalibrationModule::onCancelCalibration()
{
    if (m_worker)
        m_worker->abort(); // 设置标志位请求工作停止

    if (m_workerThread->isRunning()) {
        m_workerThread->quit(); // 请求线程退出事件循环
        if (!m_workerThread->wait(5000)) { // 等待线程退出，设置超时时间为 5 秒
            // 超时处理，可根据需要添加日志或其他操作
            qDebug() << "Thread termination timed out.";
        }
    }
}
//显示标定参数
void CalibrationModule::displayCalibrationParameters(const CalibrationParameters& params)
{
    std::stringstream ss;
    ss << "重投影误差: " << params.reprojectionError << " 像素\n";
    ss << "内参矩阵:\n" << params.cameraMatrix;
    ui->logTextEdit->setPlainText(QString::fromStdString(ss.str()));

    //填充表格
    fillTables(params);
}
//更新表格
void CalibrationModule::fillTables(const CalibrationParameters& params)
{
    // 填充 3x3 内参矩阵表格
    for (int i = 0; i < params.cameraMatrix.rows; ++i) {
        for (int j = 0; j < params.cameraMatrix.cols; ++j) {
            QTableWidgetItem *item = new QTableWidgetItem(QString::number(params.cameraMatrix.at<double>(i, j)));
            ui->cameraMatrixTable->setItem(i, j, item);
        }
    }

    // 填充 1x5 畸变系数表格
    for (int j = 0; j < params.distCoeffs.cols; ++j) {
        QTableWidgetItem *item = new QTableWidgetItem(QString::number(params.distCoeffs.at<double>(0, j)));
        ui->distortionTable->setItem(0, j, item);
    }
}
//切换标定板类型
void CalibrationModule::onCalibrationTypeChanged(int index)
{
    Q_UNUSED(index);
    // 预留：切换标定板类型
    qDebug() << "Selected text:" << index;
}
