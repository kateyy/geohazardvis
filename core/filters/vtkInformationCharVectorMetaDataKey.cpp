#include "vtkInformationCharVectorMetaDataKey.h"

#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>


vtkInformationCharVectorMetaDataKey::vtkInformationCharVectorMetaDataKey(const char * name, const char * location)
    : vtkInformationCharVectorKey(name, location)
{
}

vtkInformationCharVectorMetaDataKey::~vtkInformationCharVectorMetaDataKey() = default;

vtkInformationCharVectorMetaDataKey * vtkInformationCharVectorMetaDataKey::MakeKey(const char * name, const char * location)
{
    return new vtkInformationCharVectorMetaDataKey(name, location);
}

void vtkInformationCharVectorMetaDataKey::CopyDefaultInformation(vtkInformation * request, vtkInformation * fromInfo, vtkInformation * toInfo)
{
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
        this->ShallowCopy(fromInfo, toInfo);
    }
}
