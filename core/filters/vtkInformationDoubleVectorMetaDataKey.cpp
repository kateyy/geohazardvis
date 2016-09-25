#include "vtkInformationDoubleVectorMetaDataKey.h"

#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>


vtkInformationDoubleVectorMetaDataKey::vtkInformationDoubleVectorMetaDataKey(const char * name, const char * location)
    : vtkInformationDoubleVectorKey(name, location)
{
}

vtkInformationDoubleVectorMetaDataKey::~vtkInformationDoubleVectorMetaDataKey() = default;

vtkInformationDoubleVectorMetaDataKey * vtkInformationDoubleVectorMetaDataKey::MakeKey(const char * name, const char * location)
{
    return new vtkInformationDoubleVectorMetaDataKey(name, location);
}

void vtkInformationDoubleVectorMetaDataKey::CopyDefaultInformation(vtkInformation * request, vtkInformation * fromInfo, vtkInformation * toInfo)
{
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
        this->ShallowCopy(fromInfo, toInfo);
    }
}
