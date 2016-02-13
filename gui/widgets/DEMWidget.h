#pragma once

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
class ImageDataObject;
class PolyDataObject;
class Ui_DEMWidget;


class GUI_API DEMWidget : public DockableWidget
{
public:
    /** @param previewRenderer specify a render view that will be used to preview the new topography. If nullptr, a new view will be opened. */
    explicit DEMWidget(DataMapping & dataMapping, AbstractRenderView * previewRenderer = nullptr, QWidget * parent = nullptr, Qt::WindowFlags f = {});
    ~DEMWidget() override;

public:
    void showPreview();
    bool save();
    void saveAndClose();

    void resetParametersForCurrentInputs();

private:
    void setupPipeline();

    void rebuildTopoPreviewData();
    void configureVisualizations();

    ImageDataObject * currentDEM();
    PolyDataObject * currentTopoTemplate();
    /** Lazy computation of the template radius. Assume a template centered at (0, 0, z), 
    * search for the point with the highest distance to the center. */
    double currentTopoTemplateRadius();

    void updateAvailableDataSets();

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

    QList<PolyDataObject *> m_topographyMeshes;
    QList<ImageDataObject *> m_dems;

    DataBounds m_previousDEMBounds;

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

    ImageDataObject * m_lastDemSelection;
    PolyDataObject * m_lastTopoTemplateSelection;
    double m_lastTopoTemplateRadius;
};
