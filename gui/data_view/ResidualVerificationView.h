#pragma once

#include <array>
#include <memory>
#include <vector>

#include <vtkVector.h>

#include <gui/data_view/AbstractRenderView.h>


class QVTKWidget;
class QComboBox;

class ColorMapping;
class ImageDataObject;
class PolyDataObject;
class RendererImplementationResidual;
class RenderViewStrategyImage2D;
class vtkCameraSynchronization;


class GUI_API ResidualVerificationView : public AbstractRenderView
{
    Q_OBJECT

public:
    /** Visualization parameters  */

    /** Convenience API to set the images for the use case specific sub-view.
    These functions have the same effect as calling removeDataObject(currentImage) and
    addDataObject(newImage) for the respective sub-views. */
    void setObservationData(DataObject * observation);
    void setModelData(DataObject * model);
    void setResidualData(DataObject * residual);

    void setInSARLineOfSight(const vtkVector3d & los);
    const vtkVector3d & inSARLineOfSight() const;

    enum class InterpolationMode
    {
        modelToObservation,
        observationToModel
    };

    void setInterpolationMode(InterpolationMode mode);
    InterpolationMode interpolationMode() const;

public:
    ResidualVerificationView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~ResidualVerificationView() override;

    QString friendlyName() const override;
    QString subViewFriendlyName(unsigned int subViewIndex) const override;

    ContentType contentType() const override;

    DataObject * selectedData() const override;
    AbstractVisualizedData * selectedDataVisualization() const override;
    void lookAtData(DataObject * dataObject, vtkIdType itemId, int subViewIndex = -1) override;

    AbstractVisualizedData * visualizationFor(DataObject * dataObject, int subViewIndex = -1) const override;

    unsigned int numberOfSubViews() const override;

    vtkRenderWindow * renderWindow() override;
    const vtkRenderWindow * renderWindow() const override;

    RendererImplementation & implementation() const override;

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
    enum : unsigned int
    {
        observationIndex = 0,
        modelIndex = 1,
        residualIndex = 2,
        numberOfViews
    };

private:
    void initialize();

    // common implementation for the public interface functions
    // @delayGuiUpdate set to true and delete the objects appended to toDelete yourself, if you intend to call this function multiple times
    void setDataHelper(unsigned int subViewIndex, DataObject * data, bool delayGuiUpdate = false, std::vector<std::unique_ptr<AbstractVisualizedData>> * toDelete = nullptr);
    /** Low level function, that won't trigger GUI updates.
        @param toDelete Delete these objects after removing them from dependent components (GUI etc) */
    void setDataInternal(unsigned int subViewIndex, DataObject * dataObject, std::vector<std::unique_ptr<AbstractVisualizedData>> & toDelete);
    /** Update the residual view, show a residual if possible, discard old visualization data 
      * @param toDelete delete old visualization data after informing the GUI
      * @return Old residual object that needs to be deleted after informing the GUI */
    std::unique_ptr<DataObject> updateResidual(std::vector<std::unique_ptr<AbstractVisualizedData>> & toDelete);

    void updateGuiAfterDataChange();

    void updateGuiSelection();
    void updateComboBoxes();
    void updateObservationFromUi(int index);
    void updateModelFromUi(int index);

    DataObject * dataAt(unsigned int i) const;
    bool setDataAt(unsigned int i, DataObject * data);

private:
    QVTKWidget * m_qvtkMain;
    QComboBox * m_observationCombo;
    QComboBox * m_modelCombo;

    vtkVector3d m_inSARLineOfSight;
    InterpolationMode m_interpolationMode;
    double m_observationUnitFactor;
    double m_modelUnitFactor;

    DataObject * m_observationData;
    DataObject * m_modelData;
    std::unique_ptr<DataObject> m_residual;

    std::unique_ptr<RendererImplementationResidual> m_implementation;
    std::unique_ptr<RenderViewStrategyImage2D> m_strategy;
    std::unique_ptr<vtkCameraSynchronization> m_cameraSync;

    std::array<std::unique_ptr<AbstractVisualizedData>, 3> m_visualizations;
};
