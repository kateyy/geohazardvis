#include "TableView.h"

#include <QMouseEvent>


void TableView::mouseDoubleClickEvent(QMouseEvent * event)
{
    emit mouseDoubleClicked(
        columnAt(event->pos().x()), 
        rowAt(event->pos().y()));
}
