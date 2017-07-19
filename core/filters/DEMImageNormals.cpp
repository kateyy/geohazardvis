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

#include "DEMImageNormals.h"

#include <vtkArrayDispatch.h>
#include <vtkAssume.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataArrayAccessor.h>
#include <vtkExecutive.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkVector.h>

#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


vtkStandardNewMacro(DEMImageNormals)


namespace
{

struct CalcNormalsWorker
{
    CalcNormalsWorker(vtkImageData & image, ImageExtent workerExtent,
        double elevationUnitScale, double coordinateUnitScale)
        : Image{ image }
        , WorkerExtent{ workerExtent }
        , ElevationUnitScale{elevationUnitScale}
        , CoordinatesUnitScale{ coordinateUnitScale }
    {
    }

    vtkImageData & Image;
    ImageExtent WorkerExtent;

    double ElevationUnitScale;
    double CoordinatesUnitScale;

    template <typename ElevationArray, typename NormalArray>
    void operator()(ElevationArray * elevations, NormalArray * normals)
    {
        VTK_ASSUME(elevations->GetNumberOfComponents() == 1);
        VTK_ASSUME(normals->GetNumberOfComponents() == 3);
        VTK_ASSUME(elevations->GetNumberOfTuples() == normals->GetNumberOfTuples());

        using NormalValueType = typename vtkDataArrayAccessor<NormalArray>::APIType;

        ImageExtent wholeExtent;
        this->Image.GetExtent(wholeExtent.data());

        assert(wholeExtent.contains(this->WorkerExtent));
        assert(!this->WorkerExtent.isEmpty());

        vtkVector3d spacing;
        this->Image.GetSpacing(spacing.GetData());

        const auto xySpacing = convertTo<2>(spacing) * this->CoordinatesUnitScale;
        const auto xyScale = convertTo<NormalValueType>(this->ElevationUnitScale / xySpacing);

        vtkDataArrayAccessor<ElevationArray> e(elevations);
        vtkDataArrayAccessor<NormalArray> n(normals);
        vtkVector3<NormalValueType> normal;

        vtkVector3<vtkIdType> incs;
        this->Image.GetIncrements(incs.GetData());

        for (int z = this->WorkerExtent[4]; z <= this->WorkerExtent[5]; ++z)
        {
            for (int y = this->WorkerExtent[2]; y <= this->WorkerExtent[3]; ++y)
            {
                const int yDirection = y == wholeExtent[3] ? -1 : 1;
                for (int x = this->WorkerExtent[0]; x <= this->WorkerExtent[1]; ++x)
                {
                    const int xDirection = x == wholeExtent[1] ? -1 : 1;

                    const auto idx_pos = ((x - wholeExtent[0]) * incs[0] + (y - wholeExtent[2]) * incs[1] + (z - wholeExtent[4]) * incs[2]);
                    const auto idx_east = ((x + xDirection - wholeExtent[0]) * incs[0] + (y - wholeExtent[2]) * incs[1] + (z - wholeExtent[4]) * incs[2]);
                    const auto idx_south = ((x - wholeExtent[0]) * incs[0] + (y + yDirection - wholeExtent[2]) * incs[1] + (z - wholeExtent[4]) * incs[2]);

                    const auto e_pos = e.Get(idx_pos, 0);
                    const auto e_diffEast = e_pos - e.Get(idx_east, 0);
                    const auto e_diffSouth = e_pos - e.Get(idx_south, 0);

                    normal = {
                        static_cast<NormalValueType>(e_diffEast * xyScale[0]),
                        static_cast<NormalValueType>(e_diffSouth * xyScale[1]),
                        static_cast<NormalValueType>(1.0)
                    };
                    normal.Normalize();

                    n.Set(idx_pos, normal.GetData());
                }
            }
        }
    }
};

}


DEMImageNormals::DEMImageNormals()
    : Superclass()
    , CoordinatesUnitScale{ 1000.0 } // coordinate in km by default
    , ElevationUnitScale{ 1.0 / 100.0 } // Elevations in cm by default
{
}

DEMImageNormals::~DEMImageNormals() = default;

int DEMImageNormals::RequestInformation(vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    if (!Superclass::RequestInformation(request, inputVector, outputVector))
    {
        return 0;
    }

    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    int scalarType = -1;
    int numTuples = -1;
    const int numComponents = 3;  // output: normals

    if (auto inScalarInfo = vtkDataObject::GetActiveFieldInformation(inInfo,
        vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS))
    {
        scalarType = inScalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
        numTuples = inScalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES());
    }
    if (scalarType <= 0)
    {
        scalarType = vtkImageData::GetScalarType(inInfo);
    }
    if (scalarType <= 0)
    {
        // default as fall back
        scalarType = VTK_DOUBLE;
    }
    if (numTuples <= 0)
    {
        ImageExtent extent;

        if (inInfo->Has(vtkDataObject::DATA_EXTENT()))
        {
            inInfo->Get(vtkDataObject::DATA_EXTENT(), extent.data());
        }
        else if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
        {
            inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent.data());
        }

        if (!extent.isEmpty())
        {
            numTuples = static_cast<decltype(numTuples)>(extent.numberOfPoints());
        }
    }
    if (numTuples <= 0)
    {
        numTuples = -1;
    }

    vtkDataObject::SetPointDataActiveScalarInfo(outInfo, scalarType, 1);

    vtkDataObject::SetActiveAttribute(outInfo,
        vtkDataObject::FIELD_ASSOCIATION_POINTS, "Normals", vtkDataSetAttributes::NORMALS);
    vtkDataObject::SetActiveAttributeInfo(outInfo,
        vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::NORMALS, "Normals",
        scalarType, numComponents, numTuples);

    return 1;
}

int DEMImageNormals::RequestData(vtkInformation * request,
    vtkInformationVector ** inputVector,
    vtkInformationVector * outputVector)
{
    auto inInfo = inputVector[0]->GetInformationObject(0);
    auto outInfo = outputVector->GetInformationObject(0);

    auto inImage = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    auto outImage = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    auto elevations = inImage->GetPointData()->GetScalars();
    if (!elevations || elevations->GetNumberOfComponents() != 1 || !elevations->GetName())
    {
        vtkErrorMacro("Could not find valid elevations in image scalars.");
        return 0;
    }

    ImageExtent inExtent(inImage->GetExtent());
    ImageExtent outExtent(vtkStreamingDemandDrivenPipeline::GetUpdateExtent(outInfo));

    if (inExtent != outExtent)
    {
        vtkErrorMacro("Different input and output (update) extents are not implemented.");
        return 0;
    }

    vtkSmartPointer<vtkDataArray> normals = outImage->GetPointData()->GetNormals();
    if (!normals)
    {
        normals.TakeReference(vtkDataArray::CreateDataArray(elevations->GetDataType()));
    }
    normals->SetNumberOfComponents(3);
    normals->SetNumberOfTuples(outExtent.numberOfPoints());
    normals->SetName("Normals");
    outImage->GetPointData()->SetNormals(normals);

    if (minComponent(convertTo<2>(inExtent.componentSize())) > 1)
    {
        if (!Superclass::RequestData(request, inputVector, outputVector))
        {
            return 0;
        }
    }
    else
    {
        vtkDebugMacro("Not computing normals, image is too small.");
    }

    // preserve current scalars
    outImage->GetPointData()->SetScalars(elevations);

    return 1;
}

void DEMImageNormals::CopyAttributeData(vtkImageData * in, vtkImageData * out, vtkInformationVector ** inputVector)
{
    vtkSmartPointer<vtkDataArray> normals = out->GetPointData()->GetNormals();
    Superclass::CopyAttributeData(in, out, inputVector);
    out->GetPointData()->SetNormals(normals);
}

void DEMImageNormals::ThreadedRequestData(vtkInformation * /*request*/,
    vtkInformationVector ** /*inputVector*/,
    vtkInformationVector * /*outputVector*/,
    vtkImageData *** inData,
    vtkImageData ** outData,
    int outExt[6], int /*id*/)
{
    auto inImage = inData[0][0];
    auto outImage = outData[0];

    auto elevations = inImage->GetPointData()->GetScalars();
    auto normals = outImage->GetPointData()->GetNormals();

    CalcNormalsWorker worker(
        *inImage, ImageExtent(outExt),
        this->ElevationUnitScale, this->CoordinatesUnitScale);

    using Dispatcher = vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals> ;

    if (!Dispatcher::Execute(elevations, normals, worker))
    {
        worker(elevations, normals);
    }
}
