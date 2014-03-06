#include "renderwidget.h"

#include <cassert>

#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>

RenderWidget::RenderWidget(QWidget* parent, Qt::WindowFlags f)
: QVTKWidget(parent, f)
{
    setAcceptDrops(true);
}

RenderWidget::~RenderWidget()
{
}

void RenderWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void RenderWidget::dropEvent(QDropEvent *event)
{
    assert(event->mimeData()->hasUrls());
    QString filename = event->mimeData()->urls().first().toLocalFile();
    qDebug() << filename;

    emit onInputFileDropped(filename);

    event->acceptProposedAction();
}
