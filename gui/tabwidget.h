#pragma once

#include <QTabWidget>

class QPushButton;

class TabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit TabWidget(QWidget * parent = nullptr);

signals:
    void tabPopOutClicked(int index);

protected:
    virtual void tabInserted(int index) override;

protected slots:
    void popOutButtonClicked(bool checked);
    void closeButtonReleased();

};
