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

#include <gtest/gtest.h>

#include <vtkDiskSource.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <core/table_model/QVtkTableModelImage.h>
#include <core/table_model/QVtkTableModelPolyData.h>
#include <core/table_model/QVtkTableModelProfileData.h>
#include <core/table_model/QVtkTableModelRawVector.h>
#include <core/table_model/QVtkTableModelVectorGrid3D.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/DataProfile2DDataObject.h>
#include <core/data_objects/RawVectorData.h>
#include <core/data_objects/VectorGrid3DDataObject.h>


class QVtkTableModel_test : public ::testing::Test
{
public:
    static bool readAllCells(QVtkTableModel & tableModel)
    {
        for (int c = 0; c < tableModel.columnCount(); ++c)
        {
            for (int r = 0; r < tableModel.rowCount(); ++r)
            {
                if (!tableModel.data(tableModel.index(r, c)).isValid())
                {
                    return false;
                }
            }
        }
        return true;
    }

    static vtkSmartPointer<vtkPolyData> generateMesh()
    {
        auto disk = vtkSmartPointer<vtkDiskSource>::New();
        disk->SetInnerRadius(0);
        disk->SetOuterRadius(1);
        disk->SetCircumferentialResolution(6);
        disk->SetRadialResolution(3);
        disk->Update();

        return disk->GetOutput();
    }
};

TEST_F(QVtkTableModel_test, init_QVtkTableModelImage)
{
    auto dataSet = vtkSmartPointer<vtkImageData>::New();
    dataSet->SetExtent(0, 2, 0, 3, 0, 0);
    dataSet->AllocateScalars(VTK_FLOAT, 1);

    ImageDataObject dataObject("dataObject", *dataSet);
    QVtkTableModelImage tableModel;
    tableModel.setDataObject(&dataObject);

    ASSERT_TRUE(readAllCells(tableModel));
}

TEST_F(QVtkTableModel_test, init_QVtkTableModelPolyData)
{
    auto dataSet = generateMesh();

    PolyDataObject dataObject("dataObject", *dataSet);
    QVtkTableModelPolyData tableModel;
    tableModel.setDataObject(&dataObject);

    ASSERT_TRUE(readAllCells(tableModel));
}

TEST_F(QVtkTableModel_test, init_QVtkTableModelProfileData)
{
    auto dataSet = vtkSmartPointer<vtkImageData>::New();
    dataSet->SetExtent(0, 2, 0, 3, 0, 0);
    dataSet->AllocateScalars(VTK_FLOAT, 1);
    ImageDataObject image("image", *dataSet);
    DataProfile2DDataObject profile("profile", image, "ImageScalars", IndexType::points, 0);
    ASSERT_TRUE(profile.isValid());
    profile.setProfileLinePoints({ 0, 0 }, { 0, 2 });

    QVtkTableModelProfileData tableModel;
    tableModel.setDataObject(&profile);

    ASSERT_TRUE(readAllCells(tableModel));
}

TEST_F(QVtkTableModel_test, init_QVtkTableModelRawVector)
{
    auto farray = vtkSmartPointer<vtkFloatArray>::New();
    farray->SetNumberOfComponents(3);
    farray->SetNumberOfValues(5);

    RawVectorData rawVector("dataObject", *farray);
    QVtkTableModelRawVector tableModel;
    tableModel.setDataObject(&rawVector);

    ASSERT_TRUE(readAllCells(tableModel));
}

TEST_F(QVtkTableModel_test, init_QVtkTableModelVectorGrid3D)
{
    auto dataSet = vtkSmartPointer<vtkImageData>::New();
    dataSet->SetExtent(0, 2, 0, 3, 0, 0);
    dataSet->AllocateScalars(VTK_FLOAT, 3);

    ImageDataObject dataObject("dataObject", *dataSet);
    QVtkTableModelVectorGrid3D tableModel;
    tableModel.setDataObject(&dataObject);

    ASSERT_TRUE(readAllCells(tableModel));
}