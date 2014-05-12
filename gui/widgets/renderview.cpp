#include "renderview.h"
#include "ui_renderview.h"

RenderView::RenderView(QWidget * parent)
: QDockWidget(parent)
, m_ui(new Ui_RenderView())
{
    m_ui->setupUi(this);
}

vtkRenderWindow * RenderView::renderWindow()
{
    return m_ui->qvtkMain->GetRenderWindow();
}

const vtkRenderWindow * RenderView::renderWindow() const
{
    return m_ui->qvtkMain->GetRenderWindow();
}
