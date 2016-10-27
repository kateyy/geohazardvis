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

    bool isClosed() const;

signals:
    void closed();

protected:
    /** Call this in subclass destructors to make sure that closed() is signaled before the object 
      * gets invalidated.
      * This is only required to fix cases where the widget is never shown. */
    void signalClosing();

    bool eventFilter(QObject * obj, QEvent * ev) override;
    void closeEvent(QCloseEvent * event) override;
    
private:
    QDockWidget * m_dockWidgetParent;

    bool m_closingReported;

private:
    Q_DISABLE_COPY(DockableWidget)
};
