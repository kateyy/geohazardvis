/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <array>
#include <memory>
#include <vector>

#include <vtkVector.h>

#include <core/data_objects/DataObject.h>
#include <gui/data_view/AbstractRenderView.h>


template<typename T> class QFutureWatcher;
class QProgressBar;

class vtkDataSet;
class vtkLookupTable;

class DataSetResidualHelper;
class RendererImplementationResidual;
class vtkCameraSynchronization;


class GUI_API ResidualVerificationView : public AbstractRenderView
{
    Q_OBJECT

public:
    /** Visualization parameters  */

    /**
     * Convenience API to set the images for the use case specific sub-view.
     * These functions have the same effect as calling removeDataObject(currentImage) and
     * addDataObject(newImage) for the respective sub-views.
     */
    void setObservationData(DataObject * observation);
    void setModelData(DataObject * model);

    DataObject * observationData();
    DataObject * modelData();
    DataObject * residualData();

    int observationUnitDecimalExponent() const;
    void setObservationUnitDecimalExponent(int exponent);
    int modelUnitDecimalExponent() const;
    void setModelUnitDecimalExponent(int exponent);

    void setDeformationLineOfSight(const vtkVector3d & los);
    const vtkVector3d & deformationLineOfSight() const;

    enum class InputData
    {
        observation,
        model
    };

    void setResidualGeometrySource(InputData geometrySource);
    InputData residualGeometrySource() const;

    /** Blocks until current residual computation finished. */
    void waitForResidualUpdate();

public:
    ResidualVerificationView(DataMapping & dataMapping, int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~ResidualVerificationView() override;

    void updateResidual();

    ContentType contentType() const override;

    void lookAtData(const DataSelection & selection, int subViewIndex = -1) override;
    void lookAtData(const VisualizationSelection & selection, int subViewIndex = -1) override;

    AbstractVisualizedData * visualizationFor(DataObject * dataObject, int subViewIndex = -1) const override;
    int subViewContaining(const AbstractVisualizedData & visualizedData) const override;

    bool isEmpty() const override;

    unsigned int numberOfSubViews() const override;

    RendererImplementation & implementation() const override;
    /** Convenience method returning the concrete subclass of implementation() superclass API. */
    RendererImplementationResidual & implementationResidual() const;

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

    void onCoordinateSystemChanged(const CoordinateSystemSpecification & spec) override;

signals:
    void inputDataChanged();
    void residualGeometrySourceChanged(InputData geometrySource);
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
    /** common implementation for the public interface functions */
    void setDataHelper(unsigned int subViewIndex, DataObject * dataObject, bool skipResidualUpdate = false);
    /** Low level function, that won't trigger GUI updates. */
    void setInputDataInternal(unsigned int subViewIndex, DataObject * newData);
    void setResidualDataInternal(std::unique_ptr<DataObject> newResidual);
    void updateVisualizationForSubView(unsigned int subViewIndex, DataObject * newData);

    void updateResidualAsync();
    void handleUpdateFinished();
    void updateResidualInternal();

    void updateGuiAfterDataChange();
    void cleanOldGuiData();
    void updateVisualizations();

    void updateGuiSelection();

    DataObject * dataAt(unsigned int i) const;
    void setDataAt(unsigned int i, DataObject * dataObject);

    std::pair<QString, IndexType> findDataSetAttributeName(
        DataObject & dataObject,
        unsigned int inputType) const;

    vtkLookupTable & residualGradient();

private:
    std::unique_ptr<DataSetResidualHelper> m_residualHelper;

    QProgressBar * m_progressBar;

    InputData m_residualGeometrySource;
    int m_observationUnitDecimalExponent;
    int m_modelUnitDecimalExponent;

    std::unique_ptr<RendererImplementationResidual> m_implementation;
    std::unique_ptr<vtkCameraSynchronization> m_cameraSync;

    std::array<std::unique_ptr<AbstractVisualizedData>, numberOfViews> m_visualizations;
    vtkSmartPointer<vtkLookupTable> m_residualGradient;

    std::unique_ptr<QFutureWatcher<void>> m_updateWatcher;
    bool m_inResidualUpdate;
    std::vector<ScopedEventDeferral> m_eventDeferrals;
    std::unique_ptr<DataObject> m_residual;
    std::unique_ptr<DataObject> m_oldResidualToDeleteAfterUpdate;
    std::vector<std::unique_ptr<AbstractVisualizedData>> m_visToDeleteAfterUpdate;
    bool m_deferringVisualizationUpdate;
    bool m_destructorCalled;

private:
    Q_DISABLE_COPY(ResidualVerificationView)
};
