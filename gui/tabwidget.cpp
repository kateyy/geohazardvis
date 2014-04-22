﻿#include "tabwidget.h"

#include <cassert>

#include <QPushButton>
#include <QTabBar>
#include <QHBoxLayout>
#include <QDebug>

TabWidget::TabWidget(QWidget * parent)
: QTabWidget(parent)
{
}

void TabWidget::tabInserted(int index)
{
    QWidget * buttonContainer = new QWidget();

    QPushButton * undockButton = new QPushButton(buttonContainer);
    undockButton->setText("/\\");
    undockButton->setGeometry(0, 0, 20, 20);
    connect(undockButton, &QPushButton::clicked, this, &TabWidget::popOutButtonClicked);

    QPushButton * closeButton = new QPushButton(buttonContainer);
    closeButton->setText("X");
    closeButton->setGeometry(23, 0, 20, 20);
    connect(closeButton, &QPushButton::released, this, &TabWidget::closeButtonReleased);

    buttonContainer->setGeometry(0, 0, 43, 20);

    tabBar()->setTabButton(index, QTabBar::ButtonPosition::RightSide, buttonContainer);
}

void TabWidget::popOutButtonClicked(bool checked)
{
    assert(sender());
    QWidget * widget = dynamic_cast<QWidget*>(sender()->parent());
    assert(widget);

    int index = tabBar()->tabAt(QPoint(widget->geometry().x(), widget->geometry().y()));
    assert(index >= 0);

    emit tabPopOutClicked(index);
}

void TabWidget::closeButtonReleased()
{
    assert(sender());
    QWidget * widget = dynamic_cast<QWidget*>(sender()->parent());
    assert(widget);

    int index = tabBar()->tabAt(QPoint(widget->geometry().x(), widget->geometry().y()));
    assert(index >= 0);

    emit tabCloseRequested(index);
}
