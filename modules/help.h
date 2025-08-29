#ifndef HELP_MODULE_H
#define HELP_MODULE_H

#include <QWidget>
#include <QTextBrowser>
#include <QHelpEngine>

namespace Ui {
class HelpModule;
}

class HelpModule : public QWidget
{
    Q_OBJECT

public:
    explicit HelpModule(QWidget *parent = nullptr);
    ~HelpModule() override;
    
    // 显示帮助内容
    void showHelpContent(const QString& topic = "index");
    
    // 显示关于对话框
    void showAboutDialog();

signals:
    // 状态变化信号
    void statusChanged(const QString& message);

private slots:
    // UI交互槽函数
    void onContentsItemClicked(const QModelIndex& index);
    void onSearchButtonClicked();
    void onTopicChanged(const QUrl& url);
    void onShowIndexClicked();
    void onShowContentsClicked();
    void onShowSearchClicked();
    void onAboutButtonClicked();

private:
    Ui::HelpModule *ui;
    
    // 帮助引擎
    QHelpEngine* m_helpEngine;
    
    // 初始化UI
    void initUI();
    
    // 初始化帮助引擎
    void initHelpEngine();
    
    // 加载帮助内容
    bool loadHelpContent();
};

#endif // HELP_MODULE_H
