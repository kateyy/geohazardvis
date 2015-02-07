#include <QList>
#include <QWidget>

#include <vtkSmartPointer.h>

#include <gui/gui_api.h>


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
    Q_OBJECT

public:
    DEMWidget(QWidget * parent = nullptr, Qt::WindowFlags f = 0);
    ~DEMWidget() override;

public slots:
    bool save();
    void saveAndClose();

protected:
    void showEvent(QShowEvent * event) override;

private:
    void updatePreview();
    void setupDEMStages();

    void updateDEMGeoPosition();
    void updateMeshScale();
    void updateView();

private:
    Ui_DEMWidget * m_ui;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;

    QList<PolyDataObject *> m_surfacesMeshes;
    QList<ImageDataObject *> m_dems;

    vtkSmartPointer<vtkImageChangeInformation> m_demTranslate;
    vtkSmartPointer<vtkImageChangeInformation> m_demScale;
    vtkSmartPointer<vtkTransformFilter> m_meshTransform;
    vtkSmartPointer<vtkWarpScalar> m_demWarpElevation;

    PolyDataObject * m_dataPreview;
    RenderedData * m_renderedPreview;
};
