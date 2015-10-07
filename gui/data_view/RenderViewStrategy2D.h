#pragma once

#include <memory>
#include <vector>

#include <QMap>

#include <vtkSmartPointer.h>

#include <gui/data_view/RenderViewStrategy.h>


class QAction;
class vtkObject;
class vtkLineWidget2;

class AbstractRenderView;
class DataObject;


class GUI_API RenderViewStrategy2D : public RenderViewStrategy
{
public:
    RenderViewStrategy2D(RendererImplementationBase3D & context);
    ~RenderViewStrategy2D() override;

    /** Explicitly define a list of images to create a profile plot for. 
        The same plot line will be applied to all images.
        When specifying an empty list here, the first image contained in the Strategy's context will be used. */
    void setInputData(const QList<DataObject *> & images);

    QString name() const override;

    bool contains3dData() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const override;

    /** Start or refresh the current profile plot, open a new preview renderer if required. */
    void startProfilePlot();
    void acceptProfilePlot();
    void abortProfilePlot();

protected:
    QString defaultInteractorStyle() const override;

    void onActivateEvent() override;
    void onDeactivateEvent() override;

private:
    void initialize();

    /** Delete current plots, but do not change the GUI state */
    void clearProfilePlots();

    void lineMoved();

    void updateAutomaticPlots();

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

    QList<DataObject *> m_activeInputData;  // currently used input data
    QList<DataObject *> m_inputData;        // input data that was explicitly set

    vtkSmartPointer<vtkLineWidget2> m_lineWidget;
    QMultiMap<vtkSmartPointer<vtkObject>, unsigned long> m_observerTags;
};
