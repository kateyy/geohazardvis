#include "RenderView_test_tools.h"

#include <core/data_objects/DataObject.h>
#include <gui/data_view/RenderView.h>


SignalHelper::SignalHelper()
    : QObject()
{
    connect(this, &SignalHelper::deleteObject, this, &SignalHelper::do_deleteObject, Qt::QueuedConnection);
    connect(this, &SignalHelper::deleteObjectRepeat, this, &SignalHelper::do_deleteObjectRepeat, Qt::QueuedConnection);
}

void SignalHelper::emitQueuedDelete(DataObject * object, AbstractRenderView * renderView)
{
    emit deleteObject(object, renderView);
}

void SignalHelper::emitRepeatedQueuedDelete(DataObject * object, AbstractRenderView * renderView)
{
    emit deleteObjectRepeat(object, renderView);
}

void SignalHelper::do_deleteObject(DataObject * object, AbstractRenderView * renderView)
{
    renderView->prepareDeleteData({ object });
}

void SignalHelper::do_deleteObjectRepeat(DataObject * object, AbstractRenderView * renderView)
{
    do_deleteObject(object, renderView);

    emit deleteObject(object, renderView);
}
