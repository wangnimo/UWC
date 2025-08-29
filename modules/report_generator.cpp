#include "report_generator.h"
#include "ui_report_generator.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QTextDocument>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextCursor>
#include <QTextTable>
#include <QStandardPaths>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QPainter>


ReportGeneratorModule::ReportGeneratorModule(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ReportGeneratorModule)
{
    ui->setupUi(this);
    initUI();
    initConnections();
}

ReportGeneratorModule::~ReportGeneratorModule()
{
    delete ui;
}

void ReportGeneratorModule::initUI()
{
    // 设置默认报告标题和作者
    ui->reportTitleEdit->setText(tr("水下相机标定报告"));
    ui->authorEdit->setText(tr("操作员"));
    
    // 设置报告预览为只读
    ui->reportPreviewEdit->setReadOnly(true);
}

void ReportGeneratorModule::initConnections()
{
    connect(ui->generateReportButton, &QPushButton::clicked,
            this, &ReportGeneratorModule::onGenerateReportClicked);
    // connect(ui->previewReportClicked, &QPushButton::clicked,
    //         this, &ReportGeneratorModule::onPreviewReportClicked);
    connect(ui->exportPdfButton, &QPushButton::clicked,
            this, &ReportGeneratorModule::onExportPdfClicked);
    // connect(ui->exportCsvButton, &QPushButton::clicked,
    //         this, &ReportGeneratorModule::onExportCsvClicked);
    // connect(ui->exportJsonButton, &QPushButton::clicked,
    //         this, &ReportGeneratorModule::onExportJsonClicked);
    connect(ui->saveTemplateButton, &QPushButton::clicked,
            this, &ReportGeneratorModule::onSaveTemplateClicked);
    // connect(ui->loadTemplateButton, &QPushButton::clicked,
    //         this, &ReportGeneratorModule::onLoadTemplateClicked);
}

void ReportGeneratorModule::setCalibrationResult(const CalibrationResult& result)
{
    m_currentResult = result;
    
    if (result.success) {
        ui->statusLabel->setText(tr("已加载标定结果，平均重投影误差: %1")
                               .arg(result.params.reprojectionError, 0, 'f', 6));
    } else {
        ui->statusLabel->setText(tr("标定结果无效"));
    }
}

void ReportGeneratorModule::onGenerateReportClicked()
{
    if (!m_currentResult.success) {
        QMessageBox::warning(this, tr("警告"), tr("没有有效的标定结果，无法生成报告"));
        return;
    }
    
    // 生成报告内容
    QString reportContent = generateReportContent();
    
    // 更新预览
    ui->reportPreviewEdit->setHtml(reportContent);
    
    // 更新报告文档
    m_reportDocument.setHtml(reportContent);
    
    emit statusChanged(tr("报告已生成"));
}

QString ReportGeneratorModule::generateReportContent()
{
    if (!m_currentResult.success) {
        return tr("<h1>错误</h1><p>没有有效的标定结果</p>");
    }
    
    // 获取报告设置
    QString title = ui->reportTitleEdit->text();
    QString author = ui->authorEdit->text();
    QString company = ui->companyEdit->text();
    QString date = QDateTime::currentDateTime().toString("yyyy年MM月dd日 HH:mm:ss");
    
    // 开始构建HTML报告
    QString html;
    
    // HTML头部
    html += "<!DOCTYPE html><html><head>";
    html += "<meta charset=\"UTF-8\">";
    html += QString("<title>%1</title>").arg(title);
    html += "<style>";
    html += "body { font-family: SimHei, Arial, sans-serif; line-height: 1.6; }";
    html += "h1, h2, h3 { color: #2c3e50; border-bottom: 1px solid #eee; padding-bottom: 5px; }";
    html += "table { border-collapse: collapse; width: 100%; margin: 10px 0; }";
    html += "th, td { border: 1px solid #ddd; padding: 8px 12px; text-align: left; }";
    html += "th { background-color: #f5f5f5; }";
    html += ".header { text-align: center; margin-bottom: 30px; }";
    html += ".summary { background-color: #f8f9fa; padding: 15px; border-radius: 5px; margin: 15px 0; }";
    html += ".image-container { text-align: center; margin: 20px 0; }";
    html += ".image-container img { max-width: 80%; height: auto; border: 1px solid #ddd; padding: 5px; }";
    html += "</style>";
    html += "</head><body>";
    
    // 报告标题和基本信息
    html += "<div class=\"header\">";
    html += QString("<h1>%1</h1>").arg(title);
    html += QString("<p>生成日期: %1</p>").arg(date);
    html += QString("<p>作者: %1</p>").arg(author);
    if (!company.isEmpty()) {
        html += QString("<p>单位: %1</p>").arg(company);
    }
    html += "</div>";
    
    // 摘要部分
    html += "<h2>1. 标定摘要</h2>";
    html += "<div class=\"summary\">";
    html += QString("<p><strong>标定时间:</strong> %1</p>").arg(m_currentResult.params.timestamp);
    html += QString("<p><strong>标定板规格:</strong> %1x%2 方格，每格 %3 mm</p>")
        .arg(m_currentResult.params.boardSize.width)
        .arg(m_currentResult.params.boardSize.height)
        .arg(m_currentResult.params.squareSize);
    html += QString("<p><strong>使用图像数量:</strong> %1</p>")
        .arg(m_currentResult.perViewErrors.size());
    html += QString("<p><strong>平均重投影误差:</strong> %1 像素</p>")
        .arg(m_currentResult.params.reprojectionError, 0, 'f', 6);
    
    // 计算最大和最小误差
    if (!m_currentResult.perViewErrors.empty()) {
        double minError = *std::min_element(m_currentResult.perViewErrors.begin(), 
                                           m_currentResult.perViewErrors.end());
        double maxError = *std::max_element(m_currentResult.perViewErrors.begin(), 
                                           m_currentResult.perViewErrors.end());
        html += QString("<p><strong>最小重投影误差:</strong> %1 像素</p>").arg(minError, 0, 'f', 6);
        html += QString("<p><strong>最大重投影误差:</strong> %1 像素</p>").arg(maxError, 0, 'f', 6);
    }
    html += "</div>";
    
    // 标定参数部分
    if (ui->includeParametersCheck->isChecked()) {
        html += "<h2>2. 标定参数</h2>";
        
        // 内参矩阵
        html += "<h3>2.1 相机内参矩阵</h3>";
        html += "<table>";
        cv::Mat cameraMatrix = m_currentResult.params.cameraMatrix;
        for (int i = 0; i < 3; i++) {
            html += "<tr>";
            for (int j = 0; j < 3; j++) {
                html += QString("<td>%1</td>").arg(cameraMatrix.at<double>(i, j), 0, 'f', 6);
            }
            html += "</tr>";
        }
        html += "</table>";
        
        // 畸变系数
        html += "<h3>2.2 畸变系数</h3>";
        html += "<p><em>k1, k2, p1, p2, k3</em></p>";
        html += "<table><tr>";
        cv::Mat distCoeffs = m_currentResult.params.distCoeffs;
        for (int i = 0; i < 5 && i < distCoeffs.rows; i++) {
            html += QString("<td>%1</td>").arg(distCoeffs.at<double>(i), 0, 'f', 6);
        }
        html += "</tr></table>";
    }
    
    // 误差分析部分
    if (ui->includeErrorsCheck->isChecked() && !m_currentResult.perViewErrors.empty()) {
        html += "<h2>3. 误差分析</h2>";
        
        // 误差表格
        html += "<h3>3.1 各图像重投影误差</h3>";
        html += "<table>";
        html += "<tr><th>图像序号</th><th>重投影误差 (像素)</th></tr>";
        for (int i = 0; i < m_currentResult.perViewErrors.size(); i++) {
            html += QString("<tr><td>%1</td><td>%2</td></tr>")
                .arg(i + 1)
                .arg(m_currentResult.perViewErrors[i], 0, 'f', 6);
        }
        html += "</table>";
    }
    
    // HTML尾部
    html += "</body></html>";
    
    return html;
}

void ReportGeneratorModule::onPreviewReportClicked()
{
    if (m_reportDocument.isEmpty()) {
        onGenerateReportClicked();
    }
    
    if (m_reportDocument.isEmpty()) {
        return;
    }
    
    // 创建预览对话框
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("报告预览"));
    dialog->resize(800, 600);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    QTextEdit* previewEdit = new QTextEdit();
    previewEdit->setReadOnly(true);
    previewEdit->setHtml(m_reportDocument.toHtml());
    layout->addWidget(previewEdit);
    
    QPushButton* closeButton = new QPushButton(tr("关闭"));
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::close);
    layout->addWidget(closeButton);
    
    dialog->exec();
    delete dialog;
}

bool ReportGeneratorModule::exportToPdf(const QString& filePath)
{
    if (m_reportDocument.isEmpty()) {
        return false;
    }
    
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize::A4);
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    printer.setCreator("水下相机标定系统");
    printer.setDocName(ui->reportTitleEdit->text());
    
    QPainter painter;
    if (!painter.begin(&printer)) {
        return false;
    }
    
    // 绘制文档
    m_reportDocument.drawContents(&painter);
    
    return painter.end();
}

void ReportGeneratorModule::onExportPdfClicked()
{
    if (m_reportDocument.isEmpty()) {
        onGenerateReportClicked();
    }
    
    if (m_reportDocument.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先生成报告"));
        return;
    }
    
    QString defaultName = QString("%1_%2.pdf")
        .arg(ui->reportTitleEdit->text().replace(" ", "_"))
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    
    QString filePath = QFileDialog::getSaveFileName(this, tr("导出PDF报告"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + defaultName,
        tr("PDF文件 (*.pdf)"));
    
    if (filePath.isEmpty()) {
        return;
    }
    
    if (exportToPdf(filePath)) {
        emit statusChanged(tr("PDF报告已导出到: %1").arg(filePath));
        QMessageBox::information(this, tr("成功"), tr("PDF报告已导出"));
    } else {
        emit statusChanged(tr("导出PDF报告失败"));
        QMessageBox::critical(this, tr("失败"), tr("无法导出PDF报告"));
    }
}

bool ReportGeneratorModule::exportToCsv(const QString& filePath)
{
    if (!m_currentResult.success) {
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    
    // 写入标定信息
    out << "标定报告信息,,,\n";
    out << "标题," << ui->reportTitleEdit->text() << ",,\n";
    out << "作者," << ui->authorEdit->text() << ",,\n";
    out << "单位," << ui->companyEdit->text() << ",,\n";
    out << "生成日期," << QDateTime::currentDateTime().toString() << ",,\n";
    out << "标定时间," << m_currentResult.params.timestamp << ",,\n";
    out << "标定板规格," << m_currentResult.params.boardSize.width 
        << "x" << m_currentResult.params.boardSize.height << ",mm\n";
    out << "方格大小," << m_currentResult.params.squareSize << ",mm\n";
    out << "平均重投影误差," << m_currentResult.params.reprojectionError << ",像素\n\n";
    
    // 写入内参矩阵
    out << "内参矩阵,,, \n";
    cv::Mat cameraMatrix = m_currentResult.params.cameraMatrix;
    for (int i = 0; i < 3; i++) {
        out << "," << cameraMatrix.at<double>(i, 0) 
            << "," << cameraMatrix.at<double>(i, 1) 
            << "," << cameraMatrix.at<double>(i, 2) << "\n";
    }
    out << "\n";
    
    // 写入畸变系数
    out << "畸变系数 (k1, k2, p1, p2, k3),,,\n";
    cv::Mat distCoeffs = m_currentResult.params.distCoeffs;
    out << ",";
    for (int i = 0; i < 5 && i < distCoeffs.rows; i++) {
        out << distCoeffs.at<double>(i) << ",";
    }
    out << "\n\n";
    
    // 写入各图像误差
    out << "图像序号,重投影误差 (像素),,\n";
    for (int i = 0; i < m_currentResult.perViewErrors.size(); i++) {
        out << (i + 1) << "," << m_currentResult.perViewErrors[i] << ",,\n";
    }
    
    file.close();
    return true;
}

void ReportGeneratorModule::onExportCsvClicked()
{
    if (!m_currentResult.success) {
        QMessageBox::warning(this, tr("警告"), tr("没有有效的标定结果"));
        return;
    }
    
    QString defaultName = QString("%1_%2.csv")
        .arg(ui->reportTitleEdit->text().replace(" ", "_"))
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    
    QString filePath = QFileDialog::getSaveFileName(this, tr("导出CSV数据"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + defaultName,
        tr("CSV文件 (*.csv)"));
    
    if (filePath.isEmpty()) {
        return;
    }
    
    if (exportToCsv(filePath)) {
        emit statusChanged(tr("CSV数据已导出到: %1").arg(filePath));
        QMessageBox::information(this, tr("成功"), tr("CSV数据已导出"));
    } else {
        emit statusChanged(tr("导出CSV数据失败"));
        QMessageBox::critical(this, tr("失败"), tr("无法导出CSV数据"));
    }
}

bool ReportGeneratorModule::exportToJson(const QString& filePath)
{
    if (!m_currentResult.success) {
        return false;
    }
    
    QJsonObject root;
    
    // 报告信息
    QJsonObject reportInfo;
    reportInfo["title"] = ui->reportTitleEdit->text();
    reportInfo["author"] = ui->authorEdit->text();
    reportInfo["company"] = ui->companyEdit->text();
    reportInfo["generated_date"] = QDateTime::currentDateTime().toString();
    root["report_info"] = reportInfo;
    
    // 标定信息
    QJsonObject calibrationInfo;
    calibrationInfo["calibration_time"] = m_currentResult.params.timestamp;
    calibrationInfo["board_width"] = m_currentResult.params.boardSize.width;
    calibrationInfo["board_height"] = m_currentResult.params.boardSize.height;
    calibrationInfo["square_size"] = m_currentResult.params.squareSize;
    calibrationInfo["average_reprojection_error"] = m_currentResult.params.reprojectionError;
    root["calibration_info"] = calibrationInfo;
    
    // 内参矩阵
    QJsonArray cameraMatrix;
    for (int i = 0; i < 3; i++) {
        QJsonArray row;
        for (int j = 0; j < 3; j++) {
            row.append(m_currentResult.params.cameraMatrix.at<double>(i, j));
        }
        cameraMatrix.append(row);
    }
    root["camera_matrix"] = cameraMatrix;
    
    // 畸变系数
    QJsonArray distCoeffs;
    for (int i = 0; i < 5 && i < m_currentResult.params.distCoeffs.rows; i++) {
        distCoeffs.append(m_currentResult.params.distCoeffs.at<double>(i));
    }
    root["distortion_coefficients"] = distCoeffs;
    
    // 各图像误差
    QJsonArray perViewErrors;
    for (double error : m_currentResult.perViewErrors) {
        perViewErrors.append(error);
    }
    root["per_view_errors"] = perViewErrors;
    
    // 保存JSON文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}

void ReportGeneratorModule::onExportJsonClicked()
{
    if (!m_currentResult.success) {
        QMessageBox::warning(this, tr("警告"), tr("没有有效的标定结果"));
        return;
    }
    
    QString defaultName = QString("%1_%2.json")
        .arg(ui->reportTitleEdit->text().replace(" ", "_"))
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    
    QString filePath = QFileDialog::getSaveFileName(this, tr("导出JSON数据"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + defaultName,
        tr("JSON文件 (*.json)"));
    
    if (filePath.isEmpty()) {
        return;
    }
    
    if (exportToJson(filePath)) {
        emit statusChanged(tr("JSON数据已导出到: %1").arg(filePath));
        QMessageBox::information(this, tr("成功"), tr("JSON数据已导出"));
    } else {
        emit statusChanged(tr("导出JSON数据失败"));
        QMessageBox::critical(this, tr("失败"), tr("无法导出JSON数据"));
    }
}

bool ReportGeneratorModule::loadReportTemplate(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        return false;
    }
    
    QJsonObject root = doc.object();
    
    // 加载模板信息
    if (root.contains("report_settings")) {
        QJsonObject settings = root["report_settings"].toObject();
        if (settings.contains("title")) ui->reportTitleEdit->setText(settings["title"].toString());
        if (settings.contains("author")) ui->authorEdit->setText(settings["author"].toString());
        if (settings.contains("company")) ui->companyEdit->setText(settings["company"].toString());
        if (settings.contains("report_type")) {
            int type = settings["report_type"].toInt();
            if (type >= 0 && type < ui->reportTypeCombo->count()) {
                ui->reportTypeCombo->setCurrentIndex(type);
            }
        }
        
        // 加载包含选项
        if (settings.contains("include_parameters")) {
            ui->includeParametersCheck->setChecked(settings["include_parameters"].toBool());
        }
        if (settings.contains("include_errors")) {
            ui->includeErrorsCheck->setChecked(settings["include_errors"].toBool());
        }
        if (settings.contains("include_visualizations")) {
            ui->includeVisualizationsCheck->setChecked(settings["include_visualizations"].toBool());
        }
        if (settings.contains("include_images")) {
            ui->includeImagesCheck->setChecked(settings["include_images"].toBool());
        }
    }
    
    return true;
}

void ReportGeneratorModule::onLoadTemplateClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("加载报告模板"), "",
        tr("模板文件 (*.rtpl);;所有文件 (*)"));
    
    if (filePath.isEmpty()) {
        return;
    }
    
    if (loadReportTemplate(filePath)) {
        emit statusChanged(tr("报告模板已加载: %1").arg(filePath));
    } else {
        emit statusChanged(tr("加载报告模板失败"));
        QMessageBox::critical(this, tr("失败"), tr("无法加载报告模板或模板格式无效"));
    }
}

bool ReportGeneratorModule::saveReportTemplate(const QString& filePath)
{
    QJsonObject root;
    
    // 保存报告设置
    QJsonObject settings;
    settings["title"] = ui->reportTitleEdit->text();
    settings["author"] = ui->authorEdit->text();
    settings["company"] = ui->companyEdit->text();
    settings["report_type"] = ui->reportTypeCombo->currentIndex();
    settings["include_parameters"] = ui->includeParametersCheck->isChecked();
    settings["include_errors"] = ui->includeErrorsCheck->isChecked();
    settings["include_visualizations"] = ui->includeVisualizationsCheck->isChecked();
    settings["include_images"] = ui->includeImagesCheck->isChecked();
    settings["created_date"] = QDateTime::currentDateTime().toString();
    
    root["report_settings"] = settings;
    
    // 保存模板文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}

void ReportGeneratorModule::onSaveTemplateClicked()
{
    QString defaultName = QString("report_template_%1.rtpl")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    
    QString filePath = QFileDialog::getSaveFileName(this, tr("保存报告模板"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + defaultName,
        tr("模板文件 (*.rtpl)"));
    
    if (filePath.isEmpty()) {
        return;
    }
    
    if (saveReportTemplate(filePath)) {
        emit statusChanged(tr("报告模板已保存到: %1").arg(filePath));
        QMessageBox::information(this, tr("成功"), tr("报告模板已保存"));
    } else {
        emit statusChanged(tr("保存报告模板失败"));
        QMessageBox::critical(this, tr("失败"), tr("无法保存报告模板"));
    }
}
    
