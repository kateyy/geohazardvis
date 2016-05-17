#pragma once

#include <QObject>

#include <vtkWeakPointer.h>

#include <gui/gui_api.h>


namespace Qt
{
enum CursorShape;
}
class QWidget;
class vtkObject;
class vtkRenderWindow;


class GUI_API RenderWindowCursorCallback : public QObject
{
public:
    explicit RenderWindowCursorCallback(QObject * parent = nullptr);
    ~RenderWindowCursorCallback() override;

    void setRenderWindow(vtkRenderWindow * renderWindow);
    void setQWidget(QWidget * widget);

    static Qt::CursorShape vtkToQtCursor(int vtkCursorId, bool holdingMouse);

private:
    bool eventFilter(QObject * object, QEvent * event) override;

    void callback(vtkObject *, unsigned long, void *);

    void updateQtWidgetCursor(bool forceUpdate = false);

private:
    vtkWeakPointer<vtkRenderWindow> m_renderWindow;
    unsigned long m_observerTag;
    QWidget * m_qWidget;
    int m_currentVtkCursor;

    bool m_holdingMouse;
};
