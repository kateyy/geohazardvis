#pragma once

#include <functional>
#include <memory>

#include <QList>
#include <QPointer>

#include <vtkSmartPointer.h>

#include <core/utility/DataExtent.h>
#include <gui/widgets/DockableWidget.h>


class vtkAlgorithm;
class vtkImageShiftScale;
class vtkTransform;

class AbstractRenderView;
class DataMapping;
class DataObject;
class DataSetFilter;
class DataSetHandler;
class ImageDataObject;
class PolyDataObject;
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

public:
    /** Show a preview for the currently configured topography and setup default visualization parameters */
    void showPreview();
    bool save();
    void saveAndClose();

    void resetParametersForCurrentInputs();

private:
    using t_filterFunction = std::function<bool(DataObject *, const DataSetHandler &)>;
    explicit DEMWidget(DataMapping & dataMapping, AbstractRenderView * previewRenderer,
        QWidget * parent, Qt::WindowFlags f,
        t_filterFunction topoDataSetFilter,
        t_filterFunction demDataSetFilter);

    void setupPipeline();

    ImageDataObject * currentDEM();
    PolyDataObject * currentTopoTemplate();
    /** Lazy computation of the template radius. Assume a template centered at (0, 0, z), 
    * search for the point with the highest distance to the center. */
    double currentTopoTemplateRadius();

    /** Update the output topography according to input parameters and data.
      * @return true if the function built a new preview data set */
    bool updateData();
    /** If currently a render view is opened call forceUpdatePreview */
    void updatePreview();
    /** Calls updateData, opens a new render view if required, and updates available visualizations */
    void forceUpdatePreview();
    void configureVisualizations();

    void releasePreviewData();

    void updateMeshTransform();
    void updatePipeline();
    void updateView();

    void matchTopoMeshRadius();
    void centerTopoMesh();
    void updateTopoUIRanges();
    void updateForChangedTransformParameters();

private:
    DataMapping & m_dataMapping;

    std::unique_ptr<Ui_DEMWidget> m_ui;

    std::unique_ptr<DataSetFilter> m_topographyMeshes;
    std::unique_ptr<DataSetFilter> m_dems;

    vtkSmartPointer<vtkAlgorithm> m_demPipelineStart;
    vtkSmartPointer<vtkImageShiftScale> m_demUnitScale;
    vtkSmartPointer<vtkAlgorithm> m_meshPipelineStart;
    vtkSmartPointer<vtkTransform> m_meshTransform;
    vtkSmartPointer<vtkAlgorithm> m_pipelineEnd;

    double m_demUnitDecimalExponent;
    double m_topoRadius;
    vtkVector2d m_topoShiftXY;

    QPointer<AbstractRenderView> m_previewRenderer;

    std::unique_ptr<PolyDataObject> m_dataPreview;
    bool m_topoRebuildRequired;

    DataObject * m_lastPreviewedDEM;
    ImageDataObject * m_demSelection;
    PolyDataObject * m_topoTemplateSelection;
    double m_lastTopoTemplateRadius;
};
