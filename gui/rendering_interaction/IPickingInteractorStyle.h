#pragma once

#include <QObject>

#include <vtkType.h>


template<typename T> class QList;

class DataObject;
class RenderedData;


class IPickingInteractorStyle : public QObject
{
    Q_OBJECT

public:
    virtual void setRenderedData(QList<RenderedData *> renderedData) = 0;

signals:
    void pointInfoSent(const QStringList &info) const;
    /** rendered data at the mouse position, after releasing (and not moving) the left mouse button
        This is only emitted if a actor was picked at the mouse position. */
    void dataPicked(RenderedData * renderedData);
    void cellPicked(DataObject * dataObject, vtkIdType cellId) const;

public slots:
    virtual void highlightCell(DataObject * dataObject, vtkIdType cellId) = 0;
    virtual void lookAtCell(DataObject * dataObject, vtkIdType cellId) = 0;
};
