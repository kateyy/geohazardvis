#pragma once

#include <QObject>

#include <vtkType.h>


template<typename T> class QList;

class vtkActor;
class vtkPolyData;

class DataObject;
class RenderedData;


class IPickingInteractorStyle : public QObject
{
    Q_OBJECT

public:
    virtual void setRenderedDataList(const QList<RenderedData *> * renderedData) = 0;

signals:
    void pointInfoSent(const QStringList &info) const;
    /** actor at the mouse position, after releasing (and not moving) the left mouse button
    This is only emitted if a actor was picked at the mouse position. */
    void actorPicked(vtkActor * actor);
    void cellPicked(DataObject * dataObject, vtkIdType cellId) const;

public slots:
    virtual void highlightCell(vtkIdType cellId, DataObject * dataObject) = 0;
    virtual void lookAtCell(vtkPolyData * polyData, vtkIdType cellId) = 0;
};
