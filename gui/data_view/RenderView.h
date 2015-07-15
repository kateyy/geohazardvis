#pragma once

#include <memory>
#include <vector>

#include <QList>
#include <QMap>
#include <QSet>

#include <gui/data_view/AbstractRenderView.h>


class Ui_RenderView;
class RendererImplementationSwitch;


class GUI_API RenderView : public AbstractRenderView
{
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

public:
    void render() override;

protected:
    void closeEvent(QCloseEvent * event) override;

    QWidget * contentWidget() override;

    void highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId) override;

    void showDataObjectsImpl(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects,
        unsigned int subViewIndex) override;
    void hideDataObjectsImpl(const QList<DataObject *> & dataObjects, unsigned int subViewIndex) override;
    QList<DataObject *> dataObjectsImpl(int subViewIndex) const override;
    void prepareDeleteDataImpl(const QList<DataObject *> & dataObjects) override;

    QList<AbstractVisualizedData *> visualizationsImpl(int subViewIndex) const override;

    void axesEnabledChangedEvent(bool enabled) override;

private:
    void updateImplementation(const QList<DataObject *> & contents);

    // data handling

    AbstractVisualizedData * addDataObject(DataObject * dataObject);
    void removeDataObject(DataObject * dataObject);

    // remove some data objects from internal lists
    // @return list of dangling rendered data object that you have to delete.
    std::vector<std::unique_ptr<AbstractVisualizedData>> removeFromInternalLists(QList<DataObject *> dataObjects = {});


private:
    /** update configuration widgets to focus on my content. */
    void updateGuiForSelectedData(AbstractVisualizedData * content);
    void updateGuiForRemovedData();

private:
    std::unique_ptr<Ui_RenderView> m_ui;
    std::unique_ptr<RendererImplementationSwitch> m_implementationSwitch;
    bool m_closingRequested;

    // rendered representations of data objects for this view
    std::vector<std::unique_ptr<AbstractVisualizedData>> m_contents;
    // objects that were loaded to the GPU but are currently not rendered 
    std::vector<std::unique_ptr<AbstractVisualizedData>> m_contentCache;
    QMap<DataObject *, AbstractVisualizedData *> m_dataObjectToVisualization;
    // DataObjects, that emitted deleted() and that we didn't remove yet
    QSet<DataObject *> m_deletedData;
};
