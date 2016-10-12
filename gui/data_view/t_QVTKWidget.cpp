#include "t_QVTKWidget.h"

#include <QEvent>

#include <QVTKInteractor.h>
#include <vtkGenericOpenGLRenderWindow.h>


namespace
{

QGLFormat defaultQVTKFormat()
{
    auto format = QVTKWidget2::GetDefaultVTKFormat();
    format.setSamples(1);
    format.setOption(
        QGL::FormatOption::DeprecatedFunctions
        | QGL::FormatOption::DoubleBuffer
        | QGL::FormatOption::DepthBuffer
        | QGL::FormatOption::AlphaChannel
    );
    return format;
}

}

t_QVTKWidget::t_QVTKWidget(QWidget * parent, Qt::WindowFlags f)
    : Superclass(defaultQVTKFormat(), parent, nullptr, f)
{
    auto renWin = GetRenderWindow();
    renWin->MakeCurrent();
    renWin->OpenGLInitContext();
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

bool t_QVTKWidget::event(QEvent * event)
{
    if (event->type() == QEvent::ToolTip)
    {
        emit beforeTooltipPopup();
    }

    return Superclass::event(event);
}
