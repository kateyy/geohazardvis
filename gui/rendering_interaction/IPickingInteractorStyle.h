#pragma once

#include <QObject>

#include <vtkType.h>

#include <gui/gui_api.h>


template<typename T> class QList;

class DataObject;
class AbstractVisualizedData;


class GUI_API IPickingInteractorStyle : public QObject
{
    Q_OBJECT

signals:
    void pointInfoSent(const QStringList &info) const;
    /** rendered data at the mouse position, after releasing (and not moving) the left mouse button
        This is only emitted if a actor was picked at the mouse position. */
    void dataPicked(AbstractVisualizedData * visualizedData);
    void indexPicked(DataObject * dataObject, vtkIdType index) const;

public:
    virtual void highlightIndex(DataObject * dataObject, vtkIdType index) = 0;
    virtual void lookAtIndex(DataObject * dataObject, vtkIdType index) = 0;

    virtual vtkIdType highlightedIndex() const = 0;
    virtual DataObject * highlightedDataObject() const = 0;
};
