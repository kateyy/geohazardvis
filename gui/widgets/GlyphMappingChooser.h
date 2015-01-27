#pragma once

#include <QDockWidget>
#include <QItemSelection>


class QItemSelection;

namespace reflectionzeug
{
    class PropertyGroup;
}

class RenderView;
class AbstractVisualizedData;
class GlyphMapping;
class GlyphMappingChooserListModel;
class DataObject;
class Ui_GlyphMappingChooser;


class GlyphMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    GlyphMappingChooser(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    ~GlyphMappingChooser() override;

signals:
    void renderSetupChanged();

public slots:
    void setCurrentRenderView(RenderView * renderView = nullptr);
    /** switch to specified dataObject, in case it is visible in my current render view */
    void setSelectedData(DataObject * dataObject);

private slots:
    void updateGuiForSelection(const QItemSelection & selection = QItemSelection());
    void updateVectorsList();

    /** remove data from the UI if we currently hold it */
    void checkRemovedData(AbstractVisualizedData * content);

private:
    void updateTitle();

private:
    Ui_GlyphMappingChooser * m_ui;

    RenderView * m_renderView;
    GlyphMapping * m_mapping;
    GlyphMappingChooserListModel * m_listModel;
    QList<reflectionzeug::PropertyGroup *> m_propertyGroups;
    QMetaObject::Connection m_startingIndexConnection;
};
