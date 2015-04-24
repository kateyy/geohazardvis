#pragma once

#include <QList>
#include <QMap>

#include <vtkSmartPointer.h>

#include <gui/data_view/AbstractRenderView.h>


class Ui_RenderView;
class RendererImplementationSwitch;


class GUI_API RenderView : public AbstractRenderView
{
    Q_OBJECT

public:
    RenderView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~RenderView() override;

    QString friendlyName() const override;

    ContentType contentType() const override;

    DataObject * selectedData() const override;
    AbstractVisualizedData * selectedDataVisualization() const override;
    void lookAtData(DataObject * dataObject, vtkIdType itemId) override;

    AbstractVisualizedData * visualizationFor(DataObject * dataObject, int subViewIndex = -1) const override;

    vtkRenderWindow * renderWindow() override;
    const vtkRenderWindow * renderWindow() const override;

    // remove from public interface as soon as possible
    RendererImplementation & implementation() const override;

public slots:
    void render() override;

protected:
    QWidget * contentWidget() override;

    void highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId) override;

    void addDataObjectsImpl(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects,
        unsigned int subViewIndex) override;
    void hideDataObjectsImpl(const QList<DataObject *> & dataObjects, unsigned int subViewIndex) override;
    void removeDataObjectsImpl(const QList<DataObject *> & dataObjects) override;

    QList<AbstractVisualizedData *> visualizationsImpl(int subViewIndex) const override;

    void axesEnabledChangedEvent(bool enabled) override;

private:
    void updateImplementation(const QList<DataObject *> & contents);

    // data handling

    AbstractVisualizedData * addDataObject(DataObject * dataObject);
    void removeDataObject(DataObject * dataObject);

    // remove some data objects from internal lists
    // @return list of dangling rendered data object that you have to delete.
    QList<AbstractVisualizedData *> removeFromInternalLists(QList<DataObject *> dataObjects = {});


private slots:
    /** update configuration widgets to focus on my content. */
    void updateGuiForContent();
    void updateGuiForSelectedData(AbstractVisualizedData * content);
    void updateGuiForRemovedData();

private:
    Ui_RenderView * m_ui;
    RendererImplementationSwitch * m_implementationSwitch;

    // rendered representations of data objects for this view
    QList<AbstractVisualizedData *> m_contents;
    // objects that were loaded to the GPU but are currently not rendered 
    QList<AbstractVisualizedData *> m_contentCache;
    QMap<DataObject *, AbstractVisualizedData *> m_dataObjectToVisualization;
};
