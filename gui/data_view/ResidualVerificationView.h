#pragma once

#include <QVector>

#include <vtkSmartPointer.h>

#include <gui/data_view/AbstractRenderView.h>


class QVTKWidget;
class QComboBox;

class ColorMapping;
class ImageDataObject;
class RendererImplementationBase3D;
class RenderViewStrategyImage2D;


class GUI_API ResidualVerificationView : public AbstractRenderView
{
    Q_OBJECT

public:
    ResidualVerificationView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~ResidualVerificationView() override;

    QString friendlyName() const override;

    ContentType contentType() const override;

    DataObject * selectedData() const override;
    AbstractVisualizedData * selectedDataVisualization() const override;
    void lookAtData(DataObject * dataObject, vtkIdType itemId) override;

    AbstractVisualizedData * visualizationFor(DataObject * dataObject, int subViewIndex = -1) const override;

    /** Convenience API to set the images for the use case specific sub-view.
        These functions have the same effect as calling removeDataObject(currentImage) and
        addDataObject(newImage) for the respective sub-views. */
    void setObservationData(ImageDataObject * observation);
    void setModelData(ImageDataObject * model);
    void setResidualData(ImageDataObject * residual);

    unsigned int numberOfSubViews() const override;

    vtkRenderWindow * renderWindow() override;
    const vtkRenderWindow * renderWindow() const override;

    RendererImplementation & implementation() const override;

public slots:
    void render() override;

protected:
    void showEvent(QShowEvent * event) override;

    QWidget * contentWidget() override;

    void highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId) override;

    void addDataObjectsImpl(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects,
        unsigned int subViewIndex) override;
    void hideDataObjectsImpl(const QList<DataObject *> & dataObjects, unsigned int subViewIndex) override;
    void removeDataObjectsImpl(const QList<DataObject *> & dataObjects) override;
    QList<AbstractVisualizedData *> visualizationsImpl(int subViewIndex) const override;

    void axesEnabledChangedEvent(bool enabled) override;

private:
    void initialize();

    // low level function, that won't trigger GUI updates
    void setData(unsigned int subViewIndex, ImageDataObject * dataObject);
    void updateResidual();

    void updateGuiSelection();
    void updateComboBoxes();
    void updateObservationFromUi(int index);
    void updateModelFromUi(int index);

private:
    QVTKWidget * m_qvtkMain;
    QComboBox * m_observationCombo;
    QComboBox * m_modelCombo;
    bool m_disableGuiUpdate;

    RendererImplementationBase3D * m_implementation;
    RenderViewStrategyImage2D * m_strategy;
    QVector<ImageDataObject *> m_images;
    QVector<AbstractVisualizedData *> m_visualizations;
};
