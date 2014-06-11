#pragma once

#include <vtkSmartPointer.h>

#include <QAbstractTableModel>

#include <core/core_api.h>

class vtkDataSet;
class vtkPolyData;
class vtkImageData;


CORE_API enum class DisplayData {
    Triangles,
    Grid
};

class CORE_API QVtkTableModel : public QAbstractTableModel
{
public:
    QVtkTableModel(QObject * parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    void showData(vtkDataSet * data);

protected:
    void showPolyData(vtkPolyData * polyData);
    void showGridData(vtkImageData * gridData);

    vtkSmartPointer<vtkPolyData> m_vtkPolyData;
    vtkSmartPointer<vtkImageData> m_vtkImageData;

    DisplayData m_displayData;
};