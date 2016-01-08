#pragma once

#include <functional>
#include <memory>

#include <QObject>
#include <QMap>
#include <QStringList>

#include <vtkType.h>

#include <core/t_QVTKWidgetFwd.h>
#include <gui/gui_api.h>


template<typename T> class QList;
class vtkIdTypeArray;
class vtkRenderWindowInteractor;

class AbstractRenderView;
class AbstractVisualizedData;
enum class ContentType;
enum class IndexType;
class DataObject;
class DataMapping;


class GUI_API RendererImplementation : public QObject
{
    Q_OBJECT

public:
    explicit RendererImplementation(AbstractRenderView & renderView);
    ~RendererImplementation() override;

    AbstractRenderView & renderView() const;
    DataMapping & dataMapping() const;

    virtual QString name() const = 0;

    virtual ContentType contentType() const = 0;

    /** @param dataObjects Check if we can render these objects with the current view content.
        @param incompatibleObjects Fills this list with discarded objects.
        @return compatible objects that we will render. */
    virtual QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) = 0;

    const QStringList & supportedInteractionStrategies() const;
    void setInteractionStrategy(const QString & strategyName);
    const QString & currentInteractionStrategy() const;

    virtual void activate(t_QVTKWidget & qvtkWidget);
    virtual void deactivate(t_QVTKWidget & qvtkWidget);

    virtual void render() = 0;

    virtual vtkRenderWindowInteractor * interactor() = 0;

    virtual unsigned int subViewIndexAtPos(const QPoint pixelCoordinate) const;

    /** add newly created rendered data
        actual data visibility depends on the object's configuration */
    void addContent(AbstractVisualizedData * content, unsigned int subViewIndex);
    /** remove all references to the object and its contents */
    void removeContent(AbstractVisualizedData * content, unsigned int subViewIndex);
    /** This is called once after a set of data objects was removed or added to the render view */
    void renderViewContentsChanged();

    /** mark dataObject (and, if set, it's point/cell) as current selection */
    virtual void setSelectedData(AbstractVisualizedData * vis, vtkIdType index, IndexType indexType) = 0;
    virtual void setSelectedData(AbstractVisualizedData * vis, vtkIdTypeArray & indices, IndexType indexType) = 0;
    virtual void clearSelection() = 0;
    virtual AbstractVisualizedData * selectedData() const = 0;
    virtual vtkIdType selectedIndex() const = 0;
    virtual IndexType selectedIndexType() const = 0;
    /** Move camera so that the specified part of the visualization becomes visible. */
    virtual void lookAtData(AbstractVisualizedData & vis, vtkIdType itemId, IndexType indexType, unsigned int subViewIndex) = 0;

    /** Resets the camera so that all view content are visible.
        Moves back to the initial position if requested. */
    virtual void resetCamera(bool toInitialPosition, unsigned int subViewIndex) = 0;

    virtual void setAxesVisibility(bool visible) = 0;

    virtual bool canApplyTo(const QList<DataObject *> & data) = 0;

    virtual std::unique_ptr<AbstractVisualizedData> requestVisualization(DataObject & dataObject) const = 0;

    using ImplementationConstructor = std::function<std::unique_ptr<RendererImplementation>(AbstractRenderView & view)>;
    static const QList<ImplementationConstructor> & constructors();

signals:
    void dataSelectionChanged(AbstractVisualizedData * selectedData);

    void interactionStrategyChanged(const QString & strategyName);
    void supportedInteractionStrategiesChanged(const QStringList & stategyNames);

protected:

    /** Override to add visual contents to the view.
        In the overrider, call addConnectionForContent for each Qt signal/slot 
        connection related to this content */
    virtual void onAddContent(AbstractVisualizedData * content, unsigned int subViewIndex) = 0;
    virtual void onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex) = 0;
    virtual void onRenderViewContentsChanged();

    /** Call in sub-classes, to update the list of supported interaction strategies
    * @param strategyName Names of supported strategies. These names must be unique. 
    * @param currentStrategy Set the strategy that will be used now. Must be contained in strategyNames. */
    void setSupportedInteractionStrategies(const QStringList & strategyNames, const QString & currentStrategy);
    /** Subclasses should implement this, if they support multiple interaction styles.
    * Implement the actual switch to the new strategy here. */
    virtual void updateForCurrentInteractionStrategy(const QString & strategyName);

    void addConnectionForContent(AbstractVisualizedData * content,
        const QMetaObject::Connection & connection);

    template <typename ImplType> static bool registerImplementation();

protected:
    AbstractRenderView & m_renderView;

private:
    static QList<ImplementationConstructor> & s_constructors();

    QMap<AbstractVisualizedData *, QList<QMetaObject::Connection>> m_visConnections;

    QStringList m_supportedInteractionStrategies;
    QString m_currentInteractionStrategy;
};

template <typename ImplType>
bool RendererImplementation::registerImplementation()
{
    s_constructors().append(
        [](AbstractRenderView & view) { return std::make_unique<ImplType>(view); }
    );

    return true;
}
