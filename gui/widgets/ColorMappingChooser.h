#pragma once

#include <QDockWidget>
#include <QScopedPointer>
#include <QList>
#include <QMap>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <gui/gui_api.h>


class vtkLookupTable;
class vtkObject;

class Ui_ColorMappingChooser;
class AbstractRenderView;
class ColorMapping;
class RendererImplementationBase3D;
class OrientedScalarBarActor;


class GUI_API ColorMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    ColorMappingChooser(QWidget * parent = nullptr);
    ~ColorMappingChooser() override;

    vtkLookupTable * selectedGradient() const;
    vtkLookupTable * defaultGradient() const;

    void setCurrentRenderView(AbstractRenderView * renderView = nullptr);

signals:
    void renderSetupChanged();

private:
    void guiScalarsSelectionChanged(const QString & scalarsName);
    void guiGradientSelectionChanged(int selection);
    void guiComponentChanged(int guiComponent);
    void guiMinValueChanged(double value);
    void guiMaxValueChanged(double value);
    void guiResetMinToData();
    void guiResetMaxToData();
    void guiSelectNanColor();
    void guiLegendPositionChanged(const QString & position);

    void rebuildGui();

private:
    void colorLegendPositionChanged();
    void updateLegendTitleFont();
    void updateLegendLabelFont();
    void updateLegendConfig();

private:
    void loadGradientImages();
    int gradientIndex(vtkLookupTable * gradient) const;
    int defaultGradientIndex() const;

    /** A RenderView's implementation and color mapping can change whenever its content changes. */
    void checkRenderViewColorMapping();

    void updateTitle(QString rendererName = "");
    void updateGuiValueRanges();

    static vtkSmartPointer<vtkLookupTable> buildLookupTable(const QImage & image);

private:
    QScopedPointer<Ui_ColorMappingChooser> m_ui;

    QList<vtkSmartPointer<vtkLookupTable>> m_gradients;

    AbstractRenderView * m_renderView;
    RendererImplementationBase3D * m_renderViewImpl;
    ColorMapping * m_mapping;
    OrientedScalarBarActor * m_legend;
    /** check if we are moving the actor or if the user interacts */
    bool m_movingColorLegend;
    /** Mapping from subject (color legend coordinate, text property, etc) to observer id */
    QMap<vtkWeakPointer<vtkObject>, int> m_colorLegendObserverIds;
    QList<QMetaObject::Connection> m_qtConnect;
};
