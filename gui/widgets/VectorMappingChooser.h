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

class VectorMapping;
class VectorMappingChooserListModel;
class Ui_VectorMappingChooser;


class VectorMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    VectorMappingChooser(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    ~VectorMappingChooser();

    void setMapping(int rendererId = -1, VectorMapping * mapping = nullptr);
    int rendererId() const;
    const VectorMapping * mapping() const;

signals:
    void renderSetupChanged();

private slots:
    void updateGuiForSelection(const QItemSelection & selection = QItemSelection());
    void updateVectorsList();

private:
    void updateTitle();

private:
    Ui_VectorMappingChooser * m_ui;
    propertyguizeug::PropertyBrowser * m_propertyBrowser;

    int m_rendererId;
    VectorMapping * m_mapping;
    VectorMappingChooserListModel * m_listModel;
    QList<reflectionzeug::PropertyGroup *> m_propertyGroups;
    QMetaObject::Connection m_startingIndexConnection;
};
