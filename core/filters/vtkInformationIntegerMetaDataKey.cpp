#include "vtkInformationIntegerMetaDataKey.h"

#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>


vtkInformationIntegerMetaDataKey::vtkInformationIntegerMetaDataKey(const char * name, const char * location)
    : vtkInformationIntegerKey(name, location)
{
}

vtkInformationIntegerMetaDataKey::~vtkInformationIntegerMetaDataKey() = default;

vtkInformationIntegerMetaDataKey * vtkInformationIntegerMetaDataKey::MakeKey(const char * name, const char * location)
{
    return new vtkInformationIntegerMetaDataKey(name, location);
}

void vtkInformationIntegerMetaDataKey::CopyDefaultInformation(vtkInformation * request, vtkInformation * fromInfo, vtkInformation * toInfo)
{
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
        this->ShallowCopy(fromInfo, toInfo);
    }
}
