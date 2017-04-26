#include "t_QVTKWidget.h"

#include <cassert>

#include <QEvent>
#include <QPointer>

#include <QVTKInteractor.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>

#include <core/OpenGLDriverFeatures.h>
#include <core/vtkCommandExt.h>
#include <core/utility/macros.h>


namespace
{

#if !defined(OPTION_USE_QVTKOPENGLWIDGET)
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
#endif

}


class t_QVTKWidgetObserver : public vtkCommand
{
public:
    static t_QVTKWidgetObserver * New() { return new t_QVTKWidgetObserver(); }
    vtkBaseTypeMacro(t_QVTKWidgetObserver, vtkCommand);

    t_QVTKWidgetObserver()
        : vtkCommand()
        , InRepaint{ false }
    {
    }

    void SetTarget(t_QVTKWidget * target) { this->Target = target; }

    void Execute(vtkObject * DEBUG_ONLY(subject), unsigned long eventId, void * /*callData*/) VTK_OVERRIDE
    {
#if defined(OPTION_USE_QVTKOPENGLWIDGET)
        assert(subject == this->Target->RenderWindow.Get());
#else
        assert(subject == this->Target->GetRenderWindow());
#endif

        switch (eventId)
        {
        case vtkCommandExt::ForceRepaintEvent:
#if defined(OPTION_USE_QVTKOPENGLWIDGET)
            // From QWidget::repaint() documentation:
            // "If you call repaint() in a function which may itself be called from paintEvent(),
            // you may get infinite recursion."
            if (!this->InRepaint)
            {
                struct SetUnsetBool
                {
                    explicit SetUnsetBool(bool & value)  : value{ value } { value = true; }
                    ~SetUnsetBool() { value = false; }
                private:
                    bool & value;
                };

                const SetUnsetBool setUnset{ this->InRepaint };
                this->Target->repaint();
            }
#else
            this->Target->update();
#endif
            break;
        }
    }

private:
    QPointer<t_QVTKWidget> Target;
    bool InRepaint;
};


void t_QVTKWidget::initializeDefaultSurfaceFormat()
{
#if defined(OPTION_USE_QVTKOPENGLWIDGET)
    auto format = QVTKOpenGLWidget::defaultFormat();
    format.setSamples(0);
    // This is required for Intel HD 3000 (and similar?) broken Windows drivers.
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setOption(QSurfaceFormat::DeprecatedFunctions, true);
    QSurfaceFormat::setDefaultFormat(format);
#endif
}

#if defined(OPTION_USE_QVTKOPENGLWIDGET)
t_QVTKWidget::t_QVTKWidget(QWidget * parent, Qt::WindowFlags f)
    : Superclass(parent, f)
    , ToolTipWasShown{ false }
    , Observer{ vtkSmartPointer<t_QVTKWidgetObserver>::New() }
{
    this->Observer->SetTarget(this);
}
#else
t_QVTKWidget::t_QVTKWidget(QWidget * parent, Qt::WindowFlags f)
    : Superclass(defaultQVTKFormat(), parent, nullptr, f)
    , Observer{ vtkSmartPointer<t_QVTKWidgetObserver>::New() }
{
#if defined(__linux__)
    auto renWin = GetRenderWindow();
    renWin->MakeCurrent();
    renWin->OpenGLInitContext();
#endif

    this->Observer->SetTarget(this);
}
#endif

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
#if defined(OPTION_USE_QVTKOPENGLWIDGET)
    vtkSmartPointer<vtkRenderWindow> previousWindow = this->RenderWindow;
#else
    vtkSmartPointer<vtkRenderWindow> previousWindow = this->mRenWin;
#endif
    const bool windowChanged = renderWindow != previousWindow.Get();

    if (windowChanged && previousWindow)
    {
        previousWindow->RemoveObserver(this->Observer);

        if (renderWindow)
        {
            renderWindow->SetDPI(previousWindow->GetDPI());
        }
    }

    Superclass::SetRenderWindow(renderWindow);

#if !defined(OPTION_USE_QVTKOPENGLWIDGET)
    // QGLWidget::OpenGLInitContext is only called once when setting up the widget, but not after
    // switching to a different vtkRenderWindow. However, vtkOpenGLRenderWindow::OpenGLInitContext
    // sets up VTK GLEW states etc.

    if (renderWindow && windowChanged)
    {
        makeCurrent();
        renderWindow->OpenGLInitContext();
    }
#endif

    if (windowChanged && renderWindow)
    {
        renderWindow->AddObserver(vtkCommandExt::ForceRepaintEvent, this->Observer);
    }
}

#if defined(OPTION_USE_QVTKOPENGLWIDGET)
vtkRenderWindow * t_QVTKWidget::GetRenderWindow()
{
    if (auto renWin = Superclass::GetRenderWindow())
    {
        return renWin;
    }
    vtkNew<vtkGenericOpenGLRenderWindow> renWin;
    SetRenderWindow(renWin.Get());
    return Superclass::GetRenderWindow();
}
#endif

bool t_QVTKWidget::event(QEvent * event)
{
    if (event->type() == QEvent::ToolTip)
    {
        emit beforeTooltipPopup();
#if defined (OPTION_USE_QVTKOPENGLWIDGET)
        this->ToolTipWasShown = true;
#endif
    }

#if defined (OPTION_USE_QVTKOPENGLWIDGET)
    else if (this->ToolTipWasShown)
    {
        // Tool tips are implemented as child widgets. When leaving the widget, this child widget
        // is removed. In some cases, this leads to artifacts when the widget layout is refreshed.
        // Events are emitted as follows:
        //      Leave (-> layout refresh), ChildRemoved (-> tool tip gone)
        // In most cases, updating on Leave fixes the rendering errors. But in some cases, the
        // widget update seems to be triggered later on. This is fixed by calling update() in Enter
        // again (which is not optimal, but works for most cases).
        switch (event->type())
        {
        case QEvent::Leave:
            update();
            break;
        case QEvent::Enter:
            update();
            this->ToolTipWasShown = false;
            break;
        default:
            break;
        }
    }
#endif

    return Superclass::event(event);
}

#if defined(OPTION_USE_QVTKOPENGLWIDGET)
void t_QVTKWidget::paintGL()
{
    OpenGLDriverFeatures::initializeInCurrentContext();

    if (!Superclass::GetRenderWindow())
    {
        return;
    }
    Superclass::paintGL();

    OpenGLDriverFeatures::setFeaturesAfterPaintGL(this->RenderWindow);
}
#endif
