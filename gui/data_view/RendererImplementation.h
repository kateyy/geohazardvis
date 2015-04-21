#pragma once

#include <functional>

#include <QObject>
#include <QList>
#include <QMap>

#include <vtkType.h>


class vtkRenderWindow;
class vtkRenderWindowInteractor;
class QVTKWidget;

class AbstractRenderView;
class AbstractVisualizedData;
enum class ContentType;
class DataObject;


class RendererImplementation : public QObject
{
    Q_OBJECT

public:
    RendererImplementation(AbstractRenderView & renderView, QObject * parent = nullptr);
    ~RendererImplementation() override;

    AbstractRenderView & renderView() const;

    virtual QString name() const = 0;

    virtual ContentType contentType() const = 0;

    /** @param dataObjects Check if we can render these objects with the current view content.
        @param incompatibleObjects Fills this list with discarded objects.
        @return compatible objects that we will render. */
    virtual QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) = 0;

    virtual void activate(QVTKWidget * qvtkWidget);
    virtual void deactivate(QVTKWidget * qvtkWidget);

    virtual void render() = 0;

    virtual vtkRenderWindowInteractor * interactor() = 0;

    /** add newly created rendered data
        actual data visibility depends on the object's configuration */
    void addContent(AbstractVisualizedData * content);
    /** remove all references to the object and its contents */
    void removeContent(AbstractVisualizedData * content);

    /** mark dataObject (and, if set, it's point/cell) as current selection */
    virtual void setSelectedData(DataObject * dataObject, vtkIdType itemId = -1) = 0;
    virtual DataObject * selectedData() = 0;
    virtual void lookAtData(DataObject * dataObject, vtkIdType itemId) = 0;

    /** Resets the camera so that all view content are visible.
        Moves back to the initial position if requested. */
    virtual void resetCamera(bool toInitialPosition) = 0;

    virtual void setAxesVisibility(bool visible) = 0;

    virtual bool canApplyTo(const QList<DataObject *> & data) = 0;

    virtual AbstractVisualizedData * requestVisualization(DataObject * dataObject) const = 0;

    using ImplementationConstructor = std::function<RendererImplementation *(AbstractRenderView & view)>;
    static const QList<ImplementationConstructor> & constructors();

protected:

    /** Override to add visual contents to the view.
        In the overrider, call addConnectionForContent for each Qt signal/slot 
        connection related to this content */
    virtual void onAddContent(AbstractVisualizedData * content) = 0;
    virtual void onRemoveContent(AbstractVisualizedData * content) = 0;

    void addConnectionForContent(AbstractVisualizedData * content,
        const QMetaObject::Connection & connection);

    template <typename ImplType> static bool registerImplementation();

signals:
    void dataSelectionChanged(AbstractVisualizedData * selectedData);

protected:
    AbstractRenderView & m_renderView;

private:
    static QList<ImplementationConstructor> & s_constructors();

    QMap<AbstractVisualizedData *, QList<QMetaObject::Connection>> m_visConnections;
};

template <typename ImplType>
bool RendererImplementation::registerImplementation()
{
    s_constructors().append(
        [] (AbstractRenderView & view) { return new ImplType(view); }
    );

    return true;
}
