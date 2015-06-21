#pragma once

#include <memory>
#include <vector>

#include <vtkSmartPointer.h>

#include <gui/data_view/RenderViewStrategy.h>


class QAction;
class vtkLineWidget2;
class vtkEventQtSlotConnect;

class AbstractRenderView;
class DataObject;
class ImageDataObject;
class ImageProfileData;


class GUI_API RenderViewStrategyImage2D : public RenderViewStrategy
{
    Q_OBJECT

public:
    RenderViewStrategyImage2D(RendererImplementationBase3D & context, QObject * parent = nullptr);
    ~RenderViewStrategyImage2D() override;

    /** Explicitly define a list of images to create a profile plot for. 
        The same plot line will be applied to all images.
        When specifying an empty list here, the first image contained in the Strategy's context will be used. */
    void setInputImages(const QList<ImageDataObject *> & images);

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
    std::vector<std::unique_ptr<DataObject>> m_previewProfiles;
    AbstractRenderView * m_previewRenderer;
    QList<QMetaObject::Connection> m_previewRendererConnections;

    QList<ImageDataObject *> m_currentPlottingImages; // currently used input images
    QList<ImageDataObject *> m_inputImages; // input images that were explicitly set

    vtkSmartPointer<vtkLineWidget2> m_lineWidget;
    vtkSmartPointer<vtkEventQtSlotConnect> m_vtkConnect;
};
