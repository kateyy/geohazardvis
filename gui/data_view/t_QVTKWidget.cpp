/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "t_QVTKWidget.h"

#include <cassert>

#include <QEvent>
#include <QPointer>
#include <QScreen>
#include <QWindow>

#include <QVTKInteractor.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkVersionMacros.h>

#include <core/OpenGLDriverFeatures.h>
#include <core/vtkCommandExt.h>
#include <core/utility/macros.h>


namespace
{

struct SetUnsetBool
{
    explicit SetUnsetBool(bool & value)  : value{ value } { value = true; }
    ~SetUnsetBool() { value = false; }
private:
    bool & value;
};

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
        if (!this->Target)
        {
            return;
        }

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
                const SetUnsetBool setUnset{ this->InRepaint };
#if VTK_CHECK_VERSION(8,0,0)
                this->Target->renderVTK();
#endif
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
    // However, using the compatibility profile brakes some features of the VTK rendering system on
    // Windows, e.g., vtkPlotPoints markers (whatever...).
    // Seems that Intel users must live with such drawbacks.
    if (OpenGLDriverFeatures::isBrokenIntelDriver())
    {
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
    }
    format.setOption(QSurfaceFormat::DeprecatedFunctions, true);

    QSurfaceFormat::setDefaultFormat(format);
#endif
}

#if defined(OPTION_USE_QVTKOPENGLWIDGET)
t_QVTKWidget::t_QVTKWidget(QWidget * parent, Qt::WindowFlags f)
    : Superclass(parent, f)
    , IsInitialized{ false }
    , InSetRenderWindow{ false }
    , Observer{ vtkSmartPointer<t_QVTKWidgetObserver>::New() }
{
    this->Observer->SetTarget(this);
}
#else
t_QVTKWidget::t_QVTKWidget(QWidget * parent, Qt::WindowFlags f)
    : Superclass(defaultQVTKFormat(), parent, nullptr, f)
    , Observer{ vtkSmartPointer<t_QVTKWidgetObserver>::New() }
    , IsInitialized{ false }
    , InSetRenderWindow{ false }

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
    const SetUnsetBool setResetInSetRenderWindow{ this->InSetRenderWindow };

#if defined(OPTION_USE_QVTKOPENGLWIDGET)
    vtkSmartPointer<vtkRenderWindow> previousWindow = this->RenderWindow;
#else
    vtkSmartPointer<vtkRenderWindow> previousWindow = this->mRenWin;
#endif
    const bool windowChanged = renderWindow != previousWindow.Get();

    if (windowChanged && previousWindow)
    {
        previousWindow->RemoveObserver(this->Observer);
    }

    Superclass::SetRenderWindow(renderWindow);

    this->updateRenderWindowDPI();

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
    initialize();

    if (event->type() == QEvent::ToolTip)
    {
        emit beforeTooltipPopup();
    }
    else if (event->type() == QEvent::Show)
    {
        disconnect(this->ScreenChangedConnection);
        if (auto nativeParent = nativeParentWidget())
        {
            this->ScreenChangedConnection = connect(
                nativeParent->windowHandle(), &QWindow::screenChanged,
                [this] () {
                this->updateRenderWindowDPI();
                auto currentRenWin = vtkGenericOpenGLRenderWindow::SafeDownCast(this->Superclass::GetRenderWindow());
                if (currentRenWin
                    && currentRenWin->GetReadyForRendering()
                    && !this->InSetRenderWindow)
                {
                    this->renderVTK();
                }
            });
            this->updateRenderWindowDPI();
        }
    }

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

void t_QVTKWidget::updateRenderWindowDPI()
{
    auto renderWindow = this->Superclass::GetRenderWindow();
    if (!renderWindow)
    {
        return;
    }
    if (auto nativeParent = this->nativeParentWidget())
    {
        const auto dpi = static_cast<int>(nativeParent->windowHandle()->screen()->logicalDotsPerInch());
        renderWindow->SetDPI(dpi);
#if defined(OPTION_USE_QVTKOPENGLWIDGET) && VTK_CHECK_VERSION(8,0,0)
        this->OriginalDPI = dpi;
#endif
    }
}

void t_QVTKWidget::initialize()
{
    if (this->IsInitialized)
    {
        return;
    }

    this->IsInitialized = true;

#if defined(OPTION_USE_QVTKOPENGLWIDGET) && VTK_CHECK_VERSION(8,0,0)
    this->setEnableHiDPI(false);
#endif
}
