#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <vector>

#include <vtkVector.h>

#include <gui/data_view/AbstractRenderView.h>


template<typename T> class QFutureWatcher;
class QProgressBar;

class vtkDataSet;

class RendererImplementationResidual;
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

    DataObject * observationData();
    DataObject * modelData();
    DataObject * residualData();

    int observationUnitDecimalExponent() const;
    void setObservationUnitDecimalExponent(int exponent);
    int modelUnitDecimalExponent() const;
    void setModelUnitDecimalExponent(int exponent);

    void setInSARLineOfSight(const vtkVector3d & los);
    const vtkVector3d & inSARLineOfSight() const;

    enum class InterpolationMode
    {
        modelToObservation,
        observationToModel
    };

    void setInterpolationMode(InterpolationMode mode);
    InterpolationMode interpolationMode() const;

    /** Blocks until current residual computation finished. */
    void waitForResidualUpdate();

public:
    ResidualVerificationView(DataMapping & dataMapping, int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~ResidualVerificationView() override;

    void update();

    ContentType contentType() const override;

    void lookAtData(const DataSelection & selection, int subViewIndex = -1) override;
    void lookAtData(const VisualizationSelection & selection, int subViewIndex = -1) override;

    AbstractVisualizedData * visualizationFor(DataObject * dataObject, int subViewIndex = -1) const override;
    int subViewContaining(const AbstractVisualizedData & visualizedData) const override;

    unsigned int numberOfSubViews() const override;

    RendererImplementation & implementation() const override;

protected:
    void initializeRenderContext() override;

    std::pair<QString, std::vector<QString>> friendlyNameInternal() const override;

    void showDataObjectsImpl(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects,
        unsigned int subViewIndex) override;
    void hideDataObjectsImpl(const QList<DataObject *> & dataObjects, int subViewIndex) override;
    QList<DataObject *> dataObjectsImpl(int subViewIndex) const override;
    void prepareDeleteDataImpl(const QList<DataObject *> & dataObjects) override;
    QList<AbstractVisualizedData *> visualizationsImpl(int subViewIndex) const override;

    void axesEnabledChangedEvent(bool enabled) override;

signals:
    void interpolationModeChanged(InterpolationMode mode);
    void lineOfSightChanged(const vtkVector3d & los);
    void unitDecimalExponentsChanged(int observationExponent, int modelExponent);

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

    // common implementation for the public interface functions */
    void setDataHelper(unsigned int subViewIndex, DataObject * dataObject, bool skipResidualUpdate = false);
    /** Low level function, that won't trigger GUI updates.
        dataObject and ownedDataObject: set only one of them, depending on whether this view owns the specific data object
        Set none of them to clear the sub view. */
    void setDataInternal(unsigned int subViewIndex, DataObject * dataObject, std::unique_ptr<DataObject> ownedObject);

    void updateResidualAsync();
    void handleUpdateFinished();

    /** Update the residual view, show a residual if possible, discard old visualization data 
      * @param toDelete delete old visualization data after informing the GUI
      * @return Old residual object that needs to be deleted after informing the GUI */
    void updateResidual();

    void updateGuiAfterDataChange();

    void updateGuiSelection();

    DataObject * dataAt(unsigned int i) const;
    bool setDataAt(unsigned int i, DataObject * dataObject);

    static std::pair<QString, bool> findDataSetAttributeName(vtkDataSet & dataSet, unsigned int inputType);

private:
    QProgressBar * m_progressBar;

    vtkVector3d m_inSARLineOfSight;
    InterpolationMode m_interpolationMode;
    int m_observationUnitDecimalExponent;
    int m_modelUnitDecimalExponent;

    DataObject * m_observationData;
    DataObject * m_modelData;
    bool m_modelEventsDeferred;
    std::unique_ptr<DataObject> m_residual;

    std::unique_ptr<RendererImplementationResidual> m_implementation;
    std::unique_ptr<vtkCameraSynchronization> m_cameraSync;

    std::array<std::unique_ptr<AbstractVisualizedData>, numberOfViews> m_visualizations;
    std::array<std::pair<QString, bool>, numberOfViews> m_attributeNamesLocations;
    std::array<QString, numberOfViews> m_projectedAttributeNames;

    std::unique_ptr<QFutureWatcher<void>> m_updateWatcher;
    std::mutex m_updateMutex;
    /** Lock the mutex in updateResidualAsync, move the lock here and unlock it only in 
      * handleUpdateFinished. This way, only a single update invocation is possible at a time. */
    std::unique_lock<std::mutex> m_updateMutexLock;
    vtkSmartPointer<vtkDataSet> m_newResidual;
    std::unique_ptr<DataObject> m_oldResidualToDeleteAfterUpdate;
    std::vector<std::unique_ptr<AbstractVisualizedData>> m_visToDeleteAfterUpdate;
    bool m_destructorCalled;

private:
    Q_DISABLE_COPY(ResidualVerificationView)
};
