#pragma once

#include <gui/data_view/AbstractDataView.h>


class RenderView;
class ImageDataObject;


class GUI_API ResidualVerificationView : public AbstractDataView
{
public:
    ResidualVerificationView(int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);

    bool isTable() const override;
    bool isRenderer() const override;

    QString friendlyName() const override;

    void setObservationData(ImageDataObject * observation);
    void setModelData(ImageDataObject * model);

protected:
    void showEvent(QShowEvent * event) override;

    QWidget * contentWidget() override;

    void highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId) override;

private:
    void initialize();

    void updateResidual();

private:
    QList<RenderView *> m_renderViews;

    ImageDataObject * m_observation;
    ImageDataObject * m_model;
    ImageDataObject * m_residual;
};
