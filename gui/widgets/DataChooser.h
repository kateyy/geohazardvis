#pragma once

#include <QDockWidget>
#include <QList>

#include <vtkSmartPointer.h>


class vtkLookupTable;

class Ui_DataChooser;
class ScalarToColorMapping;

class DataChooser : public QDockWidget
{
    Q_OBJECT

public:
    DataChooser(QWidget * parent = nullptr);
    ~DataChooser() override;

    /** setup UI for a named rendered and a configured scalar to color mapping */
    void setMapping(QString rendererName = "", ScalarToColorMapping * mapping = nullptr);
    const ScalarToColorMapping * mapping() const;

    vtkLookupTable * selectedGradient() const;

signals:
    void renderSetupChanged();

private slots:
    void scalarsSelectionChanged(QString scalarsName);
    void gradientSelectionChanged(int selection);
    void minValueChanged(double value);
    void maxValueChanged(double value);

private:
    void loadGradientImages();
    int gradientIndex(vtkLookupTable * gradient) const;

    void updateWindowTitle(QString objectName = "");
    void rebuildGui(ScalarToColorMapping * newMapping);
    void updateGuiValueRanges();

    static vtkLookupTable * buildLookupTable(const QImage & image);

private:
    Ui_DataChooser * m_ui;

    QList<vtkSmartPointer<vtkLookupTable>> m_gradients;

    ScalarToColorMapping * m_mapping;
};
