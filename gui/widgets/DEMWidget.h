#include <QList>
#include <QWidget>

#include <vtkSmartPointer.h>

#include <gui/gui_api.h>


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

private:
    vtkSmartPointer<vtkRenderer> m_renderer;
    Ui_DEMWidget * m_ui;

    QList<PolyDataObject *> m_surfacesMeshes;
    QList<ImageDataObject *> m_dems;

    vtkSmartPointer<vtkImageShiftScale> m_demTransform;
    vtkSmartPointer<vtkTransformFilter> m_meshTransform;
    vtkSmartPointer<vtkWarpScalar> m_demWarpElevation;

    PolyDataObject * m_dataPreview;
    RenderedData * m_renderedPreview;
};
