#include "t_QVTKWidget.h"

#include <QEvent>

#include <QVTKInteractor.h>
#include <vtkGenericOpenGLRenderWindow.h>


namespace
{

QGLFormat defaultQVTKFormat()
{
    auto format = QVTKWidget2::GetDefaultVTKFormat();
    format.setOption(
        QGL::FormatOption::DeprecatedFunctions
        | QGL::FormatOption::DoubleBuffer
        | QGL::FormatOption::DepthBuffer
        | QGL::FormatOption::AlphaChannel
    );
    format.setProfile(QGLFormat::CompatibilityProfile);
    return format;
}

}

t_QVTKWidget::t_QVTKWidget(QWidget * parent, Qt::WindowFlags f)
    : Superclass(defaultQVTKFormat(), parent, nullptr, f)
{
#if defined(__linux__)
    auto renWin = GetRenderWindow();
    renWin->MakeCurrent();
    renWin->OpenGLInitContext();
#endif
}

t_QVTKWidget::~t_QVTKWidget() = default;

vtkRenderWindow * t_QVTKWidget::GetRenderWindowBase()
{
    return GetRenderWindow();
}

vtkRenderWindowInteractor * t_QVTKWidget::GetInteractorBase()
{
    return GetInteractor();
}

void t_QVTKWidget::SetRenderWindow(vtkGenericOpenGLRenderWindow * renderWindow)
{
    const bool windowChanged = renderWindow != this->mRenWin;

    Superclass::SetRenderWindow(renderWindow);

    // QGLWidget::OpenGLInitContext is only called once when setting up the widget, but not after
    // switching to a different vtkRenderWindow. However, vtkOpenGLRenderWindow::OpenGLInitContext
    // sets up VTK GLEW states etc.

    if (renderWindow && windowChanged)
    {
        makeCurrent();
        renderWindow->OpenGLInitContext();
    }
}

bool t_QVTKWidget::event(QEvent * event)
{
    if (event->type() == QEvent::ToolTip)
    {
        emit beforeTooltipPopup();
    }

    return Superclass::event(event);
}
