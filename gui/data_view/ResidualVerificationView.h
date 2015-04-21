#pragma once

#include <gui/data_view/AbstractRenderView.h>


class QVTKWidget;

class ImageDataObject;
class RendererImplementation3D;


class GUI_API ResidualVerificationView : public AbstractRenderView
{
    Q_OBJECT

public:
    ResidualVerificationView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);

    QString friendlyName() const override;

    ContentType contentType() const override;

    DataObject * selectedData() const override;
    AbstractVisualizedData * selectedDataVisualization() const override;
    void lookAtData(DataObject * dataObject, vtkIdType itemId) override;

    void setObservationData(ImageDataObject * observation);
    void setModelData(ImageDataObject * model);

    RendererImplementation & selectedViewImplementation() override;

    unsigned int numberOfSubViews() const override;

    vtkRenderWindow * renderWindow() override;
    const vtkRenderWindow * renderWindow() const override;

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

    virtual void axesEnabledChangedEvent(bool enabled) override;

private:
    void initialize();

    void updateResidual();

private:
    vtkSmartPointer<QVTKWidget> m_qvtkMain;
    RendererImplementation3D * m_viewObservation;
    RendererImplementation3D * m_viewModel;
    RendererImplementation3D * m_viewResidual;

    ImageDataObject * m_observation;
    ImageDataObject * m_model;
    ImageDataObject * m_residual;
};
