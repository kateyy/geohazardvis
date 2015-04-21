#pragma once

#include <vtkSmartPointer.h>

#include <gui/data_view/RenderViewStrategy.h>


class QAction;
class vtkLineWidget2;
class vtkEventQtSlotConnect;

class AbstractRenderView;
class ImageDataObject;
class ImageProfileData;


class GUI_API RenderViewStrategyImage2D : public RenderViewStrategy
{
    Q_OBJECT

public:
    RenderViewStrategyImage2D(RendererImplementation3D & context);
    ~RenderViewStrategyImage2D() override;

    QString name() const override;

    void activate() override;
    void deactivate() override;

    bool contains3dData() const override;

    void resetCamera(vtkCamera & camera) override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const override;
    bool canApplyTo(const QList<RenderedData *> & renderedData) override;

private slots:
    void startProfilePlot();
    void acceptProfilePlot();
    void abortProfilePlot();

    void lineMoved();

    void checkSourceExists();

private:
    void initialize();

private:
    static const bool s_isRegistered;

    bool m_isInitialized;

    QAction * m_profilePlotAction;
    QAction * m_profilePlotAcceptAction;
    QAction * m_profilePlotAbortAction;
    QList<QAction *> m_actions;
    ImageProfileData * m_previewProfile;
    AbstractRenderView * m_previewRenderer;

    ImageDataObject * m_currentPlottingImage;

    vtkSmartPointer<vtkLineWidget2> m_lineWidget;
    vtkSmartPointer<vtkEventQtSlotConnect> m_vtkConnect;
};
