#pragma once

#include <QDockWidget>


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
    const VectorsToSurfaceMapping * mapping() const;

signals:
    void renderSetupChanged();

private:
    void updateWindowTitle(int rendererId = -1);

private:
    Ui_VectorMappingChooser * m_ui;
    VectorsToSurfaceMapping * m_mapping;
    VectorMappingChooserListModel * m_listModel;
    QList<reflectionzeug::PropertyGroup *> m_propertyGroups;
};
