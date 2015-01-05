#pragma once

#include <QDockWidget>
#include <QList>

#include <vtkSmartPointer.h>


class vtkLookupTable;
class vtkEventQtSlotConnect;

class Ui_ScalarMappingChooser;
class ScalarToColorMapping;
class RenderView;
class RendererImplementation3D;

class ScalarMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    ScalarMappingChooser(QWidget * parent = nullptr);
    ~ScalarMappingChooser() override;

    vtkLookupTable * selectedGradient() const;

signals:
    void renderSetupChanged();

public slots:
    void setCurrentRenderView(RenderView * renderView = nullptr);

private slots:
    void scalarsSelectionChanged(QString scalarsName);
    void gradientSelectionChanged(int selection);
    void componentChanged(int guiComponent);
    void minValueChanged(double value);
    void maxValueChanged(double value);

    void rebuildGui();

    void on_legendPositionComboBox_currentIndexChanged(QString position);
    void colorLegendPositionChanged();

private:
    void loadGradientImages();
    int gradientIndex(vtkLookupTable * gradient) const;

    void updateTitle(QString rendererName = "");
    void updateGuiValueRanges();

    static vtkLookupTable * buildLookupTable(const QImage & image);

private:
    Ui_ScalarMappingChooser * m_ui;

    QList<vtkSmartPointer<vtkLookupTable>> m_gradients;

    ScalarToColorMapping * m_mapping;
    RenderView * m_renderView;
    RendererImplementation3D * m_renderViewImpl;
    /** check if we are moving the actor or if the user interacts */
    bool m_movingColorLegend;
    vtkSmartPointer<vtkEventQtSlotConnect> m_colorLegendConnects;
    QList<QMetaObject::Connection> m_qtConnect;
};
