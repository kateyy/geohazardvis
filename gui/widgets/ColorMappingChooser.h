#pragma once

#include <QDockWidget>
#include <QList>

#include <vtkSmartPointer.h>

#include <gui/gui_api.h>


class vtkLookupTable;
class vtkEventQtSlotConnect;

class Ui_ColorMappingChooser;
class AbstractRenderView;
class ColorMapping;
class RendererImplementationBase3D;

class GUI_API ColorMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    ColorMappingChooser(QWidget * parent = nullptr);
    ~ColorMappingChooser() override;

    vtkLookupTable * selectedGradient() const;

    void setCurrentRenderView(AbstractRenderView * renderView = nullptr);

signals:
    void renderSetupChanged();

private slots:
    void scalarsSelectionChanged(QString scalarsName);
    void gradientSelectionChanged(int selection);
    void componentChanged(int guiComponent);
    void minValueChanged(double value);
    void maxValueChanged(double value);
    void resetMinToData();
    void resetMaxToData();
    void selectNanColor();

    void rebuildGui();

    void on_legendPositionComboBox_currentIndexChanged(QString position);
    void colorLegendPositionChanged();

private:
    void loadGradientImages();
    int gradientIndex(vtkLookupTable * gradient) const;

    /** A RenderView's implementation and color mapping can change whenever its content changes. */
    void checkRenderViewColorMapping();

    void updateTitle(QString rendererName = "");
    void updateGuiValueRanges();

    static vtkSmartPointer<vtkLookupTable> buildLookupTable(const QImage & image);

private:
    Ui_ColorMappingChooser * m_ui;

    QList<vtkSmartPointer<vtkLookupTable>> m_gradients;

    AbstractRenderView * m_renderView;
    RendererImplementationBase3D * m_renderViewImpl;
    ColorMapping * m_mapping;
    /** check if we are moving the actor or if the user interacts */
    bool m_movingColorLegend;
    vtkSmartPointer<vtkEventQtSlotConnect> m_colorLegendConnects;
    QList<QMetaObject::Connection> m_qtConnect;
};
