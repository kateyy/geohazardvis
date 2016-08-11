#pragma once

#include <QVTKInteractor.h>
#include <QVTKWidget2.h>
#include <vtkGenericOpenGLRenderWindow.h>

#include <gui/gui_api.h>
#include <gui/data_view/t_QVTKWidgetFwd.h>


class GUI_API t_QVTKWidget : public QVTKWidget2
{
    Q_OBJECT

public:
    using Superclass = QVTKWidget2;

    t_QVTKWidget(QWidget * parent = nullptr, Qt::WindowFlags f = {});
    ~t_QVTKWidget() override;

signals:
    void beforeTooltipPopup();

protected:
    bool event(QEvent * event) override;
};
