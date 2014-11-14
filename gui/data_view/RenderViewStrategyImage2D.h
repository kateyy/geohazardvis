#pragma once

#include <vtkSmartPointer.h>

#include <gui/data_view/RenderViewStrategy.h>


class QAction;
class QToolBar;
class vtkLineWidget2;
class vtkEventQtSlotConnect;

class ImageProfileData;
class RenderView;


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

private:
    void initialize();

private:
    static const bool s_isRegistered;

    bool m_isInitialized;

    QToolBar * m_toolbar;
    QAction * m_profilePlotAction;
    QAction * m_profilePlotAcceptAction;
    QAction * m_profilePlotAbortAction;
    ImageProfileData * m_previewProfile;
    RenderView * m_previewRenderer;

    vtkSmartPointer<vtkLineWidget2> m_lineWidget;
    vtkSmartPointer<vtkEventQtSlotConnect> m_vtkConnect;
};
