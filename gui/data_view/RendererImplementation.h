#pragma once

#include <QObject>
#include <QList>

#include <vtkType.h>


class vtkRenderWindow;
class vtkRenderWindowInteractor;
class QVTKWidget;

class AbstractVisualizedData;
enum class ContentType;
class RenderView;
class DataObject;


class RendererImplementation : public QObject
{
    Q_OBJECT

public:
    RendererImplementation(RenderView & renderView, QObject * parent = nullptr);
    RenderView & renderView() const;

    virtual ContentType contentType() const = 0;

    /** @param dataObjects Check if we can render these objects with the current view content.
        @param incompatibleObjects Fills this list with discarded objects.
        @return compatible objects that we will render. */
    virtual QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const = 0;

    virtual void apply(QVTKWidget * qvtkWidget) = 0;

    virtual void render() = 0;

    virtual vtkRenderWindowInteractor * interactor() = 0;

    /** add newly created rendered data
        actual data visibility depends on the object's configuration */
    virtual void addContent(AbstractVisualizedData * content) = 0;
    /** remove all references to the object and its contents */
    virtual void removeContent(AbstractVisualizedData * content) = 0;

    /** mark dataObject (and, if set, it's point/cell) as current selection */
    virtual void highlightData(DataObject * dataObject, vtkIdType itemId = -1) = 0;
    virtual DataObject * highlightedData() = 0;
    virtual void lookAtData(DataObject * dataObject, vtkIdType itemId) = 0;

    /** Resets the camera so that all view content are visible.
        Moves back to the initial position if requested. */
    virtual void resetCamera(bool toInitialPosition) = 0;

    virtual void setAxesVisibility(bool visible) = 0;

protected:
    friend class RenderView;
    virtual AbstractVisualizedData * requestVisualization(DataObject * dataObject) const = 0;

signals:
    void dataSelectionChanged(AbstractVisualizedData * selectedData);

protected:
    RenderView & m_renderView;
};
