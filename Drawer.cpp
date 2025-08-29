#include "Drawer.h"
#include <QPropertyAnimation>

Drawer::Drawer(QWidget *parent)
    : QWidget(parent), m_open(true)
{
    setFixedWidth(0);
    m_anim = new QPropertyAnimation(this, "maximumWidth");
    m_anim->setDuration(220);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
}

// 按钮触发：侧边栏收缩tt
void Drawer::toggle()
{
    m_open = !m_open;
    m_anim->setStartValue(m_open ? 0 : 220);
    m_anim->setEndValue(m_open ? 220 : 0);
    m_anim->start();
}
