#pragma once

#include <vtkInformationDoubleVectorKey.h>

#include <core/core_api.h>

class CORE_API vtkInformationDoubleVectorMetaDataKey : public vtkInformationDoubleVectorKey
{
public:
    vtkTypeMacro(vtkInformationDoubleVectorMetaDataKey, vtkInformationDoubleVectorKey);

    vtkInformationDoubleVectorMetaDataKey(const char * name, const char * location);
    ~vtkInformationDoubleVectorMetaDataKey() override;

    static vtkInformationDoubleVectorMetaDataKey * MakeKey(const char * name, const char * location);

    /* Shallow copies the key from fromInfo to toInfo if request has the REQUEST_INFORMATION() key.
    * This is used by the pipeline to propagate this key downstream. */
    void CopyDefaultInformation(vtkInformation * request,
        vtkInformation * fromInfo, vtkInformation * toInfo) override;

private:
    vtkInformationDoubleVectorMetaDataKey(const vtkInformationDoubleVectorMetaDataKey &) = delete;
    void operator=(const vtkInformationDoubleVectorMetaDataKey &) = delete;
};
