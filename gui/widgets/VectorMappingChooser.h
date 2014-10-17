#pragma once

#include <QDockWidget>
#include <QItemSelection>


class QItemSelection;

namespace reflectionzeug
{
    class PropertyGroup;
}
namespace propertyguizeug
{
class PropertyBrowser;
}

class RenderView;
class VectorMapping;
class VectorMappingChooserListModel;
class DataObject;
class Ui_VectorMappingChooser;


class VectorMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    VectorMappingChooser(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    ~VectorMappingChooser() override;

signals:
    void renderSetupChanged();

public slots:
    void setCurrentRenderView(RenderView * renderView = nullptr);
    /** switch to specified dataObject, in case it is visible in my current render view */
    void setSelectedData(DataObject * dataObject);

private slots:
    void updateGuiForSelection(const QItemSelection & selection = QItemSelection());
    void updateVectorsList();

private:
    void updateTitle();

private:
    Ui_VectorMappingChooser * m_ui;
    propertyguizeug::PropertyBrowser * m_propertyBrowser;

    RenderView * m_renderView;
    VectorMapping * m_mapping;
    VectorMappingChooserListModel * m_listModel;
    QList<reflectionzeug::PropertyGroup *> m_propertyGroups;
    QMetaObject::Connection m_startingIndexConnection;
};
