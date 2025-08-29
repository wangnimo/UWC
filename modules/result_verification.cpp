#include "result_verification.h"
#include "ui_result_verification.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QStandardPaths>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <sstream>

ResultVerificationModule::ResultVerificationModule(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ResultVerificationModule)
    , m_errorChart(new QChart)
{
    ui->setupUi(this);
    initUI();
    initConnections();
}

ResultVerificationModule::~ResultVerificationModule() = default;

void ResultVerificationModule::initUI()
{
    // 摘要区
    ui->labelDeviceInfo->setText(tr("设备信息:"));
    ui->labelCalibrationStatus->setText(tr("标定状态:"));
    ui->labelCalibrationDate->setText(tr("标定日期:"));
    ui->labelImageCount->setText(tr("图像数量:"));
    ui->labelRmsError->setText(tr("RMS误差:"));

    // 图表
    m_errorChart->setTitle(tr("重投影误差分布"));
    auto *chartView = new QChartView(m_errorChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    ui->errorPlotLabel->setVisible(false);   // 隐藏占位 QLabel
    ui->errorPlotLabel->setParent(nullptr);  // 释放
    ui->verticalLayout_4->insertWidget(0, chartView); // 把图表放进 TabWidget

    // 表格
    ui->errorsTableWidget->setHorizontalHeaderLabels(
        {tr("图像编号"), tr("重投影误差 (像素)")});
}

void ResultVerificationModule::initConnections()
{
    connect(ui->visualizeButton, &QPushButton::clicked,
            this, &ResultVerificationModule::onVisualizeErrorsClicked);
    connect(ui->exportErrorsButton, &QPushButton::clicked,
            this, &ResultVerificationModule::onSaveVisualizationClicked);
    connect(ui->errorsTableWidget, &QTableWidget::itemSelectionChanged,
            this, [this] {
                onImageIndexChanged(ui->errorsTableWidget->currentRow());
            });
}

void ResultVerificationModule::onCalibrationCompleted(const CalibrationResult &result)
{
    m_currentResult = result;
    displayResultSummary(result);
    statusChanged(result.success
                      ? tr("已加载标定结果，平均误差 %1 像素").arg(result.params.reprojectionError, 0, 'f', 6)
                      : tr("加载的标定结果无效"));
}

void ResultVerificationModule::displayResultSummary(const CalibrationResult &r)
{
    ui->labelDeviceInfoValue->setText(r.params.deviceInfo);
    ui->labelCalibrationStatusValue->setText(r.success ? tr("成功") : tr("失败"));
    ui->labelCalibrationDateValue->setText(r.params.timestamp);
    ui->labelImageCountValue->setText(QString::number(r.perViewErrors.size()));
    ui->labelRmsErrorValue->setText(QString::number(r.params.reprojectionError, 'f', 6));

    // 填充表格
    ui->errorsTableWidget->setRowCount(r.perViewErrors.size());
    for (int i = 0; i < r.perViewErrors.size(); ++i) {
        ui->errorsTableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        ui->errorsTableWidget->setItem(i, 1,
                                       new QTableWidgetItem(QString::number(r.perViewErrors[i], 'f', 4)));
    }

    visualizeReprojectionErrors();
}

void ResultVerificationModule::visualizeReprojectionErrors()
{
    if (!m_currentResult.success || m_currentResult.perViewErrors.empty())
        return;

    m_errorChart->removeAllSeries();
    auto *series = new QLineSeries();
    series->setName(tr("重投影误差"));
    for (int i = 0; i < m_currentResult.perViewErrors.size(); ++i)
        series->append(i + 1, m_currentResult.perViewErrors[i]);
    m_errorChart->addSeries(series);

    auto *axisX = new QValueAxis;
    axisX->setTitleText(tr("图像序号"));
    axisX->setRange(1, m_currentResult.perViewErrors.size());
    m_errorChart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis;
    axisY->setTitleText(tr("重投影误差 (像素)"));
    axisY->setRange(0, *std::max_element(m_currentResult.perViewErrors.begin(),
                                         m_currentResult.perViewErrors.end()) * 1.2);
    m_errorChart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
}

void ResultVerificationModule::onVisualizeErrorsClicked()
{
    QMessageBox::information(this, "提示", "已显示误差曲线");
}

void ResultVerificationModule::onSaveVisualizationClicked()
{
    if (!m_currentResult.success) {
        QMessageBox::warning(this, tr("警告"), tr("无有效结果"));
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(this, tr("保存目录"));
    if (dir.isEmpty()) return;

    QString path = dir + "/error_plot.png";
    m_errorChartView->grab().save(path);
    QMessageBox::information(this, tr("成功"), tr("已保存: %1").arg(path));
}

void ResultVerificationModule::onImageIndexChanged(int index)
{
    if (index < 0 || index >= m_currentResult.perViewErrors.size()) return;
    // 后续可显示对应图像
}
