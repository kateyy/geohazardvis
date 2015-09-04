#include "RenderView_test_tools.h"

#include <core/data_objects/DataObject.h>
#include <gui/data_view/RenderView.h>


SignalHelper::SignalHelper()
    : QObject()
{
    connect(this, &SignalHelper::deleteObject, this, &SignalHelper::do_deleteObject, Qt::QueuedConnection);
}

void SignalHelper::emitQueuedDelete(DataObject * object, AbstractRenderView * renderView)
{
    emit deleteObject(object, renderView);
}

void SignalHelper::do_deleteObject(DataObject * object, AbstractRenderView * renderView)
{
    renderView->prepareDeleteData({ object });
}
