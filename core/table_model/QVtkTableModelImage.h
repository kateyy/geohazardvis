/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <vtkSmartPointer.h>

#include <core/table_model/QVtkTableModel.h>


class vtkImageData;


class CORE_API QVtkTableModelImage : public QVtkTableModel
{
public:
    explicit QVtkTableModelImage(QObject * parent = nullptr);
    ~QVtkTableModelImage() override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    IndexType indexType() const override;

protected:
    void resetDisplayData() override;

    void tableToImageCoord(int tableRow, int & imageRow, int & imageColumn) const;

private:
    vtkSmartPointer<vtkImageData> m_vtkImageData;

private:
    Q_DISABLE_COPY(QVtkTableModelImage)
};
