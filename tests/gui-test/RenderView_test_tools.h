#pragma once

#include <QObject>


class DataObject;
class AbstractRenderView;


class SignalHelper : public QObject
{
    Q_OBJECT

public:
    SignalHelper();

    void emitQueuedDelete(DataObject * object, AbstractRenderView * renderView);
    void emitRepeatedQueuedDelete(DataObject * object, AbstractRenderView * renderView);

signals:
    void deleteObject(DataObject * object, AbstractRenderView * renderView);
    void deleteObjectRepeat(DataObject * object, AbstractRenderView * renderView);

private slots:
    void do_deleteObject(DataObject * object, AbstractRenderView * renderView);
    void do_deleteObjectRepeat(DataObject * object, AbstractRenderView * renderView);
};
