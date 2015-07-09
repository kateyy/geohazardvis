#pragma once

#include <memory>

#include <QList>
#include <QWidget>

#include <vtkSmartPointer.h>

#include <gui/gui_api.h>


class vtkAlgorithm;
class vtkCubeAxesActor;
class vtkImageChangeInformation;
class vtkImageShiftScale;
class vtkRenderer;
class vtkTransformFilter;
class vtkWarpScalar;


class ImageDataObject;
class PolyDataObject;
class RenderedData;
class Ui_DEMWidget;


class GUI_API DEMWidget : public QWidget
{
public:
    DEMWidget(QWidget * parent = nullptr, Qt::WindowFlags f = 0);
    ~DEMWidget() override;

public:
    bool save();
    void saveAndClose();

protected:
    void showEvent(QShowEvent * event) override;

private:
    void updatePreview();
    void setupDEMStages();

    void updateAvailableDataSets();

    void updateDEMGeoPosition();
    void updateMeshScale();
    void updateView();

private:
    Ui_DEMWidget * m_ui;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;

    QList<PolyDataObject *> m_surfaceMeshes;
    QList<ImageDataObject *> m_dems;

    vtkSmartPointer<vtkImageChangeInformation> m_demTranslate;
    vtkSmartPointer<vtkImageChangeInformation> m_demScale;
    vtkSmartPointer<vtkAlgorithm> m_demTransformOutput;
    vtkSmartPointer<vtkTransformFilter> m_meshTransform;
    vtkSmartPointer<vtkWarpScalar> m_demWarpElevation;

    ImageDataObject * m_currentDEM;
    QString m_demScalarsName;
    std::unique_ptr<PolyDataObject> m_dataPreview;
    std::unique_ptr<RenderedData> m_renderedPreview;
};
