#include "tabwidget.h"

#include <cassert>

#include <QPushButton>
#include <QTabBar>
#include <QDebug>

TabWidget::TabWidget(QWidget * parent)
: QTabWidget(parent)
{
}

void TabWidget::tabInserted(int index)
{
    QPushButton * button = new QPushButton();

    button->setText("_");
    button->setGeometry(0, 0, 20, 20);

    connect(button, &QPushButton::clicked, this, &TabWidget::popOutButtonClicked);

    tabBar()->setTabButton(index, QTabBar::ButtonPosition::RightSide, button);
}

void TabWidget::popOutButtonClicked(bool checked)
{
    QWidget * button = dynamic_cast<QWidget*>(sender());
    assert(button);

    int index = tabBar()->tabAt(QPoint(button->geometry().x(), button->geometry().y()));
    assert(index >= 0);

    emit tabPopOutClicked(index);
}
