#pragma once

#include <map>
#include <memory>
#include <vector>

#include <QDockWidget>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <gui/gui_api.h>


class vtkLookupTable;
class vtkObject;

class AbstractRenderView;
class AbstractVisualizedData;
class ColorMapping;
class DataObject;
class OrientedScalarBarActor;
class Ui_ColorMappingChooser;


class GUI_API ColorMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    explicit ColorMappingChooser(QWidget * parent = nullptr, Qt::WindowFlags flags = {});
    ~ColorMappingChooser() override;

    QString selectedGradientName() const;

    void setCurrentRenderView(AbstractRenderView * renderView);
    /** switch to specified dataObject, in case it is visible in my current render view */
    void setSelectedData(DataObject * dataObject);
    void setSelectedVisualization(AbstractVisualizedData * visualization);

signals:
    void renderSetupChanged();

private:
    void guiScalarsSelectionChanged();
    void guiGradientSelectionChanged();
    void guiComponentChanged(int guiComponent);
    void guiMinValueChanged(double value);
    void guiMaxValueChanged(double value);
    void guiResetMinToData();
    void guiResetMaxToData();
    void guiSelectNanColor();
    void guiLegendPositionChanged(const QString & position);
    void guiLegendTitleChanged();

    void rebuildGui();
    void updateScalarsSelection();
    void updateScalarsEnabled();
    void setupGuiConnections();
    void discardGuiConnections();
    void setupValueRangeConnections();
    void discardValueRangeConnections();

    /** Update the GUI-selected scalars when the mapping is modified directly via its interface */
    void mappingScalarsChanged();

    void colorLegendPositionChanged();
    void updateLegendTitleFont();
    void updateLegendLabelFont();
    void updateLegendConfig();
    void updateNanColorButtonStyle(const QColor & color);
    void updateNanColorButtonStyle(const unsigned char color[4]);

    void loadGradientImages();

    /** remove data from the UI if we currently hold it */
    void checkRemovedData(const QList<AbstractVisualizedData *> & content);

    void updateTitle();
    void updateGuiValueRanges();

    OrientedScalarBarActor & legend();

private:
    std::unique_ptr<Ui_ColorMappingChooser> m_ui;

    AbstractRenderView * m_renderView;
    std::vector<QMetaObject::Connection> m_viewConnections;
    ColorMapping * m_mapping;

    std::vector<QMetaObject::Connection> m_mappingConnections;
    /** check if we are moving the actor or if the user interacts */
    bool m_movingColorLegend;
    /** Mapping from subject (color legend coordinate, text property, etc) to observer id */
    std::map<vtkWeakPointer<vtkObject>, unsigned long> m_colorLegendObserverIds;
    // connections for various parameters and signals related to the color mapping
    std::vector<QMetaObject::Connection> m_guiConnections;
    QMetaObject::Connection m_dataMinMaxChangedConnection;

private:
    Q_DISABLE_COPY(ColorMappingChooser)
};