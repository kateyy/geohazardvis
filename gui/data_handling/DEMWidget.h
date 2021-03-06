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

#include <functional>
#include <memory>
#include <vector>

#include <QPointer>

#include <vtkSmartPointer.h>

#include <gui/widgets/DockableWidget.h>


class vtkAlgorithm;
class vtkPassArrays;
class vtkVector2d;

class AbstractRenderView;
struct CoordinateSystemType;
class DataMapping;
class DataObject;
class DataSetFilter;
class DataSetHandler;
class DEMToTopographyMesh;
class ImageDataObject;
class PolyDataObject;
struct ReferencedCoordinateSystemSpecification;
class Ui_DEMWidget;


class GUI_API DEMWidget : public DockableWidget
{
public:
    /** @param previewRenderer specify a render view that will be used to preview the new topography. If nullptr, a new view will be opened. */
    explicit DEMWidget(DataMapping & dataMapping, AbstractRenderView * previewRenderer = nullptr, QWidget * parent = nullptr, Qt::WindowFlags f = {});
    /** Configure a topography with fixed input mesh and DEM. Related GUI elements will be disabled. */
    explicit DEMWidget(PolyDataObject & templateMesh, ImageDataObject & dem,
        DataMapping & dataMapping, AbstractRenderView * previewRenderer = nullptr,
        QWidget * parent = nullptr, Qt::WindowFlags f = {});
    ~DEMWidget() override;

    ImageDataObject * dem();
    void setDEM(ImageDataObject * dem);
    PolyDataObject * meshTemplate();
    void setMeshTemplate(PolyDataObject * meshTemplate);

    CoordinateSystemType targetCoordinateSystem() const;
    void setTargetCoordinateSystem(CoordinateSystemType targetCoordinateSystem);

    double topoRadius() const;
    void setTopoRadius(double radius);
    const vtkVector2d & topographyCenterXY() const;
    void setTopographyCenterXY(const vtkVector2d & center);
    int demUnitScaleExponent() const;
    void setDEMUnitScaleExponent(int exponent);

    bool centerTopographyMesh() const;
    void setCenterTopographyMesh(bool doCenter);

    /** Show a preview for the currently configured topography and setup default visualization parameters */
    void showPreview();
    /** Save the topography mesh as currently configured and pass it to the data set handler. */
    bool save();
    /** Call save(), and if successful close the widget afterwards. */
    void saveAndClose();
    /** Create the topography mesh for the current configuration (as in save()), and return the data object.
    * This will not pass the data object to the data set handler.
    * In case of errors, this will NOT show any message boxes (non-GUI function).
    * @return an empty pointer, if input data is missing or invalid. */
    std::unique_ptr<PolyDataObject> saveRelease();

    void matchTopoMeshRadius();
    void centerTopoMesh();

    void resetParametersForCurrentInputs();

private:
    using t_filterFunction = std::function<bool(DataObject *, const DataSetHandler &)>;
    explicit DEMWidget(DataMapping & dataMapping, AbstractRenderView * previewRenderer,
        QWidget * parent, Qt::WindowFlags f,
        t_filterFunction topoDataSetFilter,
        t_filterFunction demDataSetFilter);

    void setupPipeline();

    ImageDataObject * currentDEMChecked();
    PolyDataObject * currentTopoTemplateChecked();

    ReferencedCoordinateSystemSpecification targetCoordsDEMSpec();
    ReferencedCoordinateSystemSpecification targetCoordsTopoSpec();
    void updateForChangedDEMCoordinateSystem();

    /** Pass input DEM and mesh template to the pipeline, as far as available */
    void setPipelineInputs();
    /** Prepare output data object, if pipeline inputs are set */
    bool updatePreviewDataObject();
    /** Trigger pipeline updates while locking the output data object */
    void updatePipeline();
    /** Pass DEM and transformed topography mesh to the preview renderer if applicable */
    void updatePreviewRendererContents();
    /** Set default visualization parameters on DEM and topography mesh, reset to a default view */
    void configureDEMVisualization();
    void configureMeshVisualization();

    /** Update pipeline inputs and UI ranges, labels etc. If a preview renderer is opened, also 
    * also update preview data and visualizations. */
    void updatePreview();

    void releasePreviewData();

    void applyUIChanges();

    std::vector<QSignalBlocker> uiSignalBlockers();

    static vtkSmartPointer<vtkPassArrays> createMeshCleanupFilter();

    bool checkIfRadiusChanged() const;
    bool checkIfShiftChanged() const;

    void updateTargetCoordsComboForCurrentDEM();

private:
    DataMapping & m_dataMapping;

    std::unique_ptr<Ui_DEMWidget> m_ui;

    std::unique_ptr<DataSetFilter> m_topographyMeshes;
    std::unique_ptr<DataSetFilter> m_dems;

    int m_demUnitDecimalExponent;
    /** Mesh shift and radius are invalid, because the input data sets changed */
    bool m_meshParametersInvalid;
    /** Due to changed input data sets, the output/preview data set needs to be rebuilt */
    bool m_previewRebuildRequired;

    vtkSmartPointer<vtkAlgorithm> m_meshPipelineStart;
    vtkSmartPointer<DEMToTopographyMesh> m_demToTopoFilter;
    vtkSmartPointer<vtkPassArrays> m_cleanupOutputMeshAttributes;


    QPointer<AbstractRenderView> m_previewRenderer;

    std::unique_ptr<PolyDataObject> m_dataPreview;

    ImageDataObject * m_lastPreviewedDEM;
    ImageDataObject * m_demSelection;
    PolyDataObject * m_lastPreviewedTopo;
    PolyDataObject * m_topoTemplateSelection;

private:
    Q_DISABLE_COPY(DEMWidget)
};
