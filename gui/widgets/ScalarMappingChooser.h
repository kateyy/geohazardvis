#pragma once

#include <QDockWidget>
#include <QList>

#include <vtkSmartPointer.h>


class vtkLookupTable;
class vtkEventQtSlotConnect;

class Ui_ScalarMappingChooser;
class ScalarToColorMapping;
class RenderView;

class ScalarMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    ScalarMappingChooser(QWidget * parent = nullptr);
    ~ScalarMappingChooser() override;

    /** setup UI for a named rendered and a configured scalar to color mapping */
    void setMapping(RenderView * renderView = nullptr, ScalarToColorMapping * mapping = nullptr);
    const ScalarToColorMapping * mapping() const;

    vtkLookupTable * selectedGradient() const;

signals:
    void renderSetupChanged();

private slots:
    void scalarsSelectionChanged(QString scalarsName);
    void gradientSelectionChanged(int selection);
    void minValueChanged(double value);
    void maxValueChanged(double value);

    void rebuildGui();

    void rearrangeDataObjects();

    void on_legendPositionComboBox_currentIndexChanged(QString position);
    void colorLegendPositionChanged();
    void on_colorLegendGroupBox_toggled(bool on);

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
    /** check if we are moving the actor or if the user interacts */
    bool m_movingColorLegend;
    vtkSmartPointer<vtkEventQtSlotConnect> m_colorLegendConnects;
};
