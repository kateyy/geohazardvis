#pragma once

#include <vtkInformationStringKey.h>

#include <core/core_api.h>

class CORE_API vtkInformationStringMetaDataKey : public vtkInformationStringKey
{
public:
    vtkTypeMacro(vtkInformationStringMetaDataKey, vtkInformationStringKey);

    vtkInformationStringMetaDataKey(const char * name, const char * location);
    ~vtkInformationStringMetaDataKey() override;

    static vtkInformationStringMetaDataKey * MakeKey(const char * name, const char * location);

    /* Shallow copies the key from fromInfo to toInfo if request has the REQUEST_INFORMATION() key.
    * This is used by the pipeline to propagate this key downstream. */
    void CopyDefaultInformation(vtkInformation * request,
        vtkInformation * fromInfo, vtkInformation * toInfo) override;

private:
    vtkInformationStringMetaDataKey(const vtkInformationStringMetaDataKey &) = delete;
    void operator=(const vtkInformationStringMetaDataKey &) = delete;
};
