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

    void resetOutputNameForCurrentInputs();

private:
    void setupPipeline();

    void rebuildTopoPreviewData();
    void configureVisualizations();

    ImageDataObject * currentDEM();
    PolyDataObject * currentTopoTemplate();

    void updateAvailableDataSets();

    void updateMeshTransform();
    void updateView();

    void matchTopoMeshRadius();
    void centerTopoMesh();

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
    double m_topoRadiusScale;
    vtkVector3d m_topoShift;

    QPointer<AbstractRenderView> m_previewRenderer;

    std::unique_ptr<PolyDataObject> m_dataPreview;
    bool m_topoRebuildRequired;

    ImageDataObject * m_lastDemSelection;
    PolyDataObject * m_lastTopoTemplateSelection;
};
