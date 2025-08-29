#include "data_management.h"
#include "ui_data_management.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QBuffer>
#include <QImage>
#include <opencv2/opencv.hpp>

DataManagementModule::DataManagementModule(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataManagementModule),
    m_recordsModel(nullptr)
{
    ui->setupUi(this);
    initUI();
    initConnections();
    
    // 初始化数据库
    if (initDatabase()) {
        loadAllRecords();
        emit statusChanged(tr("数据库初始化成功"));
    } else {
        emit statusChanged(tr("数据库初始化失败"));
    }
}

DataManagementModule::~DataManagementModule()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    delete m_recordsModel;
    delete ui;
}

void DataManagementModule::initUI()
{
    // 初始化记录表格
    m_recordsModel = new QSqlTableModel(this, m_db);
    m_recordsModel->setTable("calibration_records");
    m_recordsModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    
    // 设置列标题
    m_recordsModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    m_recordsModel->setHeaderData(1, Qt::Horizontal, tr("名称"));
    m_recordsModel->setHeaderData(2, Qt::Horizontal, tr("设备型号"));
    m_recordsModel->setHeaderData(3, Qt::Horizontal, tr("序列号"));
    m_recordsModel->setHeaderData(4, Qt::Horizontal, tr("标定日期"));
    m_recordsModel->setHeaderData(5, Qt::Horizontal, tr("图像数量"));
    m_recordsModel->setHeaderData(6, Qt::Horizontal, tr("重投影误差"));
    m_recordsModel->setHeaderData(7, Qt::Horizontal, tr("参数路径"));
    m_recordsModel->setHeaderData(8, Qt::Horizontal, tr("备注"));
    
    // 设置表格视图
    ui->recordsTableView->setModel(m_recordsModel);
    ui->recordsTableView->setAlternatingRowColors(true);
    ui->recordsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->recordsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->recordsTableView->horizontalHeader()->setStretchLastSection(true);
    
    // 隐藏ID和参数路径列（可以通过代码访问，但不在UI显示）
    ui->recordsTableView->hideColumn(0);
    ui->recordsTableView->hideColumn(7);
    
    // 设置误差筛选范围
    ui->maxErrorSpinBox->setRange(0, 10);
    ui->maxErrorSpinBox->setValue(1.0);
    ui->maxErrorSpinBox->setSingleStep(0.1);
}

void DataManagementModule::initConnections()
{
    connect(ui->loadRecordButton, &QPushButton::clicked,
            this, &DataManagementModule::onLoadRecordClicked);
    connect(ui->deleteRecordButton, &QPushButton::clicked,
            this, &DataManagementModule::onDeleteRecordClicked);
    connect(ui->exportRecordButton, &QPushButton::clicked,
            this, &DataManagementModule::onExportRecordClicked);
    // connect(ui->searchRecordsButton, &QPushButton::clicked,
    //         this, &DataManagementModule::onSearchRecordsClicked);
    // connect(ui->filterRecordsButton, &QPushButton::clicked,
    //         this, &DataManagementModule::onFilterRecordsClicked);
    connect(ui->recordsTableView, &QTableView::doubleClicked,
            this, &DataManagementModule::onRecordDoubleClicked);
    // connect(ui->syncWithDatabaseButton, &QPushButton::clicked,
    //         this, &DataManagementModule::onSyncWithDatabaseClicked);
    connect(ui->backupDatabaseButton, &QPushButton::clicked,
            this, &DataManagementModule::onBackupDatabaseClicked);
    connect(ui->restoreDatabaseButton, &QPushButton::clicked,
            this, &DataManagementModule::onRestoreDatabaseClicked);
}
