#include "help.h"
#include "ui_help.h"
#include <QHelpEngineCore>
#include <QHelpIndexWidget>
#include <QHelpContentWidget>
#include <QHelpSearchEngine>
#include <QHelpSearchQueryWidget>
#include <QHelpSearchResultWidget>
#include <QTabWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QDialog>
#include <QTextEdit>
#include <QVersionNumber>
#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QDir>

HelpModule::HelpModule(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HelpModule),
    m_helpEngine(nullptr)
{
    ui->setupUi(this);
    initUI();
    initHelpEngine();
    loadHelpContent();
}

HelpModule::~HelpModule()
{
    delete m_helpEngine;
    delete ui;
}

void HelpModule::initUI()
{
    setWindowTitle(tr("帮助与文档"));
    
    // 创建分割器
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // 创建左侧导航标签
    QTabWidget* navTabWidget = new QTabWidget();
    navTabWidget->setMaximumWidth(250);
    
    // 内容导航
    m_helpEngine = new QHelpEngine("help/underwater_calibrator.qhc", this);
    QHelpContentWidget* contentWidget = m_helpEngine->contentWidget();
    navTabWidget->addTab(contentWidget, tr("目录"));
    
    // 索引
    QHelpIndexWidget* indexWidget = m_helpEngine->indexWidget();
    navTabWidget->addTab(indexWidget, tr("索引"));
    
    // 搜索
    QWidget* searchWidget = new QWidget();
    QVBoxLayout* searchLayout = new QVBoxLayout(searchWidget);
    
    QLineEdit* searchEdit = new QLineEdit();
    searchEdit->setObjectName("searchEdit");
    searchEdit->setPlaceholderText(tr("输入搜索关键词..."));
    
    QPushButton* searchButton = new QPushButton(tr("搜索"));
    searchButton->setObjectName("searchButton");
    
    QHBoxLayout* searchBoxLayout = new QHBoxLayout();
    searchBoxLayout->addWidget(searchEdit);
    searchBoxLayout->addWidget(searchButton);
    
    QHelpSearchResultWidget* searchResultWidget = m_helpEngine->searchEngine()->resultWidget();
    
    searchLayout->addLayout(searchBoxLayout);
    searchLayout->addWidget(searchResultWidget);
    
    navTabWidget->addTab(searchWidget, tr("搜索"));
    
    // 右侧内容显示
    QTextBrowser* helpBrowser = new QTextBrowser();
    helpBrowser->setObjectName("helpBrowser");
    helpBrowser->setOpenExternalLinks(true);
    
    // 添加到分割器
    mainSplitter->addWidget(navTabWidget);
    mainSplitter->addWidget(helpBrowser);
    mainSplitter->setSizes(QList<int>() << 250 << 550);
    
    // 底部按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* aboutButton = new QPushButton(tr("关于"));
    aboutButton->setObjectName("aboutButton");
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(aboutButton);
    
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainSplitter);
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号槽
    connect(contentWidget, &QHelpContentWidget::linkActivated, 
            helpBrowser, &QTextBrowser::setSource);
    connect(indexWidget, &QHelpIndexWidget::linkActivated, 
            helpBrowser, &QTextBrowser::setSource);
    connect(m_helpEngine->searchEngine(), &QHelpSearchEngine::searchingFinished,
            [=]() {
        int count = m_helpEngine->searchEngine()->resultWidget()->colorCount();
        emit statusChanged(tr("搜索完成，找到 %1 个结果").arg(count));
    });
    connect(searchButton, &QPushButton::clicked, this, &HelpModule::onSearchButtonClicked);
    connect(aboutButton, &QPushButton::clicked, this, &HelpModule::onAboutButtonClicked);
    connect(helpBrowser, &QTextBrowser::sourceChanged, this, &HelpModule::onTopicChanged);
    
    // 设置UI
    ui->setupUi(this);
    setLayout(mainLayout);
}

void HelpModule::initHelpEngine()
{
    if (m_helpEngine) {
        bool initialized = m_helpEngine->setupData();
        if (!initialized) {
            emit statusChanged(tr("帮助文档初始化失败: %1").arg(m_helpEngine->error()));
        } else {
            emit statusChanged(tr("帮助文档已加载"));
        }
    }
}

bool HelpModule::loadHelpContent()
{
    if (!m_helpEngine) return false;
    
    // 检查帮助文件是否存在
    QFile helpFile("help/underwater_calibrator.qhc");
    if (!helpFile.exists()) {
        emit statusChanged(tr("帮助文件不存在，请检查安装"));
        return false;
    }
    
    // 显示首页
    showHelpContent("index");
    return true;
}

void HelpModule::showHelpContent(const QString &topic)
{
    if (!m_helpEngine) return;
    
    QTextBrowser* helpBrowser = findChild<QTextBrowser*>("helpBrowser");
    if (helpBrowser) {
        QUrl url = m_helpEngine->findFile(QUrl("qthelp://com.underwater.calibrator/doc/" + topic + ".html"));
        if (url.isValid()) {
            helpBrowser->setSource(url);
        } else {
            helpBrowser->setHtml(tr("<h3>找不到帮助内容</h3><p>无法找到主题 '%1' 的帮助内容。</p>").arg(topic));
            emit statusChanged(tr("找不到帮助主题: %1").arg(topic));
        }
    }
}

void HelpModule::showAboutDialog()
{
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle(tr("关于水下相机标定系统"));
    aboutDialog.setMinimumWidth(400);
    
    QVBoxLayout* layout = new QVBoxLayout(&aboutDialog);
    
    QLabel* logoLabel = new QLabel();
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setPixmap(QPixmap(":/icons/app_logo.png").scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(logoLabel);
    
    QLabel* titleLabel = new QLabel(tr("<h2>水下相机标定系统</h2>"));
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    QLabel* versionLabel = new QLabel(tr("版本: 1.0.0"));
    versionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(versionLabel);
    
    QTextEdit* infoEdit = new QTextEdit();
    infoEdit->setReadOnly(true);
    infoEdit->setHtml(tr("<p>水下相机标定系统是一款专业的相机标定软件，"
                         "适用于水下环境中的相机内参和外参标定。</p>"
                         "<p>主要功能包括：</p>"
                         "<ul>"
                         "<li>相机设备管理与参数配置</li>"
                         "<li>标定图像采集与预处理</li>"
                         "<li>相机内参和畸变系数计算</li>"
                         "<li>标定结果验证与误差分析</li>"
                         "<li>标定报告生成与导出</li>"
                         "</ul>"
                         "<p>&copy; 2023 水下成像技术实验室</p>"));
    infoEdit->setMinimumHeight(200);
    layout->addWidget(infoEdit);
    
    QPushButton* okButton = new QPushButton(tr("确定"));
    connect(okButton, &QPushButton::clicked, &aboutDialog, &QDialog::accept);
    layout->addWidget(okButton);
    
    aboutDialog.exec();
}

void HelpModule::onContentsItemClicked(const QModelIndex &index)
{
    QHelpContentModel* model = m_helpEngine->contentModel();
    if (!model) return;
    
    QHelpContentItem* item = model->contentItemAt(index);
    if (item) {
        QTextBrowser* helpBrowser = findChild<QTextBrowser*>("helpBrowser");
        if (helpBrowser) {
            helpBrowser->setSource(item->url());
        }
    }
}

void HelpModule::onSearchButtonClicked()
{
    QLineEdit* searchEdit = findChild<QLineEdit*>("searchEdit");
    if (searchEdit && !searchEdit->text().isEmpty()) {
        QString query = searchEdit->text();
        m_helpEngine->searchEngine()->search(query);
        emit statusChanged(tr("正在搜索: %1").arg(query));
    }
}

void HelpModule::onTopicChanged(const QUrl &url)
{
    if (url.isValid()) {
        QString topic = url.fileName().replace(".html", "");
        emit statusChanged(tr("查看帮助: %1").arg(topic));
    }
}

void HelpModule::onShowIndexClicked()
{
    QTabWidget* navTab = findChild<QTabWidget*>();
    if (navTab) {
        navTab->setCurrentIndex(1); // 索引标签
    }
}

void HelpModule::onShowContentsClicked()
{
    QTabWidget* navTab = findChild<QTabWidget*>();
    if (navTab) {
        navTab->setCurrentIndex(0); // 目录标签
    }
}

void HelpModule::onShowSearchClicked()
{
    QTabWidget* navTab = findChild<QTabWidget*>();
    if (navTab) {
        navTab->setCurrentIndex(2); // 搜索标签
    }
}

void HelpModule::onAboutButtonClicked()
{
    showAboutDialog();
}
