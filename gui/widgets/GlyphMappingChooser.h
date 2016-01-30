#pragma once

#include <memory>
#include <vector>

#include <QDockWidget>

#include <gui/gui_api.h>


class QItemSelection;

namespace reflectionzeug
{
    class PropertyGroup;
}

class AbstractRenderView;
class AbstractVisualizedData;
class GlyphMapping;
class GlyphMappingChooserListModel;
class DataObject;
class Ui_GlyphMappingChooser;


class GUI_API GlyphMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    GlyphMappingChooser(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    ~GlyphMappingChooser() override;

signals:
    void renderSetupChanged();

public:
    void setCurrentRenderView(AbstractRenderView * renderView = nullptr);
    /** switch to specified dataObject, in case it is visible in my current render view */
    void setSelectedData(DataObject * dataObject);
    void setSelectedData(AbstractRenderView * renderView, DataObject * dataObject);

private:
    void updateGuiForSelection(const QItemSelection & selection);
    void updateVectorsList();

    /** remove data from the UI if we currently hold it */
    void checkRemovedData(const QList<AbstractVisualizedData *> & content);

    void updateTitle();

private:
    std::unique_ptr<Ui_GlyphMappingChooser> m_ui;

    AbstractRenderView * m_renderView;
    GlyphMapping * m_mapping;
    GlyphMappingChooserListModel * m_listModel;
    std::vector<std::unique_ptr<reflectionzeug::PropertyGroup>> m_propertyGroups;
    QMetaObject::Connection m_startingIndexConnection;
};
