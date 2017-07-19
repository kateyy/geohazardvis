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
