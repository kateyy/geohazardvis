#pragma once

#include <QWidget>

#include <gui/gui_api.h>


class QDockWidget;


class GUI_API DockableWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DockableWidget(QWidget * parent = nullptr, Qt::WindowFlags f = {});
    ~DockableWidget() override;

    /** @return a QDockWidget, that contains the widget, to be embedded in QMainWindows
    * Creates the QDockWidget instance if required. */
    QDockWidget * dockWidgetParent();
    QDockWidget * dockWidgetParentOrNullptr();
    bool hasDockWidgetParent() const;

signals:
    void closed();

protected:
    bool eventFilter(QObject * obj, QEvent * ev) override;
    void closeEvent(QCloseEvent * event) override;
    
private:
    QDockWidget * m_dockWidgetParent;
};
