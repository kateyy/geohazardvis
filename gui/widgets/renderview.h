#pragma once

#include <QDockWidget>

class vtkRenderWindow;

class Ui_RenderView;

class RenderView : public QDockWidget
{
public:
    RenderView(QWidget * parent = nullptr);

    vtkRenderWindow * renderWindow();
    const vtkRenderWindow * renderWindow() const;

private:
    Ui_RenderView * m_ui;
};
