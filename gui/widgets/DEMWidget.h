#pragma once

#include <memory>

#include <QList>

#include <vtkSmartPointer.h>

#include <core/utility/DataExtent.h>
#include <gui/widgets/DockableWidget.h>


class vtkAlgorithm;
class vtkImageShiftScale;
class vtkRenderer;
class vtkTransform;
class vtkWarpScalar;

class vtkGridAxes3DActor;

class DataSetHandler;
class ImageDataObject;
class PolyDataObject;
class RenderedData;
class Ui_DEMWidget;


class GUI_API DEMWidget : public DockableWidget
{
public:
    explicit DEMWidget(DataSetHandler & dataSetHandler, QWidget * parent = nullptr, Qt::WindowFlags f = 0);
    ~DEMWidget() override;

public:
    bool save();
    void saveAndClose();

protected:
    void showEvent(QShowEvent * event) override;

private:
    void rebuildPreview();
    void setupPipeline();

    void updateAvailableDataSets();

    void updateMeshTransform();
    void updateView();

    void matchTopoMeshRadius();
    void centerTopoMesh();

private:
    DataSetHandler & m_dataSetHandler;

    std::unique_ptr<Ui_DEMWidget> m_ui;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkGridAxes3DActor> m_axesActor;

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

    std::unique_ptr<PolyDataObject> m_dataPreview;
    std::unique_ptr<RenderedData> m_renderedPreview;
};
