#pragma once

#include <vtkSmartPointer.h>

#include <QAbstractTableModel>

class vtkPolyData;
class vtkImageData;

enum class DisplayData {
    Triangles,
    Grid
};

class QVtkTableModel : public QAbstractTableModel {
public:
    QVtkTableModel(QObject * parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    void showPolyData(vtkSmartPointer<vtkPolyData> data);
    void showGridData(vtkSmartPointer<vtkImageData> grid);

protected:
    vtkSmartPointer<vtkPolyData> m_vtkPolyData;
    vtkSmartPointer<vtkImageData> m_vtkImageData;

    DisplayData m_displayData;
};