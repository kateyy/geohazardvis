#pragma once

#include <memory>
#include <vector>

#include <QDockWidget>
#include <QList>

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
    GlyphMappingChooser(QWidget * parent = nullptr, Qt::WindowFlags flags = {});
    ~GlyphMappingChooser() override;

    void setCurrentRenderView(AbstractRenderView * renderView);
    /** switch to specified dataObject, in case it is visible in my current render view */
    void setSelectedData(DataObject * dataObject);

signals:
    void renderSetupChanged();

private:
    void updateGuiForSelection(const QItemSelection & selection);
    void updateVectorsList();

    /** remove data from the UI if we currently hold it */
    void checkRemovedData(const QList<AbstractVisualizedData *> & content);

    void updateTitle();

private:
    std::unique_ptr<Ui_GlyphMappingChooser> m_ui;

    AbstractRenderView * m_renderView;
    QList<QMetaObject::Connection> m_viewConnections;
    GlyphMapping * m_mapping;
    QMetaObject::Connection m_vectorListConnection;
    QList<QMetaObject::Connection> m_vectorsRenderConnections;
    GlyphMappingChooserListModel * m_listModel;
    std::vector<std::unique_ptr<reflectionzeug::PropertyGroup>> m_propertyGroups;

private:
    Q_DISABLE_COPY(GlyphMappingChooser)
};
