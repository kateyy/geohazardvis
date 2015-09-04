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

signals:
    void deleteObject(DataObject * object, AbstractRenderView * renderView);

private slots:
    void do_deleteObject(DataObject * object, AbstractRenderView * renderView);
};
