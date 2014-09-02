#pragma once

#include <QDockWidget>


class AbstractDataView : public QDockWidget
{
    Q_OBJECT

public:
    AbstractDataView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);

    int index() const;

    virtual bool isTable() const = 0;
    virtual bool isRenderer() const = 0;

signals:
    /** signaled when the widget receive the keyboard focus (focusInEvent) */
    void focused(AbstractDataView * tableView);
    void closed();

protected:
    virtual QWidget * contentWidget() = 0;

    void showEvent(QShowEvent * event) override;
    void focusInEvent(QFocusEvent * event) override;
    void focusOutEvent(QFocusEvent * event) override;
    void closeEvent(QCloseEvent * event) override;

    bool eventFilter(QObject * obj, QEvent * ev) override;

private:
    const int m_index;
    bool m_initialized;
};
