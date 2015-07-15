#pragma once

#include <memory>
#include <vector>

#include <vtkVector.h>

#include <gui/data_view/AbstractRenderView.h>


class QVTKWidget;
class QComboBox;

class ColorMapping;
class ImageDataObject;
class PolyDataObject;
class RendererImplementationBase3D;
class RenderViewStrategy;


class GUI_API ResidualVerificationView : public AbstractRenderView
{
    Q_OBJECT

public:
    ResidualVerificationView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~ResidualVerificationView() override;

    QString friendlyName() const override;

    ContentType contentType() const override;

    DataObject * selectedData() const override;
    AbstractVisualizedData * selectedDataVisualization() const override;
    void lookAtData(DataObject * dataObject, vtkIdType itemId) override;

    AbstractVisualizedData * visualizationFor(DataObject * dataObject, int subViewIndex = -1) const override;

    /** Convenience API to set the images for the use case specific sub-view.
        These functions have the same effect as calling removeDataObject(currentImage) and
        addDataObject(newImage) for the respective sub-views. */
    void setObservationData(ImageDataObject * observation);
    void setModelData(DataObject * model);
    void setResidualData(DataObject * residual);

    void setInSARLineOfSight(const vtkVector3d & los);
    const vtkVector3d & inSARLineOfSight() const;

    void setInterpolateModelOnObservation(bool modelToObservation);
    bool interpolatemodelOnObservation() const;

    unsigned int numberOfSubViews() const override;

    vtkRenderWindow * renderWindow() override;
    const vtkRenderWindow * renderWindow() const override;

    RendererImplementation & implementation() const override;

public:
    void render() override;

protected:
    void showEvent(QShowEvent * event) override;

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

signals:
    void lineOfSightChanged(const vtkVector3d & los);

private:
    void initialize();

    // common implementation for the public interface functions
    // @delayGuiUpdate set to true and delete the objects appended to toDelete yourself, if you intend to call this function multiple times
    void setDataHelper(unsigned int subViewIndex, DataObject * data, bool delayGuiUpdate = false, std::vector<std::unique_ptr<AbstractVisualizedData>> * toDelete = nullptr);
    /** Low level function, that won't trigger GUI updates.
        @param toDelete Delete these objects after removing them from dependent components (GUI etc) */
    void setDataInternal(unsigned int subViewIndex, DataObject * dataObject, std::vector<std::unique_ptr<AbstractVisualizedData>> & toDelete);
    /** Update the residual view, show a residual if possible, discard old visualization data 
      * @param toDelete delete old visualization data after informing the GUI */
    void updateResidual(std::vector<std::unique_ptr<AbstractVisualizedData>> & toDelete);

    void updateGuiAfterDataChange();

    void updateGuiSelection();
    void updateComboBoxes();
    void updateObservationFromUi(int index);
    void updateModelFromUi(int index);

private:
    QVTKWidget * m_qvtkMain;
    QComboBox * m_observationCombo;
    QComboBox * m_modelCombo;

    vtkVector3d m_inSARLineOfSight;
    bool m_interpolateModelOnObservation;

    std::unique_ptr<RendererImplementationBase3D> m_implementation;
    RenderViewStrategy * m_strategy;
    std::vector<DataObject *> m_dataSets;
    std::vector<std::unique_ptr<AbstractVisualizedData>> m_visualizations;
};
