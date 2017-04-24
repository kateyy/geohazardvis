#include "vtkInformationStringMetaDataKey.h"

#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>


vtkInformationStringMetaDataKey::vtkInformationStringMetaDataKey(const char * name, const char * location)
    : vtkInformationStringKey(name, location)
{
}

vtkInformationStringMetaDataKey::~vtkInformationStringMetaDataKey() = default;

vtkInformationStringMetaDataKey * vtkInformationStringMetaDataKey::MakeKey(const char * name, const char * location)
{
    return new vtkInformationStringMetaDataKey(name, location);
}

void vtkInformationStringMetaDataKey::CopyDefaultInformation(vtkInformation * request, vtkInformation * fromInfo, vtkInformation * toInfo)
{
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
        this->ShallowCopy(fromInfo, toInfo);
    }
}
