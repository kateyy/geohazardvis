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

#pragma once

#include <core/config.h>
#if defined(OPTION_USE_QVTKOPENGLWIDGET)
#include <QVTKOpenGLWidget.h>
using t_QVTKWidget_superclass = QVTKOpenGLWidget;
#else
#include <QVTKWidget2.h>
using t_QVTKWidget_superclass = QVTKWidget2;
#endif

#include <gui/gui_api.h>
#include <gui/data_view/t_QVTKWidgetFwd.h>


class vtkRenderWindow;
class vtkRenderWindowInteractor;

class t_QVTKWidgetObserver;


/**
 * Qt Widget providing a OpenGL context and surface for VTK.
 *
 * The base class used for this widget is defined by CMake options:
 *  - OPTION_USE_QVTKOPENGLWIDGET
 *
 * \note Call t_QVTKWidget::initializeDefaultSurfaceFormat() in your application main() before
 *  creating a QApplication when using OpenGL features in VTK. This ensures that OpenGL context
 *  features required by VTK are available.
 */
class GUI_API t_QVTKWidget : public t_QVTKWidget_superclass
{
    Q_OBJECT

public:
    using Superclass = t_QVTKWidget_superclass;

    /** Call this once in the application setup before creating a QApplication. */
    static void initializeDefaultSurfaceFormat();

    t_QVTKWidget(QWidget * parent = nullptr, Qt::WindowFlags f = {});
    ~t_QVTKWidget() override;

    /** Convenience class to prevent inclusion of headers such as vtkGenericOpenGLRenderWindow. */
    vtkRenderWindow * GetRenderWindowBase();

    /** Convenience class to prevent inclusion of headers such as QVTKInteractor. */
    vtkRenderWindowInteractor * GetInteractorBase();

#if defined(OPTION_USE_QVTKOPENGLWIDGET)
    /**
     * Note: This function is not virtual in QVTKOpenGLWidget, so users MUST call this function
     * from a t_QVTKWidget in order to dispatch to the correct implementation.
     */
    void SetRenderWindow(vtkGenericOpenGLRenderWindow * renderWindow);
#else
    void SetRenderWindow(vtkGenericOpenGLRenderWindow * renderWindow) override;
#endif
#if defined(OPTION_USE_QVTKOPENGLWIDGET)
    vtkRenderWindow * GetRenderWindow() override;
#endif

signals:
    void beforeTooltipPopup();

protected:
    bool event(QEvent * event) override;

#if defined(OPTION_USE_QVTKOPENGLWIDGET)
    void paintGL() override;
#endif

    void updateRenderWindowDPI();

private:
    /**
     * Initialization code that requires virtual function calls and can thus not be called in the
     * constructor.
     * It is executed with the first invocation of event().
     */
    void initialize();

private:
    bool IsInitialized;
    bool InSetRenderWindow;

    QMetaObject::Connection ScreenChangedConnection;
    vtkSmartPointer<t_QVTKWidgetObserver> Observer;
    friend class t_QVTKWidgetObserver;

private:
    Q_DISABLE_COPY(t_QVTKWidget)
};
