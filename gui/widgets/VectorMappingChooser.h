#pragma once

#include <QDockWidget>
#include <QItemSelection>


class QItemSelection;

namespace reflectionzeug
{
    class PropertyGroup;
}

class VectorsToSurfaceMapping;
class VectorMappingChooserListModel;
class Ui_VectorMappingChooser;


class VectorMappingChooser : public QDockWidget
{
    Q_OBJECT

public:
    VectorMappingChooser(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    ~VectorMappingChooser();

    void setMapping(int rendererId = -1, VectorsToSurfaceMapping * mapping = nullptr);
    int rendererId() const;
    const VectorsToSurfaceMapping * mapping() const;

signals:
    void renderSetupChanged();

private slots:
    void updateGui(const QItemSelection & selection = QItemSelection());

private:
    void updateTitle();

private:
    Ui_VectorMappingChooser * m_ui;
    int m_rendererId;
    VectorsToSurfaceMapping * m_mapping;
    VectorMappingChooserListModel * m_listModel;
    QList<reflectionzeug::PropertyGroup *> m_propertyGroups;
    QMetaObject::Connection m_startingIndexConnection;
};
