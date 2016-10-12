#pragma once

#include <QVTKWidget2.h>

#include <gui/gui_api.h>
#include <gui/data_view/t_QVTKWidgetFwd.h>


class vtkRenderWindow;
class vtkRenderWindowInteractor;


class GUI_API t_QVTKWidget : public QVTKWidget2
{
    Q_OBJECT

public:
    using Superclass = QVTKWidget2;

    t_QVTKWidget(QWidget * parent = nullptr, Qt::WindowFlags f = {});
    ~t_QVTKWidget() override;

    /** Convenience class to prevent inclusion of headers such as vtkGenericOpenGLRenderWindow. */
    vtkRenderWindow * GetRenderWindowBase();

    /** Convenience class to prevent inclusion of headers such as QVTKInteractor. */
    vtkRenderWindowInteractor * GetInteractorBase();

signals:
    void beforeTooltipPopup();

protected:
    bool event(QEvent * event) override;

private:
    Q_DISABLE_COPY(t_QVTKWidget)
};
