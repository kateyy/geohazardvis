#include "t_QVTKWidget.h"

//#include <QDebug>
#include <QEvent>


t_QVTKWidget::t_QVTKWidget(QWidget * parent, Qt::WindowFlags f)
    : Superclass(parent, nullptr, f)
{
}

t_QVTKWidget::~t_QVTKWidget() = default;

bool t_QVTKWidget::event(QEvent * event)
{
    if (event->type() == QEvent::ToolTip)
    {
        emit beforeTooltipPopup();

        //auto ev = static_cast<QHelpEvent *>(event);
        //qDebug() << event;
    }

    return Superclass::event(event);
}
