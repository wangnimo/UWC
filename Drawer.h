#ifndef DRAWER_H
#define DRAWER_H
#include <QWidget>
#include <QPropertyAnimation>

class Drawer : public QWidget
{
    Q_OBJECT
public:
    explicit Drawer(QWidget *parent = nullptr);
    void toggle();
private:
    bool m_open;
    class QPropertyAnimation *m_anim;
};
#endif // DRAWER_H
